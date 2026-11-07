/*********************************************************************
 * \file   AssetSafetyTests.cpp
 * \brief  WAV・モデルの読み込み失敗と逆再生の所有期間を直接確認する回帰検証
 *********************************************************************/
#include "MAudioG.h"
#include "Model.h"
#include "ModelManager.h"
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

///=============================================================================
/// 回帰検証の共通処理
namespace {
	void Require(bool condition, const std::string &reason) {
		if(!condition) { throw std::runtime_error(reason); }
	}

	void Reject(const std::function<void()> &operation, const std::string &asset) {
		try { operation(); }
		catch(const std::runtime_error &error) {
			Require(std::string(error.what()).find(asset) != std::string::npos, "error must identify asset: " + asset);
			return;
		}
		throw std::runtime_error("invalid asset accepted: " + asset);
	}

	void Write(const fs::path &path, const std::vector<uint8_t> &bytes) {
		std::ofstream file(path, std::ios::binary);
		Require(bool(file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size())), "cannot write fixture");
	}

	void WriteText(const fs::path &path, const std::string &text) {
		Write(path, {text.begin(), text.end()});
	}

	void U32(std::vector<uint8_t> &bytes, uint32_t value) {
		for(unsigned shift = 0; shift < 32; shift += 8) { bytes.push_back(uint8_t(value >> shift)); }
	}

	void Chunk(std::vector<uint8_t> &bytes, const char *id, const std::vector<uint8_t> &data) {
		bytes.insert(bytes.end(), id, id + 4);
		U32(bytes, static_cast<uint32_t>(data.size()));
		bytes.insert(bytes.end(), data.begin(), data.end());
		if(data.size() & 1) { bytes.push_back(0); }
	}

	std::vector<uint8_t> Wav(std::vector<uint8_t> format, const std::vector<uint8_t> &data, bool unknown = false, bool includeData = true) {
		std::vector<uint8_t> chunks{'W','A','V','E'};
		if(unknown) { Chunk(chunks, "JUNK", {1,2,3}); }
		Chunk(chunks, "fmt ", format);
		if(includeData) { Chunk(chunks, "data", data); }
		std::vector<uint8_t> bytes{'R','I','F','F'};
		U32(bytes, static_cast<uint32_t>(chunks.size()));
		bytes.insert(bytes.end(), chunks.begin(), chunks.end());
		return bytes;
	}
}

