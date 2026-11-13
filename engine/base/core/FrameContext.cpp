/*********************************************************************
 * \file   FrameContext.cpp
 * \brief  フレーム単位のDirectX 12リソースを管理するクラスの実装
 *********************************************************************/
#include "FrameContext.h"
#include <stdexcept>
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						初期化
	void FrameContext::Initialize(ID3D12Device &device, D3D12_COMMAND_LIST_TYPE commandListType) {
		commandAllocator_ = nullptr;
		HRESULT hr = device.CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator_));
		// NOTE: CommandAllocatorが作れない場合は描画基盤を継続できないため、初期化失敗として停止する。
		if (FAILED(hr)) throw std::runtime_error("CreateCommandAllocator failed: HRESULT " + std::to_string(hr));
	}

	///=============================================================================
	///						リセット
	void FrameContext::ResetCommandAllocator() {
		assert(commandAllocator_);
		HRESULT hr = commandAllocator_->Reset();
		// NOTE: GPUがまだ参照中のAllocatorをResetしないことは呼び出し側の不変条件。
		if (FAILED(hr)) throw std::runtime_error("CommandAllocator::Reset failed: HRESULT " + std::to_string(hr));
	}
}
