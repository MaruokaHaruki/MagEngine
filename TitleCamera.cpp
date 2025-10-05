#include "TitleCamera.h"
#include "AffineTransformations.h"
#include "Camera.h"
#include "CameraManager.h"
#include "FollowCamera.h"
#include "ImguiSetup.h"
#include "MathFunc4x4.h"
#include "Player.h"
#include <cmath>

///=============================================================================
///                        初期化
void TitleCamera::Initialize(const std::string &cameraName) {
	cameraName_ = cameraName;

	// カメラマネージャからカメラを取得
	CameraManager *cameraManager = CameraManager::GetInstance();
	camera_ = cameraManager->GetCamera(cameraName);
}

///=============================================================================
///                        更新
void TitleCamera::Update() {
}

///=============================================================================
///                        ImGui描画
void TitleCamera::DrawImGui() {
}
