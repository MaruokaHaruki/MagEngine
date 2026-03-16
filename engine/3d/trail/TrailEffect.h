/*********************************************************************
 * \file   TrailEffect.h
 * \brief  トレイルエフェクト描画・統合管理クラス
 *
 * \author MagEngine
 * \date   March 2026
 * \note   Transformベースでトレイルエフェクトを統合管理
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "TrailEffectPreset.h"
#include "TrailEmitter.h"
#include <stdexcept>
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						前方宣言
	class TrailEffectSetup;

	///=============================================================================
	///						クラス
	class TrailEffect {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  初期化
		 * \param  setup TrailEffectSetupポインタ
		 * \param  preset トレイルエフェクトプリセット
		 */
		void Initialize(TrailEffectSetup *setup, const TrailEffectPreset &preset);

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

		/**----------------------------------------------------------------------------
		 * \brief  プリセット適用
		 * \param  preset トレイルエフェクトプリセット
		 */
		void ApplyPreset(const TrailEffectPreset &preset);

		/**----------------------------------------------------------------------------
		 * \brief  軌跡を追加
		 * \param  position 位置
		 * \param  velocity 速度
		 */
		void EmitAt(const MagMath::Vector3 &position, const MagMath::Vector3 &velocity);

		/**----------------------------------------------------------------------------
		 * \brief  すべての軌跡をクリア
		 */
		void ClearTrails();

		///--------------------------------------------------------------
		///						 入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  GetTransform Transformの取得
		 * \return Transform参照
		 */
		MagMath::Transform &GetTransform() {
			return transform_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetTransform Transformの取得（const版）
		 * \return Transform参照
		 */
		const MagMath::Transform &GetTransform() const {
			return transform_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetPosition 位置の設定
		 * \param  pos 位置
		 */
		void SetPosition(const MagMath::Vector3 &pos) {
			transform_.translate = pos;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetScale スケールの設定
		 * \param  scale スケール
		 */
		void SetScale(const MagMath::Vector3 &scale) {
			transform_.scale = scale;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetPresetName プリセット名の取得
		 * \return プリセット名
		 */
		const std::string &GetPresetName() const {
			return currentPreset_.name;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetCurrentPreset 現在のプリセット取得
		 * \return TrailEffectPreset参照
		 */
		const TrailEffectPreset &GetCurrentPreset() const {
			return currentPreset_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetMutablePreset 可変プリセット取得
		 * \return TrailEffectPreset参照（可変）
		 */
		TrailEffectPreset &GetMutablePreset() {
			return currentPreset_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetEnabled 有効/無効の設定
		 * \param  enabled 有効フラグ
		 */
		void SetEnabled(bool enabled) {
			enabled_ = enabled;
		}

		/**----------------------------------------------------------------------------
		 * \brief  IsEnabled 有効/無効の取得
		 * \return 有効フラグ
		 */
		bool IsEnabled() const {
			return enabled_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetParticleCount パーティクル数の取得
		 * \return パーティクル数
		 */
		size_t GetParticleCount() const {
			return emitter_.GetParticleCount();
		}

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		//========================================
		// Emitterと設定
		TrailEmitter emitter_;
		TrailEffectSetup *setup_ = nullptr;

		//========================================
		// Transform（トレイルの位置・回転・スケール）
		MagMath::Transform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

		//========================================
		// プリセット情報
		TrailEffectPreset currentPreset_;

		//========================================
		// その他
		bool enabled_ = true;
	};
}
