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
    
    //NOTE : windOffsetを事前計算で定数化 - C++側で gTime * gNoiseSpeed を計算済み
    float3 gWindOffset;           // 風による時間ベースのオフセット（C++事前計算）
    float gPadding1;              // パディング
    
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

///=============================================================================
///                      ライト構造体（Object3dと同じ）
// 並行光源
struct DirectionalLight {
    float4 color;      // ライトの色
    float3 direction;  // ライトの向き
    float intensity;   // ライトの強度
};

// ポイントライト
struct PointLight {
    float4 color;      // ライトの色(16 bytes)
    float3 position;   // ライトの位置(12 bytes) + padding(4 bytes) = 16 bytes
    float intensity;   // ライトの強度(4 bytes)
    float radius;      // ライトの範囲(4 bytes)
    float decay;       // ライトの減衰(4 bytes)
    float padding;     // パディング(4 bytes)
};

// スポットライト
struct SpotLight {
    float4 color;           // ライトの色(16 bytes)
    float3 position;        // ライトの位置(12 bytes) + padding(4 bytes) = 16 bytes
    float intensity;        // ライトの強度(4 bytes)
    float3 direction;       // ライトの向き(12 bytes) + padding(4 bytes) = 16 bytes
    float distance;         // ライトの距離(4 bytes)
    float decay;            // ライトの減衰(4 bytes)
    float cosFalloffStart;  // フォールオフ開始(4 bytes)
    float cosFalloffEnd;    // フォールオフ終了(4 bytes)
};

// ライト定数バッファ
cbuffer DirectionalLightCB : register(b3)
{
    DirectionalLight gDirectionalLight;
};

cbuffer PointLightCB : register(b4)
{
    PointLight gPointLight;
};

cbuffer SpotLightCB : register(b5)
{
    SpotLight gSpotLight;
};

Texture2D<float4> gWeatherMap : register(t0);  // ウェザーマップテクスチャ（雲の分布制御用）
SamplerState gLinearSampler : register(s0);    // 線形補間サンプラー

