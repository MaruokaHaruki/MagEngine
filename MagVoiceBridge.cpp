// MagVoiceBridge.cpp
// PURPOSE: WASAPI を使用したマイク入力のリアルタイム処理を実装
// REASON: Windows でのマイク入力と音声分析機能を提供するため
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "MagVoiceBridge.h"

#include <algorithm>
#include <cmath>
#include <combaseapi.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Uuid.lib")

//========================================
// コンストラクタ / デストラクタ
//========================================

MagVoiceBridge::MagVoiceBridge() {
	// PURPOSE: メンバ変数を初期化する
	// REASON: 全メンバ変数は宣言時に初期化リスト経由で初期化されます
}

MagVoiceBridge::~MagVoiceBridge() {
	// PURPOSE: リソースをクリーンアップする
	// REASON: Shutdown() は冪等性を持つため、二重解放の心配なく呼び出せます
	Shutdown();
}

//========================================
// 初期化 / シャットダウン
//========================================

bool MagVoiceBridge::Initialize() {
	HRESULT result = S_OK;

	// PURPOSE: COM ライブラリを初期化する
	// REASON: WASAPI 使用前の必須初期化処理
	result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: デバイス列挙用オブジェクトを生成する
	// REASON: デフォルト録音デバイスを取得するために必須
	result = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void **>(&deviceEnumerator_));

	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: デフォルトの録音デバイスを取得する
	// REASON: ユーザが指定したマイクから入力を取得するため
	result = deviceEnumerator_->GetDefaultAudioEndpoint(
		eCapture,
		eConsole,
		&captureDevice_);

	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: 録音デバイスの IAudioClient インターフェースを取得する
	// REASON: WASAPI のオーディオクライアント機能を使用するため
	result = captureDevice_->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		reinterpret_cast<void **>(&audioClient_));

	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: マイクのネイティブオーディオフォーマットを取得する
	// REASON: サンプリングレート、チャンネル数、ビット深度の情報が必須
	result = audioClient_->GetMixFormat(&waveFormat_);
	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: オーディオフォーマット情報をメンバ変数に保存する
	// REASON: PCM データの正規化や時間計算で使用するため
	sampleRate_ = waveFormat_->nSamplesPerSec;
	channelCount_ = waveFormat_->nChannels;
	bitsPerSample_ = waveFormat_->wBitsPerSample;

	// REVIEW: サンプリングレート 0 はあり得ない。妥当性チェック
	if (sampleRate_ == 0 || channelCount_ == 0) {
		return false;
	}

	// PURPOSE: shared mode でオーディオクライアントを初期化する
	// REASON: 他のアプリと同時にマイクを使用できるようにするため
	// OPTIMIZE: バッファ期間 10000000 (1秒) は環境に応じて調整可能
	REFERENCE_TIME bufferDuration = 10000000;

	result = audioClient_->Initialize(
		AUDCLNT_SHAREMODE_SHARED, // shared mode
		0,						  // フラグなし
		bufferDuration,			  // バッファ期間
		0,						  // デバイス期間（shared mode では 0）
		waveFormat_,			  // オーディオフォーマット
		nullptr);				  // GUID なし

	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: IAudioCaptureClient インターフェースを取得する
	// REASON: GetBuffer で PCM データを取得するため
	result = audioClient_->GetService(
		__uuidof(IAudioCaptureClient),
		reinterpret_cast<void **>(&captureClient_));

	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: 初期化完了フラグを設定する
	// REASON: 他の関数が Initialize() 完了を確認する必要があるため
	isInitialized_ = true;
	return true;
}

bool MagVoiceBridge::Start() {
	// REVIEW: Initialize() の前に Start() が呼ばれる場合を想定
	if (!isInitialized_) {
		return false;
	}

	// PURPOSE: オーディオストリームを開始する
	// REASON: WASAPI からのデータキャプチャを開始するため
	HRESULT result = audioClient_->Start();
	if (FAILED(result)) {
		return false;
	}

	// PURPOSE: キャプチャ中フラグを設定する
	// REASON: Update() が実際に処理を行うべき状態を管理するため
	isCapturing_ = true;
	return true;
}

