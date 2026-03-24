/*********************************************************************
 * \file   Sprite.h
 * \brief  スプライトクラス - 2D画像描画システム
 *
 * \author Harukichimaru
 * \date   October 2024
 * \note   軽量で汎用性の高い2Dスプライト描画システム
 *         メソッドチェーン対応で使いやすさを重視
 *********************************************************************/
#pragma once
///=============================================================================
///						インクルード
//========================================
// Windows
#include <cstdint>
#include <string>
//========================================
// DirectX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
//========================================
// 自作
#include "MagMath.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///								前方宣言
	class SpriteSetup;

	///=============================================================================
	///                            Spriteクラス
	/**
	 * \class Sprite
	 * \brief 2D画像描画を行うスプライトクラス
	 * 
	 * \note  【使用例】
	 *        sprite->SetPosition({100.0f, 200.0f})
	 *              ->SetSize({256.0f, 256.0f})
	 *              ->SetRotation(0.5f)
	 *              ->SetColor({1.0f, 1.0f, 1.0f, 0.8f});
	 * 
	 * \note  【主な機能】
	 *        - テクスチャ付き2D描画
	 *        - 位置・回転・スケール変更
	 *        - アンカーポイント設定
	 *        - 左右・上下フリップ
	 *        - テクスチャ範囲指定（スプライトシート対応）
	 *        - メソッドチェーン対応
	 */
	class Sprite {
		///--------------------------------------------------------------
		///						 公開メンバ関数
	public:
		///=============================================================================
		///                        初期化・終了
		
		/**----------------------------------------------------------------------------
		 * \brief  Initialize 初期化
		 * \param  spriteSetup スプライト管理クラス
		 * \param  textureFilePath テクスチャファイルパス
		 * \note   スプライトの初期化を行い、テクスチャを読み込む
		 */
		void Initialize(SpriteSetup *spriteSetup, std::string textureFilePath);

		///=============================================================================
		///                        更新・描画

		/**----------------------------------------------------------------------------
		 * \brief  Update 更新
		 * \param  viewMatrix ビュー行列（デフォルトは単位行列）
		 * \note   スプライトの変換行列を更新する
		 *         ビュー行列を指定することで特殊なカメラ効果を適用可能
		 */
		void Update(MagMath::Matrix4x4 viewMatrix = MagMath::Identity4x4());

		/**----------------------------------------------------------------------------
		 * \brief  Draw 描画
		 * \note   スプライトをコマンドリストに積む
		 *         事前にSpriteSetup::CommonDrawSetup()を呼び出すこと
		 */
		void Draw();

		///=============================================================================
		///                        基本設定（メソッドチェーン対応）

		/**----------------------------------------------------------------------------
		 * \brief  SetPosition 座標の設定
		 * \param  position 座標（ピクセル単位）
		 * \return this メソッドチェーン用
		 * \note   画面座標系で指定（左上が原点）
		 */
		Sprite *SetPosition(const MagMath::Vector2 &position) {
			this->position_ = position;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetRotation 回転の設定
		 * \param  rotation 回転角度（ラジアン）
		 * \return this メソッドチェーン用
		 * \note   正の値で時計回り
		 */
		Sprite *SetRotation(float rotation) {
			this->rotation_ = rotation;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetSize サイズの設定
		 * \param  size サイズ（ピクセル単位）
		 * \return this メソッドチェーン用
		 * \note   スプライトの描画サイズを指定
		 */
		Sprite *SetSize(const MagMath::Vector2 &size) {
			this->size_ = size;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetColor 色の設定
		 * \param  color 色（RGBA: 0.0f～1.0f）
		 * \return this メソッドチェーン用
		 * \note   テクスチャの色を乗算。アルファ値で透明度を制御
		 */
		Sprite *SetColor(const MagMath::Vector4 &color) {
			materialData_->color = color;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetAnchorPoint アンカーポイントの設定
		 * \param  anchorPoint アンカーポイント（0.0f～1.0f）
		 * \return this メソッドチェーン用
		 * \note   {0.0f, 0.0f}=左上、{0.5f, 0.5f}=中央、{1.0f, 1.0f}=右下
		 */
		Sprite *SetAnchorPoint(const MagMath::Vector2 &anchorPoint) {
			this->anchorPoint_ = anchorPoint;
			return this;
		}

		///=============================================================================
		///                        フリップ・反転

		/**----------------------------------------------------------------------------
		 * \brief  SetFlipX 左右フリップの設定
		 * \param  isFlipX 左右反転フラグ
		 * \return this メソッドチェーン用
		 * \note   trueで左右反転
		 */
		Sprite *SetFlipX(bool isFlipX) {
			this->isFlipX_ = isFlipX;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetFlipY 上下フリップの設定
		 * \param  isFlipY 上下反転フラグ
		 * \return this メソッドチェーン用
		 * \note   trueで上下反転
		 */
		Sprite *SetFlipY(bool isFlipY) {
			this->isFlipY_ = isFlipY;
			return this;
		}

		///=============================================================================
		///                        テクスチャ操作

		/**----------------------------------------------------------------------------
		 * \brief  SetTexture テクスチャの差し替え
		 * \param  textureFilePath テクスチャファイルパス
		 * \return this メソッドチェーン用
		 * \note   動的にテクスチャを変更する場合に使用
		 */
		Sprite *SetTexture(const std::string &textureFilePath) {
			this->textureFilePath_ = textureFilePath;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetTextureRect テクスチャ矩形範囲の設定
		 * \param  leftTop 左上座標（ピクセル単位）
		 * \param  size 切り出しサイズ（ピクセル単位）
		 * \return this メソッドチェーン用
		 * \note   スプライトシートから特定の範囲を切り出す
		 */
		Sprite *SetTextureRect(const MagMath::Vector2 &leftTop, const MagMath::Vector2 &size) {
			this->textureLeftTop_ = leftTop;
			this->textureSize_ = size;
			return this;
		}

		///=============================================================================
		///                        便利機能

		/**----------------------------------------------------------------------------
		 * \brief  SetAlpha 透明度の設定
		 * \param  alpha 透明度（0.0f～1.0f）
		 * \return this メソッドチェーン用
		 * \note   色を変更せず透明度のみ変更
		 */
		Sprite *SetAlpha(float alpha) {
			materialData_->color.w = alpha;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  FitToTexture テクスチャサイズに合わせる
		 * \return this メソッドチェーン用
		 * \note   スプライトのサイズをテクスチャの元サイズに合わせる
		 */
		Sprite *FitToTexture();

		/**----------------------------------------------------------------------------
		 * \brief  SetScale スケール倍率の設定
		 * \param  scale スケール倍率
		 * \return this メソッドチェーン用
		 * \note   現在のサイズに対して倍率をかける
		 */
		Sprite *SetScale(float scale) {
			this->size_.x *= scale;
			this->size_.y *= scale;
			return this;
		}

		/**----------------------------------------------------------------------------
		 * \brief  CenterOnScreen 画面中央に配置
		 * \return this メソッドチェーン用
		 * \note   スプライトを画面中央に配置（アンカーポイントも中央に設定）
		 */
		Sprite *CenterOnScreen();

		///=============================================================================
		///                        ゲッター（const参照返却）

		/**----------------------------------------------------------------------------
		 * \brief  GetPosition 座標の取得
		 * \return 座標
		 */
		const MagMath::Vector2 &GetPosition() const {
			return position_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetRotation 回転の取得
		 * \return 回転角度（ラジアン）
		 */
		const float &GetRotation() const {
			return rotation_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetSize サイズの取得
		 * \return サイズ
		 */
		const MagMath::Vector2 &GetSize() const {
			return size_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetColor 色の取得
		 * \return 色（RGBA）
		 */
		const MagMath::Vector4 &GetColor() const {
			return materialData_->color;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetAnchorPoint アンカーポイントの取得
		 * \return アンカーポイント
		 */
		const MagMath::Vector2 &GetAnchorPoint() const {
			return anchorPoint_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetFlipX 左右フリップの取得
		 * \return 左右反転フラグ
		 */
		const bool &GetFlipX() const {
			return isFlipX_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetFlipY 上下フリップの取得
		 * \return 上下反転フラグ
		 */
		const bool &GetFlipY() const {
			return isFlipY_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetTextureLeftTop テクスチャ左上座標の取得
		 * \return テクスチャ左上座標
		 */
		const MagMath::Vector2 &GetTextureLeftTop() const {
			return textureLeftTop_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetTextureSize テクスチャ切り出しサイズの取得
		 * \return テクスチャ切り出しサイズ
		 */
		const MagMath::Vector2 &GetTextureSize() const {
			return textureSize_;
		}

		///--------------------------------------------------------------
		///						 プライベート関数
	private:
		///=============================================================================
		///                        バッファ作成

		/**----------------------------------------------------------------------------
		 * \brief  CreateVertexBuffer 頂点バッファの作成
		 * \note   4頂点の矩形メッシュを作成
		 */
		void CreateVertexBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  CreateIndexBuffer インデックスバッファの作成
		 * \note   2つの三角形（6インデックス）を作成
		 */
		void CreateIndexBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  CreateMaterialBuffer マテリアルバッファの作成
		 * \note   色情報とUV変換行列を格納
		 */
		void CreateMaterialBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  CreateTransformationMatrixBuffer トランスフォーメーション行列バッファの作成
		 * \note   ワールド・ビュー・プロジェクション行列を格納
		 */
		void CreateTransformationMatrixBuffer();

		///=============================================================================
		///                        更新処理

		/**----------------------------------------------------------------------------
		 * \brief  ReflectSRT SRT（スケール・回転・平行移動）の反映
		 * \note   座標・回転・サイズをTransformに反映
		 */
		void ReflectSRT();

		/**----------------------------------------------------------------------------
		 * \brief  ReflectAnchorPointAndFlip アンカーポイントとフリップの反映
		 * \note   アンカーポイントに基づいて頂点位置を調整し、
		 *         左右・上下フリップを適用
		 */
		void ReflectAnchorPointAndFlip();

		/**----------------------------------------------------------------------------
		 * \brief  ReflectTextureRange テクスチャ範囲指定の反映
		 * \note   テクスチャ座標（UV）を更新
		 */
		void ReflectTextureRange();

		/**----------------------------------------------------------------------------
		 * \brief  AdjustTextureSize テクスチャサイズの調整
		 * \note   テクスチャの元サイズをスプライトサイズに設定
		 */
		void AdjustTextureSize();

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		///---------------------------------------
		/// スプライト管理クラス
		SpriteSetup *spriteSetup_ = nullptr;

		///---------------------------------------
		/// GPUリソース - バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;					  // 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_ = nullptr;					  // インデックスバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_ = nullptr;				  // マテリアルバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixBuffer_ = nullptr;	  // トランスフォーム行列バッファ

		///---------------------------------------
		/// バッファマップ済みポインタ
		MagMath::VertexData *vertexData_ = nullptr;					  // 頂点データ
		uint32_t *indexData_ = nullptr;								  // インデックスデータ
		MagMath::Material *materialData_ = nullptr;					  // マテリアルデータ
		MagMath::TransformationMatrix *transformationMatrixData_ = nullptr;	  // トランスフォーム行列データ

		///---------------------------------------
		/// バッファビュー
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};	  // 頂点バッファビュー
		D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};		  // インデックスバッファビュー

		///---------------------------------------
		/// トランスフォーム
		MagMath::Transform transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};	  // 3D変換用
		MagMath::Vector2 position_ = {0.0f, 0.0f};	  // 座標（ピクセル）
		float rotation_ = 0.0f;						  // 回転（ラジアン）
		MagMath::Vector2 size_ = {1.0f, 1.0f};		  // サイズ（ピクセル）

		///---------------------------------------
		/// テクスチャ
		std::string textureFilePath_ = "";	  // テクスチャファイルパス

		///---------------------------------------
		/// 表示設定
		MagMath::Vector2 anchorPoint_ = {0.0f, 0.0f};	  // アンカーポイント（0.0～1.0）
		bool isFlipX_ = false;							  // 左右フリップ
		bool isFlipY_ = false;							  // 上下フリップ

		///---------------------------------------
		/// テクスチャ範囲指定（スプライトシート用）
		MagMath::Vector2 textureLeftTop_ = {0.0f, 0.0f};	  // テクスチャ左上座標
		MagMath::Vector2 textureSize_ = {0.0f, 0.0f};		  // テクスチャ切り出しサイズ
	};
}
