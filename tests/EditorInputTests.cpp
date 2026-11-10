/*********************************************************************
 * \file EditorInputTests.cpp
 * \brief OS入力取得後のゲーム向け入力遮断を検証する
 *********************************************************************/
#include "Input.h"
#include <iostream>
#include <stdexcept>
namespace MagEngine {
	struct EditorInputTests {
		static void Run() {
			auto *input = Input::GetInstance();
			// 外部デバイス境界の取得済み状態を再現。OSデバイスは初期化しない。
			input->keyState_[DIK_W] = 0x80; input->keyStatePrev_[DIK_W] = 0;
			input->mouseButtons_[0] = true; input->mouseButtonsPrev_[0] = false;
			input->controllerConnected_ = true; input->controllerState_.Gamepad.wButtons = XINPUT_GAMEPAD_A;
			input->controllerState_.Gamepad.sThumbLX = 32767;
			if (!input->PushKey(DIK_W) || !input->TriggerKey(DIK_W) || !input->PushMouseButton(0) || !input->PushButton(XINPUT_GAMEPAD_A)) { throw std::runtime_error("fixture input"); }
			input->SetApplicationInputEnabled(false);
			if (input->PushKey(DIK_W) || input->TriggerKey(DIK_W) || input->PushMouseButton(0) || input->TriggerMouseButton(0) || input->PushButton(XINPUT_GAMEPAD_A) || input->GetLeftStickX() != 0) { throw std::runtime_error("EditorInputMustNotLeakToGameplay"); }
			input->SetApplicationInputEnabled(true);
			if (!input->PushKey(DIK_W)) { throw std::runtime_error("RestoreApplicationInput"); }
			EditorViewportInputState viewport; viewport.imageRect = {100, 200, 640, 360}; viewport.renderSize = {1280, 720}; viewport.focused = true; viewport.allowInput = true;
			input->mousePos_ = {420, 380}; input->mousePosPrev_ = {410, 375}; input->SetApplicationViewport(viewport);
			if (input->GetMousePosFromWindowCenter().x != 0 || input->GetMouseMove().x != 20 || input->GetMouseMove().y != 10) { throw std::runtime_error("ScaledImageMapsToRenderCenter"); }
			input->mousePos_ = {580, 470};
			if (input->GetMousePosFromWindowCenter().x != 320 || input->GetMousePosFromWindowCenter().y != 180) { throw std::runtime_error("ScaledImageMapsOffset"); }
			viewport.allowInput = false; input->SetApplicationViewport(viewport);
			input->SetUiInputReadEnabled(true);
			if (!input->PushKey(DIK_W) || input->GetLeftStickX() == 0) { throw std::runtime_error("UiDiagnosticsReadsRawInput"); }
			input->SetUiInputReadEnabled(false);
			if (input->PushKey(DIK_W)) { throw std::runtime_error("GameplayMaskRestoredAfterUi"); }
			viewport.allowInput = true; input->SetApplicationViewport(viewport);
			if (input->TriggerKey(DIK_W) || !input->PushKey(DIK_W)) { throw std::runtime_error("BlockedPressDoesNotReplayOnRestore"); }
			input->SetApplicationViewport(std::nullopt);
		}
	};
}
int main() { try { MagEngine::EditorInputTests::Run(); std::cout << "EditorInput PASS\n"; return 0; } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
