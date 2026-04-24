# MagVoiceBridge 使用方法ガイド

## 概要

**MagVoiceBridge** は、Windows WASAPI を使用してマイクからリアルタイムで音声を入力し、以下の機能を提供するクラスです：

- 🎤 マイク音声のリアルタイムキャプチャ
- 📊 音量計測（複数の形式に対応）
- 🔊 音声検出（人の声とノイズを区別）
- 🎯 ゲーム連携用の数値化出力
- ⚙️ パラメータの動的調整機能

---

## 初期化と開始

### 基本的な使用フロー

```cpp
#include "MagVoiceBridge.h"

// 1. インスタンスを作成
std::unique_ptr<MagVoiceBridge> voiceBridge = std::make_unique<MagVoiceBridge>();

// 2. 初期化（WASAPI デバイスの準備）
if (!voiceBridge->Initialize()) {
    Logger::Log("Failed to initialize MagVoiceBridge", Logger::LogLevel::Error);
    return;
}

// 3. キャプチャを開始
if (!voiceBridge->Start()) {
    Logger::Log("Failed to start microphone", Logger::LogLevel::Error);
    return;
}

// 4. 毎フレーム Update を呼び出す
void Update() {
    voiceBridge->Update();  // 必ず毎フレーム呼ぶ
}

// 5. 終了時にシャットダウン
voiceBridge->Shutdown();
```

---

## 音量計測の取得

### 1. 現在の音量（RMS 値）

```cpp
// 正規化値（0.0 ～ 1.0）
float volume = voiceBridge->GetCurrentVolume();

// デシベル値（-80 dB ～ 0 dB）
float volumeDB = voiceBridge->GetCurrentVolumeDB();

// 例：UI メーターに表示
ImGui::ProgressBar(volume, ImVec2(100, 20), "");
ImGui::Text("Volume: %.2f dB", volumeDB);
```

### 2. ピークレベル（瞬間最大値）

```cpp
// ピークレベル（最大振幅）を取得
float peakVolume = voiceBridge->GetPeakVolume();          // 0.0～1.0
float peakVolumeDB = voiceBridge->GetPeakVolumeDB();      // dB 値

// 例：ピークメーター表示
ImGui::Text("Peak: %.4f (%.2f dB)", peakVolume, peakVolumeDB);
ImGui::ProgressBar(peakVolume, ImVec2(100, 15), "");
```

### 3. スムージング済み音量（推奨）

**UI 表示や ゲーム制御には、スムージング済み値を使用することを推奨します。**

```cpp
// スムージング済み音量（0.0～1.0）
// ちらつきが少なく、安定した表示が得られます
float smoothedVolume = voiceBridge->GetSmoothedVolume();

// スムージング済み dB 値
float smoothedVolumeDB = voiceBridge->GetSmoothedVolumeDB();

// パーセンテージ形式（0～100）
float volumePercent = voiceBridge->GetVolumePercentage();

// 例：ゲーム内で使用
if (volumePercent > 50.0f) {
    // 大きな声が出ている
    PlayLoudVoiceSFX();
}
```

### 4. 全統計情報の一括取得

**複数の値を効率的に取得したい場合に便利です。**

```cpp
MagVoiceBridge::VolumeStats stats = voiceBridge->GetVolumeStats();

// stats 構造体に含まれる値：
// - stats.currentRMS       : 現在の RMS（0.0～1.0）
// - stats.currentRMSDB     : 現在の RMS（dB）
// - stats.peakValue        : ピークレベル（0.0～1.0）
// - stats.peakDB           : ピークレベル（dB）
// - stats.smoothedRMS      : スムージング済み RMS（0.0～1.0）
// - stats.smoothedRMSDB    : スムージング済み RMS（dB）
// - stats.percentage       : パーセンテージ（0～100）
// - stats.voiceScore       : 音声特性スコア（0.0～1.0）
// - stats.isVoiceDetected  : 音声検出フラグ

// 例：ゲーム画面に複数表示
ImGui::Text("Volume:     %.1f %%", stats.percentage);
ImGui::Text("Voice Score: %.2f", stats.voiceScore);
ImGui::Text("Detected:    %s", stats.isVoiceDetected ? "YES" : "NO");
```

