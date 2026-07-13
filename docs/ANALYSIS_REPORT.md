# MagEngine RenderGraph Analysis Report

## 1. 対象改修

`RenderGraph::BuildTransitionPlan()`で生成したTransition Planを使い、以下4件だけを限定的に自動発行する構成へ移行した。

```text
SceneColor:
- PIXEL_SHADER_RESOURCE -> RENDER_TARGET
- RENDER_TARGET -> PIXEL_SHADER_RESOURCE

PresentColor:
- PRESENT -> RENDER_TARGET
- RENDER_TARGET -> PRESENT
```

`SceneDepth`、PostEffect内部Resource、Texture Upload、Copy、UAVは今回の自動発行対象外とした。

## 2. 責務分離

`RenderGraph`はTransition Planの生成と検索に限定し、GPU Resource、CommandList、Descriptorを所有しない。

実Resourceの解決とBarrier発行はGraph外へ分離した。

```text
RenderGraph:
- Transition Plan生成
- Pass間Resource Stateの検証

IRenderResourceResolver / DirectXCore:
- SceneColor Resource解決
- 現在のBackBuffer Resource解決

RenderTransitionExecutor:
- Boundaryに対応するPlan取得
- 自動化対象のホワイトリスト判定
- Resource解決
- RenderBarrierRecorder経由でBarrier発行

RenderBarrierRecorder:
- D3D12 ResourceBarrier発行
- 実行済みBarrierの記録
```

この分離により、RenderGraphがDirectX 12の実体に依存しない構成を維持している。

## 3. 実装内容

`RenderTransitionExecutor`を追加し、Boundary単位でTransition Planを実行できるようにした。

```text
RenderTexturePreDraw:
- SceneColor: PIXEL_SHADER_RESOURCE -> RENDER_TARGET

RenderTexturePostDraw:
- SceneColor: RENDER_TARGET -> PIXEL_SHADER_RESOURCE

BeginPresentRenderTarget:
- PresentColor: PRESENT -> RENDER_TARGET

BeforePresent:
- PresentColor: RENDER_TARGET -> PRESENT
```

旧手動Barrier呼び出しは、上記4箇所についてExecutor経由へ置き換えた。

ただし、Renderer初期化前にもDirectXCore側の初期化処理で同じ関数が呼ばれるため、`RenderTransitionExecutor`未設定時のみ従来の手動Barrierへフォールバックする。

## 4. 自動化対象の制限

`RenderTransitionExecutor::IsAutoTransitionSupported()`で明示ホワイトリストを設けた。

許可するのは以下のみである。

```text
- SceneColor: PixelShaderResource -> RenderTarget
- SceneColor: RenderTarget -> PixelShaderResource
- PresentColor: Present -> RenderTarget
- PresentColor: RenderTarget -> Present
```

対象外Resourceや対象外State遷移が実行対象になった場合は、Debug時に検出できるようにしている。

## 5. 二重発行防止

`RenderBarrierRecorder`の実行記録を確認し、同じResourceかつ同じBoundaryに対するTransitionを二重実行しないようにした。

二重実行を検出した場合、CPUテスト用Resultでは`DuplicateBoundaryExecution`を返す。

実行済みBarrier記録は従来どおり残しており、`RenderGraph::CompareTransitionPlanWithManualBarriers()`によるPlan比較も維持している。

期待する比較結果は以下である。

```text
Plan: 4
Executed: 4
Missing: 0
Unexpected: 0
Before mismatch: 0
After mismatch: 0
Sequence mismatch: 0
Boundary mismatch: 0
```

## 6. 診断表示

`Renderer::ReportSmokeTestDiagnostics()`でTransition Planごとに自動発行対象かどうかを出力する。

通常フレームでは大量ログを出さず、明示的に`Report Render Diagnostics`を実行した場合に確認する方針を維持している。

```text
- Boundary
- Resource
- Before
- After
- Plan Sequence
- AutoTransitionEnabled
```

## 7. CPUテスト

GPU、Window、SwapChain、実CommandListを使わないCPUテストを追加した。

```text
正常系:
- 4Boundaryが対応するPlanだけを実行する
- 4Boundary実行後、Planと実行記録が一致する
- SceneDepthやホワイトリスト外State遷移は対象外になる

異常系:
- SceneColorまたはPresentColorを解決できない
- Planが存在しないBoundary
- 同一Boundary二重実行
- ホワイトリスト外State遷移
```

結果は以下である。

```text
Render validation tests: 70/70 passed
```

## 8. Debug x64ビルド

Debug x64構成でビルド成功を確認した。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

## 9. 残リスク

現時点ではCPUテストとDebug x64ビルドまで完了している。

実GPU上でのDirectX 12 Debug Layer Smoke Testは未実施であるため、以下は今後確認が必要である。

```text
- TitleScene / GamePlaySceneで画面が黒くならないこと
- Report Render DiagnosticsでPlan 4件、Executed 4件が一致すること
- ERROR / CORRUPTION / Device Removedが発生しないこと
- PostEffect後のPresentColorがBeforePresentでPresentへ戻ること
- Scene遷移を繰り返しても前Sceneの描画やParticle / Trailが残らないこと
```

## 10. 次の改修候補

次は、PostEffect内部ResourceのBarrier計画化を検討するのがよい。

今回の自動化対象はSceneColorとPresentColorの外側4遷移に限定しているため、PostEffect内部のPing-Pong Resourceや中間TextureのState遷移はまだRenderGraph上で説明できていない。

## 11. Line描画のRenderPass移行

HUD、LockOnHUD、Debug Lineが`LineManager`へ蓄積していたLine描画を、`LineRenderPass`経由で実行する構成へ移行した。

`LineManager`の所有権は`MagFramework`に残し、`RenderWorld`には描画フレーム中だけ有効な非所有参照を登録する。

```text
RenderWorld:
- LineRenderItem
- LineManager非所有参照

LineRenderPass:
- RenderWorldからLineRenderItemを取得
- LineManager::Draw()を1回だけ実行
```

旧経路では`MagFramework::RenderPreDraw()`から`LineManager::Draw()`を直接呼んでいた。

移行後は直接Drawを削除し、`MagFramework::OpaqueRender()`でSceneの`RegisterRenderables()`完了後にLineManagerを`RenderWorld`へ登録する。

## 12. LineRenderPassのPhaseとOrder

Line描画は`PostOverlay`へ配置した。

```text
Phase: PostOverlay
Order: Particleより後
```

理由は、Line描画がHUD、LockOnHUD、Debug Lineを含み、SpriteやParticleと同じくSceneColorへ合成されるOverlay系の描画だからである。

また、PostEffect前にSceneColorへ描く点は既存のオフスクリーン描画経路と一致している。

## 13. Line Resource Usage

Line PSOは`LineSetup::CreateGraphicsPipeline()`で以下のDepth設定を使用している。

```text
DepthEnable: true
DepthWriteMask: D3D12_DEPTH_WRITE_MASK_ALL
DepthFunc: D3D12_COMPARISON_FUNC_LESS_EQUAL
```

そのため、RenderGraph上のResource Usageは以下とした。

```text
SceneColor:
- Access: ReadWrite
- Required State: RenderTarget

SceneDepth:
- Access: ReadWrite
- Required State: DepthWrite
```

## 14. Line移行テスト結果

`RenderValidationTests.cpp`へLine PassのResource Usage検証を追加した。

```text
正常系:
- LineRenderPassがSceneColor = RenderTargetで検証成功
- LineRenderPassがSceneDepth = DepthWriteで検証成功
- PostOverlay内でParticleより後のOrderであることを確認

異常系:
- SceneColor Usage未宣言をLine Pass用Usageチェックで検出
- SceneDepth Usage未宣言をLine Pass用Usageチェックで検出
```

CPUテスト結果は以下である。

```text
Render validation tests: 76/76 passed
```

## 15. Line移行後の残リスク

実GPU上でのDirectX 12 Debug Layer Smoke Testは未実施である。

今後は以下を確認する必要がある。

```text
- HUD / LockOnHUDのLineが意図した前後関係で表示されること
- Debug LineがParticle後、PostEffect前に表示されること
- SceneDepth書き込みによるHUD Lineの隠れ方が既存見た目と許容差内であること
- Scene遷移後にLineが不自然に残らないこと
```

## 16. RenderPass / RenderGraph / Barrier安定化

今回までのRenderPass移行範囲は以下である。

```text
- SkyboxRenderPass
- OpaqueRenderPass
- CloudRenderPass
- TrailRenderPass
- SpriteRenderPass
- ParticleRenderPass
- LineRenderPass
- PostEffectRenderPass
```

### 16.1 LineRenderPassの旧順序確認

旧実装では、`EngineApp::Draw()`内で以下の順序だった。

```text
RenderPreDraw
  - RenderTexturePreDraw
  - SrvSetup::PreDraw
  - LineManager::Draw
OpaqueRender
  - Scene RegisterRenderables
  - Scene Phase
Overlay
PostOverlay
RenderPostDraw
PostProcess
ImGui
PostDraw
```

したがって、旧`LineManager::Draw()`より前に実行されていた描画はない。

旧`LineManager::Draw()`より後に実行されていた描画は以下である。

```text
- Skybox
- Opaque
- Cloud
- Trail
- Sprite
- Particle
- PostEffect
- ImGui
```

この確認により、Lineを`PostOverlay`のParticle後へ置く構成は旧順序と一致しないと判断した。

### 16.2 最終Phase / Order

最終的に`LineRenderPass`は以下へ修正した。

```text
Phase: Scene
Order: 50
位置: Skyboxより前
```

これは旧`RenderPreDraw()`内の直接Line描画が、Scene描画より前に実行されていたことに合わせるためである。

### 16.3 Line Resource Usage

Line PSOは以下のDepth設定である。

```text
DepthEnable: true
DepthWriteMask: D3D12_DEPTH_WRITE_MASK_ALL
DepthFunc: D3D12_COMPARISON_FUNC_LESS_EQUAL
```

そのため、RenderGraph上の宣言は以下のままとした。

```text
SceneColor:
- Access: ReadWrite
- Required State: RenderTarget

SceneDepth:
- Access: ReadWrite
- Required State: DepthWrite
```

### 16.4 限定自動Barrier発行の対象範囲

`RenderTransitionExecutor::IsAutoTransitionSupported()`で許可する自動発行対象は、引き続き以下4遷移のみである。

```text
SceneColor:
- PixelShaderResource -> RenderTarget
- RenderTarget -> PixelShaderResource

PresentColor:
- Present -> RenderTarget
- RenderTarget -> Present
```

以下は自動発行対象外である。

```text
- SceneDepth
- PostEffect内部Resource
- Texture Upload
- Copy
- UAV
- Line固有Resource
```

### 16.5 テストとビルド

CPUテスト結果は以下である。

