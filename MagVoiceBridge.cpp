// MagVoiceBridge.cpp
// 目的: WASAPI を使用したマイク入力のリアルタイム処理の実装
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "MagVoiceBridge.h"

#include <algorithm>
#include <cmath>
#include <combaseapi.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Uuid.lib")

// ===== コンストラクタ / デストラクタ =====

MagVoiceBridge::MagVoiceBridge() {
	// 目的: すべてのメンバ変数がコンストラクタ初期化リストで初期化される
}

MagVoiceBridge::~MagVoiceBridge() {
	// 目的: リソースを確実に解放するため Shutdown() を呼ぶ
	Shutdown();
}

// ===== 初期化 / シャットダウン =====

bool MagVoiceBridge::Initialize() {
	HRESULT result = S_OK;

	// 目的: COM ライブラリを初期化する
	// なぜ必要か: WASAPI を使用するには COM 初期化が必須
	result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(result)) {
		return false;
	}

	// 目的: デバイス列挙用オブジェクトを生成する
	// なぜ必要か: デフォルト録音デバイスを取得するため
	result = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void **>(&deviceEnumerator_));

	if (FAILED(result)) {
		return false;
	}

	// 目的: デフォルトの録音デバイスを取得する
	// なぜ必要か: ユーザが指定したマイクから入力を取得するため
	result = deviceEnumerator_->GetDefaultAudioEndpoint(
		eCapture,
		eConsole,
		&captureDevice_);

	if (FAILED(result)) {
		return false;
	}

	// 目的: 録音デバイスの IAudioClient インターフェースを取得する
	// なぜ必要か: WASAPI のオーディオクライアント機能を使用するため
	result = captureDevice_->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		reinterpret_cast<void **>(&audioClient_));

	if (FAILED(result)) {
		return false;
	}

	// 目的: マイクのネイティブオーディオフォーマットを取得する
	// なぜ必要か: サンプリングレート、チャンネル数、ビット深度の情報が必要
	result = audioClient_->GetMixFormat(&waveFormat_);
	if (FAILED(result)) {
		return false;
	}

	// 目的: オーディオフォーマット情報をメンバ変数に保存する
	// なぜ必要か: PCM データの正規化や時間計算で使用するため
	sampleRate_ = waveFormat_->nSamplesPerSec;
	channelCount_ = waveFormat_->nChannels;
	bitsPerSample_ = waveFormat_->wBitsPerSample;

	// 目的: フォーマット情報の妥当性をチェックする
	// なぜ必要か: 不正なフォーマットでは処理ができないため
	if (sampleRate_ == 0 || channelCount_ == 0) {
		return false;
	}

	// 目的: shared mode でオーディオクライアントを初期化する
	// なぜ必要か: 他のアプリと同時にマイクを使用できるようにするため
	// バッファ期間: 10000000 = 1秒（100ナノ秒単位）
	REFERENCE_TIME bufferDuration = 10000000;

	result = audioClient_->Initialize(
		AUDCLNT_SHAREMODE_SHARED,  // shared mode
		0,                          // フラグなし
		bufferDuration,             // バッファ期間
		0,                          // デバイス期間（shared mode では 0）
		waveFormat_,                // オーディオフォーマット
		nullptr);                   // GUID なし

	if (FAILED(result)) {
		return false;
	}

	// 目的: IAudioCaptureClient インターフェースを取得する
	// なぜ必要か: GetBuffer で PCM データを取得するため
	result = audioClient_->GetService(
		__uuidof(IAudioCaptureClient),
		reinterpret_cast<void **>(&captureClient_));

	if (FAILED(result)) {
		return false;
	}

	// 目的: 初期化完了フラグを設定する
	isInitialized_ = true;
	return true;
}

bool MagVoiceBridge::Start() {
	// 目的: 初期化が完了していることを確認する
	// なぜ必要か: Start() 前に Initialize() を呼ぶ必要があるため
	if (!isInitialized_) {
		return false;
	}

	// 目的: オーディオストリームを開始する
	// なぜ必要か: WASAPI からのデータキャプチャを開始するため
	HRESULT result = audioClient_->Start();
	if (FAILED(result)) {
		return false;
	}

	// 目的: キャプチャ中フラグを設定する
	isCapturing_ = true;
	return true;
}

void MagVoiceBridge::Stop() {
	// 目的: オーディオストリームを停止する
	// なぜ必要か: キャプチャを一時停止するため
	if (audioClient_ && isCapturing_) {
		audioClient_->Stop();
		isCapturing_ = false;
	}
}

