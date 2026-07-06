/*********************************************************************
 * \file   PostEffectParameterSet.h
 * \brief  PostEffect専用Root Parameter Binding補助
 *
 * \author Harukichimaru
 * \date   July 2026
 *********************************************************************/
#pragma once

#include <cassert>
#include <cstdint>
#include <d3d12.h>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	struct PostEffectBindingLayout {
		static constexpr uint32_t kInvalidRootParameter = UINT32_MAX;

		uint32_t sourceTextureRootParameter = kInvalidRootParameter;
		uint32_t constantBufferRootParameter = kInvalidRootParameter;

		[[nodiscard]]
		bool HasSourceTexture() const {
			return sourceTextureRootParameter != kInvalidRootParameter;
		}

		[[nodiscard]]
		bool HasConstantBuffer() const {
			return constantBufferRootParameter != kInvalidRootParameter;
		}
	};

	enum class PostEffectParameterValidationError : uint8_t {
		None,
		MissingSourceTextureRootParameter,
		MissingConstantBufferRootParameter,
		DuplicateRootParameter,
	};

	struct PostEffectParameterValidationResult {
		bool isValid = true;
		PostEffectParameterValidationError error = PostEffectParameterValidationError::None;
	};

	class PostEffectParameterSet {
	public:
		explicit PostEffectParameterSet(PostEffectBindingLayout layout)
			: layout_(layout) {
		}

		[[nodiscard]]
		const PostEffectBindingLayout &GetLayout() const {
			return layout_;
		}

		void SetSourceTexture(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
			sourceTexture_ = handle;
		}

		void SetConstantBuffer(D3D12_GPU_VIRTUAL_ADDRESS address) {
			constantBuffer_ = address;
		}

		[[nodiscard]]
		D3D12_GPU_DESCRIPTOR_HANDLE GetSourceTextureForTesting() const {
			return sourceTexture_;
		}

		[[nodiscard]]
		D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferForTesting() const {
			return constantBuffer_;
		}

		[[nodiscard]]
		PostEffectParameterValidationResult Validate() const {
			if(!layout_.HasSourceTexture()) {
				return {false, PostEffectParameterValidationError::MissingSourceTextureRootParameter};
			}
			if(constantBuffer_ != 0 && !layout_.HasConstantBuffer()) {
				return {false, PostEffectParameterValidationError::MissingConstantBufferRootParameter};
			}
			if(layout_.HasConstantBuffer() && layout_.sourceTextureRootParameter == layout_.constantBufferRootParameter) {
				return {false, PostEffectParameterValidationError::DuplicateRootParameter};
			}
			return {};
		}

		void Bind(ID3D12GraphicsCommandList &commandList) const {
			const PostEffectParameterValidationResult validation = Validate();
			assert(validation.isValid && "PostEffectParameterSet has invalid Root Parameter layout.");

			// NOTE: 現行PostEffect RootSignatureはSRVのみだが、将来のCBV付きEffectでも呼び出し側を増やさないためここへ集約する。
			if(sourceTexture_.ptr != 0 && layout_.HasSourceTexture()) {
				commandList.SetGraphicsRootDescriptorTable(layout_.sourceTextureRootParameter, sourceTexture_);
			}
			if(constantBuffer_ != 0 && layout_.HasConstantBuffer()) {
				commandList.SetGraphicsRootConstantBufferView(layout_.constantBufferRootParameter, constantBuffer_);
			}
		}

	private:
		PostEffectBindingLayout layout_{};
		D3D12_GPU_DESCRIPTOR_HANDLE sourceTexture_{};
		D3D12_GPU_VIRTUAL_ADDRESS constantBuffer_ = 0;
	};
}