///=============================================================================
/// エンジンの非公開CPU境界と実Voiceの限定検証
namespace MagEngine {
	struct AssetSafetyTests {
		static void Cpu(const fs::path &root, const fs::path &fixtures) {
			//========================================
			// RIFF境界・形式・失敗後の再試行
			const std::vector<uint8_t> format{1,0,2,0,0x44,0xac,0,0,0x10,0xb1,2,0,4,0,16,0};
			const std::vector<uint8_t> frames{1,2,3,4,5,6,7,8,9,10,11,12};
			const auto valid = Wav(format, frames, true);
			const auto wavPath = fixtures / "fixture.wav";
			Write(wavPath, valid);
			auto sound = MAudioG::ReadWav(wavPath.string());
			Require(sound.buffer == frames && sound.wfex.nBlockAlign == 4, "odd unknown chunk or PCM decoding failed");
			for(uint8_t channels : {1, 2}) {
				for(uint8_t bits : {8, 16}) {
					auto pcm = format;
					pcm[2] = channels; pcm[14] = bits; pcm[12] = uint8_t(channels * (bits / 8));
					const uint32_t rate = 44100u * pcm[12];
					for(unsigned i = 0; i < 4; ++i) { pcm[8 + i] = uint8_t(rate >> (8 * i)); }
					Write(wavPath, Wav(pcm, frames));
					Require(MAudioG::ReadWav(wavPath.string()).buffer == frames, "supported PCM rejected");
				}
			}
			for(size_t length = 0; length < valid.size(); ++length) {
				Write(wavPath, {valid.begin(), valid.begin() + length});
				Reject([&] { MAudioG::ReadWav(wavPath.string()); }, "fixture.wav");
			}
			std::vector<std::vector<uint8_t>> invalidFormats;
			auto extension = format; extension.resize(40); invalidFormats.push_back(extension);
			for(const auto [index, value] : {std::pair<size_t,uint8_t>{0,3}, {2,0}, {12,0}, {14,24}, {8,0}, {4,0}}) {
				auto bad = format; bad[index] = value; invalidFormats.push_back(bad);
			}
			for(const auto &bad : invalidFormats) {
				Write(wavPath, Wav(bad, frames));
				Reject([&] { MAudioG::ReadWav(wavPath.string()); }, "fixture.wav");
			}
			for(const auto &bad : {Wav(format, frames, false, false), Wav(format, {}), Wav(format, {1,2,3})}) {
				Write(wavPath, bad);
				Reject([&] { MAudioG::ReadWav(wavPath.string()); }, "fixture.wav");
			}
			auto oversized = Wav(format, frames);
			for(size_t i = 40; i < 44; ++i) { oversized[i] = 255; }
			Write(wavPath, oversized);
			Reject([&] { MAudioG::ReadWav(wavPath.string()); }, "fixture.wav");
			extension = format; extension.resize(18);
			Write(wavPath, Wav(extension, frames));
			Require(MAudioG::ReadWav(wavPath.string()).buffer == frames, "18-byte PCM fmt rejected");
			extension[16] = 1;
			Write(wavPath, Wav(extension, frames));
			Reject([&] { MAudioG::ReadWav(wavPath.string()); }, "fixture.wav");
			Reject([&] { MAudioG::ReadWav((fixtures / "missing.wav").string()); }, "missing.wav");

			MAudioG audio;
			audio.directoryPath_ = fixtures.string() + "/";
			audio.LoadWav("fixture.wav");
			Require(audio.soundDataMap_.empty(), "failed load registered data");
			audio.PlayWav("fixture.wav");
			audio.PlayWavReverse("fixture.wav");
			Require(audio.voiceMap_.empty() && audio.soundDataMap_.empty(), "uninitialized play changed registry");
			Write(wavPath, valid);
			audio.LoadWav("fixture.wav");
			Require(audio.soundDataMap_.at("fixture.wav").buffer == frames, "retry retained failed data");
			audio.Unload(&audio.soundDataMap_.at("fixture.wav"));
			Require(audio.soundDataMap_.empty(), "unload did not remove cache entry");
			for(int i = 0; i < MAudioG::kMaxSoundData; ++i) { audio.soundDataMap_.emplace(std::to_string(i), sound); }
			audio.LoadWav("fixture.wav");
			Require(!audio.soundDataMap_.contains("fixture.wav"), "cache limit bypassed");
			audio.Finalize();
			size_t wavCount = 0;
			for(const auto &entry : fs::directory_iterator(root / "resources/sound")) {
				if(entry.path().extension() == ".wav") {
					Require(!MAudioG::ReadWav(entry.path().string()).buffer.empty(), "bundled WAV rejected");
					++wavCount;
				}
			}
			std::cout << "PASS WAV boundaries, retry, cache limit, bundled assets=" << wavCount << '\n';

			//========================================
			// サンプル内とチャンネル内のバイト順を保持するフレーム反転
			Require(MAudioG::ReverseFrames(sound) == std::vector<uint8_t>({9,10,11,12,5,6,7,8,1,2,3,4}), "stereo frames corrupted");
			sound.wfex.nChannels = 1; sound.wfex.nBlockAlign = 2; sound.wfex.nAvgBytesPerSec = 88200;
			Require(MAudioG::ReverseFrames(sound) == std::vector<uint8_t>({11,12,9,10,7,8,5,6,3,4,1,2}), "mono samples corrupted");
			sound.buffer.pop_back();
			Reject([&] { MAudioG::ReverseFrames(sound); }, "fixture.wav");
			std::cout << "PASS mono/stereo PCM frame reversal and partial-frame rejection\n";

			//========================================
			// GPU生成前のモデル検証
			WriteText(fixtures / "fixture.mtl", "newmtl test\nmap_Kd fixture.png\n");
			const std::string vertices = "mtllib fixture.mtl\nv 0 0 1\nv 1 0 1\nv 0 1 1\nv 1 1 1\nvt 0 0\nvt 1 0\nvt 0 1\nvt 1 1\nvn 0 0 1\nusemtl test\n";
			const auto obj = fixtures / "fixture.obj";
			for(const auto &bad : {std::string{}, vertices, vertices + "f 1/1 2/2 3/3\n", vertices + "f 1//1 2//1 3//1\n",
				vertices + "f 1/1/1 2/2/1 3/3/1 4/4/1\n", vertices + "f 99/1/1 2/2/1 3/3/1\n"}) {
				WriteText(obj, bad);
				Reject([&] { Model::LoadModelFile(fixtures.string(), "fixture.obj"); }, "fixture.obj");
			}
			Reject([&] { Model::LoadModelFile(fixtures.string(), "missing.obj"); }, "missing.obj");
			WriteText(obj, vertices + "f 1/1/1 2/2/1 3/3/1\n");
			auto data = Model::LoadModelFile(fixtures.string(), "fixture.obj");
			Require(data.vertices.size() == 3 && data.material.textureFilePath == "fixture.png", "valid model rejected or changed");
			Require(data.vertices[0].position.z == -1.0f && data.vertices[0].normal.z == -1.0f, "left-handed conversion changed");
			for(const auto *name : {"jet.obj", "Missile.obj", "Bullet.obj", "ground.obj", "skydome.obj", "axisPlus.obj", "ball.obj", "terrain.obj"}) {
				auto bundled = Model::LoadModelFile((root / "resources/model").string(), name);
				Require(!bundled.vertices.empty() && !bundled.material.textureFilePath.empty(), std::string("bundled model rejected: ") + name);
				std::cout << "MODEL " << name << " vertices=" << bundled.vertices.size() << " texture=" << bundled.material.textureFilePath << '\n';
			}
			auto &manager = *ModelManager::GetInstance();
			Reject([&] { manager.LoadModel("missing.obj"); }, "missing.obj");
			Require(manager.FindModel("missing.obj") == nullptr, "failed model cached");
			Model model;
			Reject([&] { model.Initialize(nullptr, fixtures.string(), "fixture.obj"); }, "fixture.obj");
			Require(!model.vertexBuffer_ && !model.materialBuffer_ && !model.materialData_, "failed initialization published GPU state");
			std::cout << "PASS model failures, cache exclusion and bundled models\n";
		}

