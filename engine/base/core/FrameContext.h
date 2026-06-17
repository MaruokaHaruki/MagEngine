/*********************************************************************
 * \file   FrameContext.h
 * \brief  フレーム単位のDirectX 12リソースを管理するクラス
 *********************************************************************/
#pragma once

#include <cassert>
#include <cstdint>

#include <d3d12.h>
#include <wrl/client.h>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						クラス
	class FrameContext {
	public:
		/// @brief フレーム専用のCommandAllocatorを初期化
		void Initialize(ID3D12Device &device, D3D12_COMMAND_LIST_TYPE commandListType);

		/// @brief CommandAllocatorを再利用可能な状態へ戻す
		void ResetCommandAllocator();

		/// @brief CommandAllocatorの取得
		ID3D12CommandAllocator *GetCommandAllocator() const {
			return commandAllocator_.Get();
		}

		/// @brief このフレームがGPUへ投入されたときのFence値を取得
		uint64_t GetFenceValue() const {
			return fenceValue_;
		}

		/// @brief このフレームがGPUへ投入されたときのFence値を保存
		void SetFenceValue(uint64_t fenceValue) {
			fenceValue_ = fenceValue;
		}

	private:
		// NOTE: Reset前のGPU完了保証はDirectXCore側で行う。FrameContext単体ではFenceを所有しない。
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
		uint64_t fenceValue_ = 0;
	};
}