```text
Render validation tests: 76/76 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 16.6 GPU確認状況

この作業ではGUIアプリを短時間起動したが、`MagEngine.exe`は約20秒以内に`exit code 3`で終了した。

この起動方法ではVisual Studio OutputのDebug Layerメッセージを取得できていないため、実GPU Smoke Testは未完了として扱う。

未確認項目は以下である。

```text
- TitleScene表示
- GamePlayScene表示
- HUD / LockOnHUD / Debug Line表示
- LineがSkybox前の旧順序相当で描画されること
- PostEffect後にImGuiが表示されること
- Scene遷移後にLineが残らないこと
- D3D12 Debug Layer ERROR / CORRUPTIONが0件であること
- Report Render DiagnosticsでPlan / Executed / mismatchなしであること
```

### 16.7 現在残るリスク

Line描画は旧GPU実行位置に合わせてScene先頭へ移したが、RenderWorld登録後にLinePassが実行されるため、旧実装で発生していた可能性のある1フレーム遅延とは完全には同一ではない。

ただし、GPU上の描画順としては、SceneColor描画開始後かつSkyboxより前という旧順序に最も近い。

### 16.8 区切りとする理由

主要描画経路はRenderPass化され、SceneColor / SceneDepth / PresentColorのRequired State検証、Transition Plan、手動Barrier比較、4遷移限定の自動Barrier実行まで確認できた。

CPUテストとDebug x64ビルドも成功しているため、ここを一区切りとして実GPU Smoke Testで挙動確認へ進める状態である。

### 16.9 次の改修候補

次に進む場合は、PostEffect内部ResourceのBarrier計画化を検討する。

## 17. PipelineRecipe / PipelineBuilder導入

Sprite、Line、ParticleのPSO生成処理を、将来のShader / PSO追加がしやすい最小構成へ整理した。

今回の対象は以下に限定した。

```text
- SpriteSetup
- LineSetup
- ParticleSetup
```

Object3D、Skybox、Cloud、Trail、PostEffect、Fullscreen系のPSO生成は変更していない。

### 17.1 改修前の課題

各Setupの`CreateGraphicsPipeline()`内に、InputLayout、BlendState、RasterizerState、DepthStencilState、Shader Compile、PSO生成がまとまっていた。

そのため、新しいShaderやPSOを追加するたびに、D3D12の定型コードを複製する必要があった。

### 17.2 PipelineRecipeの責務

`PipelineRecipe`は、描画機能ごとのPSO設定値だけを保持する。

```text
- VertexShader / PixelShader
- RootSignature
- InputLayout
- BlendState
- RasterizerState
- DepthStencilState
- RTV / DSV Format
- PrimitiveTopologyType
```

RecipeはGPU Resource、CommandList、Descriptorを所有しない。

CPUテストで必須項目を確認できるように、最小限の`Validate()`も追加した。

### 17.3 PipelineBuilderの責務

`PipelineBuilder`は、RecipeからD3D12 PSOを生成する定型処理だけを担当する。

```text
- Recipe検証
- 既存DirectXCore::CompileShader()によるVS / PS Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- ID3D12Device::CreateGraphicsPipelineState()呼び出し
```

BuilderはPipelineManagerではなく、PSOキャッシュ、Shader enum、中央switch、RootSignature自動生成、Reflection、Hot Reloadは持たない。

### 17.4 移行したPSO設定

Spriteは`Sprite.VS.hlsl` / `Sprite.PS.hlsl`、SrcAlpha / InvSrcAlpha Blend、Back Cull、Depth Write All、Triangle Topologyを維持した。

Lineは`Line.VS.hlsl` / `Line.PS.hlsl`、WriteMaskのみのBlend、Back Cull、Depth Write All、Line Topologyを維持した。

Particleは`Particle.VS.hlsl` / `Particle.PS.hlsl`、SrcAlpha / Oneの加算Blend、Cull None、Depth Write Zero、Triangle Topologyを維持した。

3種類ともRTVは`R8G8B8A8_UNORM_SRGB`、DSVは`D24_UNORM_S8_UINT`のままである。

### 17.5 新しいShader / PSO追加手順

今後、同系統の小規模PSOを追加する場合は、Setup側で専用Recipeを作り、Builderへ渡す。

```text
1. SetupでRootSignatureを作成
2. CreateDefaultRecipe(rootSignature)で固有設定を記述
3. PipelineBuilder::CreateGraphicsPipeline(recipe)でPSOを作成
```

この手順により、D3D12の定型PSO生成コードをSetupごとに複製しにくくなる。

### 17.6 テストとビルド

CPUテストへRecipe検証を追加した。

```text
- Sprite RecipeのShader / RTV / Topology確認
- Line RecipeのDepth Write / Topology確認
- Particle Recipeの加算Blend / Depth Write Zero確認
- RootSignature、RTV、VS、PSの未設定検出
```

結果は以下である。

```text
Render validation tests: 93/93 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 17.7 残リスク

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上でのDirectX 12 Debug Layer Smoke Testは未実施のため、Sprite、Line、Particleの見た目が旧実装と一致するかは今後確認が必要である。

### 17.8 次の改修候補

次に進む場合は、Recipe化対象を広げる前に、Debug Layer Smoke TestでSprite、Line、Particleの描画結果とPSO生成結果を確認する。

## 18. Object3D PSO生成のPipelineRecipe移行

Object3D系PSO生成を、既存の`PipelineRecipe` + `PipelineBuilder`へ移行した。

今回の対象は`Object3dSetup`のみであり、Skybox、Cloud、Trail、PostEffect、RenderGraph、Barrier処理は変更していない。

### 18.1 移行前のPSO生成構造

移行前は`Object3dSetup::CreateGraphicsPipeline()`内で、以下をすべて直接実行していた。

```text
- RootSignature作成
- InputLayout定義
- BlendState定義
- RasterizerState定義
- Object3d.VS.hlsl / Object3d.PS.hlslのCompile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

この構造では、Object3D固有設定とD3D12 PSO生成の定型処理が同じ関数内に混在していた。

### 18.2 Object3D Recipeの内容

Object3D固有のPSO設定は`Object3dSetup::CreateDefaultRecipe()`と`CreatePipelineRecipe()`へ移した。

維持した設定は以下である。

```text
VS: resources/shader/Object3d.VS.hlsl
PS: resources/shader/Object3d.PS.hlsl
InputLayout: POSITION / TEXCOORD / NORMAL
Blend: 無効
Rasterizer: Back Cull / Solid
Depth: Enable / Write All / LessEqual
RTV: R8G8B8A8_UNORM_SRGB
DSV: D24_UNORM_S8_UINT
Topology: Triangle
```

Root Signatureは従来どおり`Object3dSetup`が所有し、Recipeには生成済みRoot Signatureの非所有ポインタだけを渡す。

### 18.3 PipelineBuilderへ移した責務

`PipelineBuilder`へ移した責務は以下に限定した。

```text
- Recipe検証
- DirectXCore::CompileShader()によるShader Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

BuilderにはObject3D専用のif、switch、専用APIは追加していない。

### 18.4 Object3D側に残した責務

Object3D側には以下を残している。

```text
- Root Signature定義
- Object3D固有のRecipe定義
- CommonDrawSetupでのRoot Signature / PSO / PrimitiveTopology設定
```

描画順、RenderPass、Descriptor、Resource Barrier、RenderGraphの構造は変更していない。

### 18.5 追加テスト

CPUテストへObject3D Recipe検証を追加した。

```text
正常系:
- VS / PS Shader Path
- RootSignature
- RTV Format
- InputLayout
- Depth Enable / Depth Write / DepthFunc
- Cull Mode
- Primitive Topology

異常系:
- RootSignature未設定
- RTV Format未設定
```

既存のRecipe異常系テストで、VS未設定、PS未設定も引き続き検出している。

結果は以下である。

```text
Render validation tests: 106/106 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 18.6 GPU実行で未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を今後確認する必要がある。

```text
- Object3Dの見た目が旧実装と一致すること
- Lighting / Environment Map用Root Parameterの動作が変わらないこと
- OpaqueRenderPassでPSO設定が従来どおり有効であること
- DirectX 12 Debug LayerでPSO / Root Signature不整合が出ないこと
```

### 18.7 次の改修候補

次に進む場合は、Object3DのGPU Smoke Testを行い、問題がなければSkyboxのPSO生成を同じRecipe方式へ移行する。

## 19. Skybox PSO生成のPipelineRecipe移行

Skybox系PSO生成を、既存の`PipelineRecipe` + `PipelineBuilder`へ移行した。

今回の対象は`SkyboxSetup`のみであり、Object3D、Sprite、Line、Particle、Cloud、Trail、PostEffect、RenderGraph、Barrier処理は変更していない。

### 19.1 移行前のPSO生成構造

移行前は`SkyboxSetup::CreateGraphicsPipeline()`内で、以下をすべて直接実行していた。

```text
- RootSignature作成
- InputLayout定義
- BlendState定義
- RasterizerState定義
- Skybox.VS.hlsl / Skybox.PS.hlslのCompile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

Cubemap用Descriptor TableはRoot Signature内で定義され、実Draw時には`Skybox::Draw()`から`SetGraphicsRootDescriptorTable(1, ...)`で設定されていた。

### 19.2 Skybox Recipeの内容

Skybox固有のPSO設定は`SkyboxSetup::CreateDefaultRecipe()`と`CreatePipelineRecipe()`へ移した。

維持した設定は以下である。

```text
VS: resources/shader/Skybox.VS.hlsl
PS: resources/shader/Skybox.PS.hlsl
InputLayout: POSITION
Blend: 無効
Rasterizer: Cull None / Solid
Depth: Enable / Write Zero / LessEqual
RTV: R8G8B8A8_UNORM_SRGB
DSV: D24_UNORM_S8_UINT
Topology: Triangle
```

Skyboxは背景として描画するため、Depth Writeは従来どおり無効のままとした。

### 19.3 PipelineBuilderへ移した責務

`PipelineBuilder`へ移した責務は以下に限定した。

```text
- Recipe検証
- DirectXCore::CompileShader()によるShader Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

BuilderにはSkybox専用API、if、switch、Shader名判定は追加していない。

### 19.4 Skybox側に残した責務

Skybox側には以下を残している。

```text
- Root Signature定義
- Cubemap用Descriptor Table定義
- Skybox固有のRecipe定義
- CommonDrawSetupでのRoot Signature / PSO / PrimitiveTopology設定
- Skybox::Draw()でのCubemap Descriptor設定
```

Cubemap Descriptor設定、定数バッファ設定、Draw呼び出し、SkyboxRenderPassのPhase / Orderは変更していない。

### 19.5 追加テスト

CPUテストへSkybox Recipe検証を追加した。

```text
正常系:
- VS / PS Shader Path
- RootSignature
- RTV Format
- Primitive Topology
- Cull None / Fill Solid
- Depth Enable / Depth Write Zero / LessEqual

異常系:
- VS未設定
- PS未設定
- RootSignature未設定
- RTV Format未設定
```

結果は以下である。

```text
Render validation tests: 121/121 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 19.6 GPU実行で未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を今後確認する必要がある。

```text
- Skyboxの見た目が旧実装と一致すること
- Cubemap SRVが従来どおりRoot Parameter 1へ設定されること
- Depth Write Zero / LessEqualに起因する前後関係が変わらないこと
- DirectX 12 Debug LayerでPSO / Root Signature不整合が出ないこと
```

