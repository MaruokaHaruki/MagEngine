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
	/// @details 0.1 より小さい入力値は無視される
	/// @note スティックのドリフト対策。値が大きいほど反応が悪くなる
	constexpr float STICK_DEADZONE = 0.1f;

	/// @brief トリガー入力の有効判定閾値
	/// @details 0.3 以上の入力で射撃/ミサイル発射が開始
	/// @note 軽い押し込みを無視し、意図的な操作のみを認識
	constexpr float TRIGGER_THRESHOLD = 0.3f;

	//========================================
	//          武装デフォルト設定
	//========================================
	namespace Weapon {
		//---- 弾（機銃）関連 ----
		/// @brief 弾の飛行速度（units/秒）
		/// @details 敵が回避困難になる速度。値が小さいと敵が素早く回避可能
		/// @note BULLET_LIFETIME との積が有効距離: 180 × 3 = 540 units
		constexpr float BULLET_SPEED = 180.0f;

		/// @brief 弾の生存時間（秒）
		/// @details この時間を超えた弾は自動削除される
		/// @warning ステージサイズが大きい場合、この値を増やすと処理負荷が増加
		constexpr float BULLET_LIFETIME = 3.0f;

		/// @brief 弾の当たり判定半径（units）
		/// @details 敵の当たり判定との交差判定に使用
		/// @note 見た目より大きいと違和感、小さいと当たりにくくなる
		constexpr float BULLET_RADIUS = 0.65f;

		/// @brief 連射クールタイム（秒）
		/// @details この時間ごとに1発ずつ発射（逆数が発射レート）
		/// @note 1/0.1 = 毎秒10発。値が大きいほど低速連射
		constexpr float SHOOT_COOLDOWN = 0.1f;

		//---- ミサイル関連 ----
		/// @brief ミサイルの飛行速度（units/秒）
		/// @details 敵追尾のためにBULLET_SPEEDより遅く設定
		/// @note 速すぎるとロックオンの意味が薄れ、遅すぎると到達不可能
		/// @note MISSILE_LIFETIME × MISSILE_SPEED = 最大追尾距離（750 units）
		constexpr float MISSILE_SPEED = 50.0f;

		/// @brief ミサイルの最大旋回速度（度/秒）
		/// @details 敵を追尾する際の最大回転速度
		/// @warning 大きすぎると発射地点に戻って自爆の危険
		/// @warning 小さすぎると敵が素早く移動すると追従不可能
		constexpr float MISSILE_TURN_RATE = 120.0f;

		/// @brief ミサイルの生存時間（秒）
		/// @details ターゲット命中時またはこの時間経過で自動削除
		/// @note ステージサイズと敵の最大移動距離を考慮して調整
		constexpr float MISSILE_LIFETIME = 15.0f;

		/// @brief ミサイル最大残弾数
		/// @details 同時に保持できる最大ミサイル本数
		/// @note ゲーム難易度の重要な要素。敵の同時ロック数に直結
		constexpr int MISSILE_MAX_AMMO = 3;

		/// @brief ミサイル1発あたりの回復時間（秒）
		/// @details 1発のミサイルが回復するまでの時間
		/// @note 総回復時間 = MISSILE_MAX_AMMO × MISSILE_RECOVERY_TIME = 9秒
		/// @note 回復が遅いと敵が逃げ切りやすくなる
		constexpr float MISSILE_RECOVERY_TIME = 3.0f;
	}

	//========================================
	//          移動システム定数
	//========================================
	namespace Movement {
		/// @brief 基本移動速度（units/秒）
		/// @details キー入力時の通常移動速度
		/// @note ブースト中は Boost::SPEED_MULTIPLIER が適用されて 10.0 units/秒になる
		constexpr float DEFAULT_MOVE_SPEED = 5.0f;

		/// @brief 加速度（速度補間係数）
		/// @details 入力値から実際の速度への補間係数（0-1）
		/// @note 値が大きいほど瞬時に加速、小さいほど滑らか
		/// @note 0.1 で約6フレームで目標速度の98%に到達（63フレーム/秒時）
		constexpr float DEFAULT_ACCELERATION = 0.1f;

		/// @brief 回転の滑らかさ（回転補間係数）
		/// @details 機体方向の補間係数。小さいほどカメラ追従が滑らか
		/// @note 値が大きいほど敏捷で素早い向き変え
		constexpr float DEFAULT_ROTATION_SMOOTHING = 0.1f;

		/// @brief 最大ロール角度（度）
		/// @details 左右への傾き最大値。視覚的フィードバック用
		/// @note カメラ演出に使用。操作可能範囲は制限されない
		constexpr float MAX_ROLL_ANGLE = 30.0f;

		/// @brief 最大ピッチ角度（度）
		/// @details 上下への傾き最大値。視覚的フィードバック用
		/// @note 実際の移動には影響しない（旋回用）
		constexpr float MAX_PITCH_ANGLE = 15.0f;
	}

	//========================================
	//          ブーストシステム定数
	//========================================
	namespace Boost {
		/// @brief ブーストゲージ最大値
		/// @details HUD のゲージが 0 ～ 100 で表示される
		/// @note 満タン時に継続ブースト可能時間 = 100 / CONSUMPTION_RATE ≈ 3.3秒
		constexpr float MAX_GAUGE = 100.0f;

		/// @brief ブースト時の速度倍率
		/// @details ブースト中: DEFAULT_MOVE_SPEED × 2.0 = 10.0 units/秒
		/// @note 敵弾を避けるため、充分な速度上昇が必要
		constexpr float SPEED_MULTIPLIER = 2.0f;

		/// @brief ブーストゲージ消費速度（per 秒）
		/// @details ブースト押下中、毎秒 30 ゲージが消費される
		/// @note ゲージ 100 で約 3.3 秒間ブースト可能
		constexpr float CONSUMPTION_RATE = 30.0f;

		/// @brief ブーストゲージ回復速度（per 秒）
		/// @details ブースト非使用時、毎秒 15 ゲージが回復
		/// @note ジャスト回避成功時は JustAvoidance::BOOST_REWARD で追加回復
		/// @note 消費 > 回復（30 > 15）のため、継続使用不可
		constexpr float RECOVERY_RATE = 15.0f;
	}

	//========================================
	//          バレルロールシステム定数
	//========================================
	namespace BarrelRoll {
		/// @brief バレルロール実行時間（秒）
		/// @details ロール アニメーション完了までの時間
		/// @note この間、プレイヤーは敵弾を検知しない（一部判定無視）
		constexpr float DURATION = 0.6f;

		/// @brief バレルロールクールダウン時間（秒）
		/// @details 実行完了後、次のロールまでの待機時間
		/// @warning COOLDOWN > DURATION なため、連続実行不可
		constexpr float COOLDOWN = 1.2f;

		/// @brief バレルロール実行コスト（ゲージ消費量）
		/// @details 1回のロール実行でブーストゲージが 30 消費される
		/// @note 最大ゲージ 100 で約 3 回ロール実行可能
		constexpr float COST = 30.0f;

		/// @brief バレルロール時の加速度倍率
		/// @details ロール中の移動加速度が通常の 2 倍になる
		/// @note 敵弾から素早く逃げるための機動性向上
		constexpr float ACCELERATION_MULTIPLIER = 2.0f;

		/// @brief 回転角度（360度 = 2π）
		/// @details バレルロール時の 1 回転の総角度（ラジアン）
		/// @note 正確には 2π = 6.283185... (計算値を定数化)
		constexpr float ROTATION_ANGLE_RADIANS = 6.28318530718f; // 2 * π
	}

	//========================================
	//          ロックオンシステム定数
	//========================================
	namespace LockOn {
		/// @brief ロックオン有効距離（units）
		/// @details この距離内の敵のみロック対象になる
		/// @note 広すぎると操作困難、狭すぎるとロック不可能になる
		constexpr float RANGE = 50.0f;

		/// @brief ロックオン有効視野角（度）
		/// @details 180度 = 前方180度以内の敵のみロック対象
		/// @note 360度に設定すると背後の敵もロック可能
		constexpr float FOV_DEGREES = 180.0f;

		/// @brief ロックオン獲得間隔（秒）
		/// @details この時間ごとに新しいロック対象を獲得
		/// @note 短いほど敵をロックしやすい、長いほど難しい
		/// @note 0.35秒 ≈ 21フレーム（60FPS時）
		constexpr float ACQUISITION_INTERVAL = 0.35f;

		/// @brief 最大同時ロックオン数
		/// @details 同時にロック可能な敵の最大数
		/// @note ゲーム難易度とミサイル最大数に直結
		/// @warning MAX_TARGETS > Weapon::MISSILE_MAX_AMMO にすることで
		///          敵を選別してミサイルを発射できる
		constexpr int MAX_TARGETS = 3;

		/// @brief ロックオン保持時間（ターゲットが範囲外に出た後）
		/// @details この時間内に敵が範囲内に戻ればロック継続
		/// @note 範囲外に出たら即座にロック解除となるわけではない
		constexpr float RETENTION_TIME = 0.5f;
	}

	//========================================
	//          ジャストアボイダンスシステム定数
	//========================================
	namespace JustAvoidance {
		/// @brief ジャスト回避判定ウィンドウサイズ（秒）
		/// @details 敵弾接近時、この時間帯内のボタン押下でジャスト判定
		/// @note 0.3秒 ≈ 18フレーム（60FPS時）。短いほど難易度が高い
		/// @warning ウィンドウが小さすぎるとプレイヤーが回避困難になる
		constexpr float WINDOW_SIZE = 0.3f;

		/// @brief ジャスト回避成功時のブーストゲージ報酬
		/// @details ジャスト成功で ブーストゲージが 30 回復される
		/// @note Boost::RECOVERY_RATE (15/秒) より多くの回復が得られる
		constexpr float BOOST_REWARD = 30.0f;

		/// @brief 被弾予告情報のタイムアウト時間（秒）
		/// @details 被弾通知が表示されて、この時間で消える
		/// @note UI のフェードアウト速度の制御に使用
		constexpr float DAMAGE_TIMEOUT = 1.0f;

		/// @brief ジャスト判定の精度閾値（0-1, 1.0=完璧なタイミング）
		/// @details スロー強度と攻撃力倍率の計算に使用
		/// @note 0.8 以上で「完璧」判定となり、最大スロー効果が適用
		/// @note 0.5 未満だと判定失敗扱い
		constexpr float PERFECT_TIMING_THRESHOLD = 0.8f;
	}

	//========================================
	//          ヘルスシステム定数
	//========================================
	namespace Health {
		/// @brief デフォルト最大HP
		/// @details プレイヤーの初期体力上限
		/// @note ゲーム難易度調整で頻繁に変更されるパラメータ
		constexpr int DEFAULT_MAX_HP = 100;

		/// @brief 被弾後の無敵時間（秒）
		/// @details ダメージ受け取り後、この時間は追加ダメージを受けない
		/// @note 連続被弾から守るため、1.0秒間保護される
		/// @warning 無敵時間中のダメージは UI で警告表示される
		constexpr float INVINCIBILITY_DURATION = 1.0f;

		/// @brief 敵弾による標準ダメージ量
		/// @details 通常の敵弾が与えるダメージ
		/// @note 100HP で 約 6～7発で敗北
		constexpr int ENEMY_BULLET_DAMAGE = 15;

		/// @brief 敵との衝突による標準ダメージ量
		/// @details 敵本体との直接衝突で受けるダメージ
		/// @note 敵弾より少ないが、接近戦の危険性を表現
		constexpr int COLLISION_DAMAGE = 10;
	}

	//========================================
	//          敗北演出定数
	//========================================
	namespace Defeat {
		/// @brief 敗北演出全体の長さ（秒）
		/// @details ノーズダイブ + 落下による総演出時間
		/// @note この時間でゲームオーバー画面へ自動遷移
		constexpr float ANIMATION_DURATION = 3.0f;

		/// @brief フェーズ1の割合（0-1）
		/// @details ノーズダイブ フェーズの割合
		/// @note フェーズ1: 3.0 × 0.6 = 1.8秒
		/// @note フェーズ2（落下）: 3.0 × 0.4 = 1.2秒
		constexpr float PHASE1_RATIO = 0.6f;

		/// @brief 落下時の重力加速度（units/秒²）
		/// @details 敗北演出のフェーズ2で適用される重力
		/// @note リアルな落下表現のため、現実的な値を設定
		constexpr float GRAVITY_ACCELERATION = 15.0f;

		/// @brief 演出終了判定のY座標閾値
		/// @details この高さに到達したら演出終了と判定
		/// @note 画面外（下方）を示す負の値
		/// @warning ステージレイアウトに応じて調整が必要な場合がある
		constexpr float GROUND_Y_THRESHOLD = -50.0f;

		/// @brief ノーズダイブ角度（度）
		/// @details フェーズ1で機体の傾く角度
		/// @note -90度 = 真っ逆さま。視覚的な衝撃を表現
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
