/*********************************************************************
 * \file   TrailEffectManager.h
 * \brief  トレイルエフェクトマネージャ
 *
 * \author MagEngine
 * \date   March 2026
 * \note   複数のトレイルエフェクトインスタンスとプリセットを管理
 *********************************************************************/
#pragma once
#include "TrailEffect.h"
#include "TrailEffectPreset.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						前方宣言
	class TrailEffectSetup;

	///=============================================================================
	///						クラス
	class TrailEffectManager {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  初期化
		 * \param  setup TrailEffectSetupポインタ
		 */
		void Initialize(TrailEffectSetup *setup);

		/**----------------------------------------------------------------------------
		 * \brief  更新処理
		 * \param  deltaTime フレーム間の経過時間
		 */
		void Update(float deltaTime);

		/**----------------------------------------------------------------------------
		 * \brief  描画
		 */
		void Draw();

		/**----------------------------------------------------------------------------
		 * \brief  ImGui描画
		 */
		void DrawImGui();

		///--------------------------------------------------------------
		///						 プリセット管理
	public:
		/**----------------------------------------------------------------------------
		 * \brief  Setup を取得
		 * \return TrailEffectSetupポインタ
		 */
		TrailEffectSetup *GetSetup() const {
			return setup_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  プリセートを登録
		 * \param  preset 登録するプリセット
		 */
		void RegisterPreset(const TrailEffectPreset &preset);

		/**----------------------------------------------------------------------------
		 * \brief  プリセットを削除
		 * \param  presetName 削除するプリセット名
		 */
		void UnregisterPreset(const std::string &presetName);

		/**----------------------------------------------------------------------------
		 * \brief  プリセットを更新
		 * \param  presetName プリセット名
		 * \param  newPreset 新しいプリセット内容
		 */
		void UpdatePreset(const std::string &presetName, const TrailEffectPreset &newPreset);

		/**----------------------------------------------------------------------------
		 * \brief  プリセットを取得
		 * \param  presetName プリセット名
		 * \return プリセットポインタ（見つからない場合はnullptr）
		 */
		TrailEffectPreset *GetPreset(const std::string &presetName);

		/**----------------------------------------------------------------------------
		 * \brief  プリセット一覧を取得
		 * \return プリセット名のリスト
		 */
		std::vector<std::string> GetPresetNames() const;

		///--------------------------------------------------------------
		///						 インスタンス管理
	public:
		/**----------------------------------------------------------------------------
		 * \brief  プリセットからインスタンスを作成
		 * \param  presetName プリセット名
		 * \param  instanceName インスタンス名
		 * \return TrailEffectポインタ（失敗時はnullptr）
		 */
		TrailEffect *CreateFromPreset(const std::string &presetName, const std::string &instanceName);

		/**----------------------------------------------------------------------------
		 * \brief  インスタンスを破棄
		 * \param  instanceName インスタンス名
		 */
		void DestroyEffect(const std::string &instanceName);

		/**----------------------------------------------------------------------------
		 * \brief  インスタンスを取得
		 * \param  instanceName インスタンス名
		 * \return TrailEffectポインタ（見つからない場合はnullptr）
		 */
		TrailEffect *GetEffect(const std::string &instanceName);

		/**----------------------------------------------------------------------------
		 * \brief  インスタンスを取得（const版）
		 * \param  instanceName インスタンス名
		 * \return TrailEffect const ポインタ（見つからない場合はnullptr）
		 */
		const TrailEffect *GetEffect(const std::string &instanceName) const;

		///--------------------------------------------------------------
		///						 一括操作
	public:
		/**----------------------------------------------------------------------------
		 * \brief  すべてのエフェクトを破棄
		 */
		void DestroyAllEffects();

		/**----------------------------------------------------------------------------
		 * \brief  エフェクト数を取得
		 * \return エフェクト数
		 */
		size_t GetEffectCount() const {
			return activeEffects_.size();
		}

		/**----------------------------------------------------------------------------
		 * \brief  プリセット数を取得
		 * \return プリセット数
		 */
		size_t GetPresetCount() const {
			return presets_.size();
		}

		///--------------------------------------------------------------
		///						 JSON保存・読み込み
	public:
		/**----------------------------------------------------------------------------
		 * \brief  プリセットをJSONファイルに保存
		 * \param  presetName プリセット名
		 * \param  filePath 保存ファイルパス
		 * \return 成功時はtrue
		 */
		bool SavePresetToJson(const std::string &presetName, const std::string &filePath);

		/**----------------------------------------------------------------------------
		 * \brief  JSONファイルからプリセットを読み込み
		 * \param  filePath 読み込みファイルパス
		 * \return 成功時はtrue
		 */
		bool LoadPresetFromJson(const std::string &filePath);

		/**----------------------------------------------------------------------------
		 * \brief  すべてのプリセットをJSONファイルに保存
		 * \param  filePath 保存ファイルパス
		 * \return 成功時はtrue
		 */
		bool SaveAllPresetsToJson(const std::string &filePath);

		/**----------------------------------------------------------------------------
		 * \brief  JSONファイルからすべてのプリセットを読み込み
		 * \param  filePath 読み込みファイルパス
		 * \return 成功時はtrue
		 */
		bool LoadAllPresetsFromJson(const std::string &filePath);

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		//========================================
		// Setup
		TrailEffectSetup *setup_ = nullptr;

		//========================================
		// プリセット管理
		std::map<std::string, TrailEffectPreset> presets_;

		//========================================
		// インスタンス管理
		std::map<std::string, std::unique_ptr<TrailEffect>> activeEffects_;

		//========================================
		// エディター用
		std::string editingPresetName_;		  ///< 編集中のプリセット名
		bool editingPresetActive_ = false;	  ///< 編集中か
		TrailEffectPreset editingPresetCopy_; ///< 編集中のプリセット（コピー）
		char presetNameBuffer_[128] = {};	  ///< プリセット名入力バッファ
	};
}