### 19.7 次の改修候補

次に進む場合は、Skyboxを含むGPU Smoke Testを行い、問題がなければCloudのPSO生成をRecipe方式へ移行する。

## 20. Cloud PSO生成のPipelineRecipe移行

### 20.1 移行前のPSO生成構造

Cloudは`CloudSetup::CreateGraphicsPipeline()`内で、以下を直接実行していた。

```text
- Cloud.VS.hlsl / Cloud.PS.hlslのShader Compile
- D3D12_INPUT_ELEMENT_DESC定義
- D3D12_BLEND_DESC定義
- D3D12_RASTERIZER_DESC定義
- D3D12_DEPTH_STENCIL_DESC定義
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

Root Signatureは`CloudSetup`が所有しており、Noise / SDF / Texture Descriptor、ConstantBuffer、Draw、CloudRenderPassのPhase / Orderは今回の対象外とした。

### 20.2 Cloud Recipeの内容

Cloud固有のPSO設定は`CloudSetup::CreateDefaultRecipe()`へ移した。

```text
VS: resources/shader/Cloud.VS.hlsl
PS: resources/shader/Cloud.PS.hlsl
InputLayout:
- POSITION / R32G32B32_FLOAT
- TEXCOORD / R32G32_FLOAT
Blend:
- BlendEnable = TRUE
- SrcBlend = SRC_ALPHA
- DestBlend = INV_SRC_ALPHA
- BlendOp = ADD
- SrcBlendAlpha = ONE
- DestBlendAlpha = INV_SRC_ALPHA
- BlendOpAlpha = ADD
Rasterizer:
- CullMode = NONE
- FillMode = SOLID
Depth:
- DepthEnable = TRUE
- DepthWriteMask = ALL
- DepthFunc = LESS_EQUAL
RTV: R8G8B8A8_UNORM_SRGB
DSV: D24_UNORM_S8_UINT
Topology: TRIANGLE
```

### 20.3 PipelineBuilderへ移した責務

`PipelineBuilder`へ移した責務は以下に限定した。

```text
- Recipe検証
- DirectXCore::CompileShader()によるShader Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

BuilderにはCloud専用API、if、switch、Shader名判定、PipelineManager、Shader enumは追加していない。

### 20.4 Cloud側に残した責務

Cloud側には以下を残している。

```text
- Root Signature定義
- Cloud固有のRecipe定義
- CommonDrawSetupでのRoot Signature / PSO / PrimitiveTopology設定
- Noise / SDF / Texture Descriptor設定
- ConstantBuffer設定
- Cloud生成、更新、アニメーション
- Cloud用Draw処理
```

Noise / SDF Descriptor処理、Cloud生成・更新、Draw呼び出し、CloudRenderPassのPhase / Order、RenderGraph Usage、Barrier処理は変更していない。

### 20.5 追加テスト

CPUテストへCloud Recipe検証を追加した。

```text
正常系:
- VS / PS Shader Path
- RootSignature
- RTV Format
- Primitive Topology
- InputLayout
- Blend Enable / Src / Dest
- Cull None / Fill Solid
- Depth Enable / Depth Write All / LessEqual

異常系:
- VS未設定
- PS未設定
- RootSignature未設定
- RTV Format未設定
```

結果は以下である。

```text
Render validation tests: 140/140 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 20.6 GPU実行で未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を今後確認する必要がある。

```text
- Cloudの透明表現が旧実装と一致すること
- Depth Write All / LessEqualに起因する前後関係が変わらないこと
- Noise / SDF / Texture Descriptor参照が従来どおり動作すること
- DirectX 12 Debug LayerでPSO / Root Signature不整合が出ないこと
```

### 20.7 次の改修候補

次に進む場合は、Trail系PSO生成を同じ`PipelineRecipe + PipelineBuilder`方式へ移行する。

## 21. Trail PSO生成のPipelineRecipe移行

### 21.1 移行前のPSO生成構造

Trailは`TrailEffectSetup::CreateGraphicsPipeline()`内で、以下を直接実行していた。

```text
- Trail.VS.hlsl / Trail.PS.hlslのShader Compile
- D3D12_INPUT_ELEMENT_DESC定義
- D3D12_BLEND_DESC定義
- D3D12_RASTERIZER_DESC定義
- D3D12_DEPTH_STENCIL_DESC定義
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

Root Signatureは`TrailEffectSetup`が所有しており、Trail頂点生成、VertexBuffer更新、ConstantBuffer設定、Draw、TrailRenderPassのPhase / Orderは今回の対象外とした。

### 21.2 Trail Recipeの内容

Trail固有のPSO設定は`TrailEffectSetup::CreateDefaultRecipe()`へ移した。

```text
VS: resources/shader/Trail.VS.hlsl
PS: resources/shader/Trail.PS.hlsl
InputLayout:
- POSITION / R32G32B32_FLOAT
- NORMAL / R32G32B32_FLOAT
- TEXCOORD / R32_FLOAT
Blend:
- BlendEnable = TRUE
- SrcBlend = SRC_ALPHA
- DestBlend = INV_SRC_ALPHA
- BlendOp = ADD
- SrcBlendAlpha = ONE
- DestBlendAlpha = INV_SRC_ALPHA
- BlendOpAlpha = ADD
Rasterizer:
- CullMode = NONE
- FillMode = SOLID
Depth:
- DepthEnable = TRUE
- DepthWriteMask = ALL
- DepthFunc = LESS_EQUAL
RTV: R8G8B8A8_UNORM_SRGB
DSV: D24_UNORM_S8_UINT
Topology: TRIANGLE
```

### 21.3 PipelineBuilderへ移した責務

`PipelineBuilder`へ移した責務は以下に限定した。

```text
- Recipe検証
- DirectXCore::CompileShader()によるShader Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

BuilderにはTrail専用API、if、switch、Shader名判定、PipelineManager、Shader enumは追加していない。

### 21.4 Trail側に残した責務

Trail側には以下を残している。

```text
- Root Signature定義
- Trail固有のRecipe定義
- CommonDrawSetupでのRoot Signature / PSO / PrimitiveTopology設定
- Trail頂点生成
- VertexBuffer / IndexBuffer更新
- ConstantBuffer設定
- Trail用Draw処理
- Trail更新・寿命管理
```

VertexBuffer更新、ConstantBuffer設定、Draw呼び出し、TrailRenderPassのPhase / Order、RenderGraph Usage、Barrier処理は変更していない。

### 21.5 追加テスト

CPUテストへTrail Recipe検証を追加した。

```text
正常系:
- VS / PS Shader Path
- RootSignature
- RTV Format
- Primitive Topology
- InputLayout
- Blend Enable / Src / Dest
- Cull None / Fill Solid
- Depth Enable / Depth Write All / LessEqual

異常系:
- VS未設定
- PS未設定
- RootSignature未設定
- RTV Format未設定
```

結果は以下である。

```text
Render validation tests: 159/159 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 21.6 GPU実行で未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を今後確認する必要がある。

```text
- Trailの透明表現と残像が旧実装と一致すること
- Depth Write All / LessEqualに起因する前後関係が変わらないこと
- VertexBuffer / IndexBuffer更新後の描画が従来どおり動作すること
- DirectX 12 Debug LayerでPSO / Root Signature不整合が出ないこと
```

### 21.7 次の改修候補

次に進む場合は、PostEffect / Fullscreen系PSO生成をRecipe方式へ移行する前に、Recipe化済み描画経路のGPU Smoke Testを行う。

## 22. PostEffect / Fullscreen PSO生成のPipelineRecipe移行

### 22.1 移行前のPSO生成構造

PostEffect / Fullscreen系は、複数のクラスがそれぞれPSOを直接生成していた。

```text
- DirectXCore::CreateOffScreenPipeLine()
- FullscreenPassRendere::CreatePipeline()
- GrayscaleEffect::CreatePipeline()
- Vignetting::CreatePipeline()
```

各箇所で以下を直接実行していた。

```text
- FullScreen.VS.hlslと各Pixel ShaderのShader Compile
- D3D12_INPUT_LAYOUT_DESC定義
- D3D12_BLEND_DESC定義
- D3D12_RASTERIZER_DESC定義
- D3D12_DEPTH_STENCIL_DESC定義
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

### 22.2 Recipe化したPSO一覧

今回Recipe化したPSOは以下である。

```text
DirectXCore RenderTexture Fullscreen:
- VS: resources/shader/FullScreen.VS.hlsl
- PS: resources/shader/FullScreen.PS.hlsl

FullscreenPassRendere:
- VS: resources/shader/FullScreen.VS.hlsl
- PS: resources/shader/Fullscreen.PS.hlsl

GrayscaleEffect:
- VS: resources/shader/FullScreen.VS.hlsl
- PS: resources/shader/Grayscale.PS.hlsl

Vignetting:
- VS: resources/shader/FullScreen.VS.hlsl
- PS: resources/shader/Vignetting.hlsl
```

EffectごとのShader PathやPSO設定は統合していない。

### 22.3 PostEffect Recipeの内容

PostEffect / Fullscreen系Recipeは既存PSO設定を維持している。

```text
InputLayout:
- 空
Blend:
- RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL
Rasterizer:
- CullMode = NONE
- FillMode = SOLID
Depth:
- DepthEnable = FALSE
RTV: R8G8B8A8_UNORM_SRGB
DSV: D24_UNORM_S8_UINT
Topology: TRIANGLE
```

Fullscreen描画は既存Shader側で頂点を生成する前提のため、InputLayoutは空のまま維持した。

### 22.4 PipelineBuilderへ移した責務

`PipelineBuilder`へ移した責務は以下に限定した。

```text
- Recipe検証
- DirectXCore::CompileShader()によるShader Compile
- D3D12_GRAPHICS_PIPELINE_STATE_DESC構築
- CreateGraphicsPipelineState呼び出し
```

BuilderにはPostEffect専用API、if、switch、Shader名判定、PipelineManager、Shader enumは追加していない。

### 22.5 PostEffect側に残した責務

PostEffect側には以下を残している。

```text
- Root Signature定義
- EffectごとのRecipe定義
- RenderTexture管理
- Ping-Pong切り替え
- SRV / RTV / Descriptor設定
- ConstantBuffer設定
- Fullscreen Draw処理
- PostEffect適用順
- PostEffect内部Resourceの手動Barrier
```

RenderTexture、Ping-Pong Resource、Descriptor、Draw呼び出し、PostEffect順序、RenderGraph、RenderBarrierRecorder、RenderTransitionExecutorは変更していない。

### 22.6 追加テスト

CPUテストへPostEffect / Fullscreen Recipe検証を追加した。

```text
正常系:
- VS / PS Shader Path
- RootSignature
- RTV Format
- Primitive Topology
- InputLayoutが空
- WriteMask
- Cull None / Fill Solid
- Depth無効

異常系:
- VS未設定
- PS未設定
- RootSignature未設定
- RTV Format未設定
```

結果は以下である。

```text
Render validation tests: 207/207 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 22.7 GPU実行で未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を今後確認する必要がある。

