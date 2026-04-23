// MagVoiceBridge.h
// 目的: WASAPI を使用してデフォルトマイクから音声入力を取得し、
//      リアルタイム音声分析（音量・音声検出）を行うクラス
#pragma once

#include <atomic>
#include <audioclient.h>
#include <cstdint>
#include <mmdeviceapi.h>
#include <mutex>
#include <vector>
#include <windows.h>

class MagVoiceBridge {
public:
	MagVoiceBridge();
	~MagVoiceBridge();

	// 目的: WASAPI を初期化し、デフォルト録音デバイスを取得する
	// なぜ必要か: COM インターフェース初期化と WASAPI デバイスの準備に必須
	bool Initialize();

	// 目的: 音声キャプチャを開始する（shared mode）
	// なぜ必要か: WASAPI のオーディオクライアントを開始するため
	bool Start();

	// 目的: 音声キャプチャを停止する
	// なぜ必要か: キャプチャを一時停止するため
	void Stop();

	// 目的: WASAPI リソースを解放し、システムをクリーンアップする
	// なぜ必要か: COM インターフェースの適切なリリースとメモリ解放のため
	void Shutdown();

	// 目的: WASAPI から最新のオーディオデータを取得し、サンプルバッファに追加する
	// なぜ必要か: 毎フレーム呼び出してリアルタイムに音声データを更新するため
	void Update();

	// 目的: 現在保持している音声サンプルを返す
	// なぜ必要か: 外部で波形表示などの用途に音声データを使用するため
	std::vector<float> GetSamples();

	// 目的: 保持しているサンプルバッファをクリアする
	// なぜ必要か: メモリ肥大化防止と新しい記録開始時のリセット用
	void ClearSamples();

	// 目的: 現在の音量を RMS で計算し、0.0～1.0 で返す
	// なぜ必要か: リアルタイム音量表示に必要
	float GetCurrentVolume();

	// 目的: 現在の音量を dB 単位で返す
	// なぜ必要か: デシベル単位での詳細な音量分析に必要
	float GetCurrentVolumeDB();

	// 目的: サンプリングレートを取得する
	// なぜ必要か: 周波数分析や時間計算で必要
	uint32_t GetSampleRate() const;

	// 目的: チャンネル数を取得する
	// なぜ必要か: 音声フォーマット情報の確認に必要
	uint16_t GetChannelCount() const;

	// 目的: ゼロクロス率と音量を用いて人の声らしさを簡易判定する
	// なぜ必要か: ノイズと音声を区別するため
	// 返り値: true = 人の声の可能性あり, false = 音声未検出
	bool IsVoiceDetected();

	// 目的: 最新のサンプルデータからゼロクロス率を計算する（デバッグ用）
	// なぜ必要か: 人の声判定の指標として使用するため
	float CalculateZeroCrossingRate();

	// 目的: ピークレベル（最大振幅）を 0.0～1.0 で返す
	// なぜ必要か: ゲームUIで音量の瞬間的な最大値を表示するため
	float GetPeakVolume();

	// 目的: ピークレベルを dB 単位で返す
	// なぜ必要か: dB単位での詳細な音量分析に必要
	float GetPeakVolumeDB();

	// 目的: スムージング（移動平均）された音量を取得する
	// なぜ必要か: ジッターを低減し、ゲーム表示を滑らかにするため
	float GetSmoothedVolume();

	// 目的: スムージングされた音量を dB 単位で返す
	// なぜ必要か: dB単位での安定した音量表示に必要
	float GetSmoothedVolumeDB();

	// 目的: 音量をパーセンテージ（0～100）で返す
	// なぜ必要か: ゲームUIでパーセンテージ表示する場合に便利
	float GetVolumePercentage();

	// 目的: ゼロクロス率を利用した音声の周波数特性スコア（0.0～1.0）を返す
	// なぜ必要か: 人の声らしさを数値化するため（1.0に近いほど人の声）
	float GetVoiceCharacteristicScore();

	// 目的: スムージング係数を設定する（0.0～1.0）
	// なぜ必要か: リアルタイム性と安定性のバランスを調整するため
	// 高い値 = より安定（応答遅延あり）、低い値 = より反応的
	void SetSmoothingFactor(float factor);

	// 目的: 音量の正規化範囲を設定する
	// なぜ必要か: マイク感度に応じて使用可能な音量範囲を動的に調整するため
	// minDb: 最小基準（例：-80dB = 無音）
	// maxDb: 最大基準（例：-6dB = 大声）
	void SetVolumeRange(float minDb, float maxDb);