void MagVoiceBridge::Shutdown() {
	// 目的: キャプチャを停止する
	// なぜ必要か: リソース解放前にストリームを止める必要があるため
	Stop();

	// 目的: COM インターフェースをリリースする
	// なぜ必要か: メモリリークを防ぐため
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

	// 目的: waveFormat_ のメモリを解放する
	// なぜ必要か: CoTaskMemFree で確保されたメモリを解放するため
	if (waveFormat_) {
		CoTaskMemFree(waveFormat_);
		waveFormat_ = nullptr;
	}

	// 目的: COM ライブラリをアンニューション化する
	// なぜ必要か: COM 初期化の対応処理として必須
	CoUninitialize();

	// 目的: 初期化フラグをリセットする
	isInitialized_ = false;
	isCapturing_ = false;
}

// ===== メイン処理 =====

void MagVoiceBridge::Update() {
	// 目的: キャプチャ中でない場合は何もしない
	// なぜ必要か: Start() 前に Update() が呼ばれることを想定
	if (!isCapturing_) {
		return;
	}

	// 目的: WASAPI からオーディオデータを取得する
	CaptureAudioData();

	// 目的: 最新のサンプルから音量を計算する
	float rmsLevel = CalculateRMSLevel();
	currentVolume_ = rmsLevel;

	// 目的: RMS をデシベルに変換する
	// なぜ必要か: dB 単位での音量表示に必要
	// 計算: dB = 20 * log10(RMS)
	// 防止: log(0) エラーを避けるため最小値 0.00001f を使用
	float volumeDB = 20.0f * std::log10(std::max(0.00001f, rmsLevel));
	currentVolumeDB_ = volumeDB;

	// 目的: ピークレベルを計算する（最大振幅）
	// なぜ必要か: 瞬間的な最大音量を把握するため
	float peakLevel = CalculatePeakLevel();
	peakVolume_ = peakLevel;
	
	// 目的: ピークレベルを dB に変換する
	float peakDB = 20.0f * std::log10(std::max(0.00001f, peakLevel));
	peakVolumeDB_ = peakDB;

	// 目的: スムージング処理を適用する
	// なぜ必要か: リアルタイム値の変動を平滑化し、UIちらつきを防ぐため
	float currentSmoothed = smoothedVolume_.load();
	float newSmoothed = currentSmoothed * (1.0f - smoothingFactor_) + rmsLevel * smoothingFactor_;
	smoothedVolume_ = newSmoothed;

	float currentSmoothedDB = smoothedVolumeDB_.load();
	float newSmoothedDB = currentSmoothedDB * (1.0f - smoothingFactor_) + volumeDB * smoothingFactor_;
	smoothedVolumeDB_ = newSmoothedDB;

	// 目的: 音声特性スコアを計算する
	// なぜ必要か: ゼロクロス率に基づいて人の声らしさを定量化するため
	float zeroCrossingRate = CalculateZeroCrossingRate();
	// スコア: ゼロクロス率が低い（0.0に近い）ほど高スコア（1.0に近い）
	// 例: ZCR=0.1 → スコア=0.6, ZCR=0.3 → スコア=0.2
	float voiceScore = std::max(0.0f, 1.0f - (zeroCrossingRate / ZERO_CROSSING_RATE_THRESHOLD));
	voiceCharacteristicScore_ = voiceScore;

	// 目的: ゼロクロス率と音量から人の声かどうかを判定する
	bool voiceDetected = IsVoiceDetected();
	isVoiceDetected_ = voiceDetected;
}

