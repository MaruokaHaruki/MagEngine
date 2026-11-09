/*********************************************************************
 * \file EditorTypeRegistry.h
 * \brief 描画型の設定検証と編集表示の明示登録
 *********************************************************************/
#pragma once
#include "EditorViewportMath.h"
#include <functional>
#include <map>
#include <memory>
#include "engine/base/time/FrameTime.h"
namespace MagEngine {
	class Camera;
	class RenderWorld;
	/// @brief 型ごとの描画実体。所有者は編集Host。GPU待機後に破棄する
	class EditorRenderable {
	public:
		virtual ~EditorRenderable() = default;
		virtual void Update(const EditorObjectRecord &record, Camera &camera, const FrameTime &time) = 0;
		virtual void RegisterRenderables(RenderWorld &world) = 0;
		virtual EditorBounds GetBounds() const = 0;
	};
	struct EditorTypeDescriptor {
		std::string typeId;
		nlohmann::json defaultSettings = nlohmann::json::object();
		std::function<bool(const EditorObjectRecord &, std::string &)> validate;
		// 非所有の対象を受け、候補値のみ変更する。実体と履歴は呼出側が扱う。
		std::function<bool(EditorObjectRecord &)> drawSettings;
		std::function<EditorBounds(const EditorObjectRecord &)> getBounds;
		std::function<std::unique_ptr<EditorRenderable>(const EditorObjectRecord &)> create;
	};
	class EditorTypeRegistry {
	public:
		/// @brief 同じ型IDや空のIDは登録しない
		bool Register(EditorTypeDescriptor descriptor) {
			if (descriptor.typeId.empty()) { return false; }
			return types_.emplace(descriptor.typeId, std::move(descriptor)).second;
		}
		/// @brief 登録内容はRegistryの寿命まで有効。不在時nullptr
		const EditorTypeDescriptor *Find(std::string_view typeId) const {
			const auto found = types_.find(std::string(typeId)); return found == types_.end() ? nullptr : &found->second;
		}
	private:
		std::map<std::string, EditorTypeDescriptor> types_;
	};
}