	// 目的: ノイズフロア（最小検出音量）を設定する
	// なぜ必要か: 背景ノイズを除外し、音量判定を改善するため
	void SetNoiseFloor(float noiseFloorDB);

	// 目的: 音声検出の閾値パラメータを設定する
	// なぜ必要か: 使用環境に応じて判定精度を調整するため
	// zcRateThreshold: ゼロクロス率の閾値（通常 0.15～0.35）
	// volumeThresholdDB: 音量の閾値（通常 -50～-30dB）
	void SetVoiceDetectionThresholds(float zcRateThreshold, float volumeThresholdDB);

	// 目的: 現在の音量統計情報を取得する
	// なぜ必要か: ゲーム内で音量の統計情報を利用するため
	struct VolumeStats {
		float currentRMS{};      // 現在の RMS 値（0.0～1.0）
		float currentRMSDB{};    // 現在の RMS 値（dB）
		float peakValue{};       // ピーク値（0.0～1.0）
		float peakDB{};          // ピーク値（dB）
		float smoothedRMS{};     // スムージング済み RMS（0.0～1.0）
		float smoothedRMSDB{};   // スムージング済み RMS（dB）
		float percentage{};      // パーセンテージ（0～100）
		float voiceScore{};      // 音声特性スコア（0.0～1.0）
		bool isVoiceDetected{};  // 音声検出フラグ
	};

	// 目的: 音量統計情報を一括取得する
	// なぜ必要か: ImGuiなどで複数の値を効率的に表示するため
	VolumeStats GetVolumeStats();

private:
	// 目的: WASAPI からオーディオデータを取得し、サンプルバッファに追加する
	// なぜ必要か: PCM データの取得と正規化処理を行うため
	void CaptureAudioData();

	// 目的: 最新のサンプルデータから RMS を計算する
	// なぜ必要か: 正確な音量計測に必要
	float CalculateRMSLevel();

	// 目的: 最新のサンプルデータから最大振幅（ピークレベル）を計算する
	// なぜ必要か: 瞬間的な最大音量を把握するため
	float CalculatePeakLevel();

private:
	// ===== COM インターフェース =====
	// 目的: デバイスの列挙に使用
	// なぜ必要か: デフォルト録音デバイスを取得するため
	IMMDeviceEnumerator *deviceEnumerator_ = nullptr;
	
	// 目的: デフォルト録音デバイスを表す
	// なぜ必要か: マイク入力にアクセスするため
	IMMDevice *captureDevice_ = nullptr;
	
	// 目的: オーディオストリームを制御する
	// なぜ必要か: GetMixFormat、Initialize、Start/Stop を呼ぶため
	IAudioClient *audioClient_ = nullptr;
	
	// 目的: キャプチャされたデータを取得する
	// なぜ必要か: GetBuffer で PCM データを取得するため
	IAudioCaptureClient *captureClient_ = nullptr;
	
	// 目的: WASAPI が使用するオーディオフォーマット
	// なぜ必要か: サンプルレート、チャンネル数、ビット深度の情報を保持するため
	WAVEFORMATEX *waveFormat_ = nullptr;

	// ===== 制御フラグ =====
	// 目的: Initialize() が完了したかを示す
	// なぜ atomic か: メインスレッドからアクセスするため
	std::atomic<bool> isInitialized_ = false;
	
	// 目的: Start() が完了し、キャプチャ中かを示す
	// なぜ atomic か: メインスレッドからアクセスするため
	std::atomic<bool> isCapturing_ = false;

	// ===== サンプルバッファ管理 =====
	// 目的: スレッド安全なアクセスのためのミューテックス
	// なぜ必要か: Update() と GetSamples() 間の競合を回避するため
	std::mutex sampleMutex_;
	
	// 目的: キャプチャされた音声サンプル（正規化済み float [-1.0f, 1.0f]）
	// なぜ必要か: 波形表示や分析に使用するため
	std::vector<float> sampleBuffer_;

	// ===== オーディオフォーマット情報 =====
	// 目的: GetMixFormat() から取得したサンプリングレート（Hz）
	// なぜ必要か: 時間計算や周波数分析に必要
	uint32_t sampleRate_ = 0;
	
	// 目的: GetMixFormat() から取得したチャンネル数
	// なぜ必要か: マルチチャンネル対応に必要
	uint16_t channelCount_ = 0;
	
