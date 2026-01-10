/*********************************************************************
 * \file   LevelDataLoader.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note   レベルデータ"だけ"の読み込みを行うクラス
 *        Blender（右手系）からエンジン（左手系）への座標変換も担当
 *        読み込んだデータをObject3Dクラスのリストに変換・配置する機能も提供
 *********************************************************************/
#pragma once
#include "Object3d.h"
#include "MagMath.h"
#include "Logger.h"
#include "externals/json.hpp"
#include <memory>
#include <string>
#include <vector>

// 名前衝突を避けるためLevelColliderに変更
struct LevelCollider {
	std::string type; // コライダータイプ（"BOX", "SPHERE"等）
	MagMath::Vector3 center;	  // コライダー中心位置
	MagMath::Vector3 size;	  // コライダーサイズ
};

struct LevelObject {
	std::string name;									// オブジェクト名
	std::string type;									// オブジェクトタイプ（"MESH", "EMPTY"等）
	std::string file_name;								// モデルファイル名（空の場合あり）
	MagMath::Transform transform;								// トランスフォーム情報
	std::unique_ptr<LevelCollider> collider;			// コライダー情報（nullptrの場合あり）
	std::vector<std::unique_ptr<LevelObject>> children; // 子オブジェクト
};

struct LevelData {
	std::string name;								   // シーン名
	std::vector<std::unique_ptr<LevelObject>> objects; // ルートオブジェクト群
};

class LevelDataLoader {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize();

	/// \brief JSONファイルからレベルデータを読み込み
	/// \param filePath JSONファイルのパス
	/// \return 読み込み成功時true、失敗時false
	bool LoadLevelFromJson(const std::string &filePath);

	/// \brief 読み込んだレベルデータをObject3Dリストに変換してシーンに配置
	/// \param object3dSetup Object3D初期化用のセットアップクラス
	/// \param outObjectList 配置されたObject3Dを格納するリスト（参照渡し）
	/// \return 配置成功時true、失敗時false
	/// \note 事前にLoadLevelFromJson()でデータを読み込んでおく必要がある
	///       既存のoutObjectListの内容はクリアされる
	bool CreateObjectsFromLevelData(Object3dSetup *object3dSetup, std::vector<std::unique_ptr<Object3d>> &outObjectList);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief 読み込まれたレベルデータを取得
	/// \return レベルデータの参照
	const LevelData &GetLevelData() const {
		return levelData_;
	}

	/// \brief レベルデータが正常に読み込まれているかチェック
	/// \return 読み込み済みの場合true
	bool IsLoaded() const {
		return isLoaded_;
	}

	/// \brief ImGui描画（デバッグ用）
	/// \param outObjectList 操作対象のObject3Dリスト
	void ImGuiDraw(std::vector<std::unique_ptr<Object3d>> &outObjectList);

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief JSONオブジェクトからLevelObjectを作成（再帰処理）
	/// \param jsonObj JSONオブジェクト
	/// \return 作成されたLevelObjectのunique_ptr
	std::unique_ptr<LevelObject> ParseObjectFromJson(const nlohmann::json &jsonObj);

	/// \brief LevelObjectからObject3Dを再帰的に作成してリストに追加
	/// \param levelObject 変換元のLevelObject
	/// \param object3dSetup Object3D初期化用のセットアップクラス
	/// \param outObjectList Object3Dを格納するリスト
	/// \param parentTransform 親のトランスフォーム（階層構造用）
	/// \note 子オブジェクトも再帰的に処理し、親の座標変換を適用する
	///       file_nameが空のオブジェクトは空のObject3Dとして作成される
	void CreateObject3DFromLevelObject(const std::unique_ptr<LevelObject> &levelObject,
									   Object3dSetup *object3dSetup,
									   std::vector<std::unique_ptr<Object3d>> &outObjectList,
									   const MagMath::Transform &parentTransform = {{1, 1, 1}, {0, 0, 0}, {0, 0, 0}});

	/// \brief 2つのTransformを合成（親→子の順で適用）
	/// \param parent 親のTransform
	/// \param child 子のTransform
	/// \return 合成されたTransform
	/// \note スケール・回転・平行移動の順で適用
	///       回転は度数法で計算される点に注意
	MagMath::Transform CombineTransforms(const MagMath::Transform &parent, const MagMath::Transform &child);

	/// \brief Blender（右手系）からエンジン（左手系）への座標変換
	/// \param blenderPos Blender座標系の位置ベクトル
	/// \return エンジン座標系の位置ベクトル
	/// \note Blender: Y-up, 右手系 → エンジン: Y-up, 左手系
	///       変換: X' = X, Y' = Y, Z' = -Z
	MagMath::Vector3 ConvertPositionFromBlender(const MagMath::Vector3 &blenderPos);

	/// \brief Blender（右手系）からエンジン（左手系）への回転変換
	/// \param blenderRot Blender座標系の回転（オイラー角、度数法）
	/// \return エンジン座標系の回転（オイラー角、度数法）
	/// \note 右手系から左手系への回転変換を適用
	///       Y軸とZ軸の回転方向が反転する
	MagMath::Vector3 ConvertRotationFromBlender(const MagMath::Vector3 &blenderRot);

	/// \brief JSONからMagMath::Vector3を安全に取得
	/// \param jsonArray JSON配列
	/// \param defaultValue デフォルト値
	/// \return MagMath::Vector3値
	MagMath::Vector3 GetVector3FromJson(const nlohmann::json &jsonArray, const MagMath::Vector3 &defaultValue = {0, 0, 0});

	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	LevelData levelData_; // 読み込まれたレベルデータ
	bool isLoaded_;		  // データが正常に読み込まれたかのフラグ

	// ImGui用の選択されたオブジェクトのインデックス
	int selectedObjectIndex_;
};
