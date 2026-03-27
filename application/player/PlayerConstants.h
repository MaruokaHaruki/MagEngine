/*********************************************************************
 * \file   PlayerConstants.h
 * \brief  プレイヤーシステム全体で使用する定数定義
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   マジックナンバーを排除し、一元管理することで
 *         保守性とバランス調整の容易性を向上
 *********************************************************************/
#pragma once

///=============================================================================
///						プレイヤーシステム定数名前空間
namespace PlayerConstants {

	//========================================
	//          タイミング関連定数
	//========================================
	/// @brief 固定フレームタイム（60FPS想定）
	constexpr float FRAME_TIME = 1.0f / 60.0f;

	/// @brief フレームレート
	constexpr float FRAME_RATE = 60.0f;

	//========================================
	//          入力関連定数
	//========================================
	/// @brief アナログスティックのデッドゾーン閾値
	constexpr float STICK_DEADZONE = 0.1f;

	/// @brief トリガー入力の有効判定閾値
	constexpr float TRIGGER_THRESHOLD = 0.3f;

	//========================================
	//          武装デフォルト設定
	//========================================
	namespace Weapon {
		//---- 弾（機銃）関連 ----
		/// @brief 弾の飛行速度（units/秒）
		constexpr float BULLET_SPEED = 128.0f;

		/// @brief 弾の生存時間（秒）
		constexpr float BULLET_LIFETIME = 3.0f;

		/// @brief 弾の当たり判定半径（units）
		constexpr float BULLET_RADIUS = 0.5f;

		/// @brief 連射クールタイム（秒）
		constexpr float SHOOT_COOLDOWN = 0.1f;

		//---- ミサイル関連 ----
		/// @brief ミサイルの飛行速度（units/秒）
		constexpr float MISSILE_SPEED = 50.0f;

		/// @brief ミサイルの最大旋回速度（度/秒）
		constexpr float MISSILE_TURN_RATE = 120.0f;

		/// @brief ミサイルの生存時間（秒）
		constexpr float MISSILE_LIFETIME = 15.0f;

		/// @brief ミサイル最大残弾数
		constexpr int MISSILE_MAX_AMMO = 3;

		/// @brief ミサイル1発あたりの回復時間（秒）
		constexpr float MISSILE_RECOVERY_TIME = 3.0f;
	}

	//========================================
	//          移動システム定数
	//========================================
	namespace Movement {
		/// @brief 基本移動速度（units/秒）
		constexpr float DEFAULT_MOVE_SPEED = 5.0f;

		/// @brief 加速度（速度補間係数）
		constexpr float DEFAULT_ACCELERATION = 0.1f;

		/// @brief 回転の滑らかさ（回転補間係数）
		constexpr float DEFAULT_ROTATION_SMOOTHING = 0.1f;

		/// @brief 最大ロール角度（度）
		constexpr float MAX_ROLL_ANGLE = 30.0f;

		/// @brief 最大ピッチ角度（度）
		constexpr float MAX_PITCH_ANGLE = 15.0f;
	}

	//========================================
	//          ブーストシステム定数
	//========================================
	namespace Boost {
		/// @brief ブーストゲージ最大値
		constexpr float MAX_GAUGE = 100.0f;

		/// @brief ブースト時の速度倍率
		constexpr float SPEED_MULTIPLIER = 2.0f;

		/// @brief ブーストゲージ消費速度（per 秒）
		constexpr float CONSUMPTION_RATE = 30.0f;

		/// @brief ブーストゲージ回復速度（per 秒）
		constexpr float RECOVERY_RATE = 15.0f;
	}

	//========================================
	//          バレルロールシステム定数
	//========================================
	namespace BarrelRoll {
		/// @brief バレルロール実行時間（秒）
		constexpr float DURATION = 0.6f;

		/// @brief バレルロールクールダウン時間（秒）
		constexpr float COOLDOWN = 1.2f;

		/// @brief バレルロール実行コスト（ゲージ消費量）
		constexpr float COST = 30.0f;

		/// @brief バレルロール時の加速度倍率
		constexpr float ACCELERATION_MULTIPLIER = 2.0f;

		/// @brief 回転角度（360度 = 2π）
		constexpr float ROTATION_ANGLE_RADIANS = 6.28318530718f; // 2 * π
	}

	//========================================
	//          ロックオンシステム定数
	//========================================
	namespace LockOn {
		/// @brief ロックオン有効距離（units）
		constexpr float RANGE = 50.0f;

		/// @brief ロックオン有効視野角（度）
		constexpr float FOV_DEGREES = 180.0f;

		/// @brief ロックオン獲得間隔（秒）
		constexpr float ACQUISITION_INTERVAL = 0.35f;

		/// @brief 最大同時ロックオン数
		constexpr int MAX_TARGETS = 3;

		/// @brief ロックオン保持時間（ターゲットが範囲外に出た後）
		constexpr float RETENTION_TIME = 0.5f;
	}

	//========================================
	//          ジャストアボイダンスシステム定数
	//========================================
	namespace JustAvoidance {
		/// @brief ジャスト回避判定ウィンドウサイズ（秒）
		constexpr float WINDOW_SIZE = 0.3f;

		/// @brief ジャスト回避成功時のブーストゲージ報酬
		constexpr float BOOST_REWARD = 30.0f;

		/// @brief 被弾予告情報のタイムアウト時間（秒）
		constexpr float DAMAGE_TIMEOUT = 1.0f;

		/// @brief ジャスト判定の精度閾値（0-1, 1.0=完璧なタイミング）
		constexpr float PERFECT_TIMING_THRESHOLD = 0.8f;
	}

	//========================================
	//          ヘルスシステム定数
	//========================================
	namespace Health {
		/// @brief デフォルト最大HP
		constexpr int DEFAULT_MAX_HP = 100;

		/// @brief 被弾後の無敵時間（秒）
		constexpr float INVINCIBILITY_DURATION = 1.0f;

		/// @brief 敵弾による標準ダメージ量
		constexpr int ENEMY_BULLET_DAMAGE = 15;

		/// @brief 敵との衝突による標準ダメージ量
		constexpr int COLLISION_DAMAGE = 10;
	}

	//========================================
	//          敗北演出定数
	//========================================
	namespace Defeat {
		/// @brief 敗北演出全体の長さ（秒）
		constexpr float ANIMATION_DURATION = 3.0f;

		/// @brief フェーズ1の割合（0-1）
		constexpr float PHASE1_RATIO = 0.6f;

		/// @brief 落下時の重力加速度（units/秒²）
		constexpr float GRAVITY_ACCELERATION = 15.0f;

		/// @brief 演出終了判定のY座標閾値
		constexpr float GROUND_Y_THRESHOLD = -50.0f;

		/// @brief ノーズダイブ角度（度）
		constexpr float NOSE_DIVE_ANGLE = -90.0f;
	}

	//========================================
	//          デバッグ/開発用定数
	//========================================
	namespace Debug {
		/// @brief デバッグライン表示時の色（RGB）
		constexpr float LINE_COLOR_R = 1.0f;
		constexpr float LINE_COLOR_G = 0.0f;
		constexpr float LINE_COLOR_B = 0.0f;

		/// @brief デバッグコーン表示の分割数
		constexpr int CONE_SEGMENTS = 16;
	}

} // namespace PlayerConstants