void MagVoiceBridge::CaptureAudioData() {
	// 目的: WASAPI バッファに利用可能なフレーム数を取得する
	// なぜ必要か: GetBuffer で取得すべきデータサイズを判定するため
	uint32_t packetLength = 0;
	HRESULT result = captureClient_->GetNextPacketSize(&packetLength);
	
	if (FAILED(result) || packetLength == 0) {
		return;  // データが利用できない場合は処理をスキップ
	}

	// 目的: WASAPI バッファからオーディオデータを取得する
	// なぜ必要か: PCM サンプルにアクセスするため
	BYTE *pData = nullptr;
	DWORD flags = 0;
	uint32_t numFramesAvailable = 0;

	result = captureClient_->GetBuffer(
		&pData,                 // データへのポインタ
		&numFramesAvailable,    // 利用可能なフレーム数
		&flags,                 // フラグ（サイレンス判定など）
		nullptr,                // デバイス位置（不使用）
		nullptr);               // バッファ位置（不使用）

	if (FAILED(result) || !pData) {
		return;
	}

	// 目的: フレーム数を確認する
	packetLength = numFramesAvailable;
	// なぜ必要か: 内部処理を統一した形式で行うため
	{
		std::lock_guard<std::mutex> lock(sampleMutex_);

		// 目的: WASAPI は通常 32-bit float でデータを返すため、それに対応する
		// なぜ必要か: GetMixFormat() で取得したフォーマットを正確に処理するため
		// NOTE: WASAPI Shared Mode では必ず 32-bit float に変換される
		float *pFloatData = reinterpret_cast<float *>(pData);
		
		// 目的: マルチチャンネルの場合、全チャンネルを合成または選択する
		// なぜ必要か: モノラル処理に統一するため（ステレオの場合は左右の平均値を取る）
		for (uint32_t frame = 0; frame < packetLength; ++frame) {
			float channelAverage = 0.0f;
			
			// 目的: 全チャンネルの値を加算する
			for (uint16_t ch = 0; ch < channelCount_; ++ch) {
				uint32_t sampleIndex = frame * channelCount_ + ch;
				channelAverage += std::abs(pFloatData[sampleIndex]);
			}
			
			// 目的: チャンネル数で平均化してモノラルに統一する
			channelAverage /= static_cast<float>(channelCount_);
			
			// 目的: サンプルをバッファに追加する
			// NOTE: WASAPI が返すのは既に正規化された [-1.0, 1.0] 範囲の値
			sampleBuffer_.push_back(channelAverage);
		}

		// 目的: サンプルバッファが肥大化しないようサイズを制限する
		// なぜ必要か: メモリ使用量を抑えるため
		// 制限: 最大 10 秒分（480,000 サンプル @ 48kHz）
		if (sampleBuffer_.size() > MAX_SAMPLE_BUFFER_SIZE) {
			// 目的: バッファの最後の MAX_SAMPLE_BUFFER_SIZE サンプルのみを保持する
			// なぜ必要か: メモリ使用量を制限するため
			sampleBuffer_.erase(
				sampleBuffer_.begin(),
				sampleBuffer_.end() - MAX_SAMPLE_BUFFER_SIZE);
		}
	}

	// 目的: WASAPI バッファを解放する
	// なぜ必要か: GetBuffer で確保したバッファを使用済みにマークするため
	captureClient_->ReleaseBuffer(packetLength);
}

float MagVoiceBridge::CalculateZeroCrossingRate() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// 目的: サンプルバッファが空の場合は 0 を返す
	if (sampleBuffer_.size() < 2) {
		return 0.0f;
	}

	// 目的: 最新の 2048 サンプルでゼロクロス率を計算する
	// なぜ必要か: リアルタイム判定のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW 
		? sampleBuffer_.size() - ANALYSIS_WINDOW 
		: 0;

	// 目的: ゼロクロッシング（符号変化）の回数を数える
	// なぜ必要か: 周波数情報を得るため（高周波はゼロクロッシング多い）
	int zeroCrossingCount = 0;
	for (size_t i = startIdx + 1; i < sampleBuffer_.size(); ++i) {
		// 目的: 前後のサンプルで符号が変わっているかを確認する
		if ((sampleBuffer_[i - 1] < 0.0f && sampleBuffer_[i] >= 0.0f) ||
			(sampleBuffer_[i - 1] >= 0.0f && sampleBuffer_[i] < 0.0f)) {
			zeroCrossingCount++;
		}
	}

	// 目的: ゼロクロス率を計算する（0.0～1.0）
	// なぜ必要か: 周波数特性を定量化するため
	// ゼロクロス率 = (ゼロクロッシング数) / (サンプル数 - 1)
	size_t sampleCount = sampleBuffer_.size() - startIdx;
	float zeroCrossingRate = static_cast<float>(zeroCrossingCount) / static_cast<float>(sampleCount);

	return zeroCrossingRate;
}

float MagVoiceBridge::CalculateRMSLevel() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// 目的: サンプルバッファが空の場合は 0 を返す
	if (sampleBuffer_.empty()) {
		return 0.0f;
	}

	// 目的: 最新の 2048 サンプルで RMS を計算する
	// なぜ必要か: リアルタイム音量表示のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW 
		? sampleBuffer_.size() - ANALYSIS_WINDOW 
		: 0;

	// 目的: RMS（二乗平均平方根）を計算する
	// なぜ必要か: 音量を正確に計測するため
	// 計算: RMS = sqrt( sum(sample^2) / N )
	float sumOfSquares = 0.0f;
	for (size_t i = startIdx; i < sampleBuffer_.size(); ++i) {
		float sample = sampleBuffer_[i];
		sumOfSquares += sample * sample;
	}

	size_t sampleCount = sampleBuffer_.size() - startIdx;
	float rmsLevel = std::sqrt(sumOfSquares / static_cast<float>(sampleCount));

	return rmsLevel;
}