```text
- Effectなし時のRenderTexture Fullscreen表示が旧実装と一致すること
- Grayscale / Vignetting適用結果が旧実装と一致すること
- Ping-Pong切り替え後の入力SRV / 出力RTVが従来どおり動作すること
- DirectX 12 Debug LayerでPSO / Root Signature不整合が出ないこと
```

### 22.8 次の改修候補

次に進む場合は、Recipe化済みPSO全体を対象にGPU Smoke Testを行い、その後にPostEffect内部ResourceのBarrier計画化を検討する。

## 23. PostEffect内部Ping-Pong ResourceのTransition計画化

### 23.1 全体RenderGraphへ追加しない理由

PostEffect内部で使用するPing-Pong Resourceは、`DirectXCore::renderTextureResources_[2]`が所有している。

`DirectXCore::ResolveRenderResource(RenderResourceId::SceneColor)`も現在の`renderResourceIndex_`に対応する同じRenderTexture実体を返すため、PostEffect内部Resourceを`RenderResourceId`へ追加すると、SceneColorと同一実体を別IDとして扱う危険がある。

そのため、全体RenderGraphは従来どおり以下だけを追跡する。

```text
- SceneColor
- SceneDepth
- PresentColor
```

PostEffect内部のPing-Pong遷移は、`PostEffectManager`内部だけでPlan、実行記録、比較結果を保持する。

### 23.2 内部状態追跡の責務

`PostEffectManager`へ以下の内部Transition型を追加した。

```text
PostEffectResourceSlot:
- Ping
- Pong

PostEffectStage:
- BeforeEffect
- AfterEffect

PostEffectResourceTransition:
- Slot
- Before State
- After State
- Stage
- Sequence
- ResourceIndex
```

この型は、PostEffect内部でどのRenderTextureを、どの順番で、どのStateへ遷移したかを表すだけであり、GPU ResourceやCommandListの所有は行わない。

### 23.3 Ping-Pong Transition Plan

`PostEffectManager::BuildResourceTransitionPlan()`を追加し、既存の実Barrier順に合わせてPlanを生成する。

既存実装では、1 Effectのみの場合は中間RTVを使わず、現在のRenderTextureをSRVとして読んでPresentColorへ直接描画する。

そのため、1 Effect時の内部Transition Planは0件である。

2 Effect以上の場合は、最後のEffect以外で中間出力先を以下の順で遷移する。

```text
BeforeEffect:
- Ping / Pong: PIXEL_SHADER_RESOURCE -> RENDER_TARGET

AfterEffect:
- Ping / Pong: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
```

Plan件数は`2 * (有効Effect数 - 1)`であり、Source側の推測Barrierは追加していない。

### 23.4 Recorder経由のBarrier発行

PostEffect内部Barrierは、`RenderBarrierRecorder`の外部Resource用 overload を経由する。

```text
PostEffectManager
  -> DirectXCore::GetRenderTextureResource(index)
  -> RenderBarrierRecorder::Transition(commandList, resource, before, after)
```

この overload は全体RenderGraphの手動Barrier記録へ混ぜないため、SceneColor / PresentColorの4遷移比較には影響しない。

実行時の記録は`PostEffectManager::resourceTransitions_`へ保持し、Planは`resourceTransitionPlan_`へ保持する。

### 23.5 Planと実行記録の比較

`PostEffectManager::CompareResourceTransitionPlanWithRecordedTransitions()`を追加した。

比較対象は以下である。

```text
- Count
- Slot
- Before State
- After State
- Stage
- Sequence
```

不足、余分な遷移、Slot不一致、Before不一致、After不一致、Stage不一致、Sequence不一致を検出できる。

`Report Render Diagnostics`実行時には、以下を要求時だけログ出力する。

```text
- PostEffect Internal Plan
- PostEffect Internal Executed
- PostEffect Internal Mismatch
```

通常フレームで大量ログは出さない。

### 23.6 全体4遷移との分離

今回の改修では、以下は変更していない。

```text
- RenderResourceId
- RenderGraphの追跡Resource
- RenderTransitionExecutorのホワイトリスト
- SceneColor / PresentColorの4遷移自動発行
- Ping-Pong Resource所有権
- SRV / RTV切替
- Effect順
- Shader / PSO / Root Signature
```

全体自動Barrier対象は引き続き以下4件のみである。

```text
SceneColor:
- PixelShaderResource -> RenderTarget
- RenderTarget -> PixelShaderResource

PresentColor:
- Present -> RenderTarget
- RenderTarget -> Present
```

### 23.7 CPUテストとビルド結果

GPU、実CommandList、Window、SwapChainを使わないCPUテストを追加した。

```text
正常系:
- 1 Effect時の内部Transition Planが0件
- 2 Effect時のPing-Pong Transition Planが2件
- 初期入力Indexが反転した場合のPing / Pong切替
- Planと記録の完全一致

異常系:
- 遷移不足
- Slot不一致
- Before State不一致
- After State不一致
- Stage不一致
- Sequence不一致
```

結果は以下である。

```text
Render validation tests: 233/233 passed
```

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 23.8 GPUで未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

ユーザー報告の直近GPU Smoke Testでは以下が確認済みである。

```text
D3D12 ERROR: 0
D3D12 CORRUPTION: 0
Device Removedなし
Transition Plan: 4
Executed Barriers: 4
mismatch: 0
```

ただし、今回追加したPostEffect内部Transition Plan / Executed / Mismatchの診断ログは、実GPU上では未確認である。

### 23.9 残リスク

内部Planは既存Barrier順に合わせているため、現在実行されていないSource側遷移は計画していない。

今後Effect数が増え、各Effectが別Resourceや別Stateを要求する場合は、PostEffectManager内部でEffect適用単位の状態追跡を拡張する必要がある。

### 23.10 次の改修候補

次に進む場合は、実GPU Smoke Testで`Report Render Diagnostics`を実行し、PostEffect Internal Plan / Executed / Mismatchが期待どおり出力されることを確認する。

## 24. PostEffectParameterSet導入

### 24.1 導入前の課題

PostEffect系の描画では、入力Texture SRVをRoot Parameterへ設定する処理が`PostEffectManager`側に直接書かれていた。

現状のGrayscale、Vignetting、Fullscreen系Root Signatureはいずれも以下の構造である。

```text
Root Parameter 0:
- SRV Descriptor Table
- t0
- Pixel Shader
```

この状態のままEffectを追加すると、Root Parameter番号、入力SRV設定、将来のConstantBuffer設定がEffectごと、またはManager側の分岐へ散らばりやすい。

### 24.2 PostEffectParameterSetの責務

PostEffect専用の軽量型として`PostEffectParameterSet`を追加した。

責務は以下に限定している。

```text
- Source Texture SRVの保持
- Effect ConstantBuffer Addressの保持
- 明示的なRoot Parameter番号の保持
- CommandListへのBind
- BindingLayoutの最小検証
```

MaterialManager、文字列ベースParameter検索、Shader Reflection、Root Parameter番号の自動推測は追加していない。

### 24.3 BindingLayoutの役割

`PostEffectBindingLayout`は、EffectごとのRoot Parameter番号を明示するための型である。

```text
sourceTextureRootParameter:
- 現行Effectでは0

constantBufferRootParameter:
- 現行Effectでは未使用
- 将来CBV付きEffectを追加する場合だけ明示
```

未使用Root Parameterは`kInvalidRootParameter`で表現し、CBVが設定されているのにRoot Parameterが未設定の場合は`Validate()`で検出する。

### 24.4 Grayscale / Vignetting / Fullscreenへの適用

Grayscale、Vignetting、FullscreenPassRendereへ`CreateBindingLayout()`を追加した。

```text
Grayscale:
- Source Texture Root Parameter = 0
- ConstantBufferなし

Vignetting:
- Source Texture Root Parameter = 0
- ConstantBufferなし

FullscreenPassRendere:
- Source Texture Root Parameter = 0
- ConstantBufferなし
```

`PostEffectManager`では、直接`SetGraphicsRootDescriptorTable(0, ...)`を呼ばず、`PostEffectParameterSet`へSRVを設定して`Bind()`する構成へ変更した。

### 24.5 変更しなかった項目

今回の改修では以下を変更していない。

```text
- Shader
- PSO
- Root Signature構造
- Descriptor Heap管理
- Descriptor取得元
- Effect適用順
- Fullscreen Draw呼び出し
- RenderGraph
- Barrier処理
- Ping-Pong切替
- PipelineRecipe / PipelineBuilder
```

PostEffect以外のObject3D、Sprite、Line、Particle、Cloud、Trailへは展開していない。

### 24.6 CPUテスト

GPU、実CommandList、実Descriptor Heapを使わないCPUテストを追加した。

```text
正常系:
- Source Texture handle設定
- ConstantBuffer address設定
- BindingLayoutのRoot Parameter番号保持
- Grayscale用Layout
- Vignetting用Layout
- Fullscreen用Layout

異常系:
- Source Texture Root Parameter未設定
- ConstantBuffer Root Parameter未設定
- Source / ConstantBufferのRoot Parameter重複
```

結果は以下である。

```text
Render validation tests: 251/251 passed
```

### 24.7 ビルド結果

Debug x64ビルド結果は以下である。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 24.8 GPUで未確認の項目

今回の確認はCPUテストとDebug x64ビルドまでである。

実GPU上では以下を確認する必要がある。

```text
- EffectなしFullscreen表示が従来どおりであること
- Grayscale / Vignettingの入力SRVがRoot Parameter 0へ正しくBindされること
- PostEffect切替時にRoot Signature不整合が出ないこと
- D3D12 Debug LayerでDescriptor / Root Parameter関連ERRORが出ないこと
```

### 24.9 次の改修候補

次に進む場合は、PostEffectに小さなConstantBuffer付きEffectを1つ追加し、`PostEffectParameterSet`のCBV経路を実GPU上で検証する。

## 25. 提出前プロジェクト構成整理

### 25.1 目的

企業提出前にVisual Studio上の視認性とプロジェクト管理情報の整合性を改善するため、物理フォルダ移動を行わずに`MagEngine.vcxproj`と`MagEngine.vcxproj.filters`を整理した。

### 25.2 vcxproj登録漏れの対応

以下の実装ファイルを`ClCompile`へ追加した。

```text
application\player\config\PlayerConfigLoader.cpp
engine\graphics\trail\TrailEffectLibrary.cpp
```

以下のヘッダーを`ClInclude`へ追加した。

```text
application\player\config\PlayerConfigLoader.h
application\player\PlayerConstants.h
application\player\ResourcePaths.h
application\player\MathUtility.h
engine\graphics\particle\ParticlePreset.h
engine\graphics\trail\TrailEffectLibrary.h
engine\math\MathConstants.h
scene\base\SceneContext.h
```

`tests\RenderValidationTests.cpp`は独立した`main`を持つため、メイン実行ファイルへ二重にコンパイルされないよう`None`としてVisual Studio上に表示する方針にした。

