/*********************************************************************
 * \file   CameraManager.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "CameraManager.h"
#include "AffineTransformations.h"
#include "ImguiSetup.h"
#include "Input.h"
#include "LineManager.h" // LineManager をインクルード
#include "MathFunc4x4.h"

///=============================================================================
///						シングルトンインスタンスの取得
CameraManager *CameraManager::GetInstance() {
	static CameraManager instance;
	return &instance;
}

///=============================================================================
///						終了処理
void CameraManager::Finalize() {
	cameras_.clear();
}

///=============================================================================
///						初期化
void CameraManager::Initialize() {
	// デバックカメラの追加
	AddCamera("DebugCamera");
	cameraDebugViewFlags_["DebugCamera"] = false; // 初期状態ではデバッグカメラの視覚化はオフ
	// デフォルトカメラの追加
	AddCamera("DefaultCamera");
	cameraDebugViewFlags_["DefaultCamera"] = false; // 初期状態ではデフォルトカメラの視覚化はオフ
	// デフォルトカメラの設定
	SetCurrentCamera("DebugCamera");

	// デバッグカメラ用パラメータの初期化
	debugCameraTarget_ = {0.0f, 0.0f, 0.0f};
	debugCameraDistanceToTarget_ = 20.0f; // 初期距離を調整
	isDebugCameraTargetLocked_ = true;	  // 最初はターゲット追従モード
	debugCameraMoveSpeed_ = 0.2f;		  // フリーカメラ時の移動速度
	debugCameraRotateSpeed_ = 0.005f;	  // フリーカメラ時の回転速度

	// 初期デバッグカメラのトランスフォーム設定
	ResetDebugCameraTransform();
}

///=============================================================================
///                     カメラの追加
void CameraManager::AddCamera(const std::string &name) {
	// カメラを作成
	std::unique_ptr<Camera> camera = std::make_unique<Camera>();
	camera->Initialize();

	// カメラを登録
	cameras_[name] = std::move(camera);
	// 新しいカメラのデバッグ表示フラグを初期化 (デフォルトはオフ)
	cameraDebugViewFlags_[name] = false;
}

///=============================================================================
///						カメラの取得
Camera *CameraManager::GetCamera(const std::string &name) const {
	auto it = cameras_.find(name);
	if (it != cameras_.end()) {
		return it->second.get();
	}
	return nullptr;
}

///=============================================================================
///						使用カメラの設定
void CameraManager::SetCurrentCamera(const std::string &name) {
	if (cameras_.find(name) != cameras_.end()) {
		currentCameraName_ = name;
	}
}

///=============================================================================
///                     現在のカメラの取得
Camera *CameraManager::GetCurrentCamera() const {
	return GetCamera(currentCameraName_);
}

///=============================================================================
///                     カメラの更新
void CameraManager::UpdateAll() {
	for (auto &pair : cameras_) {
		if (pair.second) {
			pair.second->Update();
		}
	}

	// 現在のカメラをLineManagerに設定 (メインの描画用)
	LineManager *lineManager = LineManager::GetInstance();
	if (lineManager) {
		Camera *currentCam = GetCurrentCamera();
		if (currentCam) {
			lineManager->SetDefaultCamera(currentCam);
		}
	}

	// デバックカメラの更新
	DebugCameraUpdate();

	// デバッグ用の視覚情報を描画登録
	DrawDebugVisualizations();
}

///=============================================================================
///						デバックカメラの更新
void CameraManager::DebugCameraUpdate() {
	// デバッグカメラを取得
	Camera *debugCamera = GetCamera("DebugCamera");

	// カメラが存在しない場合は処理しない
	if (!debugCamera) {
		return;
	}

	// 入力クラスのインスタンスを取得
	Input *input = Input::GetInstance();

	// マウスの移動量を取得
	float mouseDx = input->GetMouseMove().x;
	float mouseDy = input->GetMouseMove().y;

	// マウスのホイール量を取得（ズームに使用）
	float mouseWheel = input->GetMouseWheel();

	// マウスのボタン状態を取得
	bool isMiddleButtonPressed = input->PushMouseButton(1);
	// Shiftキーの状態を取得
	bool isShiftPressed = input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT);

	// カメラのトランスフォームを取得
	Transform cameraTransform = debugCamera->GetTransform();

	if (isDebugCameraTargetLocked_) {
		// ターゲット追従モード
		Matrix4x4 rotationMatrix = MakeRotateMatrix(cameraTransform.rotate);

		// パン（Shift + 中マウスボタンドラッグ）
		if (isMiddleButtonPressed && isShiftPressed) {
			const float panSpeed = 0.01f;
			Vector3 right = {rotationMatrix.m[0][0], rotationMatrix.m[1][0], rotationMatrix.m[2][0]};
			Vector3 up = {rotationMatrix.m[0][1], rotationMatrix.m[1][1], rotationMatrix.m[2][1]};
			Vector3 moveAmount = (right * (-mouseDx * panSpeed)) + (up * (mouseDy * panSpeed));
			cameraTransform.translate = cameraTransform.translate + moveAmount;
			debugCameraTarget_ = debugCameraTarget_ + moveAmount;
		}
		// 回転 (中マウスボタンドラッグ)
		else if (isMiddleButtonPressed) {
			const float rotateSpeed = 0.005f;
			cameraTransform.rotate.y += mouseDx * rotateSpeed;
			cameraTransform.rotate.x += mouseDy * rotateSpeed;

			const float maxPitch = 1.55f;
			if (cameraTransform.rotate.x > maxPitch)
				cameraTransform.rotate.x = maxPitch;
			if (cameraTransform.rotate.x < -maxPitch)
				cameraTransform.rotate.x = -maxPitch;

			rotationMatrix = MakeRotateMatrix(cameraTransform.rotate);
			Vector3 directionToCamera = {0.0f, 0.0f, -1.0f};
			directionToCamera = Conversion(directionToCamera, rotationMatrix);
			cameraTransform.translate = debugCameraTarget_ + directionToCamera * debugCameraDistanceToTarget_;
		}

		// WASDまたは矢印キー（中心点の平行移動）
		const float targetMoveSpeed = 0.1f;
		rotationMatrix = MakeRotateMatrix(cameraTransform.rotate); // パンや回転で更新されている可能性があるので再取得
		Vector3 forward = {rotationMatrix.m[0][2], rotationMatrix.m[1][2], rotationMatrix.m[2][2]};
		Vector3 right = {rotationMatrix.m[0][0], rotationMatrix.m[1][0], rotationMatrix.m[2][0]};
		Vector3 moveDirection = {0.0f, 0.0f, 0.0f};

		if (input->PushKey(DIK_UPARROW)) {
			moveDirection = moveDirection + forward * targetMoveSpeed;
		}
		if (input->PushKey(DIK_DOWNARROW)) {
			moveDirection = moveDirection - forward * targetMoveSpeed;
		}
		if (input->PushKey(DIK_LEFTARROW)) {
			moveDirection = moveDirection - right * targetMoveSpeed;
		}
		if (input->PushKey(DIK_RIGHTARROW)) {
			moveDirection = moveDirection + right * targetMoveSpeed;
		}

		if (Length(moveDirection) > 0.001f) {
			debugCameraTarget_ = debugCameraTarget_ + moveDirection;
			Vector3 directionToCamera = {0.0f, 0.0f, -1.0f};
			directionToCamera = Conversion(directionToCamera, MakeRotateMatrix(cameraTransform.rotate));
			cameraTransform.translate = debugCameraTarget_ + directionToCamera * debugCameraDistanceToTarget_;
		}

		// ズーム（中心点とカメラの距離の変更）
		if (mouseWheel != 0.0f) {
			const float zoomSpeed = 0.5f;
			debugCameraDistanceToTarget_ -= mouseWheel * zoomSpeed * 0.1f;
			if (debugCameraDistanceToTarget_ < 0.1f) {
				debugCameraDistanceToTarget_ = 0.1f;
			}
			Vector3 directionToCamera = {0.0f, 0.0f, -1.0f};
			directionToCamera = Conversion(directionToCamera, MakeRotateMatrix(cameraTransform.rotate));
			cameraTransform.translate = debugCameraTarget_ + directionToCamera * debugCameraDistanceToTarget_;
		}
	} else {
		// フリーカメラモード
		Matrix4x4 rotationMatrix = MakeRotateMatrix(cameraTransform.rotate);

		// パン（Shift + 中マウスボタンドラッグ）
		if (isMiddleButtonPressed && isShiftPressed) {
			const float panSpeed = 0.02f; // フリーカメラ用のパン速度
			Vector3 camRight = {rotationMatrix.m[0][0], rotationMatrix.m[1][0], rotationMatrix.m[2][0]};
			Vector3 camUp = {rotationMatrix.m[0][1], rotationMatrix.m[1][1], rotationMatrix.m[2][1]};
			cameraTransform.translate = cameraTransform.translate + (camRight * (-mouseDx * panSpeed)) + (camUp * (mouseDy * panSpeed));
		}
		// 回転 (中マウスボタンドラッグ)
		else if (isMiddleButtonPressed) {
			cameraTransform.rotate.y += mouseDx * debugCameraRotateSpeed_;
			cameraTransform.rotate.x += mouseDy * debugCameraRotateSpeed_;

			const float maxPitch = 1.55f;
			if (cameraTransform.rotate.x > maxPitch)
				cameraTransform.rotate.x = maxPitch;
			if (cameraTransform.rotate.x < -maxPitch)
				cameraTransform.rotate.x = -maxPitch;
		}

		// キーボード移動 (WASDQE)
		Vector3 moveDirection = {0.0f, 0.0f, 0.0f};
		// 回転後の行列を再計算
		rotationMatrix = MakeRotateMatrix(cameraTransform.rotate);
		Vector3 camForward = {rotationMatrix.m[0][2], rotationMatrix.m[1][2], rotationMatrix.m[2][2]};
		Vector3 camRight = {rotationMatrix.m[0][0], rotationMatrix.m[1][0], rotationMatrix.m[2][0]};
		Vector3 worldUp = {0.0f, 1.0f, 0.0f}; // ワールドY軸で上下移動

		if (input->PushKey(DIK_UPARROW)) {
			moveDirection = moveDirection + camForward;
		}
		if (input->PushKey(DIK_DOWNARROW)) {
			moveDirection = moveDirection - camForward;
		}
		if (input->PushKey(DIK_LEFTARROW)) {
			moveDirection = moveDirection - camRight;
		}
		if (input->PushKey(DIK_RIGHTARROW)) {
			moveDirection = moveDirection + camRight;
		}
		if (input->PushKey(DIK_E)) { // 上昇
			moveDirection = moveDirection + worldUp;
		}
		if (input->PushKey(DIK_Q)) { // 下降
			moveDirection = moveDirection - worldUp;
		}

		if (Length(moveDirection) > 0.001f) {
			moveDirection = Normalize(moveDirection);
			cameraTransform.translate = cameraTransform.translate + moveDirection * debugCameraMoveSpeed_;
		}

		// マウスホイールで前後移動 (カメラの向きに沿って)
		if (mouseWheel != 0.0f) {
			const float wheelMoveFactor = 5.0f; // ホイール感度調整用
			cameraTransform.translate = cameraTransform.translate + camForward * (mouseWheel * debugCameraMoveSpeed_ * wheelMoveFactor * 0.1f);
		}
	}

	// 更新されたトランスフォームをカメラに反映
	debugCamera->SetTransform(cameraTransform);
}

///=============================================================================
///						デバックカメラへの切り替え
void CameraManager::ChangeDebugCamera() {
	if (currentCameraName_ != "DebugCamera") {
		previousCameraName_ = currentCameraName_;
		SetCurrentCamera("DebugCamera");
	} else {
		SetCurrentCamera(previousCameraName_);
	}
}

///=============================================================================
///						Imgui描画
void CameraManager::DrawImGui() {
	// カメラの情報を表示
	ImGui::Begin("Camera Manager");
	ImGui::Text("Current Camera: %s", currentCameraName_.c_str());
	if (ImGui::Button("Switch Main/Debug Camera")) {
		ChangeDebugCamera();
	}
	ImGui::Separator();

	ImGui::Text("Debug Camera Controls (for 'DebugCamera'):");
	if (ImGui::Checkbox("Lock Target", &isDebugCameraTargetLocked_)) {
		// モード切替時にカメラのトランスフォームを調整することも可能
		// 例えば、フリーからロックに切り替えた際、現在のカメラ位置と向きからターゲットを再計算するなど
		// 今回はシンプルにフラグ切り替えのみ
		if (isDebugCameraTargetLocked_) { // フリーからロックへ
			Camera *debugCam = GetCamera("DebugCamera");
			if (debugCam) {
				// 現在のカメラ位置と回転から、前方にターゲットを設定する
				Transform currentCamTransform = debugCam->GetTransform();
				Matrix4x4 rotMat = MakeRotateMatrix(currentCamTransform.rotate);
				Vector3 forwardVec = {rotMat.m[0][2], rotMat.m[1][2], rotMat.m[2][2]};
				debugCameraTarget_ = currentCamTransform.translate + forwardVec * debugCameraDistanceToTarget_;
			}
		}
	}
	if (ImGui::Button("Reset Debug Camera")) {
		ResetDebugCameraTransform();
	}

	if (isDebugCameraTargetLocked_) {
		ImGui::DragFloat3("Target Position", &debugCameraTarget_.x, 0.1f);
		ImGui::DragFloat("Distance to Target", &debugCameraDistanceToTarget_, 0.1f, 0.1f, 1000.0f);
	} else {
		ImGui::DragFloat("Move Speed (Free)", &debugCameraMoveSpeed_, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat("Rotate Speed (Free)", &debugCameraRotateSpeed_, 0.001f, 0.001f, 0.1f);
	}

	ImGui::Separator();
	ImGui::Text("All Cameras Info & Debug View:");
	for (auto &pair : cameras_) {
		if (pair.second) {
			ImGui::PushID(pair.first.c_str()); // IDの重複を避ける
			ImGui::Text("Name: %s", pair.first.c_str());
			Transform camTransform = pair.second->GetTransform();
			ImGui::Text("Pos: %.2f, %.2f, %.2f", camTransform.translate.x, camTransform.translate.y, camTransform.translate.z);
			ImGui::Text("Rot: %.2f, %.2f, %.2f", camTransform.rotate.x, camTransform.rotate.y, camTransform.rotate.z);
			if (ImGui::Button("Set as Current")) {
				SetCurrentCamera(pair.first);
			}
			// 現在使用していないカメラの場合のみ、デバッグ表示のトグルを表示
			if (pair.first != currentCameraName_) {
				ImGui::SameLine();
				bool debugViewEnabled = cameraDebugViewFlags_[pair.first];
				if (ImGui::Checkbox("Show Debug Info", &debugViewEnabled)) {
					ToggleCameraDebugView(pair.first);
				}
			}
			ImGui::PopID();
			ImGui::Separator();
		}
	}
	ImGui::End();
}

///=============================================================================
///                     デバッグカメラのトランスフォームをリセット
void CameraManager::ResetDebugCameraTransform() {
	debugCameraTarget_ = {0.0f, 0.0f, 0.0f};
	debugCameraDistanceToTarget_ = 20.0f;
	// isDebugCameraTargetLocked_ は現在の設定を維持

	Camera *debugCamera = GetCamera("DebugCamera");
	if (debugCamera) {
		Transform camTransform;
		camTransform.scale = {1.0f, 1.0f, 1.0f};
		// 少し見下ろすような初期回転
		camTransform.rotate = {0.3f, 0.0f, 0.0f};

		Matrix4x4 rotationMatrix = MakeRotateMatrix(camTransform.rotate);
		Vector3 directionToCamera = {0.0f, 0.0f, -1.0f}; // カメラはターゲットのZ軸負方向
		directionToCamera = Conversion(directionToCamera, rotationMatrix);
		camTransform.translate = debugCameraTarget_ + directionToCamera * debugCameraDistanceToTarget_;

		debugCamera->SetTransform(camTransform);
	}
}

///=============================================================================
///                     デバッグカメラのターゲット追従モードを切り替え
void CameraManager::ToggleDebugCameraTargetLock() {
	isDebugCameraTargetLocked_ = !isDebugCameraTargetLocked_;
	// モード切替時の調整はDrawImGui内のCheckboxで行う
}

///=============================================================================
///						デバッグ用の視覚情報を描画
void CameraManager::DrawDebugVisualizations() {
	LineManager *lineManager = LineManager::GetInstance();
	if (!lineManager) {
		return;
	}

	// LineManagerが現在使用しているカメラを一時的に保存
	Camera *originalLineManagerCamera = lineManager->GetDefaultCamera();
	Camera *currentActiveMainCamera = GetCurrentCamera(); // メインの描画に使用するカメラ

	for (auto const &[name, cam_ptr] : cameras_) {
		// 現在アクティブなカメラはスキップ
		if (name == currentCameraName_) {
			continue;
		}

		// デバッグ表示フラグがオフのカメラはスキップ
		if (cameraDebugViewFlags_.count(name) && !cameraDebugViewFlags_[name]) {
			continue;
		}

		Camera *targetCamera = cam_ptr.get();
		if (!targetCamera) {
			continue;
		}

		// LineManagerのカメラを一時的にメインの描画カメラに設定
		// これにより、非アクティブカメラの情報が、現在アクティブなカメラの視点から描画される
		if (currentActiveMainCamera) {
			lineManager->SetDefaultCamera(currentActiveMainCamera);
		}

		Transform camTransform = targetCamera->GetTransform();
		Matrix4x4 rotationMatrix = MakeRotateMatrix(camTransform.rotate);

		Vector3 camPos = camTransform.translate;
		float axisLength = 1.5f;
		float arrowHeadSize = 0.15f;

		// カメラの前方ベクトル (Z軸) - 青
		Vector3 forward = {0.0f, 0.0f, 1.0f};
		forward = Conversion(forward, rotationMatrix);
		forward = Normalize(forward);
		lineManager->DrawArrow(camPos, camPos + forward * axisLength, {0.0f, 0.0f, 1.0f, 1.0f}, arrowHeadSize);

		// カメラの上方ベクトル (Y軸) - 緑
		Vector3 up = {0.0f, 1.0f, 0.0f};
		up = Conversion(up, rotationMatrix);
		up = Normalize(up);
		lineManager->DrawArrow(camPos, camPos + up * axisLength, {0.0f, 1.0f, 0.0f, 1.0f}, arrowHeadSize);

		// カメラの右方ベクトル (X軸) - 赤
		Vector3 right = {1.0f, 0.0f, 0.0f};
		right = Conversion(right, rotationMatrix);
		right = Normalize(right);
		lineManager->DrawArrow(camPos, camPos + right * axisLength, {1.0f, 0.0f, 0.0f, 1.0f}, arrowHeadSize);

		// "DebugCamera" の場合、ターゲット情報を表示 (ターゲット追従モードがオンの場合)
		if (name == "DebugCamera" && isDebugCameraTargetLocked_) {
			lineManager->DrawLine(camPos, debugCameraTarget_, {1.0f, 1.0f, 0.0f, 1.0f});
			lineManager->DrawSphere(debugCameraTarget_, 0.1f, {1.0f, 1.0f, 0.0f, 1.0f}, 8);
		}
	}

	// LineManagerのカメラを元に戻す
	if (originalLineManagerCamera) {
		lineManager->SetDefaultCamera(originalLineManagerCamera);
	} else if (currentActiveMainCamera) {
		lineManager->SetDefaultCamera(currentActiveMainCamera);
	}
}

///=============================================================================
///                     特定のカメラのデバッグ表示を切り替える
void CameraManager::ToggleCameraDebugView(const std::string &cameraName) {
	if (cameraDebugViewFlags_.count(cameraName)) {
		cameraDebugViewFlags_[cameraName] = !cameraDebugViewFlags_[cameraName];
	}
}

///=============================================================================
///                     デストラクタ
CameraManager::~CameraManager() {
	cameras_.clear();
}
