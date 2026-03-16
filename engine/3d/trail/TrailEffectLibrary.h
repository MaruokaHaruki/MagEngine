/*********************************************************************
 * \file   TrailEffectLibrary.h
 * \brief  トレイルエフェクトライブラリ
 *
 * \author MagEngine
 * \date   March 2026
 * \note   トレイルエフェクトプリセットのJSON永続化を管理
 *********************************************************************/
#pragma once
#include "TrailEffectPreset.h"
#include <map>
#include <string>
#include <vector>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						前方宣言
	class TrailEffectManager;

	///=============================================================================
	///						クラス
	class TrailEffectLibrary {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  複数プリセットをファイルから読み込み
		 * \param  filePath ファイルパス
		 * \param  manager TrailEffectManagerリファレンス
		 * \return 成功時true
		 */
		bool LoadPresets(const std::string &filePath, TrailEffectManager &manager);

		/**----------------------------------------------------------------------------
		 * \brief  複数プリセットをファイルに保存
		 * \param  filePath ファイルパス
		 * \param  manager TrailEffectManagerリファレンス
		 * \return 成功時true
		 */
		bool SavePresets(const std::string &filePath, const TrailEffectManager &manager);

		/**----------------------------------------------------------------------------
		 * \brief  単一プリセットをファイルから読み込み
		 * \param  filePath ファイルパス
		 * \param  outPreset 読み込み結果
		 * \return 成功時true
		 */
		bool LoadPreset(const std::string &filePath, TrailEffectPreset &outPreset);

		/**----------------------------------------------------------------------------
		 * \brief  単一プリセットをファイルに保存
		 * \param  filePath ファイルパス
		 * \param  preset 保存するプリセット
		 * \return 成功時true
		 */
		bool SavePreset(const std::string &filePath, const TrailEffectPreset &preset);

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  JSONオブジェクトからプリセットに変換
		 * \param  j JSONオブジェクト
		 * \return TrailEffectPreset
		 */
		static TrailEffectPreset JsonToPreset(const void *j);

		/**----------------------------------------------------------------------------
		 * \brief  プリセットからJSONオブジェクトに変換
		 * \param  preset プリセット
		 * \return JSONオブジェクト（void*で返却）
		 */
		static void *PresetToJson(const TrailEffectPreset &preset);
	};
}