// 目的: 最大振幅（ピークレベル）を計算する
// なぜ必要か: 瞬間的な最大音量を把握するため
float MagVoiceBridge::CalculatePeakLevel() {
	std::lock_guard<std::mutex> lock(sampleMutex_);

	// 目的: サンプルバッファが空の場合は 0 を返す
	if (sampleBuffer_.empty()) {
		return 0.0f;
	}

	// 目的: 最新の 2048 サンプルで最大振幅を計算する
	// なぜ必要か: リアルタイム表示のため全サンプルではなく最新部分を使用
	const size_t ANALYSIS_WINDOW = 2048;
	const size_t startIdx = sampleBuffer_.size() > ANALYSIS_WINDOW 
		? sampleBuffer_.size() - ANALYSIS_WINDOW 
		: 0;

	// 目的: 最大振幅を見つける
	float maxAbsValue = 0.0f;
	for (size_t i = startIdx; i < sampleBuffer_.size(); ++i) {
		float absValue = std::abs(sampleBuffer_[i]);
		if (absValue > maxAbsValue) {
			maxAbsValue = absValue;
		}
	}

	return maxAbsValue;
}

// ===== ゲッター =====

std::vector<float> MagVoiceBridge::GetSamples() {
	// 目的: 現在のサンプルバッファをコピーして返す
	// なぜ必要か: 外部で波形表示など用途に音声データを使用するため
	// thread-safe: mutex で保護
	std::lock_guard<std::mutex> lock(sampleMutex_);
	return sampleBuffer_;
}

void MagVoiceBridge::ClearSamples() {
	// 目的: サンプルバッファをクリアする
	// なぜ必要か: 新しい記録開始時やメモリ管理の目的
	// thread-safe: mutex で保護
	std::lock_guard<std::mutex> lock(sampleMutex_);
	sampleBuffer_.clear();
}

float MagVoiceBridge::GetCurrentVolume() {
	// 目的: 現在の RMS 音量を 0.0～1.0 で返す
	// なぜ必要か: 正規化された音量値をゲッターから取得するため
	// thread-safe: atomic で保護
	return currentVolume_.load();
}

float MagVoiceBridge::GetCurrentVolumeDB() {
	// 目的: 現在の RMS 音量を dB で返す
	// なぜ必要か: デシベル単位での詳細な音量分析に必要
	// thread-safe: atomic で保護
	return currentVolumeDB_.load();
}

uint32_t MagVoiceBridge::GetSampleRate() const {
	// 目的: オーディオフォーマットのサンプリングレートを返す
	// なぜ必要か: 時間計算や周波数分析で必要
	return sampleRate_;
}

uint16_t MagVoiceBridge::GetChannelCount() const {
	// 目的: オーディオフォーマットのチャンネル数を返す
	// なぜ必要か: マルチチャンネル対応で必要
	return channelCount_;
}

bool MagVoiceBridge::IsVoiceDetected() {
	// 目的: ゼロクロス率と音量から人の声かどうかを判定する
	// なぜ必要か: ノイズと音声を区別するため
	
	// 目的: ゼロクロス率を計算する
	// ゼロクロス率: 周波数情報を提供（低周波＝人の声）
	float zeroCrossingRate = CalculateZeroCrossingRate();
	
	// 目的: 音量（dB）を取得する
	// なぜ必要か: 無音や小さすぎる音を除外するため
	float volumeDB = GetCurrentVolumeDB();
	
	// 目的: ノイズフロアより大きいかを確認する
	// なぜ必要か: 背景ノイズを除外するため
	if (volumeDB < noiseFloorDB_) {
		return false;
	}
	
	// 目的: 人の声判定
	// なぜ必要か: 両方の条件を満たすときのみ音声と判定
	// 条件 1: ゼロクロス率が低い（高周波成分が少ない）
	//        → 人の声は低周波が多く、ノイズは高周波が多い
	// 条件 2: 音量が閾値以上（無音や小さすぎる音を除外）
	bool isVoice = (zeroCrossingRate < zcRateThreshold_) &&
	               (volumeDB > volumeThresholdDB_);
	
	return isVoice;
}

