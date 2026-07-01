#include "PipelineRecipe.h"

namespace MagEngine {
	PipelineRecipeValidationResult PipelineRecipe::Validate() const {
		if(vertexShader.path.empty() || vertexShader.profile.empty()) {
			return PipelineRecipeValidationResult{false, PipelineRecipeValidationError::MissingVertexShader};
		}
		if(pixelShader.path.empty() || pixelShader.profile.empty()) {
			return PipelineRecipeValidationResult{false, PipelineRecipeValidationError::MissingPixelShader};
		}
		if(rootSignature == nullptr) {
			return PipelineRecipeValidationResult{false, PipelineRecipeValidationError::MissingRootSignature};
		}
		if(renderTargetFormat == DXGI_FORMAT_UNKNOWN) {
			return PipelineRecipeValidationResult{false, PipelineRecipeValidationError::MissingRenderTargetFormat};
		}
		return PipelineRecipeValidationResult{};
	}
}