### 25.3 resourcesとtestsのVisual Studio表示

ビルド対象にしないリソース確認用ファイルとして、以下を`None`へ追加した。

```text
resources\shader\*.hlsl
resources\shader\*.hlsli
resources\config\**\*.json
resources\config\**\*.md
resources\levels\*.json
resources\trail\*.json
tests\RenderValidationTests.cpp
README.md
ANALYSIS_REPORT.md
MagVoiceBridge_Usage.md
```

### 25.4 filters再編方針

`MagEngine.vcxproj.filters`は実フォルダに近い粒度へ割り当て直した。

```text
application
application\player
application\player\config
application\enemy
application\ui
engine
engine\base
engine\render
engine\math
engine\graphics
engine\graphics\particle
engine\graphics
engine\graphics\cloud
engine\graphics\trail
scene
scene\base
resources
resources\shader
resources\config
resources\trail
tests
docs
```

未割当Filterは整理前86件、整理後0件である。

### 25.5 Git管理対象と.gitignoreのズレ

`.gitignore`上はローカル状態または生成物扱いだが、以下はGit追跡済みである。

```text
imgui.ini
.vscode\launch.json
.vscode\settings.json
.vscode\tasks.json
```

今回の整理では追跡解除や物理削除は行っていない。`imgui.ini`はローカル状態ファイルのため、提出前に`git rm --cached imgui.ini`の候補である。`.vscode`は共有ビルド設定として残すか、個人設定として追跡解除するかを別途判断する。

また、Debug x64ビルド確認時に`assimp-vc143-mtd.lib`が見つからないリンクエラーを確認した。実ファイルは`externals\assimp\lib\Debug`と`externals\assimp\lib\Release`に配置されていたため、`AdditionalLibraryDirectories`を実配置に合わせた。

### 25.6 物理移動を保留した理由

今回はincludeパス、Shader相対パス、Resource相対パスを壊さないことを優先し、物理フォルダ移動は行っていない。

特にルート直下の`MagVoiceBridge.cpp`と`MagVoiceBridge.h`は`MagEngine.vcxproj`へ登録済みであり、移動するとincludeパスやリンク関係の追加確認が必要になるため、今回は現状維持とした。

### 25.7 ルート直下ファイルの整理候補

```text
MagVoiceBridge.cpp / MagVoiceBridge.h:
  今回は移動しない。将来の移動先候補はapplication、engine\audio、engine\integration。

MagVoiceBridge_Usage.md:
  docs配下への移動候補。今回はVisual Studio上のdocsフィルターに表示するのみ。

ANALYSIS_REPORT.md:
  docs配下への移動候補。既存の作業記録としてルート維持。

MagEngine.filters:
  MagEngine.sln、MagEngine.vcxproj、MagEngine.vcxproj.filtersからの参照は確認できなかった。今回は削除せず、削除候補として扱う。
```

### 25.8 削除候補と判断保留

削除候補は以下である。

```text
src
x64
MagEngine.filters
```

判断保留は以下である。

```text
.agents
.codex
.vscode
imgui.ini
MagVoiceBridge.cpp
MagVoiceBridge.h
MagVoiceBridge_Usage.md
ANALYSIS_REPORT.md
```

### 25.9 ビルド / テスト結果

```text
Debug x64 build:
  Success
  Warnings: 0
  Errors: 0

Render Validation Tests:
  Render validation tests: 251/251 passed

git diff --check:
  Success
```

---

## 27. Sprite DirectionalLight CBVバインド安定化

### 27.1 不整合内容

`SpriteSetup.cpp`のRoot Signatureは`Root Parameter 3`に`DirectionalLight CBV (b1)`を定義していたが、`Sprite::Draw()`は`Root Parameter 0 / 1 / 2`のみをBindしていた。

`Sprite.PS.hlsl`は`gDirectionalLight : register(b1)`を持ち、`enableLighting != 0`の場合に参照するため、ライト有効Spriteが未Bind CBVを読む可能性があった。

### 27.2 修正したBinding契約

SpriteのRoot Parameter契約を以下として定数化した。

```text
0: Material CBV
1: Transformation CBV
2: Texture SRV
3: DirectionalLight CBV
```

`Sprite::Draw()`では`SpriteDrawBinding`を作成し、Material / Transform / Texture / DirectionalLightを検証してからBindする。`enableLighting == 0`でもShader側のb1契約を満たすため、`Root Parameter 3`には常に有効なGPU Virtual AddressをBindする。

### 27.3 DirectionalLight CBVの所有

`SpriteSetup`がSprite用のデフォルトDirectionalLight CBVを所有する。理由は、Sprite Shaderのb1契約を全Spriteで満たし、個別SpriteがライトCBVの寿命を持たないようにするため。

既定値は白色、正規化済み前提の方向、強度1.0で初期化する。将来LightManager連携を行う場合も、`SpriteSetup::SetDirectionalLight()`経由で差し替えられる。

### 27.4 UI / World Sprite分類方針

`SpriteRenderMode`を追加した。

```text
Ui
World
```

現状のSprite利用箇所はTitle、SceneTransition、GameOver/GameClear、Menu、OperationGuideなど画面座標のUI用途が中心だったため、既定値は`Ui`にした。World用途は既存見た目を壊さないよう、`Sprite::SetRenderMode(SpriteRenderMode::World)`で明示した場合のみ使用する。

### 27.5 Depth方針

UI Sprite用PSO:

```text
DepthEnable = false
DepthWriteMask = ZERO
```

UI SpriteはOverlay合成用途のため、後続のOverlay / ParticleをSceneDepthで隠さない。

World Sprite用PSO:

```text
DepthEnable = true
DepthWriteMask = ALL
DepthFunc = LESS_EQUAL
```

既存Sprite PSOのDepth方針はWorld用として維持し、将来の奥行き判定が必要なSpriteの見た目を変えない。

### 27.6 RenderPass / RenderGraph上の扱い

`SpriteRenderPass`は`SpriteRenderItem::renderMode`に応じてUI用PSO / World用PSOをBindする。

現在の登録経路はUI Spriteが中心のため、Renderer上のSprite Passの`SceneDepth` Usageは`Read`へ寄せた。RenderGraph構造やPass分割は行っていない。

### 27.7 CPUテスト

`tests\RenderValidationTests.cpp`へ以下を追加した。

```text
Sprite Root Parameter定数の契約
DirectionalLight CBVを含むDraw Binding検証
DirectionalLight CBV Address未設定検出
UI Sprite RecipeがDepth Write Allではないこと
World Sprite Recipeが既存Depth Write Allを維持すること
SpriteRenderModeがRenderWorldのSpriteRenderItemへ保持されること
```

実行結果:

```text
Render validation tests: 271/271 passed
```

CPUテスト実行時、既存Debug objを再利用したため未使用GPU初期化関数の未解決シンボル表示が出た。テスト対象のCPU契約コードは実行され、全テストは成功した。

### 27.8 ビルド結果

```text
Debug x64 build:
  Success
  Warnings: 0
  Errors: 0
```

### 27.9 GPU確認

実GPUでのHUD / LockOnHUD / enableLighting表示確認は未実施。今回の確認はDebug x64ビルド、CPU RenderValidationTests、静的検索まで。

### 27.10 次の改修候補

Spriteの実利用がUI中心である状態を維持するなら、次は`SpriteSetup::SetDirectionalLight()`をLightManager更新経路へ接続するか、World Sprite専用の登録経路をScene側へ明示する。

## 27. engine内部責務別フォルダ整理

### 27.1 目的

企業提出時に`engine`内の責務が一目で分かり、今後の機能追加時に置き場所で迷わないよう、描画基盤、描画機能、描画Resource、PostEffectを物理フォルダ単位で整理した。

### 27.2 最終ディレクトリ分類方針

```text
engine\base:
  DirectXCore、WinApp、EngineApp、MagFramework、EngineContextなどの起動・実行基盤。

engine\render:
  Renderer、Pipeline、RenderGraph、RenderPass、PostEffect、Texture/Model Resource管理。

engine\graphics:
  Sprite、Particle、Object3D、Skybox、Cloud、Trail、Lineなどの描画機能単位の実装。

engine\math:
  Vector、Matrix、Transform、数学関数、描画用構造体。

engine\input:
  Keyboard、Mouse、GamePad入力。

engine\audio:
  音声再生、音声管理。

engine\integration:
  MagVoiceBridgeなどの外部API連携。
```

### 27.3 移動した主要機能

```text
engine\render\pipeline:
  PipelineRecipe
  PipelineBuilder

engine\render\graph:
  RenderGraph
  RenderBarrierRecorder
  RenderTransitionExecutor

engine\render\pass:
  IRenderPass
  RenderWorld
  RenderContext
  CloudRenderPass
  LineRenderPass
  OpaqueRenderPass
  ParticleRenderPass
  PostEffectRenderPass
  SkyboxRenderPass
  SpriteRenderPass
  TrailRenderPass

engine\render\post_effect:
  PostEffectManager
  PostEffectParameterSet
  FullscreenPassRendere
  GrayscaleEffect
  Vignetting

engine\render\resource:
  TextureManager
  Model
  ModelManager
  ModelSetup

engine\graphics:
  Sprite
  Particle
  Object3d
  Skybox
  Cloud
  Trail
  Line
```

### 27.4 application / engine / sceneの責務境界

```text
application:
  Player、Enemy、HUD、ゲーム固有UI、作品固有の制御。

engine:
  他作品でも再利用できる描画、入力、音声、数学、Resource、外部連携。

scene:
  Scene遷移、Scene基底、SceneContext、Title / GamePlay / Debug / ClearなどのScene実装。
```

今回の整理では、`application`や`scene`へ移すべきファイルは`engine`内に見当たらなかったため、移動対象にしていない。

### 27.5 更新したinclude / vcxproj / filters

以下を移動後の物理パスへ更新した。

```text
MagEngine.vcxproj
MagEngine.vcxproj.filters
README.md
tests\RenderValidationTests.cpp
docs\ANALYSIS_REPORT.md
```

`MagEngine.vcxproj.filters`は物理フォルダに合わせて再割当し、Filter未割当が発生しない構成にした。

### 27.6 保留したファイルと理由

```text
engine\base\core\DescriptorAllocator.*
engine\base\core\DescriptorHandle.h
engine\base\core\SrvSetup.*
```

Descriptor系は`DirectXCore`との結合が強く、`engine\render\resource`へ移すと基盤層との依存境界を再確認する必要があるため今回は保留した。

```text
engine\base\imGui
engine\utils\Logger.*
engine\utils\WstringUtility.*
engine\camera
engine\light
```

Debug、Editor、Utility、Camera、Lightの分類は設計判断が分かれるため、今回は物理移動せず、次段階の整理候補として残した。

### 27.7 ビルド / テスト結果

```text
Debug x64 build:
  Success
  Warnings: 0
  Errors: 0

Render Validation Tests:
  Render validation tests: 251/251 passed

git diff --check:
  Success
```

### 27.8 今後の配置ルール

