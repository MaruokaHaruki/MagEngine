/*********************************************************************
 * \file   EngineContext.h
 * \brief  エンジン共通サービスへの非所有参照をまとめるコンテキスト
 *
 * \author Harukichimaru
 * \date   June 2026
 * \note   NOTE: SceneからSingletonへの直接依存を段階的に減らすための移行用コンテキスト
 *********************************************************************/
#pragma once

#include <cassert>

namespace MagEngine {
	class Input;
	class CameraManager;
	class LightManager;
	class TextureManager;
	class ModelManager;
	class MAudioG;
	class DirectXCore;
	class SpriteSetup;
	class Object3dSetup;
	class ParticleSetup;
	class SkyboxSetup;
	class CloudSetup;
	class TrailEffectSetup;
	class TrailEffectManager;
	class DebugTextManager;
	class LineManager;
	class EditorUiSystem;

	///=============================================================================
	///                         エンジンコンテキスト
	/// NOTE: 所有権は各ManagerやFramework側に残し、Sceneへ長寿命サービスを明示的に渡す。
	struct EngineContext {
		Input *input = nullptr;
		CameraManager *cameraManager = nullptr;
		LightManager *lightManager = nullptr;
		TextureManager *textureManager = nullptr;
		ModelManager *modelManager = nullptr;
		MAudioG *audio = nullptr;
		DirectXCore *graphics = nullptr;
		SpriteSetup *spriteSetup = nullptr;
		Object3dSetup *object3dSetup = nullptr;
		ParticleSetup *particleSetup = nullptr;
		SkyboxSetup *skyboxSetup = nullptr;
		CloudSetup *cloudSetup = nullptr;
		TrailEffectSetup *trailEffectSetup = nullptr;
		TrailEffectManager *trailEffectManager = nullptr;
		DebugTextManager *debugTextManager = nullptr;
		LineManager *lineManager = nullptr;
		EditorUiSystem *editorUiSystem = nullptr;

		/// \brief 必須サービスが設定済みか検証する
		void Validate() const {
			// 処理内容：SceneとScene所有ゲームロジックが必ず使う依存を検証する
			// 理由：旧Singletonフォールバックを残さず、初期化順の誤りをDebug時に明確化するため
			assert(input);
			assert(cameraManager);
			assert(lightManager);
			assert(textureManager);
			assert(modelManager);
			assert(audio);
			assert(graphics);
			assert(spriteSetup);
			assert(object3dSetup);
			assert(particleSetup);
			assert(skyboxSetup);
			assert(cloudSetup);
			assert(trailEffectSetup);
			assert(trailEffectManager);
			assert(debugTextManager);
			assert(lineManager);
			assert(editorUiSystem);
		}
	};
}
