/*********************************************************************
 * \file   ParticleRegressionTests.cpp
 * \brief  粒子の要求数・名前・時間・共有更新を実装へ直接確認する回帰検証
 *********************************************************************/
#include "Particle.h"
#include "ParticleEmitter.h"
#include "TextureManager.h"
#include "engine/render/pass/RenderWorld.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>

namespace {
	void Require(bool condition, const std::string &message) {
		if(!condition) { throw std::runtime_error(message); }
	}
	void Near(float actual, float expected, float tolerance, const std::string &message) {
		Require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
			message + ": actual=" + std::to_string(actual) + ", expected=" + std::to_string(expected));
	}
}

///=============================================================================
/// 実装の非公開境界を使った回帰検証
namespace MagEngine {
	struct ParticleRegressionTests {
		static ParticleStr Live() {
			ParticleStr value{};
			value.lifeTime = 10.0f;
			value.transform.scale = value.initialScale = value.endScale = {1,1,1};
			value.color = {1,1,1,1};
			return value;
		}

		static ParticleGroup &Seed(Particle &particle, std::vector<ParticleForGPU> &buffer, size_t count) {
			buffer.resize(Particle::kNumMaxInstance + 1);
			buffer.back().color = {11,12,13,14}; // 上限直後の番兵で書き込み境界を確認する。
			particle.randomEngine_.seed(42);
			auto &group = particle.particleGroups["test"];
			group.instancingDataPtr = buffer.data();
			group.materialFilePath = "preserved.dds";
			group.particleList.assign(count, Live());
			return group;
		}

		static void Tick(Particle &particle, float seconds) {
			const auto identity = MagMath::Identity4x4();
			particle.UpdateGroups(seconds, identity, identity);
		}

