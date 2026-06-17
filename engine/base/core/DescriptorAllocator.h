/*********************************************************************
 * \file   DescriptorAllocator.h
 * \brief  DirectX 12 DescriptorHeapの所有と単調増加割り当て
 *
 * \author Harukichimaru
 * \date   June 2026
 * \note   永続Descriptor向け。解放はAllocator破棄時にまとめて行う。
 *********************************************************************/
#pragma once

#include "DescriptorHandle.h"
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						DescriptorCapacity
	namespace DescriptorCapacity {
		constexpr uint32_t Rtv = 32;
		constexpr uint32_t Dsv = 16;
		constexpr uint32_t Resource = 512;
		constexpr uint32_t Sampler = 64;
	}

	///=============================================================================
	///						DescriptorAllocator
	class DescriptorAllocator {
	public:
		DescriptorAllocator() = default;
		~DescriptorAllocator() = default;

		DescriptorAllocator(const DescriptorAllocator &) = delete;
		DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;

		/// @brief DescriptorHeapを生成し、単調増加Allocatorとして初期化する
		void Initialize(ID3D12Device &device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t capacity, bool shaderVisible);

		/// @brief 次の空きDescriptorを確保する
		DescriptorHandle Allocate();

		/// @brief 確保済みIndexからHandleを再取得する
		DescriptorHandle GetHandle(uint32_t index) const;

		ID3D12DescriptorHeap *GetHeap() const {
			return descriptorHeap_.Get();
		}

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const {
			return heapType_;
		}

		uint32_t GetCapacity() const {
			return capacity_;
		}

		uint32_t GetAllocatedCount() const {
			return nextIndex_;
		}

		uint32_t GetRemainingCount() const {
			return capacity_ - nextIndex_;
		}

		bool IsShaderVisible() const {
			return shaderVisible_;
		}

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;

		D3D12_DESCRIPTOR_HEAP_TYPE heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		uint32_t descriptorSize_ = 0;
		uint32_t capacity_ = 0;
		uint32_t nextIndex_ = 0;
		bool shaderVisible_ = false;
	};
}