```text
Pipeline / PSO生成:
  engine\render\pipeline

RenderGraph / Barrier / Resource State:
  engine\render\graph

RenderPass / RenderWorld / RenderContext:
  engine\render\pass

Sprite / Particle / Object3D / Skybox / Cloud / Trail / Line:
  engine\graphics\<機能名>

Texture / Modelなどの描画Resource管理:
  engine\render\resource

ゲーム固有のPlayer / Enemy / HUD:
  application

Scene遷移・Scene固有進行:
  scene
```

### 25.10 次段階の提出用ZIP作成方針

次段階では元プロジェクトを直接削除・移動せず、別フォルダへ提出用コピーを作成してからZIP化する。コピー後に`.vs`、`.user`、`imgui.ini`、生成物、不要なDebug/Release系出力が混入していないことを確認する。

## 26. 提出前物理フォルダ整理

### 26.1 目的

企業提出時にプロジェクトルート直下へ用途の異なるファイルが散らばって見えないよう、Visual Studio管理情報だけでなく物理フォルダ構成も整理した。

### 26.2 移動したファイル

以下を`docs`へ移動した。

```text
ANALYSIS_REPORT.md -> docs\ANALYSIS_REPORT.md
MagVoiceBridge_Usage.md -> docs\MagVoiceBridge_Usage.md
```

以下を`engine\integration`へ移動した。

```text
MagVoiceBridge.cpp -> engine\integration\MagVoiceBridge.cpp
MagVoiceBridge.h -> engine\integration\MagVoiceBridge.h
```

`MagVoiceBridge`はWASAPIを使ったWindowsマイク入力連携であり、ゲーム個別の挙動よりも外部音声機能との接続責務が強いため、`engine\integration`へ配置した。

### 26.3 削除した不要候補

削除前に未参照または空であることを確認し、以下を削除した。

```text
MagEngine.filters
src
x64
```

`MagEngine.filters`は`MagEngine.sln`、`MagEngine.vcxproj`、`MagEngine.vcxproj.filters`、`README.md`から参照されていなかった。`src`と`x64`はファイルを含まない空フォルダだった。

### 26.4 更新したinclude / project参照

`scene\privateScene\DebugScene.h`のincludeを以下へ更新した。

```text
#include "engine/integration/MagVoiceBridge.h"
```

`MagEngine.vcxproj`と`MagEngine.vcxproj.filters`は、移動後の以下のパスを参照するよう更新した。

```text
engine\integration\MagVoiceBridge.cpp
engine\integration\MagVoiceBridge.h
docs\ANALYSIS_REPORT.md
docs\MagVoiceBridge_Usage.md
```

`docs\MagVoiceBridge_Usage.md`内の構成例とinclude例も、移動後の`engine/integration`に合わせた。

### 26.5 ルート直下を残した理由

以下はプロジェクトを開く入口または提出時の基本情報としてルート直下に残した。

```text
MagEngine.sln
MagEngine.vcxproj
MagEngine.vcxproj.filters
README.md
.gitignore
.editorconfig
.gitattributes
main.cpp
application
engine
scene
resources
tests
docs
externals
```

`imgui.ini`、`.vscode`、`.agents`、`.codex`、`.vs`は今回削除・追跡解除を行っていない。提出用ZIP作成時に除外する。

### 26.6 提出用ZIPで除外するローカル設定

提出用ZIPでは以下を除外する。

```text
.vs
.vscode
.agents
.codex
imgui.ini
MagEngine.vcxproj.user
generated
Debug
Release
x64
```

`.vscode\tasks.json`と`.vscode\launch.json`は共有候補、`.vscode\settings.json`は個人設定候補として扱う。

### 26.7 ビルドとテスト結果

```text
Debug x64 build:
  Success
  Warnings: 0
  Errors: 0

Render Validation Tests:
  Render validation tests: 251/251 passed

git diff --check:
  Success
```

## 27. Line HUD / LockOnHUD 復旧

### 27.1 消えた原因

Line描画が`LineRenderPass`へ移行した後、HUD / LockOnHUD / Debug Lineの描画が`RenderWorld`に正しく再登録されず、さらにHUD系Lineの描画順がSceneColorの再利用境界より後ろへ回ると、描画バッファが空のまま消える。

今回の症状は、`SceneColor`を`PixelShaderResource`へ戻す境界と、HUD / LockOnHUDがLineを積む順序が噛み合っていなかったことが原因である。

### 27.2 復旧した描画経路

```text
Line生成
↓
LineManagerへ蓄積
↓
RenderWorldへLineRenderItem登録
↓
LineRenderPass::Execute()
↓
LineManager::Draw()
↓
画面表示
```

`MagFramework::OpaqueRender()`でWorld / HUD両方の`LineRenderItem`を登録し、`LineRenderPass`は`LineRenderMode`に応じて描画対象を切り替える。

### 27.3 World / HUD LineのDepth方針

```text
World Line:
- 既存Depth設定を維持
- 既存のワールド表示を壊さない

HUD Line:
- DepthEnable = false
- DepthWriteMask = ZERO
- HUD / LockOnHUDを画面前面で安定表示する
```

### 27.4 LockOn HUD構成

LockOnHUDはLineだけで構成し、四隅ブラケット、中心へ向かう短いガイド線、上下の補助線でターゲットを追従表示する。

ターゲットなしでは何も登録しない。ターゲット破棄時は安全に解除し、画面外では無効座標を描画しない。

### 27.5 RenderGraph / Barrier扱い

HUD Lineが`SceneColor`を使った後で`PixelShaderResource`へ戻すため、`RenderTexturePostDraw`の境界を`175`へ揃えた。

これに合わせて、`RenderGraph`、`RenderBarrierRecorder`、CPUテストの期待値を同じ値へ更新した。

### 27.6 CPUテスト

`RenderValidationTests.cpp`へ以下を追加した。

```text
- HUD LineがDepth Writeしないこと
- World Lineが既存Depth方針を維持すること
- LockOn対象ありで必要Line群が生成されること
- LockOn対象なしでLine群が生成されないこと
- 画面外座標で不正Lineを生成しないこと
- Scene遷移相当のClear後にLineが残らないこと
```

結果は以下である。

```text
Render validation tests: 285/285 passed
```

### 27.7 ビルド結果

Debug x64ビルドは成功した。

