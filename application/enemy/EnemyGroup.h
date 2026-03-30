/*********************************************************************
 * \file   EnemyGroup.h
 * \brief  敵のグループ管理クラス（編隊行動制御）
 *
 * \author Harukichimaru
 * \date   March 2026
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include <vector>
#include <memory>

// 前方宣言
class EnemyBase;
class Player;

///=============================================================================
///						編隊フォーメーション定義
enum class FormationType {
	VFormation,		  // V字編隊
	LineFormation,	  // 直線編隊
	CircleFormation,  // 円形編隊
	DiamondFormation, // 菱形編隊
	DynamicFormation  // 動的編隊（プレイヤー位置に応じて変更）
};

///=============================================================================
///						編隊設定構造体
struct FormationConfig {
	FormationType type;
	Vector3 offsets[8];     // 各敵の相対位置（最大8敵対応）
	float spacing;          // 敵間距離
	float cohesionStrength; // 集団結束度（0～1）
	float separationStrength; // 分離強度（敵同士の衝突回避）
	float alignmentStrength;  // 方向揃え強度
	int maxMemberCount;       // 編隊内の最大敵数
};

///=============================================================================
///						EnemyGroupクラス
/**
 * @brief 複数の敵で構成される編隊管理クラス
 *
 * 責務：
 * - 編隊内の敵の統一管理
 * - フォーメーション計算と位置制御
 * - リーダー敵とフォロワー敵の役割管理
 * - 編隊状態の管理（移動、戦闘、退却等）
 */
class EnemyGroup {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief コンストラクタ
	EnemyGroup();

	/// \brief デストラクタ
	~EnemyGroup() = default;

	/// \brief グループ初期化
	void Initialize(EnemyBase *leaderEnemy, FormationType formationType);

	/// \brief グループにメンバを追加
	void AddMember(EnemyBase *member, int positionIndex);

	/// \brief 更新（編隊制御ロジック）
	void Update(const Vector3 &playerPosition);

	/// \brief グループ内の敵削除処理
	void RemoveDeadMembers();

	/// \brief グループの活性状態確認
	bool IsActive() const;

	/// \brief リーダー敵を取得
	EnemyBase *GetLeader() const {
		return leaderEnemy_;
	}

	/// \brief メンバ敵を取得
	const std::vector<EnemyBase *> &GetMembers() const {
		return memberEnemies_;
	}

	/// \brief グループ内の生存敵数
	size_t GetAliveCount() const;

	/// \brief グループIDを設定
	void SetGroupId(int id) {
		groupId_ = id;
	}

	/// \brief グループIDを取得
	int GetGroupId() const {
		return groupId_;
	}

	///--------------------------------------------------------------
	///							フォーメーション関連
private:
	/// \brief フォーメーション設定の生成
	FormationConfig CreateFormationConfig(FormationType type);

	/// \brief V字フォーメーション設定
	FormationConfig CreateVFormation();

	/// \brief 直線フォーメーション設定
	FormationConfig CreateLineFormation();

	/// \brief 円形フォーメーション設定
	FormationConfig CreateCircleFormation();

	/// \brief 菱形フォーメーション設定
	FormationConfig CreateDiamondFormation();

	/// \brief 動的フォーメーション計算
	FormationConfig CalculateDynamicFormation(const Vector3 &playerPosition);

	/// \brief メンバの目標位置計算
	void CalculateMemberTargetPositions(const Vector3 &leaderPos, const Vector3 &playerPos);

	/// \brief メンバの相対位置追尾更新
	void UpdateMemberPositions();

	///--------------------------------------------------------------
	///							群動作ロジック
private:
	/// \brief Boid的な群制御（分離・結合・整列）
	Vector3 CalculateBoidForce(EnemyBase *member, const Vector3 &targetPos);

	/// \brief 分離処理（敵同士が近づきすぎないようにする）
	Vector3 CalculateSeparation(EnemyBase *member);

	/// \brief 結合処理（敵が集団中心に寄る）
	Vector3 CalculateCohesion(EnemyBase *member, const Vector3 &targetPos);

	/// \brief 方向整列処理（敵の向きを揃える）
	Vector3 CalculateAlignment(EnemyBase *member);

	///--------------------------------------------------------------
	///							メンバ変数
private:
	int groupId_;                           // グループID
	EnemyBase *leaderEnemy_;                // リーダー敵（nullptr=グループ非活性）
	std::vector<EnemyBase *> memberEnemies_; // フォロワー敵群

	FormationConfig currentFormation_;       // 現在のフォーメーション
	std::vector<Vector3> memberTargetPositions_; // 各メンバの目標位置

	//========================================
	// グループ状態管理
	enum class GroupState {
		Approaching, // 接近中
		Combat,      // 戦闘中
		Retreating   // 退却中
	};
	GroupState groupState_;
	float stateTimer_;

	//========================================
	// パラメータ
	float formationUpdateTimer_;  // フォーメーション更新タイマー
	float minFormationUpdateInterval_; // フォーメーション更新最小間隔（毎フレーム更新を避ける）
};
