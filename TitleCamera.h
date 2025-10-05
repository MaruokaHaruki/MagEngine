#pragma once
#include "Vector3.h"
#include <string>

//========================================
// 前方宣言
class Camera;
class Player;

//========================================
// カメラワークフェーズ
enum class TitleCameraPhase {
	Opening,	  // 【1】オープニング・イントロ
	HeroShot,	  // 【2】戦闘機登場シーン
	TitleDisplay, // 【3】タイトル表示カット
	Loop		  // 【4】ループ演出
};

//========================================
// カメラキーフレーム
struct CameraKeyframe {
	float time;
	Vector3 position;
	Vector3 target;
	float fov;
	float exposure;
};

class TitleCamera {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(const std::string &cameraName);

	/// \brief 更新
	void Update();

	/// \brief ImGui描画
	void DrawImGui();

	/// \brief プレイヤーを設定
	void SetPlayer(Player *player) {
		player_ = player;
	}

	/// \brief カメラワークをリセット
	void Reset();

	/// \brief 現在のフェーズを取得
	TitleCameraPhase GetCurrentPhase() const {
		return currentPhase_;
	}

	/// \brief デバッグ用：カメラ位置取得
	Vector3 GetCameraPosition() const {
		return cameraPosition_;
	}

	/// \brief デバッグ用：ターゲット位置取得
	Vector3 GetCameraTarget() const {
		return cameraTarget_;
	}

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	// 各フェーズの更新処理
	void UpdateOpening(float deltaTime);
	void UpdateHeroShot(float deltaTime);
	void UpdateTitleDisplay(float deltaTime);
	void UpdateLoop(float deltaTime);

	// カメラパス補間
	Vector3 InterpolatePosition(float t, const Vector3 &start, const Vector3 &end);
	Vector3 CubicBezier(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3, float t);
	float EaseInOut(float t);
	float EaseOutCubic(float t);
	float EaseInCubic(float t);

	// フェーズ遷移用
	void TransitionToNextPhase(TitleCameraPhase nextPhase);
	void UpdatePhaseTransition(float deltaTime);

	// 滑らかな追従用
	Vector3 SmoothDamp(const Vector3 &current, const Vector3 &target, Vector3 &velocity, float smoothTime, float deltaTime);

	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	Camera *camera_;		 // カメラマネージャから取得したカメラ
	Player *player_;		 // 追従対象のプレイヤー
	std::string cameraName_; // 使用するカメラ名

	// カメラワーク制御
	TitleCameraPhase currentPhase_;
	float phaseTimer_;
	float totalElapsedTime_;

	// フェーズごとの持続時間
	static constexpr float OPENING_DURATION = 3.0f;		  // 3秒
	static constexpr float HEROSHOT_DURATION = 2.5f;	  // 2.5秒
	static constexpr float TITLE_DISPLAY_DURATION = 3.0f; // 3秒
	static constexpr float TRANSITION_DURATION = 0.8f;	  // フェーズ遷移時間

	// カメラパラメータ
	Vector3 cameraPosition_;
	Vector3 cameraTarget_;
	float cameraFOV_;
	float cameraExposure_;

	// フェーズ遷移用
	bool isTransitioning_;
	float transitionTimer_;
	Vector3 transitionStartPos_;
	Vector3 transitionEndPos_;
	Vector3 transitionStartTarget_;
	Vector3 transitionEndTarget_;
	TitleCameraPhase nextPhase_;

	// ループ演出用
	float loopRotationAngle_;
	float loopTime_;

	// 滑らかな追従用
	Vector3 cameraVelocity_;
	Vector3 targetVelocity_;
	float cameraSmoothTime_;
	Vector3 lastPlayerPosition_;
};
