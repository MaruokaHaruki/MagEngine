/*********************************************************************
 * \file   Particl.h
 * \brief
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#pragma once

// パーティクルの形状を定義する列挙型 (ヘッダの先頭近くに移動)
enum class ParticleShape {
	Board,	 // 現在のボード形状
	Ring,	 // リング形状
	Cylinder // シリンダー形状
};

#include "Material.h"
#include "ModelData.h"
#include "ParticleSetup.h"
#include "VertexData.h"

//========================================
// 標準ライブラリ
#include <random>

//========================================
// DX12include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

// Particle構造体
struct ParticleStr {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

// パーティクルグループ構造体の定義
struct ParticleGroup {
	// マテリアルデータ
	std::string materialFilePath;
	int srvIndex = 0;
	// パーティクルのリスト (std::list<ParticleStr>型)
	std::list<ParticleStr> particleList = {};
	// インスタンシングデータ用SRVインデックス
	int instancingSrvIndex = 0;
	// インスタンシングリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
	// インスタンス数
	UINT instanceCount = 0;
	// インスタンシングデータを書き込むためのポインタ
	ParticleForGPU *instancingDataPtr = nullptr;

	Vector2 textureLeftTop = {0.0f, 0.0f}; // テクスチャ左上座標
	Vector2 textureSize = {0.0f, 0.0f};	   // テクスチャサイズを追加

	ParticleShape shape;   // このグループのパーティクル形状
	UINT vertexOffset = 0; // 頂点バッファのオフセット
	UINT vertexCount = 0;  // この形状の頂点数
};

class Object3dSetup;
class Camera;
class Particle {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(ParticleSetup *particleSetup);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/**----------------------------------------------------------------------------
	 * \brief  Emit
	 * \param  name
	 * \param  position
	 * \param  count
	 */
	void Emit(const std::string name, const Vector3 &position, uint32_t count);

	/**----------------------------------------------------------------------------
	 * \brief  CreateParticleGroup
	 * \param  name
	 * \param  materialFilePath
	 * \param  maxInstanceCount
	 */
	void CreateParticleGroup(const std::string &name, const std::string &textureFilePath, ParticleShape shape);

	// 形状設定用の関数 (注意: CreateVertexDataが固定形状を生成するため、現状これらの動的変更は限定的)
	void SetParticleShape(ParticleShape shape) { /* particleShape_ = shape; */
	} // particleShape_ は削除されたため、この関数は現状何もしません。グループ作成時に形状を指定してください。
	void SetRingRadius(float radius) {
		ringRadius_ = radius;
	}
	void SetCylinderParams(float height, float radius) {
		cylinderHeight_ = height;
		cylinderRadius_ = radius;
	}

	///--------------------------------------------------------------
	///						 静的メンバ関数
private:
	/**----------------------------------------------------------------------------
	 * \brief  CreateVertexData 頂点データの作成
	 */
	void CreateVertexData();

	/**----------------------------------------------------------------------------
	 * \brief  CreateVertexBufferView 頂点バッファビューの作成
	 */
	void CreateVertexBufferView();

	/**----------------------------------------------------------------------------
	 * \brief  CreateMaterialData マテリアルデータの作成
	 */
	void CreateMaterialData();

	/**----------------------------------------------------------------------------
	 * \brief  CreateNewParticle 新しいパーティクルを生成
	 * \param  randomEngine 乱数生成器
	 * \param  position 生成位置
	 */
	ParticleStr CreateNewParticle(std::mt19937 &randomEngine, const Vector3 &position);

	///--------------------------------------------------------------
	///							入出力関数
public:
	// 画像のサイズを設定
	void SetCustomTextureSize(const Vector2 &size) {
		customTextureSize = size;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//---------------------------------------
	// パーティクルセットアップポインタ
	ParticleSetup *particleSetup_ = nullptr;

	//---------------------------------------
	// パーティクルグループ
	std::unordered_map<std::string, ParticleGroup> particleGroups;

	//---------------------------------------
	// モデルデータ
	ModelData modelData_;

	//---------------------------------------
	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	// バッファリソースの使い道を指すポインタ
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
	// バッファリソース内のデータを指すポインタ
	VertexData *vertexData_ = nullptr;

	//---------------------------------------
	// マテリアルデータ
	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
	// バッファリソース内のデータを指すポインタ
	Material *materialData_ = nullptr;

	//---------------------------------------
	// インスタンシングバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingBuffer_;

	//---------------------------------------
	// 乱数生成器の初期化
	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;

	//---------------------------------------
	// その他
	// カメラ目線を使用するかどうか
	bool isUsedBillboard = false; // デフォルトを false に変更
	// 最大インスタンス数
	static const uint32_t kNumMaxInstance = 128;
	//
	const float kDeltaTime = 1.0f / 60.0f;
	// 乱数範囲の調整用
	struct RangeForRandom {
		float min;
		float max;
	};
	// パーティクルの設定
	RangeForRandom translateRange_ = {0.0f, 0.0f};
	RangeForRandom colorRange_ = {1.0f, 1.0f};
	RangeForRandom lifetimeRange_ = {1.0f, 3.0f};
	RangeForRandom velocityRange_ = {-1.1f, 1.1f};

	// TODO:設定しているテクスチャサイズを使うかどうかを変更できるようにする
	Vector2 customTextureSize = {100.0f, 100.0f};

	// パーティクルの形状 (グループごとに設定するため、クラスメンバからは削除)
	// ParticleShape particleShape_ = ParticleShape::Ring;
	// 形状パラメータ (CreateVertexData内で固定値が使用されるため、現状これらの動的変更は限定的)
	float ringRadius_ = 1.0f;	  // リングの半径
	float cylinderHeight_ = 1.0f; // シリンダーの高さ
	float cylinderRadius_ = 0.5f; // シリンダーの半径
};