---

## 音声検出

### 人の声判定

```cpp
// 人の声が検出されているかを判定
bool isVoice = voiceBridge->IsVoiceDetected();

if (isVoice) {
    // 人が話している
    Logger::Log("Voice detected!", Logger::LogLevel::Info);
} else {
    // 音声未検出またはノイズ
}
```

### 音声特性スコア

**人の声らしさを 0.0～1.0 の値で取得します。**

```cpp
// 1.0 に近い = 人の声の可能性が高い
// 0.0 に近い = ノイズの可能性が高い
float voiceScore = voiceBridge->GetVoiceCharacteristicScore();

if (voiceScore > 0.7f) {
    // 高確率で人の声
    use_voice_input = true;
} else if (voiceScore > 0.4f) {
    // 判別不明
    need_confirmation = true;
} else {
    // ノイズである可能性が高い
    ignore_input = true;
}
```

---

## パラメータの調整

### 1. スムージング係数の設定

**UI 更新の応答性と安定性のバランスを調整します。**

```cpp
// 値の範囲：0.0 ～ 1.0
// 低い値（0.1）  → より反応的（ちらつきやすい）
// 高い値（0.9）  → より安定（応答遅延がある）
// デフォルト：0.4

voiceBridge->SetSmoothingFactor(0.4f);   // バランスの取れた設定
voiceBridge->SetSmoothingFactor(0.1f);   // リアルタイム性重視
voiceBridge->SetSmoothingFactor(0.8f);   // 安定性重視
```

### 2. ノイズフロアの設定

**この値より小さい音量は完全に無視します。**

```cpp
// 値の範囲：-80 dB ～ -20 dB
// デフォルト：-50 dB

// 背景ノイズが多い環境での設定例
voiceBridge->SetNoiseFloor(-45.0f);

// 静かな環境での設定例
voiceBridge->SetNoiseFloor(-55.0f);

// エアコンの音などが多い環境での設定例
voiceBridge->SetNoiseFloor(-40.0f);
```

### 3. 音量正規化範囲の設定

**dB 値を 0～100% に変換する際の基準値を設定します。**

```cpp
// minDb : 0% に相当する dB 値（通常 -60 dB）
// maxDb : 100% に相当する dB 値（通常 -6 dB）

// デフォルト設定
voiceBridge->SetVolumeRange(-60.0f, -6.0f);

// マイク感度が低い場合（より敏感に反応させたい）
voiceBridge->SetVolumeRange(-70.0f, -10.0f);

// マイク感度が高い場合（より鈍感にしたい）
voiceBridge->SetVolumeRange(-50.0f, -0.0f);
```

### 4. 音声検出パラメータの調整

**人の声判定の精度を調整します。**

```cpp
// zcRateThreshold   : ゼロクロス率の閾値（0.05～0.5）
// volumeThresholdDB : 音量の閾値（-80～-10 dB）

// デフォルト設定
voiceBridge->SetVoiceDetectionThresholds(0.25f, -40.0f);

// 人の声を厳密に検出したい場合
voiceBridge->SetVoiceDetectionThresholds(0.15f, -35.0f);

// ノイズの多い環境での設定
voiceBridge->SetVoiceDetectionThresholds(0.30f, -30.0f);

// 低い声（男性）をより敏感に検出
voiceBridge->SetVoiceDetectionThresholds(0.20f, -45.0f);

// 高い声（女性）をより敏感に検出
voiceBridge->SetVoiceDetectionThresholds(0.30f, -40.0f);
```

---

## デバッグ・パラメータ調整（ImGui）

### DebugScene での使用

プロジェクトに含まれる `DebugScene.cpp` では、ImGui UI から以下を実行できます：

1. **マイク情報表示**
   - サンプルレート
   - チャンネル数
   - ビット深度（常に 32-bit float）

2. **リアルタイム音量表示**
   - 現在の RMS 値
   - ピークレベル
   - スムージング済み値
   - パーセンテージ

3. **音声検出結果**
   - 検出フラグ（YES/NO）
   - 音声特性スコア
   - ビジュアル評価（Human Voice / Uncertain / Noise）