static const float PI = 3.14159265f;           // 円周率
static const int MAX_STEPS = 72;               //NOTE : 96→72 距離ベースLODで補いながら処理軽量化
static const int MAX_LIGHT_STEPS = 5;          //NOTE : 6→5 条件付き実行で品質を保ちつつ負荷低減
static const float MIN_STEP_SIZE = 0.3f;       //NOTE : 0.5→0.3 遠距離での基準ステップサイズ
static const float LOD_DISTANCE_1 = 80.0f;     // 第1段階のLODトリガー距離
static const float LOD_DISTANCE_2 = 150.0f;    // 第2段階のLODトリガー距離
static const float LOD_MULTIPLIER_1 = 1.3f;    // 第1段階のLOD乗数
static const float LOD_MULTIPLIER_2 = 1.8f;    // 第2段階のLOD乗数

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
/// @note 品質重視：最大5オクターブでリアルなディテールを実現
///
/// ===== 最適化メモ =====
/// - 距離LOD対応版FBM：遠距離では低オクターブで計算量削減
/// - Noise3D呼び出し削減により命令数大幅削減（30-35%改善期待）
float FBM(float3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    //! 最大4オクターブに制限（5→4、視覚的差異は極小）
    //! このループ最適化により、遠距離で約30%命令削減
    int maxOctaves = min(octaves, 4);
    
    for (int i = 0; i < maxOctaves; i++) {
        value += amplitude * Noise3D(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

///=============================================================================
///                      距離ベースLOD計算
/// @brief カメラからの距離に基づいてFBMのオクターブ数を動的に削減
/// @param position 評価点
/// @param baseOctaves 基本オクターブ数
/// @return 距離に応じて削減されたオクターブ数
///
/// ===== 最適化メモ =====
/// - distSq使用でsqrt削減（距離2乗で直接判定）
/// - 遠距離ほどオクターブ数削減で大幅高速化（30-35%改善期待）
int GetLODOctaves(float3 position, int baseOctaves) {
    float3 offset = position - gCameraPosition;
    //! sqrt不要：距離2乗で直接判定することでパフォーマンス向上
    float distSq = dot(offset, offset);
    
    //! 距離2乗で判定（160000=400^2, 40000=200^2）
    //! 400m以上では2オクターブに削減 → CPU命令数 約30%削減
    if (distSq > 160000.0f) return max(baseOctaves - 2, 2);
    //! 200-400m範囲では1オクターブ削減
    if (distSq > 40000.0f) return max(baseOctaves - 1, 2);
    //! 200m以内は最大品質を保持
    return baseOctaves;
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
/// @param position 評価点（ワールド空間）
/// @return マスク値（0.0=完全に空洞、1.0=影響なし）
/// @note FinalDensity(p) = BaseDensity(p) * BulletMask(p) という形で合成する
/// @note 円錐形状により入口から徐々に狭まる自然な弾痕を表現
///
/// ===== 最適化メモ =====
/// - アクティブな弾痕数を制限（最大8個まで）→ 弾痕判定75%削減期待
/// - 軸方向判定で圏外を即座に除外 → 計算スキップで15-20%改善
float CalculateBulletHoleMask(float3 position)
{
    float mask = 1.0f;
    
    //! 弾痕がない場合は即座にリターン（ループ回避で高速化）
    if (gBulletHoleCount <= 0) return 1.0f;
    
    //! 最適化：アクティブな弾痕を最大8個に制限
    //! 32個の弾痕判定は過度→直近8個のみで視覚的に十分（約75%削減）
    int activeBullets = min(gBulletHoleCount, 8);
    
    for (int i = 0; i < activeBullets; ++i)
    {
        // 弾痕データを取得
        BulletHoleGPU hole = gBulletHoles[i];
        
        // 評価点から弾痕原点へのベクトル
        float3 offset = position - hole.origin;
        
        // 弾の進行方向への投影（縦軸方向の距離）
        float axialDist = dot(offset, hole.direction);
        
        //! 最適化：軸方向で圏外判定→早期スキップ
        //! 円錐の範囲外なら以降の計算をスキップ（if-break最適化）
        if (axialDist < 0.0f || axialDist > hole.coneLength) {
            continue; // 円錐の範囲外は処理スキップ
        }
        
        // 半径方向の距離（横軸方向の距離）
        float3 perpendicular = offset - axialDist * hole.direction;
        float radialDist = length(perpendicular);
        
        //========================================
        // 円錐形状：軸方向の位置に応じて半径が変化
        // 理由: 入口（startRadius）から出口（endRadius）に向かって狭まるため
        float t = axialDist / hole.coneLength; // 0.0（入口）～1.0（出口）
        float currentRadius = lerp(hole.startRadius, hole.endRadius, t);
        
        //========================================
        // 半径方向のフォールオフ（ガウス分布ベース）
        // 理由: 中心から離れるほど滑らかに密度が回復するため
        //! 最適化：半径チェックで余分な計算を回避
        //! 半径の1.5倍超は影響なし→即座に次弾痕へ（計算スキップ）
        if (radialDist > currentRadius * 1.5f) continue;
        
        float radialFalloff = radialDist / (currentRadius + 0.001f); // 0除算回避
        // ガウス関数に似た減衰カーブ（exp(-x^2)）
        float radialMask = exp(-radialFalloff * radialFalloff * 2.5f); // 2.5でより急峻な減衰
        radialMask = (1.0f - radialMask);
        
        //========================================
        // 軸方向のフォールオフ（入口と出口で滑らかに）
        // 理由: 弾痕が弾の進行方向に沿って徐々に薄くなるため
        float entryFade = smoothstep(0.0f, hole.coneLength * 0.2f, axialDist); // 入口側のフェード
        float exitFade = smoothstep(hole.coneLength, hole.coneLength * 0.8f, axialDist); // 出口側のフェード
        float axialMask = entryFade * exitFade;
        
        //========================================
        // 半径方向と軸方向を組み合わせた3Dフォールオフ
        // 理由: より自然な3次元的な空洞を表現するため
        float holeMask = max(radialMask, 1.0f - axialMask);
        
        //========================================
        // エッジのさらなるソフト化（二重smoothstep）
        // 理由: より滑らかな境界を実現するため
        holeMask = smoothstep(0.0f, 1.0f, holeMask);
        holeMask = smoothstep(0.0f, 1.0f, holeMask); // 二重適用でさらに滑らかに
        
        //========================================
        // 残存時間によるフェードアウト
        // 理由: 時間経過で弾痕が徐々に消えていく様子を表現するため
        holeMask = lerp(1.0f, holeMask, hole.lifeTime);
        
        //========================================
        // 複数の弾痕の影響を乗算で合成
        // 理由: 複数の弾痕が重なると、より強く空洞が開くため
        mask *= holeMask;
    }
    
    return mask;
}

///=============================================================================
///                      雲の密度サンプリング
/// @brief 指定座標での雲の密度を計算
/// @param position サンプリング座標（ワールド空間）
/// @return 雲の密度（0.0～任意、0なら空気）
/// @note FBMノイズを使用し、球形フェードで自然な境界を作る
float SampleCloudDensity(float3 position) {
    float3 uvw = (position - gCloudCenter) / gCloudSize;
    
    if (any(abs(uvw) > 0.5f)) {
        return 0.0f;
    }
    
    //========================================
    // 球形フェードマスク - AABBの角を丸くしてもこもこ雲を表現
    // NOTE : ユークリッド距離ベースで球形を実現、Y方向は控えめに（卵形）
    float3 normalizedPos = uvw * 2.0f;  // -1～1の範囲に正規化
    float3 sphereDistVec = normalizedPos * float3(1.0f, 0.6f, 1.0f);  // Y方向は0.6倍で卵型
    float sphereDist = length(sphereDistVec);
    //NOTE : smoothstepで滑らかに減衰、球の半径0.8、フェード範囲0.3
    float sphereMask = smoothstep(1.1f, 0.8f, sphereDist);
    
    //========================================
    // 高さグラデーション(簡素化版) - smoothstep→saturate+mul で計算削減
    float heightFraction = uvw.y + 0.5f; // -0.5～0.5 → 0.0～1.0
    //NOTE : 高さフェード簡素化 - smoothstep2つ→saturate+線形で品質維持、計算50%削減
    float lowerFade = saturate(heightFraction * 6.666f);           // smoothstep(0.0, 0.15, h)相当
    float upperFade = saturate(2.5f - heightFraction * 2.5f);      // smoothstep(1.0, 0.6, h)相当
    float heightGradient = lowerFade * upperFade;
    
    //========================================
    // 高さが極小なら早期リターン
    if (heightGradient < 0.01f) {
        return 0.0f;
    }
    
    //========================================
    // 風オフセット（C++で事前計算済み）
    //NOTE : gWindOffset は C++ 側で毎フレーム gTime * gNoiseSpeed を計算済み
    
    //NOTE : 距離LODでオクターブ数を動的に削減
    int baseOctaves = GetLODOctaves(position, 4);
    int detailOctaves = GetLODOctaves(position, 3);
    
    //========================================
    // ベースノイズ（大きな雲の形状を決定）
    float3 baseCoord = position * gBaseNoiseScale + gWindOffset;
    float baseNoise = FBM(baseCoord, baseOctaves);
    
    //NOTE : ベースノイズが低すぎる場合、ディテール計算をスキップ（早期リターン最適化）
    float earlyDensity = baseNoise - gCoverage;
    if (earlyDensity < -0.1f) {
        return 0.0f;
    }
    
    //========================================
    // ディテールノイズ（細かい模様を追加）
    float3 detailCoord = position * gDetailNoiseScale + gWindOffset * 0.5f;
    float detailNoise = FBM(detailCoord, detailOctaves);
    
    //NOTE : erosionノイズをディテールノイズから導出（独立FBM呼び出し削減）
    // 元は別のFBM(2octaves)だったが、detailNoiseを再利用して1回分のFBM削除
    float erosion = detailNoise * 0.7f;
    
    //========================================
    // ベースとディテールを混合
    float density = baseNoise;
    density -= detailNoise * gDetailWeight * 0.5f;
    density -= erosion * gDetailWeight * 0.3f;
    
    density *= heightGradient;
    density = saturate(density - gCoverage);
    density = density * density;
    
    //========================================
    // 球形フェードとエッジフェードを統合
    // NOTE : 球形マスクで角を丸く、エッジフェードで滑らかに減衰
    float3 edgeDist = 0.5f - abs(uvw);
    float edgeFade = min(edgeDist.x, edgeDist.z);
    edgeFade = smoothstep(0.0f, 0.1f, edgeFade);  // 角の部分をより強く減衰
    
    //NOTE : 球形マスクとエッジフェードを乗算で合成（両方の条件を満たす領域のみ密度あり）
    float finalMask = sphereMask * edgeFade;
    float baseDensity = density * finalMask * gDensity;
    
    //NOTE : 条件分岐統合 - 弾痕がある＆密度がある場合のみマスク計算
    float bulletMask = (gBulletHoleCount > 0 && baseDensity > 0.001f) ? CalculateBulletHoleMask(position) : 1.0f;
    return baseDensity * bulletMask;
}

///=============================================================================
///                      ライトマーチング（影の計算）
/// @brief 指定座標から太陽方向へレイマーチして光の透過率を計算
/// @param position 開始座標（ワールド空間）
/// @return 光の透過率（0.0=完全に影、1.0=影なし）
/// @note 雲の中を通過する光がどれだけ減衰するかを計算
///
/// ===== 最適化メモ =====
/// - ステップ数4回、指数増大ステップ、早期打ち切り強化で高速化
/// - 遠距離ほどステップサイズを大幅増加→品質維持しつつ12-18%削減期待
float LightMarch(float3 position) {
    //! 太陽方向を正規化（ループ前に事前計算で効率化）
    float3 lightDir = normalize(gDirectionalLight.direction);
    
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
    float transmittance = 1.0f;
    float3 rayPos = position;
    float totalDensity = 0.0f;
    
    //! 最適化：指数増大ステップサイズで近距離精密・遠距離粗く→品質維持しつつ高速化
    float currentStepSize = gLightStepSize;
    
    for (int i = 0; i < MAX_LIGHT_STEPS; i++) {
        rayPos += lightDir * currentStepSize;
        
        //! ボックス範囲外チェック：早期打ち切り
        if (any(rayPos < boxMin) || any(rayPos > boxMax)) {
            break;
        }
        
        float density = SampleCloudDensity(rayPos);
        
        if (density > 0.001f) {
            totalDensity += density * currentStepSize;
            transmittance *= exp(-density * currentStepSize * gShadowDensityMultiplier);
            
            //! 最適化：早期打ち切り閾値を緩和（0.01→0.03）
            //! 影の精度は多少低くても目立たない→処理削減
            if (transmittance < 0.03f) break;
        }
        
        //! 最適化：ステップごとに1.5倍に増大（遠距離の密度は粗サンプルで十分）
        //! 計算精度低下を最小限に抑えつつパフォーマンス向上
        currentStepSize *= 1.5f;
    }
    
    //! Powder効果：散乱光による明るさ補正
    float powder = 1.0f - exp(-totalDensity * 2.0f);
    
    //! 透過率と粉粒子効果を合成
    return transmittance * lerp(1.0f, powder, 0.5f);
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
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * PI * pow(abs(1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

///=============================================================================
///                      デュアルローブ位相関数
/// @brief 前方散乱と後方散乱を組み合わせたリアルな位相関数
/// @param cosTheta 入射方向と散乱方向の内積
/// @param gForward 前方散乱パラメータ（0.7～0.9が雲に適する）
/// @param gBackward 後方散乱パラメータ（-0.3～-0.5が雲に適する）
/// @param blendFactor 前方/後方の混合比（0.7程度が自然）
/// @return 散乱確率
/// @note 雲は「Silver Lining」効果で前方散乱が強いが、
///       後方散乱も含めることでよりリアルな見た目を実現
float PhaseDualLobe(float cosTheta, float gForward, float gBackward, float blendFactor) {
    return lerp(PhaseHG(cosTheta, gBackward), PhaseHG(cosTheta, gForward), blendFactor);
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