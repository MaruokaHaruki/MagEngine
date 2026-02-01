/*********************************************************************
 * \file   CloudBulletHole.hlsli
 * \brief  弾痕システムのSDF関数群
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   Counter-Strike風の動的スモークに弾痕を反映するための関数
 *********************************************************************/
#pragma once

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
///                      弾痕マスク計算
/// @brief すべての弾痕からの影響を計算し、雲密度へのマスク値を返す
/// @param position 評価点（ワールド座標）
/// @return マスク値（0.0=完全に空洞、1.0=影響なし）
/// @note FinalDensity(p) = BaseDensity(p) * BulletMask(p) という形で合成する
float CalculateBulletHoleMask(float3 position)
{
    float mask = 1.0f;  // 初期値：影響なし
    
    // すべての有効な弾痕をループで処理
    for (int i = 0; i < gBulletHoleCount; ++i)
    {
        // 弾痕データを取得
        BulletHoleGPU hole = gBulletHoles[i];
        
        // 円柱SDFで距離を計算
        float sdfDist = CylinderSDF(position, hole.origin, hole.direction, hole.radius);
        
        // smoothstepで滑らかなマスクを作成
        // なぜ：急激な変化ではなく、自然なフェードアウトを実現するため
        // edge0: フェード開始距離（この距離より近いと完全に空洞）
        // edge1: フェード終了距離（この距離より遠いと影響なし）
        float holeMask = smoothstep(gBulletHoleFadeStart, gBulletHoleFadeEnd, sdfDist);
        
        // 残存時間によるフェードアウト
        // なぜ：時間経過で弾痕が徐々に消えていく様子を表現するため
        holeMask = lerp(1.0f, holeMask, hole.lifeTime);
        
        // 複数の弾痕の影響を乗算で合成
        // なぜ：複数の弾痕が重なると、より強く空洞が開くため
        mask *= holeMask;
    }
    
    return mask;
}