4. **パラメータ調整スライダー**
   - Smoothing Factor
   - Noise Floor
   - ZC Rate Threshold
   - Volume Threshold

5. **パラメータ適用**
   - 「Apply Thresholds」ボタンで設定を反映

6. **波形表示**
   - リアルタイム波形グラフ
   - スケール・感度の調整スライダー

**使用例：**
```
Voice Input Monitor ウィンドウ → PARAMETER TUNING セクション
→ スライダーを調整 → Apply Thresholds ボタンをクリック
→ リアルタイムで結果が変わるのを確認
```

---

## ゲーム内での実装例

### 例1：音声によるボリューム制御

```cpp
void UpdateGameVolume() {
    float volume = voiceBridge->GetSmoothedVolume();
    
    // 音声の大きさに応じてゲーム内の効果音を制御
    audioEngine->SetMasterVolume(volume);
}
```

### 例2：音声によるキャラクター制御

```cpp
void UpdateCharacterMovement() {
    auto stats = voiceBridge->GetVolumeStats();
    
    if (!stats.isVoiceDetected) {
        return;  // 音声がなければ何もしない
    }
    
    // 音量に応じてキャラクターの力度を変更
    float powerLevel = stats.percentage / 100.0f;
    character->SetAttackPower(powerLevel * 100.0f);
}
```

### 例3：音声品質チェック

```cpp
bool IsGoodVoiceInput() {
    auto stats = voiceBridge->GetVolumeStats();
    
    // 音声検出 AND 十分な音量 AND スコアが高い
    return stats.isVoiceDetected && 
           stats.percentage > 20.0f && 
           stats.voiceScore > 0.6f;
}
```

### 例4：動的パラメータ調整（環境に応じた自動最適化）

```cpp
void OptimizeForEnvironment(EnvironmentType env) {
    switch (env) {
        case QUIET_ROOM:
            voiceBridge->SetNoiseFloor(-55.0f);
            voiceBridge->SetSmoothingFactor(0.3f);
            voiceBridge->SetVoiceDetectionThresholds(0.25f, -45.0f);
            break;
            
        case NOISY_ROOM:
            voiceBridge->SetNoiseFloor(-40.0f);
            voiceBridge->SetSmoothingFactor(0.6f);
            voiceBridge->SetVoiceDetectionThresholds(0.20f, -30.0f);
            break;
            
        case OUTDOOR:
            voiceBridge->SetNoiseFloor(-35.0f);
            voiceBridge->SetSmoothingFactor(0.7f);
            voiceBridge->SetVoiceDetectionThresholds(0.15f, -25.0f);
            break;
    }
}
```

---

## 推奨設定

### 環境別パラメータ設定表

| 環境 | Noise Floor | Smoothing | ZC Threshold | Volume Threshold |
|------|-------------|-----------|--------------|------------------|
| 静かな室内 | -55 dB | 0.3 | 0.25 | -45 dB |
| **標準環境** | **-50 dB** | **0.4** | **0.25** | **-40 dB** |
| やや騒がしい | -45 dB | 0.5 | 0.22 | -35 dB |
| 騒がしい環境 | -40 dB | 0.6 | 0.20 | -30 dB |
| 非常に騒がしい | -35 dB | 0.7 | 0.18 | -25 dB |

### 声質別パラメータ設定表

| 声質 | ZC Threshold | Volume Threshold | 備考 |
|------|--------------|------------------|------|
| 低い声（男性） | 0.20 | -45 dB | ゼロクロス率が低いため敏感に設定 |
| **中程度** | **0.25** | **-40 dB** | デフォルト設定 |
| 高い声（女性） | 0.30 | -35 dB | ゼロクロス率が高いため鈍感に設定 |
| 子どもの声 | 0.35 | -50 dB | より高周波のため敏感に設定 |

---

## トラブルシューティング

### 問題：波形が激しく上下している、ノイズが多い

**解決方法：**
1. `SetNoiseFloor()` でノイズフロアを上げる
2. `SetSmoothingFactor()` で値を 0.5f 以上に設定
3. ImGui デバッグ画面でリアルタイム調整

