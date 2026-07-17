#pragma once

#include "MagMath.h"
#include "Material.h"
#include "TransformationMatrix.h"
#include "VertexData.h"

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace MagEngine {
	class RenderWorld;
	class SpriteSetup;

	struct GlyphMetrics {
		uint32_t codePoint = 0;
		MagMath::Vector2 uvMin{};
		MagMath::Vector2 uvMax{};
		MagMath::Vector2 size{};
		MagMath::Vector2 bearing{};
		float advance = 0.0f;
	};

	struct FontMetrics { float ascender = 0.0f; float descender = 0.0f; float lineHeight = 0.0f; };

	struct TextDrawCommand {
		std::string text;
		MagMath::Vector2 position{};
		MagMath::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		float scale = 1.0f;
		float rotationRadians = 0.0f;
		float drawOrder = 0.0f;
	};

	class TextRenderer {
	public:
		void Initialize(SpriteSetup &spriteSetup, const std::string &texturePath, const std::string &metricsPath);
		void Finalize();
		void BeginFrame();
		void AddText(const TextDrawCommand &command);
		void RegisterRenderables(RenderWorld &renderWorld);
		void Draw();

		bool IsReady() const { return isReady_; }
		uint32_t GetCommandCount() const { return static_cast<uint32_t>(commands_.size()); }
		uint32_t GetGlyphCount() const { return glyphCount_; }
		uint32_t GetDrawCallCount() const { return drawCallCount_; }
		uint32_t GetMissingGlyphCount() const { return missingGlyphCount_; }
		uint32_t GetBufferCapacity() const { return glyphCapacity_; }

	private:
		struct DrawRange { uint32_t indexOffset = 0; uint32_t indexCount = 0; MagMath::Vector4 color{}; };
		bool LoadMetrics(const std::string &metricsPath, std::string &errorMessage);
		bool DecodeUtf8(const std::string &text);
		void BuildGeometry();
		void AppendGlyph(const GlyphMetrics &glyph, float penX, float penY, float scale, const MagMath::Vector2 &rotationOrigin, float rotationRadians);
		void EnsureGpuCapacity(uint32_t requiredGlyphs);
		const GlyphMetrics *FindGlyph(uint32_t codePoint) const;

		SpriteSetup *spriteSetup_ = nullptr;
		std::string texturePath_;
		FontMetrics fontMetrics_{};
		std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
		const GlyphMetrics *replacementGlyph_ = nullptr;
		std::vector<TextDrawCommand> commands_;
		std::vector<uint32_t> decodedCodePoints_;
		std::vector<MagMath::VertexData> vertices_;
		std::vector<uint32_t> indices_;
		std::vector<DrawRange> drawRanges_;
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
		Microsoft::WRL::ComPtr<ID3D12Resource> transformationBuffer_;
		MagMath::VertexData *mappedVertices_ = nullptr;
		uint32_t *mappedIndices_ = nullptr;
		MagMath::Material *materialData_ = nullptr;
		MagMath::TransformationMatrix *transformationData_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		uint32_t glyphCapacity_ = 0;
		uint32_t glyphCount_ = 0;
		uint32_t missingGlyphCount_ = 0;
		uint32_t drawCallCount_ = 0;
		bool isRegistered_ = false;
		bool isReady_ = false;
		static constexpr uint32_t kMaxGlyphsPerFrame = 2048;
	};
}
