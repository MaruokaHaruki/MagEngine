/*********************************************************************
 * \file   CameraManager.h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "Camera.h"
#include <map> // std::map を使用するためインクルード
#include <memory>
#include <string> // std::string を使用するためインクルード
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						カメラマネージャークラス
	class CameraManager {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  GetInstance インスタンスの取得
		 * \return
		 */
		static CameraManager *GetInstance();
		/**----------------------------------------------------------------------------
		 * \brief  ~CameraManager デストラクタ
		 */
		~CameraManager();
		/**----------------------------------------------------------------------------
		 * \brief  Finalize 終了処理
		 */
		void Finalize();
		/**----------------------------------------------------------------------------
		 * \brief  Initialize 初期化
		 */
		void Initialize();
		/**----------------------------------------------------------------------------
		 * \brief  AddCamera カメラの追加
		 * \param  name カメラ名
		 */
		void AddCamera(const std::string &name);
		/**----------------------------------------------------------------------------
		 * \brief  GetCamera カメラの取得
		 * \param  name カメラ名
		 * \return
		 */
		Camera *GetCamera(const std::string &name) const;

		/// \brief 現在のカメラの設定
		void SetCurrentCamera(const std::string &name);

		/// \brief 現在のカメラの取得
		Camera *GetCurrentCamera() const;

		/// \brief 全てのカメラの更新
		void UpdateAll();

		/// @brief デバックカメラの更新
		void DebugCameraUpdate();

		/// @brief デバックカメラの呼び出し
		void ChangeDebugCamera();

		/// @brief ImGuiの描画
		void DrawImGui();

		/// @brief デバッグカメラのトランスフォームをリセット
		void ResetDebugCameraTransform();

		/// @brief デバッグカメラのターゲット追従モードを切り替え
		void ToggleDebugCameraTargetLock();

		/// @brief デバッグ用の視覚情報を描画 (ラインなど)
		void DrawDebugVisualizations();

		/// @brief 特定のカメラのデバッグ表示を切り替える
		void ToggleCameraDebugView(const std::string &cameraName);

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		//========================================
		// 　コンストラクタ
		CameraManager() = default;
		//========================================
		// カメラのコンテナ
		std::map<std::string, std::unique_ptr<Camera>> cameras_;
		//========================================
		// 現在のカメラ名
		std::string currentCameraName_;
		// 過去のカメラ名
		std::string previousCameraName_;

		//========================================
		// デバッグカメラ用パラメータ
		MagMath::Vector3 debugCameraTarget_;
		float debugCameraDistanceToTarget_;
		bool isDebugCameraTargetLocked_;
		float debugCameraMoveSpeed_;
		float debugCameraRotateSpeed_;

		// 各カメラのデバッグ表示フラグ
		std::map<std::string, bool> cameraDebugViewFlags_;
	};
}