void MagVoiceBridge::Stop() {
	// PURPOSE: オーディオストリームを停止する
	// REASON: キャプチャを一時停止する必要があるため
	if (audioClient_ && isCapturing_) {
		audioClient_->Stop();
		isCapturing_ = false;
	}
}

void MagVoiceBridge::Shutdown() {
	// PURPOSE: キャプチャを停止する
	// REASON: リソース解放前にストリームを止める必要があるため
	Stop();

	// PURPOSE: COM インターフェースをリリースする
	// REASON: メモリリークを防ぐため
	if (captureClient_) {
		captureClient_->Release();
		captureClient_ = nullptr;
	}

	if (audioClient_) {
		audioClient_->Release();
		audioClient_ = nullptr;
	}

	if (captureDevice_) {
		captureDevice_->Release();
		captureDevice_ = nullptr;
	}

	if (deviceEnumerator_) {
		deviceEnumerator_->Release();
		deviceEnumerator_ = nullptr;
	}

	// PURPOSE: waveFormat_ のメモリを解放する
	// REASON: CoTaskMemFree で確保されたメモリを明示的に解放する必須
	if (waveFormat_) {
		CoTaskMemFree(waveFormat_);
		waveFormat_ = nullptr;
	}

	// PURPOSE: COM ライブラリをクリーンアップする
	// REASON: COM 初期化（CoInitializeEx）に対応する必須処理
	CoUninitialize();

	// PURPOSE: 初期化フラグをリセットする
	// REASON: Shutdown() の冪等性を確保し、二重解放を防ぐため
	isInitialized_ = false;
	isCapturing_ = false;
}

//========================================
// メイン処理
//========================================

void MagVoiceBridge::Update() {
	// PURPOSE: Start() 前の無駄な処理を回避する
	// REASON: Start() 前に Update() が呼ばれることを想定
	if (!isCapturing_) {
		return;
	}

	// PURPOSE: WASAPI からリアルタイムのオーディオデータを取得する
	// REASON: 毎フレーム呼び出してリアルタイムに音声データを更新するため
	CaptureAudioData();

	// PURPOSE: 最新のサンプルから音量を計算する
	// REASON: Update() の中核処理
	float rmsLevel = CalculateRMSLevel();
	currentVolume_ = rmsLevel;

	// PURPOSE: RMS をデシベルに変換する
	// REASON: dB 単位での音量表示に必要。log(0) エラー回避のため最小値チェック
	// NOTE: 計算式: dB = 20 * log10(RMS)
	float volumeDB = 20.0f * std::log10(std::max(0.00001f, rmsLevel));
	currentVolumeDB_ = volumeDB;

	// PURPOSE: ピークレベル（最大振幅）を計算する
	// REASON: 瞬間的な最大音量を把握するため
	float peakLevel = CalculatePeakLevel();
	peakVolume_ = peakLevel;

	// PURPOSE: ピークレベルを dB に変換する
	// REASON: dB 単位でのピークレベル表示に必要
	float peakDB = 20.0f * std::log10(std::max(0.00001f, peakLevel));
	peakVolumeDB_ = peakDB;

	// PURPOSE: スムージング処理を適用する
	// REASON: リアルタイム値の変動を平滑化し、UI ちらつきを防ぐため
	// NOTE: 式: smoothed = smoothed * (1 - factor) + current * factor
	float currentSmoothed = smoothedVolume_.load();
	float newSmoothed = currentSmoothed * (1.0f - smoothingFactor_) + rmsLevel * smoothingFactor_;
	smoothedVolume_ = newSmoothed;

	float currentSmoothedDB = smoothedVolumeDB_.load();
	float newSmoothedDB = currentSmoothedDB * (1.0f - smoothingFactor_) + volumeDB * smoothingFactor_;
	smoothedVolumeDB_ = newSmoothedDB;

	// PURPOSE: 音声特性スコアを計算する
	// REASON: ゼロクロス率に基づいて人の声らしさを定量化するため
	// NOTE: ZCR が低い（0.0に近い）ほど高スコア（1.0に近い）= 人の声らしい
	float zeroCrossingRate = CalculateZeroCrossingRate();
	float voiceScore = std::max(0.0f, 1.0f - (zeroCrossingRate / ZERO_CROSSING_RATE_THRESHOLD));
	voiceCharacteristicScore_ = voiceScore;

	// PURPOSE: ゼロクロス率と音量から人の声かどうかを判定する
	// REASON: 正確な音声検出のため、複数の指標を組み合わせる
	bool voiceDetected = IsVoiceDetected();
	isVoiceDetected_ = voiceDetected;
}

