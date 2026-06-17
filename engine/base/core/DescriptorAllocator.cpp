/*********************************************************************
 * \file   DescriptorAllocator.cpp
 * \brief  DirectX 12 DescriptorHeapの所有と単調増加割り当て実装
 *
 * \author Harukichimaru
 * \date   June 2026
 *********************************************************************/
#include "DescriptorAllocator.h"
#include "Logger.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						初期化
	void DescriptorAllocator::Initialize(ID3D12Device &device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t capacity, bool shaderVisible) {
		assert(capacity > 0);

		heapType_ = heapType;
		capacity_ = capacity;
		nextIndex_ = 0;
		shaderVisible_ = shaderVisible;
		descriptorSize_ = device.GetDescriptorHandleIncrementSize(heapType_);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType_;
		descriptorHeapDesc.NumDescriptors = capacity_;
		descriptorHeapDesc.Flags = shaderVisible_ ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		HRESULT hr = device.CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap_));
		assert(SUCCEEDED(hr));
		if(FAILED(hr)) {
			Logger::Log("Failed to create descriptor heap.", Logger::LogLevel::Error);
		}
	}

	///=============================================================================
	///						確保
	DescriptorHandle DescriptorAllocator::Allocate() {
		assert(descriptorHeap_);
		assert(nextIndex_ < capacity_);
		if(nextIndex_ >= capacity_) {
			Logger::Log("DescriptorAllocator capacity exceeded.", Logger::LogLevel::Error);
			return {};
		}

		DescriptorHandle handle = GetHandle(nextIndex_);
		nextIndex_++;
		return handle;
	}

	///=============================================================================
	///						Handle取得
	DescriptorHandle DescriptorAllocator::GetHandle(uint32_t index) const {
		assert(descriptorHeap_);
		assert(index < capacity_);

		if(!descriptorHeap_ || index >= capacity_) {
			return {};
		}

		DescriptorHandle handle{};
		handle.index = index;
		handle.cpuHandle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		handle.cpuHandle.ptr += static_cast<SIZE_T>(descriptorSize_) * index;

		if(shaderVisible_) {
			handle.gpuHandle = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
			handle.gpuHandle.ptr += static_cast<UINT64>(descriptorSize_) * index;
		}

		return handle;
	}
}
