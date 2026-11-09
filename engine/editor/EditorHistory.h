/*********************************************************************
 * \file EditorHistory.h
 * \brief 編集値のUndo／Redo。GPU資源や選択ポインタは保持しない
 *********************************************************************/
#pragma once
#include "EditorDocument.h"
#include <deque>
namespace MagEngine {
	///=============================================================================
	/// 編集履歴
	class EditorHistory {
	public:
		/// @brief 検証済みの候補を1操作として記録。同じ値または不正な値は変更しない
		bool Execute(EditorDocument &document, const EditorDocument &replacement);
		bool Undo(EditorDocument &document);
		bool Redo(EditorDocument &document);
		void Reset();
	private:
		///--------------------------------------------------------------
		/// 値の履歴。初期版は100操作に限定
		std::deque<EditorDocument> undo_;
		std::deque<EditorDocument> redo_;
	};
}