```text
Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 27.8 GPU確認状況

実GPU上の確認は未実施である。

```text
- HUD Lineが表示されること
- LockOnブラケットがターゲットへ追従すること
- Scene遷移後に古いHUD Lineが残らないこと
- HUD LineがSkybox / Cloudに隠れないこと
- D3D12 ERROR / CORRUPTIONが0件であること
- Device Removedが発生しないこと
```

### 27.9 次の改修候補

次は、LockOnHUDに画面外端インジケータを追加すると、ロックオン状態の視認性をもう一段上げられる。

## 28. Line Style / HUD Quad Line

### 28.1 導入前の制約

Line描画はWorld / HUDのBatch分離後も、頂点形式とPrimitiveTopologyはどちらもLINELISTを前提としていた。
そのためHUD用途ではLine幅がハードウェアやドライバ依存になりやすく、LockOnHUDや航空機HUD風の太線・破線表現を安定して扱いにくかった。

### 28.2 World / HUD Lineの責務分離

World Line:
- 既存のLINELIST経路を維持
- 既存Depth設定を維持
- Debug Lineやワールド補助線の見た目を変えない

HUD Line:
- HUD専用PSOをTRIANGLELISTへ変更
- DepthEnable = false
- DepthWriteMask = ZERO
- 太線はLine SegmentをQuadへ展開して描画

### 28.3 HUD Quad方式を採用した理由

HUD Lineは画面前面で安定表示する必要があるため、ハードウェアLine幅に依存しないQuad方式へ寄せた。
1本ごとのDrawは行わず、既存のLineバッチと永続Map方式を維持し、HUD Line頂点をTRIANGLELISTとしてまとめて転送する。

### 28.4 LineStyle項目

`LineStyle`を追加した。

- `mode`
- `color`
- `thickness`
- `dashed`
- `dashLength`
- `gapLength`

`thickness`は最小値へClampし、Alphaは0.0から1.0へClampする。
不正な破線指定は大量分割や無限ループを避けるため、通常線へフォールバックする。

### 28.5 追加API

`LineManager`へStyle指定APIを追加した。

- `DrawLine(start, end, LineStyle)`
- `AddLine(Vector3, Vector3, Vector4)`
- `AddLine(Vector2, Vector2, LineStyle)`
- `AddPolyline(...)`
- `AddRect(...)`
- `AddCornerBracket(...)`
- `AddArrow(...)`

既存の`DrawLine(Vector3, Vector3, Vector4, float)`は互換維持のため残している。

### 28.6 バッチ構成

World Line Batch:
- 既存`Line`インスタンス
- LINELIST
- World PSO

HUD Line Batch:
- HUD用`Line`インスタンス
- TRIANGLELIST
- HUD PSO
- Line Segmentを6頂点Quadへ展開

### 28.7 頂点上限時の扱い

既存の上限100000頂点を維持した。
HUD Quadは6頂点単位のため、上限超過時は途中まで追加せず、そのLine Segment単位で破棄する。

### 28.8 LockOnHUDへの適用

LockOnHUDのブラケットと中心十字をStyle指定APIへ移行した。
敵選択、ロックオン判定、カメラ計算、画面外判定ロジックは変更していない。

### 28.9 検証

CPUテスト:
- RenderValidationTests: 300/300 passed

Debug x64ビルド:
- 成功
- 警告0
- エラー0

GPU確認:
- この作業内では未実施
- 実機では太さ、Alpha、破線、LockOnブラケット、World Debug Lineの表示確認が必要

### 28.10 次の改修候補

HUD Lineの座標系を画面ピクセル基準へ整理すると、カメラ距離やFOV変化に対して太さをさらに安定させられる。

---

## 29. Cloud弾痕 / 雲穴 SDF Shape拡張

### 29.1 目的

Cloudの弾痕穴をCircle / Box / Diamond / Starの固定選択から、Inigo Quilez系の2D distance function形状をShape IDで選択できる構造へ拡張した。

性能上の制約として、各穴は`CloudHoleShape`で指定された1種類のSDFだけを評価する。全Shapeを順番に評価して`min`を取る方式は採用していない。

### 29.2 Shape一覧とカテゴリ

Basic:
- Circle
- RoundedBox
- ChamferBox
- Box
- OrientedBox
- Segment
- Rhombus
- Trapezoid
- Parallelogram

Polygon:
- EquilateralTriangle
- IsoscelesTriangle
- Triangle
- UnevenCapsule
- Pentagon
- Hexagon
- Octagon
- Hexagram
- Pentagram
- RegularStar

Circular:
- Pie
- CutDisk
- Arc
- Ring
- Horseshoe
- Vesica
- OrientedVesica
- Moon

Organic:
- RoundedCross
- Egg
- Heart
- Cross
- RoundedX
- Polygon
- Ellipse

Curve:
- Parabola
- ParabolaSegment
- QuadraticBezier
- BlobbyCross
- Tunnel
- Stairs
- QuadraticCircle
- Hyperbola
- CoolS
- CircleWave

Experimental:
- QuadraticBezier
- Hyperbola
- CoolS
- CircleWave
- Tunnel
- Stairs
- BlobbyCross
- Polygon

実装Shape総数は44である。`Count`は穴形状として使用しない。

### 29.3 CloudHoleData拡張

`engine/graphics/cloud/CloudHoleTypes.h`を追加し、以下を集約した。

- `CloudHoleShape`
- `CloudHoleShapeCategory`
- `CloudHoleFlags`
- `CloudHoleData`
- Shape名変換
- Category分類
- Debug用Shape循環
- Shape別Preset
- `SanitizeCloudHoleData`

`CloudHoleData`はShapeごとの個別構造体を増やさず、`shapeParams0`、`shapeParams1`、`flags`、`polygonPointCount`で共通表現する。

CircleのPresetは`aspectRatio = 1.0f`、modifierなし、追加パラメータなしを維持し、既存の円形穴の既定挙動を変えない。

### 29.4 SDF include分離

`resources/shader/CloudHoleSdf.hlsli`を追加し、SDF本体を`Cloud.hlsli`から分離した。

構成:
- 共通関数: `Dot2`、`Rotate2D`、`SafeAspectRatio`、`TransformHoleLocalPosition`
- Basic SDF
- Polygon SDF
- Circular SDF
- Organic SDF
- Curve SDF
- Modifier: Round / Onion
- `EvaluateCloudHoleSdf`

`resources/shader/CloudBulletHole.hlsli`は穴ごとの軸方向判定、局所座標変換、選択Shapeの評価、既存Cloud密度マスクへの合成だけを担当する。

### 29.5 Shape別Preset

主なPreset:
- RoundedBox: corner radius 0.2
- Trapezoid: top ratio 0.6
- RegularStar / Pentagram: point count 5、inner ratio 0.45
- Ring: thickness 0.2
- Moon: offset 0.45
- Cross: arm width 0.28
- Ellipse: aspect ratio 1.5
- Parabola: curvature 1.0
- Stairs: steps 5
- CircleWave: wave amount 0.5、frequency 6

Presetは`SanitizeCloudHoleData`を通して、安全な半径、寿命、aspectRatio、polygonPointCountへ補正する。

### 29.6 Round / Onion Modifier

`CloudHoleFlags`でRoundとOnionを任意適用できるようにした。

- Round: `d -= roundRadius`
- Onion: `d = abs(d) - thickness`

modifier未使用時は分岐に入らず、Circleの既定挙動には影響しない。

### 29.7 Debug操作

DebugSceneのCloud Hole操作を以下へ更新した。

- `J`: 選択Shapeをカメラ前方基準で生成
- `K`: 選択Shapeをランダム位置に生成
- `L`: 全雲穴クリア
- `N`: 次のShape
- `M`: 前のShape
- `B`: 次カテゴリ
- `V`: 前カテゴリ
- `R`: Round modifier切替
- `O`: Onion modifier切替

Debug UIには現在のShape、Category、Round / Onion状態、Rotation、Aspect Ratio、Experimental警告を表示する。

### 29.8 性能制約

維持した制約:
- Cloud RaymarchのMAX_STEPSは変更なし
- LightMarch回数は変更なし
- Noise / FBM構造は変更なし
- Cloud Lightingは変更なし
- RenderGraph / Barrierは変更なし
- Cloud PSOは変更なし
- Texture参照の追加なし
- GPU Resource追加なし
- 各穴で評価するSDFはShape IDに対応する1種類のみ

通常ランダム候補とExperimental候補はCPU側分類で分離できる状態にした。

### 29.9 CPUテスト

`tests/RenderValidationTests.cpp`へCloudHoleTypesのCPUテストを追加した。

検証内容:
- 全Shape IDの有効性
- `Count`をShapeとして扱わないこと
- Category分類
- Circle Presetの既定値
- 各Shape Presetの安全範囲
- Shape / Category循環
- Round / Onion flagsのmask
- aspectRatio clamp
- Polygon辺数clamp
- Star point count安全範囲
- Ring thickness安全範囲
- NaN / Infinity補正

実行結果:
- 今回の作業内では未完了
- 理由: `RenderValidationTests.cpp`は独立`main`を持つ`None`管理ファイルであり、一時exe化時に`DirectXCore`、`DirectXTex`、WinAPI、FrameContextまでリンク依存が広がったため
- Debug / Releaseの本体ビルドではコンパイル対象外である

### 29.10 ビルド / Shader確認

DXC単体確認:
- `Cloud.VS.hlsl`: 成功
- `Cloud.PS.hlsl`: 成功

Debug x64ビルド:
- 成功
- 警告0
- エラー0

Release x64ビルド:
- 成功
- 警告0
- エラー0

### 29.11 GPU確認

今回の作業内では実GPUでカテゴリ別Shape表示確認は未実施。

実機で確認すべき項目:
- Basic: Circle / RoundedBox / Rhombus
- Polygon: Hexagon / Pentagram / RegularStar
- Circular: Ring / Moon / Horseshoe
- Organic: Heart / RoundedX / Egg
- Curve: Parabola / QuadraticBezier / CircleWave
- 回転、aspectRatio、Round、Onionの反映
- 複数穴での破綻なし
- D3D12 ERROR: 0
- D3D12 CORRUPTION: 0
- Device Removedなし

## 30. Enemy Group Formation 改修

### 30.1 目的

敵個体ごとのランダム性が高い移動を、EnemyGroup主導の「進入 / 戦闘軌道 / 離脱」へ寄せた。

理由:
- 3Dシューティングでは、敵が単独でばらばらに動くより、編隊単位の軌道のほうがプレイヤーが動きを読める
- 新しい敵種別を増やす前に、既存Enemy / EnemyGunnerが共通の編隊目標へ追従できる土台が必要
- EnemyGroupがEnemy / EnemyGunnerへ`dynamic_cast`して個別APIを呼ぶ構造は、敵追加時の分岐増加につながる

### 30.2 EnemyBase共通Formation API

`EnemyBase`へ以下の共通APIを追加した。

- `SetFormationTarget`
- `ClearFormationTarget`
- `SetFormationFollowEnabled`
- `IsFormationFollowEnabled`
- `SetFormationSlotIndex`
- `GetFormationSlotIndex`
- `SetFormationFollowSpeed`
- `SetFormationAttackEnabled`
- `IsFormationAttackEnabled`
- `UpdateFormationFollow`

EnemyGroupは敵を所有しないため、`EnemyBase*`は非所有参照として扱い、寿命管理や削除はEnemyManager側の`unique_ptr`に残している。

### 30.3 EnemyGroupState

追加した状態:
- `Enter`: 視界外または視界端から戦闘中心へ進入
- `Combat`: 編隊パターンに応じた軌道を維持
- `Exit`: 戦闘中心から視界外または奥方向へ離脱
- `Finished`: Group制御終了

### 30.4 EnemyFormationPattern

追加したパターン:
- `HorizontalLine`: X方向の横一列
- `VShape`: slot 0を先頭に左右後方へ展開
- `Circle`: Group中心を基準に楕円円運動
- `FigureEight`: sin / sin(2t) による八の字
- `Column`: Z方向に時間差を持つ縦列

既存`FormationType`は設定互換のため残し、EnemyGroup内部で新Patternへ変換する。

### 30.5 軌道式

主な式:
- HorizontalLine: `center + (slotOffsetX, 0, 0)`
- VShape: `center + (side * spacing * pairIndex, -spacing * 0.35 * pairIndex, -spacing * 0.8 * pairIndex)`
- Circle: `x = cos(angle) * orbitRadiusX`, `y = sin(angle) * orbitRadiusY`
- FigureEight: `x = sin(angle) * orbitRadiusX`, `y = sin(angle * 2) * orbitRadiusY`
- Column: `center + (0, verticalOffset, -slotIndex * spacing)`

三角関数の式はEnemyGroupへ集約し、Enemy / EnemyGunnerへ重複させていない。

### 30.6 進入 / 戦闘 / 離脱

EnemyGroupが`EnemyGroupMotion`を持ち、以下を管理する。

- `entryPosition`
- `combatCenter`
- `exitPosition`
- `elapsedTime`
- `phaseTime`
- `orbitRadiusX`
- `orbitRadiusY`
- `orbitAngularSpeed`

Enter / Exitは線形補間でGroup中心を移動し、Combat中はPatternごとのslot offsetをGroup中心に加算する。

### 30.7 攻撃パターン

`EnemyGroupAttackPattern`を追加した。

- `None`
- `Staggered`
- `LeaderThenWing`
- `Alternating`

今回の既定値は`Staggered`。Groupはslotごとの攻撃許可タイミングだけを設定し、実際に撃てるかどうかはEnemyGunner側の射撃間隔で判断する。

### 30.8 dynamic_cast削減

EnemyGroup内の`Enemy` / `EnemyGunner`向け`dynamic_cast`を削除した。

Groupから派生型固有APIを呼ばず、`EnemyBase`のFormation APIだけで以下を制御する。
- slot index
- formation target
- follow enabled
- follow speed
- attack enabled

### 30.9 設定値

新しい数値は`EnemyGroup.h`内の`EnemyFormationConstants`へ集約した。

- `kEntryDuration`
- `kCombatDuration`
- `kExitDuration`
- `kFormationFollowSpeed`
- `kOrbitRadiusX`
- `kOrbitRadiusY`
- `kOrbitAngularSpeed`
- `kFormationSpacing`
- `kAttackInterval`
- `kAttackSlotDelay`
- `kAttackWindow`
- `kMaxFormationJitter`
- `kCombatForwardDistance`

今後JSON側のFormation設定が十分整理できた段階で、既存`FormationConfig` / `WaveParamConfig`へ移す余地を残している。

### 30.10 CPUテスト

`tests/RenderValidationTests.cpp`へGPU不要のFormation計算テストを追加した。

検証内容:
- HorizontalLineでslotが左右に分かれる
- VShapeでslot 0が先頭、左右が後方へ展開する
- Circleでslotごとの位相が分かれる
- FigureEightで時間経過により座標が変化する
- ColumnでZ方向に分かれる
- 空Group更新が安全にFinishedへ落ちる
- Staggeredでslotごとの攻撃タイミングがずれる
- 不正Pattern値でも有限値を返す

### 30.11 ビルド結果

Debug x64:
- 成功
- 警告0
- エラー0

Release x64:
- 成功
- 警告0
- エラー0

### 30.12 GPU確認

今回の作業内では実GPU確認は未実施。

実機で確認すべき項目:
- 横一列、V字、円、八の字が視界内で識別できる
- EnterからCombatへ自然に進入する
- Combat後にExitへ自然に離脱する
- Gunnerが編隊中でも攻撃する
- 敵全員が同一フレームで射撃しない
- 敵死亡時にGroupが安全に縮退する
- Scene遷移後に古いGroup参照が残らない
- D3D12 ERROR: 0
- D3D12 CORRUPTION: 0
- Device Removedなし

## 31. EnemyGroup編隊スポーン / 移動調整

### 31.1 目的

ゲームシーン上で発生する編隊について、出現数、進入距離、Combat中の狙いやすさ、移動の滑らかさを改善した。

既存の`EnemyGroup` / Formation API / 敵弾 / 衝突 / 死亡処理は作り直さず、Group中心とEnemy個体追従、スポーンスケジューラ、診断表示だけを調整した。

### 31.2 カクカク移動の原因と改善方式

原因:
- Enter / Exitの目標位置が線形補間で切り替わり、Group中心が急に別の目標へ向かっていた
- Enemy個体のFormation追従が固定係数寄りで、FPS差や目標更新時の見え方が安定しにくかった

改善:
- `EnemyGroup::SmoothPosition`で指数補間を追加した
- `EnemyBase::UpdateFormationFollow`で追従速度を保持し、`deltaTime`に依存する指数補間で速度を馴染ませるようにした

理由:
- 固定割合補間はフレームレートで見え方が変わるため、`1 - exp(-sharpness * deltaTime)`で追従量を時間基準へ正規化する

### 31.3 プレイヤー基準Combat Anchor

Combat中の中心を固定ワールド座標ではなく、プレイヤー現在位置から以下の基準で作るようにした。

```text
Combat Anchor
= Player Position
+ Z正面方向のCombat Distance
+ Patternごとの小さなXY Bias
```

このAnchorをさらに指数補間で追従することで、プレイヤーがXY移動しても敵が画面中央に取り残されにくく、かつプレイヤーへ完全追従して単調になりすぎないようにした。

### 31.4 Combat Area

`EnemyCombatArea`を追加し、Combat中のGroup中心をプレイヤー基準の射撃可能領域内へClampするようにした。

既定値:
- halfWidth: 30.0
- halfHeight: 14.0
- combatDistance: 72.0
- edgePadding: 4.0

円軌道 / 八の字軌道の半径もCombat Area内へ収まるようにClampしている。

### 31.5 視界外Spawn Margin / Entry Distance制約

`EnemyFormationSpawnBounds`と`EnemyManager::CalculateFormationEntryPosition`を追加し、編隊のentryPositionをプレイヤー基準の左右 / 上下視界外へ広げた。

既定値:
- offscreenMarginX: 90.0
- offscreenMarginY: 28.0
- entryLeadDistance: 18.0
- minimumSpawnDistance: 120.0

entryPositionとcombatCenter相当位置の距離が短すぎる場合は、Combat側から外方向へ伸ばして最低距離を維持する。

### 31.6 複数Group Spawn Scheduler

`EnemyManager`に最小限のSpawn Schedulerを追加した。

既定値:
- 最大同時Group: 4
- 最大Formation敵数: 22
- Spawn間隔: 3.8秒
- Pattern順: HorizontalLine -> VShape -> Circle -> FigureEight
- Circle / FigureEightは3機、それ以外は5機

毎フレーム生成は行わず、Group数と敵数の上限に達している場合は追加生成しない。

### 31.7 Patternごとの範囲制約

HorizontalLine:
- Combat Area幅からspacingをClamp

VShape:
- wingの横幅をCombat Area幅内に収める

Circle:
- radiusX / radiusYをCombat Area内へClamp

FigureEight:
- radiusX / radiusYをCombat Area内へClamp

Column:
- XYは中央寄り、Z方向の時間差表現を維持

### 31.8 CPUテスト

`tests/RenderValidationTests.cpp`へGPU不要の計算テストを追加した。

追加検証:
- `deltaTime == 0`で指数補間が動かない
- 小さい`deltaTime`と大きい`deltaTime`で目標へ安定して近づく
- Combat Area外の座標がプレイヤー基準範囲へClampされる
- Combat Distanceが維持される
- Circle / FigureEightのslot offsetがCombat Area内に収まる

実行結果:
- 今回の作業内では未実行
- 理由: `RenderValidationTests.cpp`はVisual Studio上で`None`管理の独立mainであり、本体`MagEngine.vcxproj`のビルド対象外であるため

### 31.9 ビルド結果

Debug x64:
- 成功
- 警告0
- エラー0

Release x64:
- 成功
- 警告0
- エラー0
- 初回は生成物`tlog`への書き込み権限で失敗したが、権限付き再実行で成功

`git diff --check`:
- 成功
- 空白エラーなし
- LF/CRLF警告のみ

### 31.10 GPU確認

Debug x64を30秒起動した。

結果:
- 30秒間プロセス継続
- 起動中のDevice Removedは確認されず
- こちらで終了

未確認:
- Hidden起動のため、複数Groupの見た目、進入距離、射撃可能領域への追従、円 / 八の字の収まりは目視未確認
- D3D12 ERROR / CORRUPTIONは標準出力上では確認できず

## 32. Enemy / Wave / GameClear 新進行システム移行

### 32.1 旧敵進行構造の問題

旧構造では`EnemyManager`が敵所有、旧Wave進行、固定Formation Spawn Scheduler、クリア判定を同時に持っていた。

問題:
- `EnemyManager`内の旧Wave完了条件は敵数だけを見ており、EnemyGroupのExit完了と統合されていなかった
- GamePlayScene側に初期編隊SpawnとDebug Spawnが残り、WaveController相当の責務と競合していた
- `resources/config/enemy/waves.json`、`WaveParamConfig`、`EnemyManager`内の固定Spawn値で設定が分散していた

### 32.2 新しいStage / Wave / Group / Enemy責務

追加:
- `StageDefinition`
- `WaveDefinition`
- `SpawnGroupDefinition`
- `EnemySpawnDefinition`
- `EnemyFormationMotionDefinition`
- `WaveController`
- `GameFlowController`
- `EnemyFactory`

責務:
- `GamePlayScene`: `GameFlowController`を初期化 / 更新し、UI演出へ接続する
- `GameFlowController`: Stage完了、ClearPending、Cleared、GameOverを管理する
- `WaveController`: SpawnGroupのSpawn、Wave完了、次Wave開始を管理する
- `EnemyManager`: Enemyの生成、所有、更新、削除、Active数提供だけを担当する
- `EnemyGroup`: 編隊位置、Enter / Combat / Exit、終了理由を管理する

### 32.3 GameClear不能の原因

旧クリア判定は`EnemyManager::IsGameClear()`に依存していた。

直接原因:
- Wave完了が旧Waveのスポーン / 撃破数に依存していた
- EnemyGroupがExit完了しても、Wave完了条件へ明示的に反映されなかった
- EnemyManager側の固定Spawn SchedulerがWave完了後も新しい敵を発生させ得た

### 32.4 新しいClear条件

`GameFlowController`だけがGameClearを判断する。

条件:
- StageDefinition内の全WaveがCompleted
- `EnemyManager::GetActiveEnemyCount() == 0`
- `WaveController::GetActiveGroupCount() == 0`
- `clearDelaySeconds`経過

この条件を満たすと`GameFlowState::Cleared`へ遷移し、GamePlaySceneが既存のGameClear演出を開始する。

### 32.5 設定値集約方針

新しいStage設定:
- `resources/config/stage/stage_01.json`

集約した値:
- Wave順序
- SpawnGroup定義
- Enemy Archetype / count
- FormationPattern
- AttackPattern
- entry / combat / exit duration
- combatDistance
- combatAreaHalfWidth / combatAreaHalfHeight
- formationSpacing
- orbitRadiusX / orbitRadiusY
- orbitAngularSpeed
- attackInterval / attackSlotDelay
- offscreenMargin
- minimumSpawnDistance

`EnemyFormationConstants`は新Stage設定の既定値として残し、実行時の主要設定はStage JSONから渡す。

### 32.6 EnemyFactory導入

`EnemyFactory::Create`を追加した。

生成経路:
```text
WaveController
  -> EnemyManager::CreateEnemy
  -> EnemyFactory::Create
  -> Enemy / EnemyGunner
  -> EnemyManagerがunique_ptrで所有