```cpp
voiceBridge->SetNoiseFloor(-40.0f);      // ノイズを除外
voiceBridge->SetSmoothingFactor(0.6f);   // 平滑化を強化
```

### 問題：音声が検出されない

**解決方法：**
1. マイクが接続されているか確認
2. マイクの音量レベルを確認
3. `SetVoiceDetectionThresholds()` で閾値を下げる

```cpp
// 検出しやすく調整
voiceBridge->SetVoiceDetectionThresholds(0.30f, -50.0f);
voiceBridge->SetNoiseFloor(-60.0f);
```

### 問題：ゲーム内で音量が変わらない

**解決方法：**
1. 毎フレーム `Update()` が呼ばれているか確認
2. `GetSmoothedVolume()` を使用しているか確認
3. パーセンテージが 0～100 の範囲か確認

```cpp
// デバッグ用：コンソールに値を出力
Logger::Log("Volume: " + std::to_string(voiceBridge->GetVolumePercentage()),
           Logger::LogLevel::Info);
```

---

## パフォーマンス考慮事項

- ✅ `Update()` は毎フレーム呼び出し（CPU 負荷は低い）
- ✅ `GetVolumeStats()` は複数値を効率的に取得（推奨）
- ✅ 個別ゲッターの呼び出しは atomic 操作のため快速
- ✅ バッファサイズは自動管理（最大 10 秒分に制限）

---

## API リファレンス

### 初期化系

| メソッド | 説明 |
|---------|------|
| `bool Initialize()` | WASAPI を初期化 |
| `bool Start()` | キャプチャを開始 |
| `void Stop()` | キャプチャを停止 |
| `void Shutdown()` | リソースを解放 |

### 音量計測系

| メソッド | 戻り値 | 説明 |
|---------|--------|------|
| `float GetCurrentVolume()` | 0.0～1.0 | 現在の RMS 音量 |
| `float GetCurrentVolumeDB()` | -80～0 dB | 現在の RMS（dB） |
| `float GetPeakVolume()` | 0.0～1.0 | ピークレベル |
| `float GetPeakVolumeDB()` | dB | ピークレベル（dB） |
| `float GetSmoothedVolume()` | 0.0～1.0 | スムージング済み RMS |
| `float GetSmoothedVolumeDB()` | dB | スムージング済み RMS（dB） |
| `float GetVolumePercentage()` | 0～100 | パーセンテージ |

### 音声検出系

| メソッド | 戻り値 | 説明 |
|---------|--------|------|
| `bool IsVoiceDetected()` | bool | 音声検出フラグ |
| `float GetVoiceCharacteristicScore()` | 0.0～1.0 | 人の声らしさスコア |
| `float CalculateZeroCrossingRate()` | 0.0～1.0 | ゼロクロス率（デバッグ用） |

### パラメータ調整系

| メソッド | 引数 | 説明 |
|---------|------|------|
| `void SetSmoothingFactor()` | 0.0～1.0 | スムージング係数 |
| `void SetNoiseFloor()` | dB | ノイズフロア |
| `void SetVolumeRange()` | minDb, maxDb | 音量正規化範囲 |
| `void SetVoiceDetectionThresholds()` | zcRate, volumeDb | 検出閾値 |

### その他

| メソッド | 説明 |
|---------|------|
| `void Update()` | **毎フレーム必須** |
| `std::vector<float> GetSamples()` | サンプルバッファ取得 |
| `void ClearSamples()` | バッファクリア |
| `uint32_t GetSampleRate() const` | サンプルレート |
| `uint16_t GetChannelCount() const` | チャンネル数 |
| `VolumeStats GetVolumeStats()` | 統計情報一括取得 |

---

## まとめ

MagVoiceBridge を使用することで：

✅ マイク音声を簡単に取得できる  
✅ 複数の形式（正規値、dB、パーセンテージ）で音量を取得できる  
✅ 人の声とノイズを区別できる  
✅ ゲーム内で音声パラメータとして使用できる  
✅ ImGui で動的にパラメータを調整できる  

詳細は `MagVoiceBridge.h` / `MagVoiceBridge.cpp` / `DebugScene.cpp` を参照してください。
