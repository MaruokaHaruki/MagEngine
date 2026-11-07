/*********************************************************************
* \file   MAudioG.cpp
* \brief
*  _____ _____ _____
* |     |     |   __| MAudioG.h
* | | | |  |  |  |_ | Ver4.00
* |_|_|_|__|__|_____| 2024/09/23
*
* \author Harukichimaru
* \date   January 2025
* \note
*********************************************************************/

#define NOMINMAX
#include "MAudioG.h"
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						シングルトンインスタンスの取得
	MAudioG *MAudioG::GetInstance() {
		static MAudioG instance;
		return &instance;
	}

	///=============================================================================
	///						デバイスの取得
	void MAudioG::GetAudioDevices() {
		// 省略（既存の実装）
	}

	///=============================================================================
	///						初期化
	void MAudioG::Initialize(const std::string &directoryPath, const std::wstring &deviceId) {
		Finalize();
		std::lock_guard<std::mutex> lock(voiceMutex_);
		HRESULT result;

		this->directoryPath_ = directoryPath;

		result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if(FAILED(result)) {
			std::cerr << "Failed to initialize XAudio2: " << std::hex << result << std::endl;
			return; // 初期化に失敗したら終了
		}

		result = xAudio2_->CreateMasteringVoice(&masterVoice_, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, deviceId.empty() ? nullptr : deviceId.c_str(), nullptr);
		if(FAILED(result)) {
			std::cerr << "Failed to create mastering voice: " << std::hex << result << std::endl;
			xAudio2_.Reset();
			return; // 初期化に失敗したら終了
		}

		waveSamplingRate = 44100.0f;
	}

	///=============================================================================
	///						終了処理
	void MAudioG::Finalize() {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		// NOTE: Voiceの破棄が参照終了を待ってから音声データを解放する。
		voiceMap_.clear();

		//========================================
		// ロードされたサウンドデータをクリア
		soundDataMap_.clear();

		//========================================
		// マスターボイスを破棄
		if(masterVoice_) {
			masterVoice_->DestroyVoice();
			masterVoice_ = nullptr;
		}
		//========================================
		// XAudio2のインスタンスをリセット
		xAudio2_.Reset();
	}

	///--------------------------------------------------------------
	/// PCM形式の検証
	namespace {
		void ValidatePcm(const WAVEFORMATEX &format, const std::string &name) {
			if(format.wFormatTag != WAVE_FORMAT_PCM || format.cbSize != 0 ||
			   (format.nChannels != 1 && format.nChannels != 2) ||
			   (format.wBitsPerSample != 8 && format.wBitsPerSample != 16) ||
			   format.nSamplesPerSec < XAUDIO2_MIN_SAMPLE_RATE || format.nSamplesPerSec > XAUDIO2_MAX_SAMPLE_RATE ||
			   format.nBlockAlign != format.nChannels * (format.wBitsPerSample / 8) ||
			   uint64_t(format.nAvgBytesPerSec) != uint64_t(format.nSamplesPerSec) * format.nBlockAlign) {
				throw std::runtime_error("WAV " + name + ": unsupported or inconsistent PCM format");
			}
		}
	}

	///=============================================================================
	/// デバイス非依存のWAV読み込み
	MAudioG::SoundData MAudioG::ReadWav(const std::string &filePath) {
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		auto fail = [&filePath](const char *reason) { throw std::runtime_error("WAV " + filePath + ": " + reason); };
		if(!file) { fail("cannot open file"); }
		const auto fileSize = file.tellg();
		if(fileSize < 12) { fail("truncated RIFF header"); }
		file.seekg(0);
		auto read = [&](void *destination, size_t bytes) {
			if(!file.read(static_cast<char *>(destination), static_cast<std::streamsize>(bytes))) {
				fail("truncated chunk");
			}
		};
		RiffHeader riff{};
		read(&riff, sizeof(riff));
		if(std::memcmp(riff.chunk.id, "RIFF", 4) || std::memcmp(riff.type, "WAVE", 4) || riff.chunk.size < 4) {
			fail("invalid RIFF/WAVE header");
		}
		const uint64_t riffEnd = uint64_t(riff.chunk.size) + 8;
		if(riffEnd > static_cast<uint64_t>(fileSize)) { fail("RIFF size exceeds file"); }

		//========================================
		// チャンク境界を先に検証し、成功するまでキャッシュへ公開しない
		SoundData result{};
		result.name = filePath;
		bool hasFormat = false;
		bool hasData = false;
		uint64_t position = 12;
		while(position < riffEnd) {
			if(riffEnd - position < sizeof(ChunkHeader)) { fail("truncated chunk header"); }
			ChunkHeader chunk{};
			read(&chunk, sizeof(chunk));
			position += sizeof(chunk);
			const uint64_t paddedSize = uint64_t(chunk.size) + (chunk.size & 1u);
			if(paddedSize > riffEnd - position) { fail("chunk size exceeds RIFF boundary"); }
			if(std::memcmp(chunk.id, "fmt ", 4) == 0) {
				if(hasFormat || (chunk.size != 16 && chunk.size != 18)) { fail("invalid or unsupported fmt size"); }
				// NOTE: WindowsのPCM先頭16バイトだけを読み、拡張領域の上書きを防ぐ。
				static_assert(offsetof(WAVEFORMATEX, cbSize) == 16);
				read(&result.wfex, chunk.size);
				ValidatePcm(result.wfex, filePath);
				hasFormat = true;
			} else if(std::memcmp(chunk.id, "data", 4) == 0) {
				if(!hasFormat || hasData || chunk.size == 0 || chunk.size > XAUDIO2_MAX_BUFFER_BYTES ||
				   chunk.size % result.wfex.nBlockAlign != 0) { fail("invalid data size or order"); }
				result.buffer.resize(chunk.size);
				read(result.buffer.data(), chunk.size);
				hasData = true;
			}
			position += paddedSize;
			file.seekg(static_cast<std::streamoff>(position));
			if(!file) { fail("cannot seek to next chunk"); }
		}
		if(!hasFormat || !hasData) { fail("missing fmt or data chunk"); }
		return result;
	}

	///=============================================================================
	/// キャッシュへの登録
	void MAudioG::LoadWavInternal(const std::string &filename) {
		if(soundDataMap_.contains(filename)) { return; }
		if(soundDataMap_.size() >= kMaxSoundData) { throw std::runtime_error("WAV " + filename + ": cache capacity exceeded"); }
		auto soundData = ReadWav(directoryPath_ + filename);
		soundData.name = filename;
		soundDataMap_.emplace(filename, std::move(soundData));
	}

	///=============================================================================
	/// サウンドのロード
	void MAudioG::LoadWav(const std::string &filename) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		try {
			LoadWavInternal(filename);
		} catch(const std::exception &error) {
			std::cerr << error.what() << std::endl;
		}
	}

	///=============================================================================
	/// サウンドデータのアンロード
	void MAudioG::Unload(SoundData *soundData) {
		if(!soundData) { return; }
		std::lock_guard<std::mutex> lock(voiceMutex_);
		for(auto it = soundDataMap_.begin(); it != soundDataMap_.end(); ++it) {
			if(&it->second == soundData) {
				voiceMap_.erase(it->first);
				soundDataMap_.erase(it);
				return;
			}
		}
		soundData->buffer.clear();
		soundData->name.clear();
		soundData->wfex = {};
	}

	///=============================================================================
	/// PCMフレームの反転
	std::vector<uint8_t> MAudioG::ReverseFrames(const SoundData &soundData) {
		ValidatePcm(soundData.wfex, soundData.name);
		const size_t frameBytes = soundData.wfex.nBlockAlign;
		if(soundData.buffer.empty() || soundData.buffer.size() % frameBytes != 0) {
			throw std::runtime_error("WAV " + soundData.name + ": incomplete PCM frame");
		}
		std::vector<uint8_t> reversed(soundData.buffer.size());
		for(size_t offset = 0; offset < reversed.size(); offset += frameBytes) {
			std::memcpy(reversed.data() + offset, soundData.buffer.data() + reversed.size() - offset - frameBytes, frameBytes);
		}
		return reversed;
	}

	///=============================================================================
	/// 通常・逆再生の共通経路
	void MAudioG::PlayWavInternal(const std::string &filename, bool loopFlag, float volume, float maxPlaySpeed, bool reverse) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		try {
			if(!xAudio2_ || !masterVoice_) { throw std::runtime_error("audio is not initialized"); }
			if(!std::isfinite(volume) || volume < 0.0f || volume > XAUDIO2_MAX_VOLUME_LEVEL ||
			   !std::isfinite(maxPlaySpeed) || maxPlaySpeed < XAUDIO2_MIN_FREQ_RATIO || maxPlaySpeed > XAUDIO2_MAX_FREQ_RATIO) {
				throw std::runtime_error("invalid volume or maximum playback speed");
			}
			LoadWavInternal(filename);
			const auto &soundData = soundDataMap_.at(filename);
			Voice voice{};
			if(reverse) { voice.reversedBuffer = ReverseFrames(soundData); }
			auto check = [](HRESULT result, const char *operation) {
				if(FAILED(result)) { throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(result)); }
			};
			IXAudio2SourceVoice *source = nullptr;
			const HRESULT created = xAudio2_->CreateSourceVoice(&source, &soundData.wfex, XAUDIO2_VOICE_USEFILTER, maxPlaySpeed, &voiceCallback_);
			voice.sourceVoice.reset(source);
			check(created, "CreateSourceVoice");
			check(source->SetVolume(volume), "SetVolume");
			XAUDIO2_BUFFER buffer{};
			buffer.pAudioData = reverse ? voice.reversedBuffer.data() : soundData.buffer.data();
			buffer.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
			buffer.Flags = XAUDIO2_END_OF_STREAM;
			buffer.LoopCount = loopFlag ? XAUDIO2_LOOP_INFINITE : 0;
			check(source->SubmitSourceBuffer(&buffer), "SubmitSourceBuffer");
			check(source->Start(0), "Start");
			voice.oldVolume = volume;
			// NOTE: 古いVoiceを破棄してから移すことで、代入中にも反転データの寿命を維持する。
			voiceMap_.erase(filename);
			voiceMap_.emplace(filename, std::move(voice));
		} catch(const std::exception &error) {
			std::cerr << "Play WAV " << filename << ": " << error.what() << std::endl;
		}
	}

	///=============================================================================
	/// サウンドを再生
	void MAudioG::PlayWav(const std::string &filename, bool loopFlag, float volume, float maxPlaySpeed) {
		PlayWavInternal(filename, loopFlag, volume, maxPlaySpeed, false);
	}

	///=============================================================================
	/// 逆再生
	void MAudioG::PlayWavReverse(const std::string &filename, bool loopFlag, float volume, float maxPlaySpeed) {
		PlayWavInternal(filename, loopFlag, volume, maxPlaySpeed, true);
	}

	///--------------------------------------------------------------
	///						 サウンドの停止
	void MAudioG::StopWav(const std::string &filename) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			voiceMap_.erase(it);
		}
	}

	///--------------------------------------------------------------
	///						 再生中かどうかを確認
	bool MAudioG::IsWavPlaying(const std::string &filename) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			XAUDIO2_VOICE_STATE state;
			it->second.sourceVoice->GetState(&state);
			return state.BuffersQueued > 0;
		}
		return false;
	}

	///--------------------------------------------------------------
	///						 再生を一時停止
	void MAudioG::PauseWav(const std::string &filename) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			it->second.sourceVoice->Stop();
		}
	}

	///--------------------------------------------------------------
	///						 再生再開
	void MAudioG::ResumeWav(const std::string &filename) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			it->second.sourceVoice->Start(0);
		}
	}

	///--------------------------------------------------------------
	///						 音量を設定
	void MAudioG::SetVolume(const std::string &filename, float volume) {
		float targetVolume = std::log(1.0f + volume) / std::log(2.0f); // 対数カーブを適用
		// 音量の最低ラインを決定
		targetVolume = std::max(targetVolume, 0.0f);

		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			if(targetVolume != it->second.oldVolume) {
				it->second.sourceVoice->SetVolume(targetVolume);
				it->second.oldVolume = targetVolume;
			}
		}
	}

	///--------------------------------------------------------------
	///						 音量を設定(デシベル)
	void MAudioG::SetVolumeDecibel(const std::string &filename, float dB) {
		float targetVolume = XAudio2DecibelsToAmplitudeRatio(dB);

		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			if(targetVolume != it->second.oldVolume) {
				it->second.sourceVoice->SetVolume(targetVolume);
				it->second.oldVolume = targetVolume;
			}
		}
	}

	///--------------------------------------------------------------
	///						 再生速度を設定
	void MAudioG::SetPlaybackSpeed(const std::string &filename, float speed) {
		std::lock_guard<std::mutex> lock(voiceMutex_);
		auto it = voiceMap_.find(filename);
		if(it != voiceMap_.end()) {
			if(speed != it->second.oldSpeed) {
				it->second.sourceVoice->SetFrequencyRatio(speed);
				it->second.oldSpeed = speed;
			}
		}
	}

}