```

旧`SpawnEnemy()`、`SpawnGunner()`、`SpawnFormationForGameplay()`は削除した。

### 32.7 EnemyGroup終了理由

追加:
- `EnemyGroupFinishReason::None`
- `EnemyGroupFinishReason::AllMembersDestroyed`
- `EnemyGroupFinishReason::AllMembersExited`
- `EnemyGroupFinishReason::MixedDestroyedAndExited`
- `EnemyGroupFinishReason::Cancelled`

Exit完了時は生存メンバーを`EnemyBase::MarkExited()`で非アクティブ化し、EnemyManagerのActive Enemy数から除外する。

### 32.8 所有権と削除順

所有権:
- EnemyManagerが`std::unique_ptr<EnemyBase>`でEnemyを唯一所有する
- WaveControllerが`std::unique_ptr<EnemyGroup>`でGroupを所有する
- EnemyGroup内の`EnemyBase*`は非所有参照

Scene終了時:
```text
GameFlowController::Clear()
EnemyManager::Clear()
```

この順序により、Group側の非所有参照を先に破棄してからEnemy所有を解放する。

### 32.9 旧経路削除内容

削除 / 切断:
- `EnemyManager`内の旧Wave進行
- `EnemyManager`内の固定Formation Spawn Scheduler
- `EnemyManager::SpawnEnemy`
- `EnemyManager::SpawnGunner`
- `EnemyManager::SpawnFormationForGameplay`
- `EnemyManager::IsGameClear`
- GamePlaySceneの`UpdateFormationSpawnDebug`
- DebugキーによるF9-F12直接編隊Spawn
- Debugキー`C`による直接GameClear
- `WaveParamConfig.*`
- `resources/config/enemy/waves.json`

旧経路検索:
- `SpawnEnemy(`
- `SpawnGunner(`
- `SpawnFormationForGameplay`
- `formationSpawnInterval`
- `maxFormationEnemyCount`
- `IsGameClear(`
- `UpdateWave(`
- `WaveConfig`
- `WavePhase`
- `GenerateSpawnPosition`
- `UpdateFormationSpawnDebug`

結果:
- 実行経路上の該当なし

### 32.10 CPUテスト結果

独立CPUチェック:
- `resources/config/stage/stage_01.json`を`ConvertFrom-Json`で読み込み
- Wave数
- Wave順序
- SpawnGroup存在
- Member存在
- combatDistance正値
- minimumSpawnDistanceの下限

結果:
- `stage_01.json OK`
- `EnemyStage CPU JSON tests passed`

未実施:
- Window / GPU不要のC++実行テスト

### 32.11 ビルド結果

Debug x64:
- 成功
- 警告0
- エラー0

Release x64:
- 成功
- 警告0
- エラー0

`git diff --check`:
- 成功
- 空白エラーなし
- LF / CRLF警告のみ

### 32.12 GPU確認結果

Debug x64を30秒起動した。

結果:
- 30秒間プロセス継続
- こちらで終了
- 起動時クラッシュなし
- Device Removedは確認されず

未確認:
- Hidden起動のため、Wave 1出現、敵撃破によるGroup更新、最終Wave後のGameClear遷移は目視未確認
- D3D12 ERROR / CORRUPTIONは標準出力上では確認できず