		static void Audio(const fs::path &fixtures) {
			Require(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)), "COM initialization failed");
			struct ComScope { ~ComScope() { CoUninitialize(); } } comScope;
			MAudioG audio;
			audio.Initialize(fixtures.string() + "/", L"__MagEngine_missing_device__");
			Require(!audio.xAudio2_ && !audio.masterVoice_, "mastering voice failure left partial device");
			audio.PlayWav("fixture.wav");
			Require(audio.voiceMap_.empty() && audio.soundDataMap_.empty(), "failed device initialization allowed playback");
			audio.Initialize(fixtures.string() + "/");
			Require(audio.xAudio2_ && audio.masterVoice_, "audio device unavailable");
			audio.PlayWav("missing.wav");
			audio.PlayWavReverse("missing.wav");
			audio.PlayWav("fixture.wav", false, -1.0f);
			Require(audio.voiceMap_.empty() && audio.soundDataMap_.empty(), "failed play published voice or invalid cache");
			// NOTE: 外部APIの失敗経路を直接通すため、この検証内だけでキャッシュに不正値を注入する。
			audio.LoadWav("fixture.wav");
			const auto saved = audio.soundDataMap_.at("fixture.wav");
			audio.soundDataMap_.at("fixture.wav").wfex.wFormatTag = 0xffff;
			audio.PlayWav("fixture.wav", false, 0.0f);
			Require(audio.voiceMap_.empty(), "CreateSourceVoice failure left a voice");
			audio.soundDataMap_.at("fixture.wav") = saved;
			audio.soundDataMap_.at("fixture.wav").buffer.pop_back();
			audio.PlayWav("fixture.wav", false, 0.0f);
			Require(audio.voiceMap_.empty(), "SubmitSourceBuffer failure left a voice");
			audio.soundDataMap_.at("fixture.wav") = saved;
			// NOTE: 実デバイスを使うが、聴感確認ではなく参照寿命を確認するため無音で再生する。
			audio.PlayWavReverse("fixture.wav", true, 0.0f);
			Require(audio.IsWavPlaying("fixture.wav"), "reverse playback did not start");
			const auto &voice = audio.voiceMap_.at("fixture.wav");
			Require(!voice.reversedBuffer.empty(), "reverse data lost after return");
			const auto *buffer = voice.reversedBuffer.data();
			std::this_thread::sleep_for(std::chrono::milliseconds(40));
			Require(audio.IsWavPlaying("fixture.wav") && buffer == audio.voiceMap_.at("fixture.wav").reversedBuffer.data(), "loop lost buffer");
			audio.PauseWav("fixture.wav"); audio.ResumeWav("fixture.wav");
			audio.PlayWav("fixture.wav", true, 0.0f);
			Require(audio.IsWavPlaying("fixture.wav") && audio.voiceMap_.at("fixture.wav").reversedBuffer.empty(), "normal replacement failed");
			audio.PlayWavReverse("fixture.wav", true, 0.0f);
			audio.Unload(&audio.soundDataMap_.at("fixture.wav"));
			Require(audio.voiceMap_.empty() && audio.soundDataMap_.empty(), "unload left live references");
			audio.PlayWavReverse("fixture.wav", true, 0.0f); audio.StopWav("fixture.wav");
			Require(audio.voiceMap_.empty(), "stop retained voice");
			audio.PlayWavReverse("fixture.wav", false, 0.0f);
			std::this_thread::sleep_for(std::chrono::milliseconds(80));
			Require(!audio.IsWavPlaying("fixture.wav"), "finite playback did not end");
			audio.PlayWavReverse("fixture.wav", true, 0.0f); audio.Finalize(); audio.Finalize();
			Require(audio.voiceMap_.empty() && audio.soundDataMap_.empty() && !audio.xAudio2_ && !audio.masterVoice_, "finalize left references");
			std::cout << "PASS real XAudio2 silent normal/reverse, loop, pause/resume, replacement, unload, stop, natural end, finalize\n";
		}
	};
}

///=============================================================================
/// 検証用エントリーポイント（ソリューションには追加しない）
int main(int argc, char **argv) {
	try {
		Require(argc >= 3, "usage: AssetSafetyTests repository fixture-directory [--audio]");
		const fs::path root = fs::absolute(argv[1]);
		const fs::path fixtures = fs::absolute(argv[2]);
		fs::create_directories(fixtures);
		MagEngine::AssetSafetyTests::Cpu(root, fixtures);
		if(argc > 3 && std::string(argv[3]) == "--audio") { MagEngine::AssetSafetyTests::Audio(fixtures); }
		return 0;
	} catch(const std::exception &error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}
