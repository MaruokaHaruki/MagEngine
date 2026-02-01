#pragma once

///=============================================================================
///						VertexShader
//========================================
//定数バッファ
struct VertexShaderInput {
    float3 position : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// PixelShader出力構造体を追加
struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

cbuffer CloudCameraCB : register(b0)
{
    float4x4 gInvViewProjection;  // 逆ビュープロジェクション行列（スクリーン座標→ワールド座標変換用）
    float3 gCameraPosition;       // カメラのワールド座標（レイの原点として使用）
    float  gCameraPadding;        // 16バイトアライメント用パディング
    
    // カメラの近平面・遠平面距離
    float gNearPlane;             // ニアクリップ平面（通常0.1f）
    float gFarPlane;              // ファークリップ平面（通常10000.0f）
    float gPadding4;              // パディング
    float gPadding5;              // パディング
    
    // ビュープロジェクション行列（ワールド座標→スクリーン座標変換用、深度計算に使用）
    float4x4 gViewProjection;
};

cbuffer CloudParamsCB : register(b1)
{
    //========================================
    // 雲の空間配置パラメータ
    float3 gCloudCenter;          // 雲のバウンディングボックス中心座標（ワールド空間）
    float gCloudSizeX;            // 未使用（構造体アライメント用）
    float3 gCloudSize;            // 雲のバウンディングボックスサイズ（X, Y, Z）
    float gPadding0;              // パディング
    
    //========================================
    // ライティングパラメータ
    float3 gSunDirection;         // 太陽光の方向ベクトル（正規化済み）
    float gSunIntensity;          // 太陽光の強度（明るさ倍率）
    float3 gSunColor;             // 太陽光の色（RGB、通常は暖色系）
    float gAmbient;               // 環境光の強度（影の部分の明るさ）
    
    //========================================
    // 雲の密度制御パラメータ
    float gDensity;               // 雲の全体的な密度倍率（高いほど濃い雲）
    float gCoverage;              // 雲のカバレッジ（0.0～1.0、雲の分布範囲を制御）
    float gBaseNoiseScale;        // ベースノイズのスケール（小さいほど大きな雲の塊）
    float gDetailNoiseScale;      // ディテールノイズのスケール（大きいほど細かい模様）
    
    //========================================
    // レイマーチング制御パラメータ
    float gStepSize;              // 1回のレイマーチングステップの距離（大きいほど高速だが粗い）
    float gMaxDistance;           // 最大レイマーチング距離（これを超えると打ち切り）
    float gLightStepSize;         // ライトマーチング（影計算）のステップサイズ
    float gShadowDensityMultiplier; // 影の濃さ調整倍率（高いほど濃い影）
    
    //========================================
    // アニメーション制御パラメータ
    float gTime;                  // 経過時間（秒単位、ノイズのオフセットに使用）
    float gNoiseSpeed;            // ノイズアニメーションの速度倍率
    float gDetailWeight;          // ディテールノイズの影響度（0.0～1.0）
    float gAnisotropy;            // 異方性散乱パラメータ（-1.0～1.0、光の散乱方向を制御）
    
    //========================================
    // デバッグ用パラメータ
    float gDebugFlag;             // デバッグモードフラグ（0.0=通常、1.0=デバッグ表示）
    int gBulletHoleCount;         // 有効な弾痕の数
    float gBulletHoleFadeStart;   // 弾痕フェード開始距離
    float gBulletHoleFadeEnd;     // 弾痕フェード終了距離
};

///=============================================================================
///                      弾痕データ構造体
/// @brief GPU側で使用する弾痕情報
/// @note Counter-Strike風の動的スモークで、弾が通過した軌跡をSDFで表現
struct BulletHoleGPU {
    float3 origin;       // 弾の開始位置
    float startRadius;   // 弾痕の開始半径（入口）
    float3 direction;    // 弾の正規化方向ベクトル
    float endRadius;     // 弾痕の終了半径（出口）
    float lifeTime;      // 残存時間(0.0～1.0、1.0=完全、0.0=消滅)
    float coneLength;    // 円錐の長さ
    float padding1;      // パディング
    float padding2;      // パディング
};

// 弾痕配列定数バッファ（b2）
cbuffer BulletHoleBufferCB : register(b2)
{
    BulletHoleGPU gBulletHoles[32];  // 最大32個の弾痕
};

Texture2D<float4> gWeatherMap : register(t0);  // ウェザーマップテクスチャ（雲の分布制御用）
SamplerState gLinearSampler : register(s0);    // 線形補間サンプラー

static const float PI = 3.14159265f;           // 円周率
static const int MAX_STEPS = 16;               // レイマーチングの最大ステップ数（高速化優先）
static const int MAX_LIGHT_STEPS = 2;          // ライトマーチングの最大ステップ数（パフォーマンス優先）

///=============================================================================
///                      ハッシュ関数
/// @brief 3D座標から疑似乱数を生成（ノイズ生成の基礎）
/// @param p 入力座標
/// @return 0.0～1.0の疑似乱数値
/// @note フラクショナル演算とプライム数の掛け算で高速にランダム値を生成
float Hash(float3 p) {
    // 小数部分を取得（0.0～1.0の範囲に正規化）
    p = frac(p * 0.3183099f + 0.1f);
    // プライム数17で掛け算（値を分散させる）
    p *= 17.0f;
    // 3次元の値を混ぜ合わせて1次元のハッシュ値を生成
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

///=============================================================================
///                      3Dパーリンノイズ
/// @brief 滑らかな3次元ノイズを生成
/// @param p サンプリング座標
/// @return 0.0～1.0のノイズ値
/// @note 8つの格子点のハッシュ値を三線形補間して滑らかなノイズを生成
float Noise3D(float3 p) {
    // 整数部分と小数部分に分離
    float3 i = floor(p);  // 格子点の座標
    float3 f = frac(p);   // 格子内での位置（0.0～1.0）
    
    // エルミート補間（smoothstep）で滑らかな補間曲線を作成
    // f * f * (3.0 - 2.0 * f) は3次エルミート補間関数
    f = f * f * (3.0f - 2.0f * f);

    // 立方体の8つの頂点でハッシュ値を取得し、三線形補間
    // X軸方向の補間
    // Y軸方向の補間
    // Z軸方向の補間（最終的に1つの値に）
    return lerp(
        lerp(lerp(Hash(i + float3(0, 0, 0)), Hash(i + float3(1, 0, 0)), f.x),
             lerp(Hash(i + float3(0, 1, 0)), Hash(i + float3(1, 1, 0)), f.x), f.y),
        lerp(lerp(Hash(i + float3(0, 0, 1)), Hash(i + float3(1, 0, 1)), f.x),
             lerp(Hash(i + float3(0, 1, 1)), Hash(i + float3(1, 1, 1)), f.x), f.y),
        f.z);
}

///=============================================================================
///                      FBM（Fractional Brownian Motion）ノイズ
/// @brief 複数のオクターブのノイズを重ね合わせて複雑な模様を生成
/// @param p サンプリング座標
/// @param octaves オクターブ数（重ね合わせる層の数）
/// @return 0.0～1.0のノイズ値
/// @note 高周波成分（細かい模様）と低周波成分（大きな形状）を組み合わせる
/// @note パフォーマンス最適化：最大2オクターブに制限
float FBM(float3 p, int octaves) {
    float value = 0.0f;       // 累積ノイズ値
    float amplitude = 0.5f;   // 各オクターブの振幅（初期値0.5）
    float frequency = 1.0f;   // 各オクターブの周波数（初期値1.0）
    
    // パフォーマンスのため最大2オクターブに制限
    int maxOctaves = min(octaves, 2);
    
    // オクターブ数だけループ
    for (int i = 0; i < maxOctaves; i++) {
        // 現在の周波数でノイズをサンプリングし、振幅をかけて加算
        value += amplitude * Noise3D(p * frequency);
        
        // 次のオクターブは周波数を2倍に（細かい模様）
        frequency *= 2.0f;
        // 次のオクターブは振幅を半分に（影響度を下げる）
        amplitude *= 0.5f;
    }
    return value;
}

///=============================================================================
///                      レイとAABB（軸平行境界ボックス）の交差判定
/// @brief レイが箱と交差するかを判定し、交差区間を返す
/// @param rayOrigin レイの原点
/// @param rayDir レイの方向ベクトル（正規化済み）
/// @param boxMin ボックスの最小座標
/// @param boxMax ボックスの最大座標
/// @param tNear 交差区間の開始距離（出力）
/// @param tFar 交差区間の終了距離（出力）
/// @return 交差している場合true
/// @note スラブ法を使用した効率的な交差判定
bool IntersectAABB(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax, out float tNear, out float tFar) {
    // レイ方向の逆数を計算（除算を掛け算に変換して高速化）
    // 1e-6fを加算してゼロ除算を防止
    float3 invDir = 1.0f / (rayDir + 1e-6f);
    
    // 各軸でボックスの面との交差距離を計算
    float3 t0 = (boxMin - rayOrigin) * invDir;  // 最小面との交差
    float3 t1 = (boxMax - rayOrigin) * invDir;  // 最大面との交差
    
    // レイの方向が負の場合、t0とt1が入れ替わるので正しい順序にする
    float3 tMin = min(t0, t1);  // 各軸での進入距離
    float3 tMax = max(t0, t1);  // 各軸での退出距離
    
    // 全軸での進入距離の最大値 = ボックスに入る距離
    tNear = max(max(tMin.x, tMin.y), tMin.z);
    // 全軸での退出距離の最小値 = ボックスから出る距離
    tFar = min(min(tMax.x, tMax.y), tMax.z);
    
    // 進入距離 < 退出距離 かつ 退出距離 > 0 なら交差している
    return tFar > tNear && tFar > 0.0f;
}

///=============================================================================
///                      円柱SDF（Signed Distance Function）
/// @brief 点pから円柱までの符号付き距離を計算
/// @param p 評価点（ワールド座標）
/// @param origin 円柱の中心線上の任意の点
/// @param direction 円柱の方向（正規化済み）
/// @param radius 円柱の半径
/// @return 符号付き距離（負値=内部、正値=外部）
/// @note Counter-Strike風の弾痕を表現するためのSDF
///       d = length((p - origin) - dot(p - origin, dir) * dir) - radius
float CylinderSDF(float3 p, float3 origin, float3 direction, float radius)
{
    // 評価点から原点へのベクトル
    float3 offset = p - origin;
    
    // 円柱の中心軸上への投影
    // なぜ：評価点から最も近い中心軸上の点を求めるため
    float projection = dot(offset, direction);
    
    // 中心軸からの垂直距離ベクトルを計算
    // なぜ：円柱表面までの距離を求めるため
    float3 perpendicular = offset - projection * direction;
    
    // 垂直距離から半径を引いてSDF値を返す
    // なぜ：負値なら円柱内部、正値なら外部と判定できるため
    return length(perpendicular) - radius;
}

///=============================================================================
///                      弾痕マスク計算（円錐形状対応）
/// @brief すべての弾痕からの影響を計算し、雲密度へのマスク値を返す
/// @param position 評価点（ワールド座標）
/// @return マスク値（0.0=完全に空洞、1.0=影響なし）
/// @note FinalDensity(p) = BaseDensity(p) * BulletMask(p) という形で合成する
/// @note 円錐形状により入口から徐々に狭まる自然な弾痕を表現
float CalculateBulletHoleMask(float3 position)
{
    float mask = 1.0f;  // 初期値：影響なし
    
    // すべての有効な弾痕をループで処理
    for (int i = 0; i < gBulletHoleCount; ++i)
    {
        // 弾痕データを取得
        BulletHoleGPU hole = gBulletHoles[i];
        
        // 評価点から弾痕原点へのベクトル
        float3 offset = position - hole.origin;
        
        // 弾の進行方向への投影（縦軸方向の距離）
        float axialDist = dot(offset, hole.direction);
        
        // 円錐の範囲内かどうかをチェック
        if (axialDist < 0.0f || axialDist > hole.coneLength) {
            continue; // 円錐の範囲外
        }
        
        // 半径方向の距離（横軸方向の距離）
        float3 perpendicular = offset - axialDist * hole.direction;
        float radialDist = length(perpendicular);
        
        //========================================
        // 円錐形状：軸方向の位置に応じて半径が変化
        // なぜ：入口（startRadius）から出口（endRadius）に向かって狭まるため
        float t = axialDist / hole.coneLength; // 0.0（入口）～1.0（出口）
        float currentRadius = lerp(hole.startRadius, hole.endRadius, t);
        
        //========================================
        // 半径方向のフォールオフ（ガウス分布ベース）
        // なぜ：中心から離れるほど滑らかに密度が回復するため
        float radialFalloff = radialDist / (currentRadius + 0.001f); // 0除算回避
        // ガウス関数に似た減衰カーブ（exp(-x^2)）
        float radialMask = exp(-radialFalloff * radialFalloff * 2.5f); // 2.5でより急峻な減衰
        // 半径の1.5倍以上離れたら影響なし（パフォーマンス最適化）
        radialMask = (radialDist > currentRadius * 1.5f) ? 1.0f : (1.0f - radialMask);
        
        //========================================
        // 軸方向のフォールオフ（入口と出口で滑らかに）
        // なぜ：弾痕が弾の進行方向に沿って徐々に薄くなるため
        float entryFade = smoothstep(0.0f, hole.coneLength * 0.2f, axialDist); // 入口側のフェード
        float exitFade = smoothstep(hole.coneLength, hole.coneLength * 0.8f, axialDist); // 出口側のフェード
        float axialMask = entryFade * exitFade;
        
        //========================================
        // 半径方向と軸方向を組み合わせた3Dフォールオフ
        // なぜ：より自然な3次元的な空洞を表現するため
        float holeMask = max(radialMask, 1.0f - axialMask);
        
        //========================================
        // エッジのさらなるソフト化（二重smoothstep）
        // なぜ：より滑らかな境界を実現するため
        holeMask = smoothstep(0.0f, 1.0f, holeMask);
        holeMask = smoothstep(0.0f, 1.0f, holeMask); // 二重適用でさらに滑らかに
        
        //========================================
        // 残存時間によるフェードアウト
        // なぜ：時間経過で弾痕が徐々に消えていく様子を表現するため
        holeMask = lerp(1.0f, holeMask, hole.lifeTime);
        
        //========================================
        // 複数の弾痕の影響を乗算で合成
        // なぜ：複数の弾痕が重なると、より強く空洞が開くため
        mask *= holeMask;
    }
    
    return mask;
}

///=============================================================================
///                      雲の密度サンプリング
/// @brief 指定座標での雲の密度を計算
/// @param position サンプリング座標（ワールド空間）
/// @return 雲の密度（0.0～任意、0なら空気）
/// @note FBMノイズを使用し、エッジフェードで自然な境界を作る
float SampleCloudDensity(float3 position) {
    // ワールド座標をボックス内のUVW座標（-0.5～0.5）に変換
    float3 uvw = (position - gCloudCenter) / gCloudSize;
    
    // ボックス外なら密度0を返す（早期リターンで高速化）
    if (any(abs(uvw) > 0.5f)) {
        return 0.0f;
    }
    
    //========================================
    // ベースノイズ（大きな雲の形状を決定）
    // 時間経過でZ軸方向にオフセットを加えてアニメーション
    float3 baseCoord = position * gBaseNoiseScale + float3(0.0f, 0.0f, gTime * gNoiseSpeed);
    float baseNoise = FBM(baseCoord, 2);  // 2オクターブで計算（パフォーマンス最適化）
    
    //========================================
    // ディテールノイズ（細かい模様を追加）
    // ベースより少し遅めにアニメーション（0.7倍速）
    float3 detailCoord = position * gDetailNoiseScale + float3(0.0f, 0.0f, gTime * gNoiseSpeed * 0.7f);
    float detailNoise = FBM(detailCoord, 2);  // 2オクターブで計算（滑らかさ重視）
    
    //========================================
    // ベースとディテールを混合
    // gDetailWeight = 0.0 → ベースのみ（滑らか）
    // gDetailWeight = 1.0 → ディテールのみ（ザラザラ）
    float density = lerp(baseNoise, detailNoise, gDetailWeight);
    
    // カバレッジを適用（閾値以下をカット）
    // gCoverage = 0.0 → 雲が多い
    // gCoverage = 1.0 → 雲がほとんどない
    density = saturate(density - gCoverage);
    
    //========================================
    // エッジフェード（ボックスの境界で密度を減衰）
    // ボックス境界からの距離を計算
    float3 edgeDist = 0.5f - abs(uvw);
    // 最も近い境界までの距離を取得
    float edgeFade = min(min(edgeDist.x, edgeDist.y), edgeDist.z);
    // smoothstepで滑らかにフェードアウト（0.0～0.1の範囲で）
    edgeFade = smoothstep(0.0f, 0.1f, edgeFade);
    
    // 密度 × エッジフェード × 全体密度倍率
    float baseDensity = density * edgeFade * gDensity;
    
    //========================================
    // 弾痕マスクを適用
    // なぜ：Counter-Strike風の動的スモークで、弾が通過した箇所に空洞を作るため
    // FinalDensity(p) = BaseDensity(p) * BulletMask(p)
    float bulletMask = CalculateBulletHoleMask(position);
    
    return baseDensity * bulletMask;
}

///=============================================================================
///                      ライトマーチング（影の計算）
/// @brief 指定座標から太陽方向へレイマーチして光の透過率を計算
/// @param position 開始座標（ワールド空間）
/// @return 光の透過率（0.0=完全に影、1.0=影なし）
/// @note 雲の中を通過する光がどれだけ減衰するかを計算
float LightMarch(float3 position) {
    // 太陽方向ベクトル（正規化）
    float3 lightDir = normalize(gSunDirection);
    
    // 雲のバウンディングボックス
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
    float transmittance = 1.0f;  // 透過率（初期値は完全透過）
    float3 rayPos = position;    // 現在のレイ位置
    
    // ライトマーチングループ（最大MAX_LIGHT_STEPSステップ）
    for (int i = 0; i < MAX_LIGHT_STEPS; i++) {
        // 太陽方向へステップサイズ分進む
        rayPos += lightDir * gLightStepSize;
        
        // バウンディングボックスを出たら終了（これ以上雲がない）
        if (any(rayPos < boxMin) || any(rayPos > boxMax)) {
            break;
        }
        
        // 現在位置の雲密度をサンプリング
        float density = SampleCloudDensity(rayPos);
        
        // 密度が存在する場合、透過率を減衰
        if (density > 0.001f) {
            // ベール・ランベルトの法則: I = I0 * exp(-密度 * 距離)
            // gShadowDensityMultiplierで影の濃さを調整可能
            transmittance *= exp(-density * gLightStepSize * gShadowDensityMultiplier);
            
            // 透過率がほぼ0になったら打ち切り（最適化）
            if (transmittance < 0.01f) break;
        }
    }
    
    return transmittance;
}

///=============================================================================
///                      Henyey-Greenstein位相関数
/// @brief 光の散乱方向の確率分布を計算
/// @param cosTheta 入射方向と散乱方向の内積（cos値）
/// @param g 異方性パラメータ（-1.0～1.0）
/// @return 散乱確率（正規化済み）
/// @note g = 0.0: 等方散乱、g > 0.0: 前方散乱、g < 0.0: 後方散乱
///       雲は前方散乱が強いため、太陽方向から見ると明るく見える
float PhaseHG(float cosTheta, float g) {
    float g2 = g * g;  // g^2を事前計算
    
    // Henyey-Greenstein位相関数の式
    // (1 - g^2) / (4π * (1 + g^2 - 2g*cosθ)^1.5)
    // この関数は散乱角度に対する確率密度を表す
    return (1.0f - g2) / (4.0f * PI * pow(abs(1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

///=============================================================================
///                      線形深度計算
/// @brief 非線形深度値を線形深度に変換
/// @param depth 深度バッファ値（0.0～1.0）
/// @param near ニアプレーン距離
/// @param far ファープレーン距離
/// @return 線形深度値（near～farの実際の距離）
/// @note 深度バッファは双曲線分布のため、線形化が必要な場合に使用
float LinearizeDepth(float depth, float near, float far)
{
    // 透視投影の深度値を線形化する式
    // 2 * near * far / (far + near - depth * (far - near))
    return (2.0f * near * far) / (far + near - depth * (far - near));
}