// MagVoiceBridge.h
#pragma once

#include <atomic>
#include <audioclient.h>
#include <cstdint>
#include <mmdeviceapi.h>
#include <mutex>
#include <thread>
#include <vector>
#include <windows.h>

class MagVoiceBridge {
public:
	MagVoiceBridge();
	~MagVoiceBridge();

	// マイク入力システムを初期化する
	bool Initialize();

	// マイク録音を開始する
	bool Start();

	// マイク録音を停止する
	void Stop();

	// システムを終了する
	void Shutdown();

	// 現在保持している音声サンプルを取得する
	std::vector<float> GetSamples();

	// 保持しているサンプルをクリアする
	void ClearSamples();

private:
	// マイク入力を継続取得するループ
	void CaptureLoop();

private:
	IMMDeviceEnumerator *deviceEnumerator_ = nullptr;
	IMMDevice *captureDevice_ = nullptr;
	IAudioClient *audioClient_ = nullptr;
	IAudioCaptureClient *captureClient_ = nullptr;
	WAVEFORMATEX *waveFormat_ = nullptr;

	std::atomic<bool> isInitialized_ = false;
	std::atomic<bool> isCapturing_ = false;

	std::thread captureThread_;

	std::mutex sampleMutex_;
	std::vector<float> sampleBuffer_;
};