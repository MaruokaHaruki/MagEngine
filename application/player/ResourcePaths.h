/*********************************************************************
 * \file   ResourcePaths.h
 * \brief  プレイヤーシステムで使用するリソースパスの定数定義
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   文字列リテラルのタイポを防ぎ、IDE補完を活用するため
 *         リソースパスを一元管理
 *********************************************************************/
#pragma once

///=============================================================================
///						リソースパス定数名前空間
namespace ResourcePath {

	//========================================
	//          3Dモデルパス
	//========================================
	namespace Model {
		/// @brief プレイヤー機体モデル
		constexpr const char *PLAYER = "Player.obj";

		/// @brief 弾丸モデル
		constexpr const char *BULLET = "Bullet.obj";

		/// @brief ミサイルモデル
		constexpr const char *MISSILE = "Missile.obj";
	}

	//========================================
	//          テクスチャパス
	//========================================
	namespace Texture {
		/// @brief プレイヤーデフォルトテクスチャ（未使用時は空文字列）
		constexpr const char *PLAYER_DEFAULT = "";

		/// @brief 弾丸デフォルトテクスチャ（未使用時は空文字列）
		constexpr const char *BULLET_DEFAULT = "";

		/// @brief ミサイルデフォルトテクスチャ（未使用時は空文字列）
		constexpr const char *MISSILE_DEFAULT = "";
	}

	//========================================
	//          サウンドパス（将来拡張用）
	//========================================
	namespace Sound {
		/// @brief 機銃発射音
		constexpr const char *GUN_FIRE = "sounds/gun_fire.wav";

		/// @brief ミサイル発射音
		constexpr const char *MISSILE_LAUNCH = "sounds/missile_launch.wav";

		/// @brief ロックオン完了音
		constexpr const char *LOCK_ON = "sounds/lock_on.wav";

		/// @brief ジャストアボイダンス成功音
		constexpr const char *JUST_AVOIDANCE_SUCCESS = "sounds/just_avoidance.wav";

		/// @brief ブースト起動音
		constexpr const char *BOOST_START = "sounds/boost_start.wav";

		/// @brief バレルロール音
		constexpr const char *BARREL_ROLL = "sounds/barrel_roll.wav";

		/// @brief 被弾音
		constexpr const char *DAMAGE = "sounds/damage.wav";

		/// @brief 撃墜音
		constexpr const char *DEFEAT = "sounds/defeat.wav";
	}

} // namespace ResourcePath
