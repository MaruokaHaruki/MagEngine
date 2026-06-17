/*********************************************************************
 * \file   DescriptorHandle.h
 * \brief  DirectX 12ディスクリプタのIndexとHandleをまとめる型
 *
 * \author Harukichimaru
 * \date   June 2026
 * \note   DescriptorAllocatorから返された値だけを描画側で保持する
 *********************************************************************/
#pragma once

#include <cstdint>
#include <d3d12.h>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						DescriptorHandle
	struct DescriptorHandle {
		static constexpr uint32_t InvalidIndex = UINT32_MAX;

		uint32_t index = InvalidIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};

		/// @brief 有効なディスクリプタか
		bool IsValid() const {
			return index != InvalidIndex && cpuHandle.ptr != 0;
		}
	};
}
