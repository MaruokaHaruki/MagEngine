/*********************************************************************
 * \file EditorDocumentStore.h
 * \brief 制作ルートに限定した編集文書の安全な保存
 *********************************************************************/
#pragma once
#include "EditorDocument.h"
#include <filesystem>
#include <map>
namespace MagEngine {
	class EditorTypeRegistry;
	class EditorDocumentStore {
	public:
		/// @brief rootは制作プロジェクトの絶対パス。資産はroot/resources基準
		explicit EditorDocumentStore(std::filesystem::path root, const EditorTypeRegistry *registry = nullptr);
		bool Load(const std::filesystem::path &path, EditorDocument &document, std::string &error);
		bool Save(const std::filesystem::path &path, const EditorDocument &document, std::string &error);
		bool ImportBlender(const std::filesystem::path &path, EditorDocument &document, std::string &error);
		bool ValidateAssets(const EditorDocument &document, std::string &error) const;
	private:
		std::filesystem::path root_;
		const EditorTypeRegistry *registry_ = nullptr; // 非所有。Storeより長寿命の登録情報
		std::map<std::filesystem::path, std::string> knownFiles_;
	};
}