void MagVoiceBridge::CaptureAudioData() {
	// PURPOSE: WASAPI バッファに利用可能なフレーム数を取得する
	// REASON: GetBuffer で取得すべきデータサイズを判定するため
	uint32_t packetLength = 0;
	HRESULT result = captureClient_->GetNextPacketSize(&packetLength);

	// REVIEW: データが利用できない場合は処理をスキップ
	if (FAILED(result) || packetLength == 0) {
		return;
	}

	// PURPOSE: WASAPI バッファからオーディオデータを取得する
	// REASON: PCM サンプルにアクセスするため
	BYTE *pData = nullptr;
	DWORD flags = 0;
	uint32_t numFramesAvailable = 0;

	result = captureClient_->GetBuffer(
		&pData,				 // データへのポインタ
		&numFramesAvailable, // 利用可能なフレーム数
		&flags,				 // フラグ（サイレンス判定など）
		nullptr,			 // デバイス位置（不使用）
		nullptr);			 // バッファ位置（不使用）

	// REVIEW: データ取得失敗時または nullptr の場合は処理をスキップ
	if (FAILED(result) || !pData) {
		return;
	}

	// PURPOSE: フレーム数を更新する
	// REASON: 内部処理を統一した形式で行うため
	packetLength = numFramesAvailable;

	{
		std::lock_guard<std::mutex> lock(sampleMutex_);

		// PURPOSE: WASAPI Shared Mode では 32-bit float でデータが返される
		// REASON: GetMixFormat() で取得したフォーマットに関わらず必ず float
		float *pFloatData = reinterpret_cast<float *>(pData);

		// PURPOSE: マルチチャンネルの場合、全チャンネルを合成する
		// REASON: モノラル処理に統一するため（ステレオの場合は左右の平均値を取る）
		for (uint32_t frame = 0; frame < packetLength; ++frame) {
			float channelAverage = 0.0f;

			// PURPOSE: 全チャンネルの絶対値を加算する
			// REASON: 各チャンネルの振幅を集約するため
			for (uint16_t ch = 0; ch < channelCount_; ++ch) {
				uint32_t sampleIndex = frame * channelCount_ + ch;
				channelAverage += std::abs(pFloatData[sampleIndex]);
			}

			// PURPOSE: チャンネル数で平均化してモノラルに統一する
			// REASON: RMS 計算やゼロクロス判定を統一化するため
			channelAverage /= static_cast<float>(channelCount_);

			// PURPOSE: サンプルをバッファに追加する
			// REASON: 分析用バッファにリアルタイムデータを蓄積
			// NOTE: WASAPI が返すのは既に正規化された [-1.0, 1.0] 範囲の値
			sampleBuffer_.push_back(channelAverage);
		}

		// PURPOSE: サンプルバッファが肥大化しないようサイズを制限する
		// REASON: メモリ使用量を抑えるため最大 10 秒分のサンプルのみ保持
		if (sampleBuffer_.size() > MAX_SAMPLE_BUFFER_SIZE) {
			// PURPOSE: 古いサンプルを削除して新しいサンプルのみを保持
			// REASON: メモリ使用量を制限するため
			sampleBuffer_.erase(
				sampleBuffer_.begin(),
				sampleBuffer_.end() - MAX_SAMPLE_BUFFER_SIZE);
		}
	}

	// PURPOSE: WASAPI バッファを解放する
	// REASON: GetBuffer で確保したバッファを使用済みにマークするため
	captureClient_->ReleaseBuffer(packetLength);
}