float MagVoiceBridge::GetPeakVolume() {
	// 目的: ピークレベルを 0.0～1.0 で返す
	// なぜ必要か: ゲームUIで瞬間的な最大音量を表示するため
	// thread-safe: atomic で保護
	return peakVolume_.load();
}

float MagVoiceBridge::GetPeakVolumeDB() {
	// 目的: ピークレベルを dB で返す
	// なぜ必要か: dB単位での詳細な音量分析に必要
	// thread-safe: atomic で保護
	return peakVolumeDB_.load();
}

float MagVoiceBridge::GetSmoothedVolume() {
	// 目的: スムージング済み音量を 0.0～1.0 で返す
	// なぜ必要か: UI表示のちらつきを低減するため
	// thread-safe: atomic で保護
	return smoothedVolume_.load();
}

float MagVoiceBridge::GetSmoothedVolumeDB() {
	// 目的: スムージング済み音量を dB で返す
	// なぜ必要か: UI表示の安定性を確保するため
	// thread-safe: atomic で保護
	return smoothedVolumeDB_.load();
}

float MagVoiceBridge::GetVolumePercentage() {
	// 目的: 現在の音量をパーセンテージ（0～100）で返す
	// なぜ必要か: ゲームUIでパーセンテージ形式で表示する場合に便利
	float volumeDB = currentVolumeDB_.load();
	
	// 目的: dB値を 0～100% の範囲に正規化する
	// なぜ必要か: UIに合わせた直感的な表示にするため
	float normalized = (volumeDB - volumeMinDB_) / (volumeMaxDB_ - volumeMinDB_);
	normalized = std::clamp(normalized, 0.0f, 1.0f);
	
	return normalized * 100.0f;
}

float MagVoiceBridge::GetVoiceCharacteristicScore() {
	// 目的: 音声特性スコアを返す（0.0～1.0）
	// なぜ必要か: 1.0に近いほど人の声、0.0に近いほどノイズ
	// thread-safe: atomic で保護
	return voiceCharacteristicScore_.load();
}

void MagVoiceBridge::SetSmoothingFactor(float factor) {
	// 目的: スムージング係数を設定する
	// なぜ必要か: リアルタイム性と安定性のバランスを調整するため
	// クランプ: 0.0～1.0 の有効な範囲に制限
	smoothingFactor_ = std::clamp(factor, 0.0f, 1.0f);
}

void MagVoiceBridge::SetVolumeRange(float minDb, float maxDb) {
	// 目的: 音量の正規化範囲を設定する
	// なぜ必要か: マイク感度に応じて動的に調整するため
	// 妥当性チェック: 最小値が最大値を上回らないようにする
	if (minDb < maxDb) {
		volumeMinDB_ = minDb;
		volumeMaxDB_ = maxDb;
	}
}

void MagVoiceBridge::SetNoiseFloor(float noiseFloorDB) {
	// 目的: ノイズフロアを設定する
	// なぜ必要か: 背景ノイズを除外するため
	noiseFloorDB_ = noiseFloorDB;
}

void MagVoiceBridge::SetVoiceDetectionThresholds(float zcRateThreshold, float volumeThresholdDB) {
	// 目的: 音声検出の閾値をカスタマイズする
	// なぜ必要か: 使用環境に応じて判定精度を調整するため
	zcRateThreshold_ = std::clamp(zcRateThreshold, 0.0f, 1.0f);
	volumeThresholdDB_ = volumeThresholdDB;
}

MagVoiceBridge::VolumeStats MagVoiceBridge::GetVolumeStats() {
	// 目的: 全ての音量統計情報を一括取得する
	// なぜ必要か: ImGuiなどで複数の値を効率的に表示するため
	// thread-safe: atomic値を個別に読み込み
	VolumeStats stats{};
	stats.currentRMS = currentVolume_.load();
	stats.currentRMSDB = currentVolumeDB_.load();
	stats.peakValue = peakVolume_.load();
	stats.peakDB = peakVolumeDB_.load();
	stats.smoothedRMS = smoothedVolume_.load();
	stats.smoothedRMSDB = smoothedVolumeDB_.load();
	stats.voiceScore = voiceCharacteristicScore_.load();
	stats.isVoiceDetected = isVoiceDetected_.load();
	
	// 目的: パーセンテージを計算して追加する
	stats.percentage = GetVolumePercentage();
	
	return stats;
}