		static void Cpu() {
			//========================================
			// 追加要求数・上限・未登録名・重複名
			constexpr auto limit = Particle::kNumMaxInstance;
			struct CountCase { size_t initial; uint32_t request; size_t expected; };
			for(const auto &test : {CountCase{0,20,20}, {20,20,40}, {20,0,20}, {limit-1,20,limit},
				{limit,20,limit}, {0,UINT32_MAX,limit}, {limit+1,UINT32_MAX,limit+1}}) {
				Particle particle;
				std::vector<ParticleForGPU> buffer;
				auto &group = Seed(particle, buffer, test.initial);
				particle.Emit("test", {}, test.request);
				Require(group.particleList.size() == test.expected, "Emit did not honor additional count/capacity");
			}
			Particle particle;
			std::vector<ParticleForGPU> buffer;
			auto &group = Seed(particle, buffer, 1);
			particle.Emit("missing", {}, 20);
			Require(particle.particleGroups.size() == 1, "missing name inserted a group");
			// NOTE: setupがnullでも正常に拒否されれば、依存アクセスより先の重複検出を確認できる。
			particle.CreateParticleGroup("test", "replacement.dds", ParticleShape::Cylinder);
			Require(group.materialFilePath == "preserved.dds" && group.instancingDataPtr == buffer.data() &&
				group.particleList.size() == 1 && particle.particleGroups.size() == 1, "duplicate changed existing group");
			std::cout << "PASS requested counts, zero/full/oversized capacity, unknown and duplicate names\n";

			//========================================
			// 0秒・不正時間でも描画データは累積せず、進行は止まる
			Tick(particle, 0.25f);
			const auto saved = group.particleList.front();
			for(float seconds : {0.0f, 0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
				Tick(particle, seconds);
				Require(group.instanceCount == 1, "instance snapshot accumulated");
				Near(group.particleList.front().currentTime, saved.currentTime, 0.0f, "paused clock advanced");
				Near(group.particleList.front().transform.translate.y, saved.transform.translate.y, 0.0f, "paused position advanced");
			}
			group.particleList.assign(limit+1, Live());
			Tick(particle, 0.0f);
			Require(group.particleList.size() == limit && group.instanceCount == limit && buffer.back().color.w == 14,
				"CPU/GPU capacity or write boundary exceeded");
			group.particleList.clear();
			Tick(particle, 0.0f);
			Require(!particle.HasDrawableParticles(), "empty frame retained drawable instances");
			std::cout << "PASS zero/invalid deltaTime, snapshot reset, CPU/GPU capacity and write sentinel\n";

			//========================================
			// 半陰的Euler法の既知誤差を使った30/60/120分割と寿命の比較
			float previousY = 0.0f;
			int previousSteps = 0;
			for(int steps : {30,60,120}) {
				group.particleList.assign(1, Live());
				group.particleList.front().velocity = {2,3,4};
				for(int i = 0; i < steps; ++i) { Tick(particle, 1.0f / steps); }
				const auto &value = group.particleList.front();
				Near(value.currentTime, 1.0f, 1e-5f, "lifetime progression depends on frame count");
				Near(value.transform.translate.x, 2.0f, 1e-4f, "constant x velocity changed");
				Near(value.transform.translate.z, 4.0f, 1e-4f, "constant z velocity changed");
				Near(value.transform.translate.y, 3.0f - 4.9f, 9.8f / (2 * steps) + 1e-4f, "integration error exceeded derived bound");
				if(previousSteps) {
					Near(value.transform.translate.y - previousY, 0.0f,
						4.9f * (1.0f / previousSteps - 1.0f / steps) + 1e-4f, "frame-rate difference exceeded derived bound");
				}
				previousY = value.transform.translate.y; previousSteps = steps;
				group.particleList.front().lifeTime = 1.5f;
				Tick(particle, 1.0f);
				Require(group.particleList.empty() && group.instanceCount == 0, "expired particle was still submitted");
			}
			group.particleList.assign(1, Live());
			Tick(particle, 0.1f * 0.25f);
			Near(group.particleList.front().currentTime, 0.025f, 1e-6f, "scaled time not applied");
			std::cout << "PASS 30/60/120 steps, derived integration tolerance, expiry and slow time\n";

			//========================================
			// 複数Emitterは発生だけを行い、共有Particleの時計は所有側だけが進める
			group.particleList.clear();
			particle.SetLifetimeRange(10,10);
			ParticleEmitter first(&particle, "test", {}, 1, 1.0f, true);
			ParticleEmitter second(&particle, "test", {}, 1, 1.0f, true);
			Require(group.particleList.size() == 2, "constructor emission changed");
			first.Update(0.0f); second.Update(0.0f);
			first.Update(-1.0f); second.Update(std::numeric_limits<float>::quiet_NaN());
			second.Update(std::numeric_limits<float>::infinity());
			Require(group.particleList.size() == 2, "paused/invalid emitter emitted");
			first.Update(0.1f); second.Update(0.1f);
			Require(group.particleList.size() == 4, "first positive update emission changed");
			for(const auto &value : group.particleList) { Near(value.currentTime, 0.0f, 0.0f, "emitter advanced shared simulation"); }
			Tick(particle, 0.1f);
			for(const auto &value : group.particleList) { Near(value.currentTime, 0.1f, 1e-6f, "owner update was not applied once"); }
			first.Update(5.0f);
			Require(group.particleList.size() == 5, "long frame changed one-emission-per-update policy");
			first.Update(0.0f);
			Require(group.particleList.size() == 5, "paused emitter drained backlog");
			for(float frequency : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
				ParticleEmitter invalid(&particle, "test", {}, 1, frequency, true);
				const auto count = group.particleList.size();
				invalid.SetRepeat(true); invalid.Update(10.0f);
				Require(group.particleList.size() == count, "invalid frequency repeated");
			}
			std::cout << "PASS multiple emitters, constructor/first-tick policy, pause, long frame and invalid frequency\n";
		}

		static void Gpu() {
			// NOTE: Windowを生成せず、実DescriptorとMapだけを検証する。描画・見た目の確認とは区別する。
			Microsoft::WRL::ComPtr<ID3D12Device> device;
			Require(SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))), "D3D12 device unavailable");
			DirectXCore core;
			core.SetDevice(device);
			core.InitializeDescriptorAllocators();
			core.InitializeFrameContextsAndCommandList();
			core.CreateDXCCompiler();
			SrvSetup srv;
			srv.Initialize(&core);
			TextureManager textures;
			textures.Initialize(&core, "resources/texture/", &srv);
			ParticleSetup setup;
			setup.Initialize(&core, &srv, textures);
			Particle particle;
			particle.Initialize(&setup);
			particle.CreateParticleGroup("first", "circle2.dds", ParticleShape::Board);
			auto &first = particle.particleGroups.at("first");
			const auto free = srv.GetFreeCount();
			const auto resource = first.instancingResource;
			const auto material = particle.materialBuffer_;
			particle.CreateParticleGroup("first", "missing.dds", ParticleShape::Ring);
			Require(srv.GetFreeCount() == free && particle.particleGroups.size() == 1 && first.instancingResource == resource &&
				first.materialFilePath == "circle2.dds", "duplicate allocated descriptors or replaced data");
			particle.CreateParticleGroup("circle2.dds", "circle2.dds", ParticleShape::Ring);
			Require(particle.particleGroups.size() == 2 && srv.GetFreeCount() == free - 1 && particle.materialBuffer_ == material,
				"same-texture alias failed or replaced shared material");
			particle.Emit("first", {}, 20); particle.Emit("first", {}, 20);
			particle.Emit("circle2.dds", {}, 3);
			Tick(particle, 0.1f);
			Require(first.instanceCount == 40 && particle.particleGroups.at("circle2.dds").instanceCount == 3, "mapped instance count mismatch");
			Tick(particle, 0.0f);
			Require(first.instanceCount == 40, "mapped snapshot accumulated");
			RenderWorld world;
			particle.RegisterRenderables(world);
			Require(world.GetParticleItems().size() == 1, "render registration failed");
			while(srv.GetFreeCount() > 1) { srv.Allocate(); }
			bool rejected = false;
			try { particle.CreateParticleGroup("no-room", "circle.dds", ParticleShape::Board); }
			catch(const std::runtime_error &) { rejected = true; }
			Require(rejected && !particle.particleGroups.contains("no-room") && srv.GetFreeCount() == 0,
				"texture consumed last descriptor but group creation did not stop");
			particle.CreateParticleGroup("first", "missing.dds", ParticleShape::Cylinder);
			Require(first.instancingResource == resource && srv.GetFreeCount() == 0, "full-heap duplicate changed group");
			Require(SUCCEEDED(core.GetCommandList()->Close()), "cannot close unsubmitted upload commands");
			// NOTE: 検証ではコマンドをGPUへ投入していないため、キュー実行中の資源は存在しない。
			std::cout << "PASS headless D3D12 duplicate descriptor preservation, same-texture aliases, shared material, mapped snapshots and render registration\n";
		}
	};
}

int main(int argc, char **argv) {
	try {
		if(argc > 1) { std::filesystem::current_path(argv[1]); }
		MagEngine::ParticleRegressionTests::Cpu();
		if(argc > 3 && std::string(argv[3]) == "--gpu") {
			Require(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)), "COM initialization failed");
			struct ComScope { ~ComScope() { CoUninitialize(); } } comScope;
			MagEngine::ParticleRegressionTests::Gpu();
		}
		return 0;
	} catch(const std::exception &error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}
