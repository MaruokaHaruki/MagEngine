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
		}
	};
}
int main() { try { MagEngine::EditorInputTests::Run(); std::cout << "EditorInput PASS\n"; return 0; } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