	// 目的: GetMixFormat() から取得したビット深度
	// なぜ必要か: PCM データの正規化計算に必要（16bit なら 32768.0f で除算）
	uint16_t bitsPerSample_ = 0;

	// ===== リアルタイム音量情報 =====
	// 目的: 現在の RMS レベル（0.0～1.0、スレッドセーフ）
	// なぜ atomic か: Update() と GetCurrentVolume() 間での競合回避
	std::atomic<float> currentVolume_ = 0.0f;
	
	// 目的: 現在の RMS レベル（dB、スレッドセーフ）
	// なぜ atomic か: Update() と GetCurrentVolumeDB() 間での競合回避
	std::atomic<float> currentVolumeDB_ = -80.0f;

	// 目的: ピークレベル（最大振幅、スレッドセーフ）
	// なぜ atomic か: Update() と GetPeakVolume() 間での競合回避
	std::atomic<float> peakVolume_ = 0.0f;

	// 目的: ピークレベル（dB、スレッドセーフ）
	// なぜ atomic か: Update() と GetPeakVolumeDB() 間での競合回避
	std::atomic<float> peakVolumeDB_ = -80.0f;

	// 目的: スムージング済み RMS レベル（0.0～1.0、スレッドセーフ）
	// なぜ atomic か: Update() と GetSmoothedVolume() 間での競合回避
	std::atomic<float> smoothedVolume_ = 0.0f;

	// 目的: スムージング済み RMS レベル（dB、スレッドセーフ）
	// なぜ atomic か: Update() と GetSmoothedVolumeDB() 間での競合回避
	std::atomic<float> smoothedVolumeDB_ = -80.0f;

	// 目的: 音声特性スコア（0.0～1.0、スレッドセーフ）
	// なぜ atomic か: Update() と GetVoiceCharacteristicScore() 間での競合回避
	std::atomic<float> voiceCharacteristicScore_ = 0.0f;

	// ===== 音声検出パラメータ =====
	// 目的: 人の声判定フラグ（スレッドセーフ）
	// なぜ atomic か: Update() と IsVoiceDetected() 間での競合回避
	std::atomic<bool> isVoiceDetected_ = false;

	// ===== サンプルバッファ肥大化防止 =====
	// 目的: バッファサイズの最大値（サンプル数）
	// なぜ必要か: メモリ肥大化を防止するため（例: 10秒分のサンプル）
	// 計算: 48kHz * 10秒 = 480,000 サンプル
	static constexpr size_t MAX_SAMPLE_BUFFER_SIZE = 480000;

	// ===== 音声検出パラメータ =====
	// 目的: 人の声判定のための閾値（ゼロクロス率）
	// なぜ必要か: ノイズ（高周波）と音声（低周波）を区別するため
	// 典型値: 音声は 10～20%、ノイズは 30～50%
	static constexpr float ZERO_CROSSING_RATE_THRESHOLD = 0.25f;
	
	// 目的: 人の声判定のための閾値（RMS 音量）
	// なぜ必要か: 無音や小さすぎる音を除外するため
	// 典型値: -40dB （正規化値の約 0.01）
	static constexpr float VOICE_VOLUME_THRESHOLD_DB = -40.0f;

	// ===== スムージング関連パラメータ =====
	// 目的: 音量スムージングの係数（0.0～1.0）
	// なぜ必要か: リアルタイム音量の変動を平滑化するため
	// 値: 0.1 = より反応的、0.9 = より安定
	float smoothingFactor_ = 0.4f;

	// 目的: 音量正規化の最小値（dB）
	// なぜ必要か: マイク感度の下限を定義するため
	float volumeMinDB_ = -60.0f;

	// 目的: 音量正規化の最大値（dB）
	// なぜ必要か: マイク感度の上限を定義するため
	float volumeMaxDB_ = -6.0f;

	// 目的: ノイズフロア（最小検出音量）
	// なぜ必要か: 背景ノイズを除外するため
	// 値: -60dB 以下は完全に無視
	float noiseFloorDB_ = -50.0f;

	// 目的: ゼロクロス率の閾値（調整可能）
	// なぜ必要か: 使用環境に応じて音声判定を最適化するため
	// デフォルト: 0.25（元の定数値）
	float zcRateThreshold_ = 0.25f;

	// 目的: 音量判定の閾値（調整可能）
	// なぜ必要か: 使用環境に応じて音声判定を最適化するため
	// デフォルト: -40dB（元の定数値）
	float volumeThresholdDB_ = -40.0f;
};
