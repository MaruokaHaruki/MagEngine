/*********************************************************************
 * \file   Skybox.h
 * \brief  スカイボックス描画クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "Matrix4x4.h"
#include "TextureManager.h"
#include "Transform.h"
#include "Vector4.h"
//========================================
// DX12include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// 前方宣言
class SkyboxSetup;
class Camera;

// 頂点データ構造体
struct SkyboxVertex {
	Vector4 position;
};

// ViewProjection行列用構造体
struct SkyboxViewProjection {
	Matrix4x4 viewProjection;
};

///=============================================================================
///								クラス
class Skybox {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(SkyboxSetup *skyboxSetup);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	///--------------------------------------------------------------
	///						 静的メンバ関数
private:
	/// \brief Boxの頂点データ作成
	void CreateBoxVertices();

	/// \brief ViewProjection行列バッファの作成
	void CreateViewProjectionBuffer();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief SetCamera カメラの設定
	void SetCamera(Camera *camera) {
		this->camera_ = camera;
	}

	/// \brief SetTexture テクスチャの設定
	void SetTexture(const std::string &texturePath) {
		texturePath_ = texturePath;
	}

	/// \brief GetTransform トランスフォーメーションのポインタを取得
	Transform *GetTransform() {
		return &transform_;
	}

	/// \brief SetTransform トランスフォーメーションの設定
	void SetTransform(const Transform &transform) {
		transform_ = transform;
	}

	/// \brief SetScale スケールの設定
	void SetScale(const Vector3 &scale) {
		transform_.scale = scale;
	}

	/// \brief GetScale スケールの取得
	const Vector3 &GetScale() const {
		return transform_.scale;
	}

	/// \brief SetRotation 回転の設定
	void SetRotation(const Vector3 &rotate) {
		transform_.rotate = rotate;
	}

	/// \brief GetRotation 回転の取得
	const Vector3 &GetRotation() const {
		return transform_.rotate;
	}

	/// \brief SetPosition 位置の設定
	void SetPosition(const Vector3 &translate) {
		transform_.translate = translate;
	}

	/// \brief GetPosition 位置の取得
	const Vector3 &GetPosition() const {
		return transform_.translate;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// SkyboxSetupポインタ
	SkyboxSetup *skyboxSetup_ = nullptr;

	//========================================
	// 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	SkyboxVertex *vertexData_ = nullptr;

	//========================================
	// インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t *indexData_ = nullptr;

	//========================================
	// ViewProjection行列バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionBuffer_;
	SkyboxViewProjection *viewProjectionData_ = nullptr;

	//========================================
	// カメラ
	Camera *camera_ = nullptr;

	//========================================
	// テクスチャパス
	std::string texturePath_ = "";

	//========================================
	// Transform
	Transform transform_ = {};

	//========================================
	// 定数
	static const uint32_t kVertexCount = 8; // Boxの頂点数
	static const uint32_t kIndexCount = 36; // Boxのインデックス数（6面 × 2三角形 × 3頂点）
};