float MagVoiceBridge::CalculateZeroCrossingRate() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// PURPOSE: サンプルバッファが空の場合は 0 を返す
	// REASON: 分析対象がない場合の適切なデフォルト値
	if (sampleBuffer_.size() < 2) {
		return 0.0f;
	}

	// PURPOSE: 最新の 2048 サンプルでゼロクロス率を計算する
	// REASON: リアルタイム判定のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW
								? sampleBuffer_.size() - ANALYSIS_WINDOW
								: 0;

	// PURPOSE: ゼロクロッシング（符号変化）の回数を数える
	// REASON: 周波数情報を得るため（高周波はゼロクロッシング多い、低周波は少ない）
	int zeroCrossingCount = 0;
	for (size_t i = startIdx + 1; i < sampleBuffer_.size(); ++i) {
		// PURPOSE: 前後のサンプルで符号が変わっているかを確認
		// REASON: ゼロクロッシングイベントを検出
		if ((sampleBuffer_[i - 1] < 0.0f && sampleBuffer_[i] >= 0.0f) ||
			(sampleBuffer_[i - 1] >= 0.0f && sampleBuffer_[i] < 0.0f)) {
			zeroCrossingCount++;
		}
	}

	// PURPOSE: ゼロクロス率を計算する（0.0～1.0）
	// REASON: 周波数特性を定量化するため
	// NOTE: 計算式: ZCR = (ゼロクロッシング数) / (サンプル数 - 1)
	size_t sampleCount = sampleBuffer_.size() - startIdx;
	float zeroCrossingRate = static_cast<float>(zeroCrossingCount) / static_cast<float>(sampleCount);

	return zeroCrossingRate;
}

float MagVoiceBridge::CalculateRMSLevel() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// PURPOSE: サンプルバッファが空の場合は 0 を返す
	// REASON: 分析対象がない場合の適切なデフォルト値
	if (sampleBuffer_.empty()) {
		return 0.0f;
	}

	// PURPOSE: 最新の 2048 サンプルで RMS を計算する
	// REASON: リアルタイム音量表示のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW
								? sampleBuffer_.size() - ANALYSIS_WINDOW
								: 0;

	// PURPOSE: RMS（二乗平均平方根）を計算する
	// REASON: 音量を正確に計測するため
	// NOTE: 計算式: RMS = sqrt( sum(sample^2) / N )
	float sumOfSquares = 0.0f;
	for (size_t i = startIdx; i < sampleBuffer_.size(); ++i) {
		float sample = sampleBuffer_[i];
		sumOfSquares += sample * sample;
	}

	size_t sampleCount = sampleBuffer_.size() - startIdx;
	float rmsLevel = std::sqrt(sumOfSquares / static_cast<float>(sampleCount));

	return rmsLevel;
}

float MagVoiceBridge::CalculatePeakLevel() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// PURPOSE: サンプルバッファが空の場合は 0 を返す
	// REASON: 分析対象がない場合の適切なデフォルト値
	if (sampleBuffer_.empty()) {
		return 0.0f;
	}

	// PURPOSE: 最新の 2048 サンプルで最大振幅を計算する
	// REASON: リアルタイム表示のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW
								? sampleBuffer_.size() - ANALYSIS_WINDOW
								: 0;

	// PURPOSE: 最大振幅を見つける
	// REASON: ピークメーター表示などに使用
	float maxAbsValue = 0.0f;
	for (size_t i = startIdx; i < sampleBuffer_.size(); ++i) {
		float absValue = std::abs(sampleBuffer_[i]);
		if (absValue > maxAbsValue) {
			maxAbsValue = absValue;
		}
	}

	return maxAbsValue;
}

