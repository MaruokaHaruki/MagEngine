/*********************************************************************
 * \file   MAudioG.h
 * \brief
 *
 * ███╗   ███╗ █████╗  ██████╗
 * ████╗ ████║██╔══██╗██╔════╝ 	MA.h
 * ██╔████╔██║███████║██║  ███╗	Ver5.10
 * ██║╚██╔╝██║██╔══██║██║   ██║	2026/04/23
 * ██║ ╚═╝ ██║██║  ██║╚██████╔╝
 * ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/

#pragma once
#define XAUDIO2_HELPER_FUNCTIONS
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						クラス
	class MAudioG {
		///--------------------------------------------------------------
		///							メンバ構造体
	public:
		//========================================
		// 最大音声データ数
		static const int kMaxSoundData = 256;

		//========================================
		// デバイス情報を格納する構造体の定義
		struct AudioDeviceInfo {
			std::wstring deviceId;
			std::wstring displayName;
		};

		//========================================
		// 音声データを表す構造体
		struct SoundData {
			WAVEFORMATEX wfex{};			 // 検証済みPCMフォーマット
			std::vector<uint8_t> buffer; // 音声データのバッファ
			std::string name;			 // 音声ファイルの名前
		};

		//========================================
		// 再生データを表す構造体
		struct Voice {
			struct DestroySourceVoice {
				void operator()(IXAudio2SourceVoice *voice) const noexcept { voice->DestroyVoice(); }
			};
			// NOTE: 宣言と逆順に破棄されるため、データを解放する前にDestroyVoiceで参照終了を待つ。
			std::vector<uint8_t> reversedBuffer;
			std::unique_ptr<IXAudio2SourceVoice, DestroySourceVoice> sourceVoice;
			float oldVolume = 1.0f;						// 最後に設定したボリューム
			float oldSpeed = 1.0f;						// 最後に設定した再生速度
		};

		//========================================
		// チャンクヘッダーを表す構造体
		struct ChunkHeader {
			char id[4];	   // チャンクのID（"RIFF", "fmt ", "data"など）
			uint32_t size; // チャンクのサイズ
		};

		//========================================
		// RIFFヘッダーを表す構造体
		struct RiffHeader {
			ChunkHeader chunk; // チャンクヘッダー
			char type[4];	   // ファイルタイプ（"WAVE"）
		};

		//========================================
		// フォーマットチャンクを表す構造体
		struct FormatChunk {
			ChunkHeader chunk; // チャンクヘッダー
			WAVEFORMATEX fmt;  // 波形フォーマット
		};

		//========================================
		// オーディオコールバックを表すクラス
		class XAudio2VoiceCallback : public IXAudio2VoiceCallback {
		public:
			STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32 BytesRequired) override {
				UNREFERENCED_PARAMETER(BytesRequired);
			}
			STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {
			}
			STDMETHOD_(void, OnStreamEnd)() override {
			}
			STDMETHOD_(void, OnBufferStart)(void *pBufferContext) override {
				UNREFERENCED_PARAMETER(pBufferContext);
			}
			STDMETHOD_(void, OnBufferEnd)(void *pBufferContext) override {
				UNREFERENCED_PARAMETER(pBufferContext);
			}
			STDMETHOD_(void, OnLoopEnd)(void *pBufferContext) override {
				UNREFERENCED_PARAMETER(pBufferContext);
			}
			STDMETHOD_(void, OnVoiceError)(void *pBufferContext, HRESULT Error) override {
				UNREFERENCED_PARAMETER(pBufferContext);
				std::cerr << "Voice error: " << std::hex << Error << std::endl;
			}
		};

		//========================================
		// コンストラクタとデストラクタ
		MAudioG() = default;
		~MAudioG() {
			Finalize();
		}
		MAudioG(const MAudioG &) = delete;
		MAudioG &operator=(const MAudioG &) = delete;

		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  GetInstance		シングルトンインスタンス
		 * \return
		 */
		static MAudioG *GetInstance();

		/**----------------------------------------------------------------------------
		 * \brief  GetAudioDevices	接続デバイスの検知
		 */
		void GetAudioDevices();

		/**----------------------------------------------------------------------------
		 * \brief  Initialize		初期化
		 * \param  directoryPath	ディレクトリパス
		 * \param  deviceId			デバイスID
		 * \note 呼び出しスレッドのCOM初期化が必要。初期化・終了はアプリ側で直列化する。
		 */
		void Initialize(const std::string &directoryPath = "Resources/", const std::wstring &deviceId = L"");

		/**----------------------------------------------------------------------------
		 * \brief  Finalize		終了処理
		 */
		void Finalize();

		/**----------------------------------------------------------------------------
		 * \brief  LoadWav	サウンドのロード
		 * \param  filename		ファイル名
		 * \note PCMの8/16bit・モノラル/ステレオに対応。失敗は理由を記録し、未登録のまま戻る。
		 */
		void LoadWav(const std::string &filename);

		/**----------------------------------------------------------------------------
		 * \brief  Unload		サウンドのアンロード
		 * \param  soundData	サウンドデータ
		 * \note nullptrは無視する。管理中データの場合は参照中のVoiceを先に破棄する。
		 */
		void Unload(SoundData *soundData);

		/**----------------------------------------------------------------------------
		 * \brief  PlayWav		ファイル名でWAVファイルを再生
		 * \param  filename		ファイル名
		 * \param  loopFlag		ループするかどうか
		 * \param  volume		ボリューム
		 * \param  maxPlaySpeed	最大再生速度
		 * \note 未ロードなら読み込む。初期化・入力・API失敗は記録し、新しいVoiceを残さない。
		 */
		void PlayWav(const std::string &filename,
					 bool loopFlag = false,
					 float volume = 1.0f,
					 float maxPlaySpeed = 2.0f);

		/**----------------------------------------------------------------------------
		 * \brief  PlayWavReverse		ファイル名でWAVファイルを逆再生
		 * \note PCMフレーム単位で反転する。反転データは停止・置換・FinalizeまでVoiceが保持する。
		 * \param  filename		ファイル名
		 * \param  loopFlag		ループするかどうか
		 * \param  volume		ボリューム
		 * \param  maxPlaySpeed	最大再生速度
		 */
		void PlayWavReverse(const std::string &filename,
							bool loopFlag = false,
							float volume = 1.0f,
							float maxPlaySpeed = 2.0f);

		/**----------------------------------------------------------------------------
		 * \brief  StopWav		ファイル名で再生を停止
		 * \param  filename		ファイル名
		 */
		void StopWav(const std::string &filename);

		/**----------------------------------------------------------------------------
		 * \brief  IsWavPlaying		ファイル名で再生中かどうかを確認
		 * \param  filename		ファイル名
		 * \return
		 */
		bool IsWavPlaying(const std::string &filename);

		/**----------------------------------------------------------------------------
		 * \brief  PauseWav		ファイル名で再生一時停止
		 * \param  filename		ファイル名
		 */
		void PauseWav(const std::string &filename);

		/**----------------------------------------------------------------------------
		 * \brief  ResumeWav		ファイル名で再生再開
		 * \param  filename		ファイル名
		 */
		void ResumeWav(const std::string &filename);

		/**----------------------------------------------------------------------------
		 * \brief  SetVolume	ファイル名で音量を設定
		 * \brief  NOTE: 0が無音,1が元の音源そのまま,0.3fくらいから判断
		 * \param  filename		ファイル名
		 * \param  volume		音量
		 */
		void SetVolume(const std::string &filename, float volume);

		/**----------------------------------------------------------------------------
		 * \brief  SetVolumeDecibel ファイル名で音量を設定(デシベル)
		 * \param  filename		ファイル名
		 * \param  dB			デシベル
		 */
		void SetVolumeDecibel(const std::string &filename, float dB);

		/**----------------------------------------------------------------------------
		 * \brief  SetPlaybackSpeed ファイル名で再生速度を設定
		 * \param  filename		ファイル名
		 * \param  speed		再生速度
		 */
		void SetPlaybackSpeed(const std::string &filename, float speed);

		///--------------------------------------------------------------
		/// 内部処理
	private:
		// NOTE: 読み込み境界とVoice寿命の直接回帰検証に限定したアクセス。
		friend struct AssetSafetyTests;
		/// @brief RIFF内の境界とPCM形式を検証する。失敗時はパスと理由を含む例外を送出する。
		static SoundData ReadWav(const std::string &filePath);
		/// @brief チャンネル・サンプル内の並びを維持してPCMフレームを反転する。
		static std::vector<uint8_t> ReverseFrames(const SoundData &soundData);
		/// @brief 呼び出し側がvoiceMutex_を保持する読み込み・再生経路。
		void LoadWavInternal(const std::string &filename);
		void PlayWavInternal(const std::string &filename, bool loopFlag, float volume, float maxPlaySpeed, bool reverse);

		///--------------------------------------------------------------
		/// メンバ変数
	private:
		//========================================
		// 出力オーディオ
		std::vector<AudioDeviceInfo> audioDevices_;
		//========================================
		// XAudio2インターフェース
		Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
		//========================================
		// マスターボイス
		IXAudio2MasteringVoice *masterVoice_ = nullptr;
		//========================================
		// サウンドデータのマップ（ファイル名で管理）
		std::unordered_map<std::string, SoundData> soundDataMap_;
		//========================================
		// 再生中のボイスのマップ（ファイル名で管理）
		std::unordered_map<std::string, Voice> voiceMap_;
		//========================================
		// 音声ファイルのディレクトリパス
		std::string directoryPath_;
		//========================================
		// オーディオコールバック
		XAudio2VoiceCallback voiceCallback_;
		// 音声データとVoiceを一緒に保護する。XAudio2コールバックから操作APIを呼ばない。
		std::mutex voiceMutex_;
		// サンプリングレート
		float waveSamplingRate;
	};

}
