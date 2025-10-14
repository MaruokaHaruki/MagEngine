#pragma once

///=============================================================================
///                        シーントランジションクラス
class SceneTransition {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief 初期化
	void Initialize();

	/// \brief 終了処理
	void Finalize();

	/// \brief 更新
	void Update();

	/// \brief シーン切り替え要求
	void RequestSceneChange(int sceneID);

	/// \brief シーン切り替え中かどうかを取得
	bool IsTransitioning() const;

	/// \brief 現在のシーンIDを取得
	int GetCurrentSceneID() const;

	///--------------------------------------------------------------
	///                        静的メンバ関数
private:
	///--------------------------------------------------------------
	///                        入出力関数
public:
	///--------------------------------------------------------------
	///                        メンバ変数
private:
	int currentSceneID_ = 0;	   // 現在のシーンID
	int nextSceneID_ = -1;		   // 次のシーンID（-1は切り替えなし）
	bool isTransitioning_ = false; // シーン切り替え中かどうか
};