//========================================
// ゲッター
//========================================

std::vector<float> MagVoiceBridge::GetSamples() {
	// PURPOSE: 現在のサンプルバッファをコピーして返す
	// REASON: 外部で波形表示など用途に音声データを使用する必要があるため
	std::lock_guard<std::mutex> lock(sampleMutex_);
	return sampleBuffer_;
}

void MagVoiceBridge::ClearSamples() {
	// PURPOSE: サンプルバッファをクリアする
	// REASON: メモリ肥大化防止と新しい記録開始時のリセット用
	std::lock_guard<std::mutex> lock(sampleMutex_);
	sampleBuffer_.clear();
}

float MagVoiceBridge::GetCurrentVolume() {
	// PURPOSE: 現在の RMS 音量を 0.0～1.0 で返す
	// REASON: UI 表示やゲーム処理で直感的に扱える正規化値が必要
	return currentVolume_.load();
}

float MagVoiceBridge::GetCurrentVolumeDB() {
	// PURPOSE: 現在の RMS 音量を dB 単位で返す
	// REASON: デシベル単位での詳細な音量分析に必要
	return currentVolumeDB_.load();
}

uint32_t MagVoiceBridge::GetSampleRate() const {
	// PURPOSE: オーディオフォーマットのサンプリングレートを返す
	// REASON: 周波数分析や時間計算に必須の情報
	// WARNING: Initialize() 前に呼び出すと 0 が返されます
	return sampleRate_;
}

uint16_t MagVoiceBridge::GetChannelCount() const {
	// PURPOSE: オーディオフォーマットのチャンネル数を返す
	// REASON: マルチチャンネル対応の処理判定に必要
	// WARNING: Initialize() 前に呼び出すと 0 が返されます
	return channelCount_;
}

bool MagVoiceBridge::IsVoiceDetected() {
	// PURPOSE: ゼロクロス率と音量から人の声かどうかを判定する
	// REASON: ノイズと音声を区別するため、複数指標を組み合わせた判定が正確

	// PURPOSE: ゼロクロス率を計算する
	// REASON: 周波数情報を提供（低周波 = 人の声、高周波 = ノイズ）
	float zeroCrossingRate = CalculateZeroCrossingRate();

	// PURPOSE: 音量（dB）を取得する
	// REASON: 無音や小さすぎる音を除外するため
	float volumeDB = GetCurrentVolumeDB();

	// PURPOSE: ノイズフロアより大きいかを確認する
	// REASON: 背景ノイズを除外するため
	if (volumeDB < noiseFloorDB_) {
		return false;
	}

	// PURPOSE: 人の声判定
	// REASON: 両方の条件を満たすときのみ音声と判定する必要がある
	// 条件 1: ゼロクロス率が閾値未満（高周波成分が少ない）
	// 条件 2: 音量が閾値以上（無音や小さすぎる音を除外）
	bool isVoice = (zeroCrossingRate < zcRateThreshold_) &&
				   (volumeDB > volumeThresholdDB_);

	return isVoice;
}

float MagVoiceBridge::GetPeakVolume() {
	// PURPOSE: ピークレベルを 0.0～1.0 で返す
	// REASON: ゲーム UI で音量のピークメーター表示に使用
	return peakVolume_.load();
}

float MagVoiceBridge::GetPeakVolumeDB() {
	// PURPOSE: ピークレベルを dB 単位で返す
	// REASON: dB 単位での詳細な音量分析に必要
	return peakVolumeDB_.load();
}

float MagVoiceBridge::GetSmoothedVolume() {
	// PURPOSE: スムージング済み音量を 0.0～1.0 で返す
	// REASON: UI のちらつき低減と視認性向上のため
	return smoothedVolume_.load();
}

