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

	/// @brief フォントアトラス内の1文字に対応する描画メトリクス
	/// @note UVと寸法はメトリクスファイルとアトラス画像の組み合わせに依存する。
	struct GlyphMetrics {
		uint32_t codePoint = 0;
		MagMath::Vector2 uvMin{};
		MagMath::Vector2 uvMax{};
		MagMath::Vector2 size{};
		MagMath::Vector2 bearing{};
		float advance = 0.0f;
	};

	/// @brief ベースライン計算に使用するフォント全体のメトリクス
	struct FontMetrics { float ascender = 0.0f; float descender = 0.0f; float lineHeight = 0.0f; };

	/// @brief フレーム内に追加する文字列描画要求
	/// @note textはAddText()時点でコピーされる。座標はSprite描画系と同じ画面空間として扱う。
	struct TextDrawCommand {
		std::string text;
		MagMath::Vector2 position{};
		MagMath::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		float scale = 1.0f;
		float rotationRadians = 0.0f;
		float drawOrder = 0.0f;
	};

	/// @brief フォントアトラスから文字列の頂点を生成し、RenderWorldへ描画要求を登録するクラス
	/// @details Draw要求はフレーム単位で蓄積する。BeginFrame()を呼ばないと前フレームの要求が残る。
	/// @note GPUバッファは内部で所有し、SpriteSetupとRenderWorldは呼び出し側が所有する非所有参照。
	class TextRenderer {
	public:
		/// @brief フォントアトラスと文字メトリクスを読み込む
		/// @note 読み込みに失敗した場合は例外を送出し、IsReady()はfalseのままとなる。
		void Initialize(SpriteSetup &spriteSetup, const std::string &texturePath, const std::string &metricsPath);
		/// @brief 内部GPUリソースを解放
		void Finalize();
		/// @brief 前フレームの描画要求と統計値をリセット
		/// @note 毎フレーム、AddText()より先に1回だけ呼び出す。
		void BeginFrame();
		/// @brief 文字列描画要求を追加
		/// @note kMaxGlyphsPerFrameを超える文字は描画されないため、UI文言は上限内に収める。
		void AddText(const TextDrawCommand &command);
		/// @brief 蓄積した文字列を現フレームの描画対象として登録
		/// @note RegisterRenderables()後に要求を追加しても当該フレームの描画には反映されない。
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

		SpriteSetup *spriteSetup_ = nullptr; // Engine側が所有するスプライト設定への非所有参照。Finalize()前まで有効である必要がある。
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
		static constexpr uint32_t kMaxGlyphsPerFrame = 2048; // 毎フレームの頂点・インデックス転送量を固定上限に抑える。
	};
}