float MagVoiceBridge::GetSmoothedVolumeDB() {
	// PURPOSE: スムージング済み音量を dB 単位で返す
	// REASON: UI のちらつき低減と dB 単位での安定した表示に必要
	return smoothedVolumeDB_.load();
}

float MagVoiceBridge::GetVolumePercentage() {
	// PURPOSE: 現在の音量をパーセンテージ（0～100）で返す
	// REASON: ゲーム UI でパーセンテージ表示する場合に直感的
	float volumeDB = currentVolumeDB_.load();

	// PURPOSE: dB値を 0～100% の範囲に正規化する
	// REASON: UI に合わせた直感的な表示にするため
	float normalized = (volumeDB - volumeMinDB_) / (volumeMaxDB_ - volumeMinDB_);
	normalized = std::clamp(normalized, 0.0f, 1.0f);

	return normalized * 100.0f;
}

float MagVoiceBridge::GetVoiceCharacteristicScore() {
	// PURPOSE: 音声特性スコアを返す（0.0～1.0）
	// REASON: 1.0に近いほど人の声、0.0に近いほどノイズ。判定根拠の可視化に使用
	return voiceCharacteristicScore_.load();
}

void MagVoiceBridge::SetSmoothingFactor(float factor) {
	// PURPOSE: スムージング係数を設定する（0.0～1.0）
	// REASON: リアルタイム性と安定性のバランスを調整するため
	// NOTE: デフォルト: 0.4。0.1=より反応的、0.9=より安定
	smoothingFactor_ = std::clamp(factor, 0.0f, 1.0f);
}

void MagVoiceBridge::SetVolumeRange(float minDb, float maxDb) {
	// PURPOSE: 音量の正規化範囲を設定する
	// REASON: マイク感度に応じた動的調整が必要
	// NOTE: GetVolumePercentage() の計算に使用されます
	if (minDb < maxDb) {
		volumeMinDB_ = minDb;
		volumeMaxDB_ = maxDb;
	}
}

void MagVoiceBridge::SetNoiseFloor(float noiseFloorDB) {
	// PURPOSE: ノイズフロアを設定する
	// REASON: 背景ノイズを除外するため、IsVoiceDetected() で使用
	// NOTE: この値より小さい音量は音声判定から除外されます
	noiseFloorDB_ = noiseFloorDB;
}

void MagVoiceBridge::SetVoiceDetectionThresholds(float zcRateThreshold, float volumeThresholdDB) {
	// PURPOSE: 音声検出の閾値パラメータを設定する
	// REASON: IsVoiceDetected() の判定精度を環境に応じて調整するため
	// NOTE: デフォルト: zcRate=0.25, volume=-40dB
	zcRateThreshold_ = std::clamp(zcRateThreshold, 0.0f, 1.0f);
	volumeThresholdDB_ = volumeThresholdDB;
}

MagVoiceBridge::VolumeStats MagVoiceBridge::GetVolumeStats() {
	// PURPOSE: 全ての音量統計情報を一括取得する
	// REASON: ImGui など UI 表示で複数の値を効率的に取得できる（atomic読み込みコスト削減）
	VolumeStats stats{};
	stats.currentRMS = currentVolume_.load();
	stats.currentRMSDB = currentVolumeDB_.load();
	stats.peakValue = peakVolume_.load();
	stats.peakDB = peakVolumeDB_.load();
	stats.smoothedRMS = smoothedVolume_.load();
	stats.smoothedRMSDB = smoothedVolumeDB_.load();
	stats.voiceScore = voiceCharacteristicScore_.load();
	stats.isVoiceDetected = isVoiceDetected_.load();

	// PURPOSE: パーセンテージを計算して追加する
	// REASON: 統計情報に含める必要があるため
	stats.percentage = GetVolumePercentage();

	return stats;
}
