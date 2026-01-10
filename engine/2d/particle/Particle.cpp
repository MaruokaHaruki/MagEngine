#include "Particle.h"
#include "Camera.h"
#include "TextureManager.h"
//---------------------------------------
// 数学関数　
#include "MagMath.h"
#include <cmath>
#include <numbers>

///=============================================================================
///						初期化処理
void Particle::Initialize(ParticleSetup *particleSetup) {
	//========================================
	// 引数からSetupを受け取る
	this->particleSetup_ = particleSetup;
	// RandomEngineの初期化
	randomEngine_.seed(std::random_device()());
	//========================================
	// 頂点データの作成
	CreateVertexData();
	// 頂点バッファビューの作成
	CreateVertexBufferView();
	//========================================
	// 書き込むためのアドレスを取得
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
	// 頂点データをリソースにコピー
	std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(MagMath::VertexData) * modelData_.vertices.size());
}

///=============================================================================
///						更新処理
void Particle::Update() {
	//========================================
	// カメラの取得
	Camera *camera = particleSetup_->GetDefaultCamera();
	// カメラ行列の取得
	MagMath::Matrix4x4 cameraMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f},
											  camera->GetRotate(), camera->GetTranslate());
	// ビュー行列の取得
	MagMath::Matrix4x4 viewMatrix = Inverse4x4(cameraMatrix);
	// プロジェクション行列の取得
	MagMath::Matrix4x4 projectionMatrix = MagMath::MakePerspectiveFovMatrix(0.45f,
														  float(particleSetup_->GetDXManager()->GetWinApp().kWindowWidth_) / float(particleSetup_->GetDXManager()->GetWinApp().kWindowHeight_),
														  0.1f, 100.0f);
	// ビュープロジェクション行列の取得
	MagMath::Matrix4x4 viewProjectionMatrix = Multiply4x4(viewMatrix, projectionMatrix);

	//========================================
	// ビルボード行列の取得
	MagMath::Matrix4x4 backToFrontMatrix = MagMath::MakeRotateYMatrix(std::numbers::pi_v<float>);
	MagMath::Matrix4x4 billboardMatrix{};
	if (isUsedBillboard) {
		billboardMatrix = Multiply4x4(backToFrontMatrix, cameraMatrix);
		// 平行移動成分は無視
		billboardMatrix.m[3][0] = 0.0f;
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;
	} else {
		billboardMatrix = MagMath::Identity4x4();
	}

	// スケール調整用の倍率を設定
	// constexpr float scaleMultiplier = 0.01f; // 必要に応じて調整

	//========================================
	// パーティクルの更新
	for (auto &group : particleGroups) {
		// テクスチャサイズの取得
		MagMath::Vector2 textureSize = group.second.textureSize;
		// インスタンス数の初期化
		for (auto it = group.second.particleList.begin(); it != group.second.particleList.end();) {
			// パーティクルの参照
			ParticleStr &particle = *it;
			// パーティクルの寿命が尽きた場合は削除
			if (particle.lifeTime <= particle.currentTime) {
				it = group.second.particleList.erase(it);
				continue;
			}

			// 経過時間に対する割合
			float timeRatio = particle.currentTime / particle.lifeTime;

			// スケールの線形補間
			particle.transform.scale.x = std::lerp(particle.initialScale.x, particle.endScale.x, timeRatio);
			particle.transform.scale.y = std::lerp(particle.initialScale.y, particle.endScale.y, timeRatio);
			particle.transform.scale.z = std::lerp(particle.initialScale.z, particle.endScale.z, timeRatio);

			// 回転の線形補間
			particle.transform.rotate.x = std::lerp(particle.initialRotation.x, particle.endRotation.x, timeRatio);
			particle.transform.rotate.y = std::lerp(particle.initialRotation.y, particle.endRotation.y, timeRatio);
			particle.transform.rotate.z = std::lerp(particle.initialRotation.z, particle.endRotation.z, timeRatio);

			// 重力の適用
			particle.velocity = AddVec3(particle.velocity, MultiplyVec3(kDeltaTime, gravity_));

			// 位置の更新
			particle.transform.translate = AddVec3(particle.transform.translate, MultiplyVec3(kDeltaTime, particle.velocity));
			// 経過時間を更新
			particle.currentTime += kDeltaTime;
			// ワールド行列の計算a
			MagMath::Matrix4x4 worldMatrix = Multiply4x4(
				billboardMatrix,
				MakeAffineMatrix(particle.transform.scale, particle.transform.rotate, particle.transform.translate));
			// ビュー・プロジェクションを掛け合わせて最終行列を計算
			MagMath::Matrix4x4 worldviewProjectionMatrix = Multiply4x4(worldMatrix, viewProjectionMatrix);
			//---------------------------------------
			// インスタンシングデータの設定
			if (group.second.instanceCount < kNumMaxInstance) {
				group.second.instancingDataPtr[group.second.instanceCount].WVP = worldviewProjectionMatrix;
				group.second.instancingDataPtr[group.second.instanceCount].World = worldMatrix;
				// カラーを設定し、フェードイン・アウトを適用
				group.second.instancingDataPtr[group.second.instanceCount].color = particle.color;

				// フェードイン・アウトの計算
				float alpha = 1.0f;
				if (timeRatio < fadeInRatio_) {
					// フェードイン
					alpha = timeRatio / fadeInRatio_;
				} else if (timeRatio > fadeOutRatio_) {
					// フェードアウト
					alpha = 1.0f - ((timeRatio - fadeOutRatio_) / (1.0f - fadeOutRatio_));
				}
				alpha = std::clamp(alpha, 0.0f, 1.0f);

				group.second.instancingDataPtr[group.second.instanceCount].color.w = particle.color.w * alpha;
				// インスタンス数を増やす
				++group.second.instanceCount;
			}
			// 次のパーティクルへ
			++it;
		}
	}
}

///=============================================================================
///						描画
void Particle::Draw() {
	// コマンドリストの取得
	ID3D12GraphicsCommandList *commandList = particleSetup_->GetDXManager()->GetCommandList().Get();

	// 共通の描画設定を適用
	particleSetup_->CommonDrawSetup();

	// 頂点バッファをセット (ループの外で一度だけ)
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// 全てのパーティクルグループを処理
	for (auto &groupPair : particleGroups) {
		ParticleGroup &group = groupPair.second; // グループへの参照を取得
		if (group.instanceCount == 0)
			continue; // インスタンスが無い場合はスキップ

		// マテリアルCBufferの場所を設定
		commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());

		// テクスチャのSRVのDescriptorTableを設定
		commandList->SetGraphicsRootDescriptorTable(2, particleSetup_->GetSrvSetup()->GetSRVGPUDescriptorHandle(group.srvIndex));

		// インスタンシングデータのSRVのDescriptorTableを設定
		commandList->SetGraphicsRootDescriptorTable(1, particleSetup_->GetSrvSetup()->GetSRVGPUDescriptorHandle(group.instancingSrvIndex));

		// Draw Call (インスタンシング描画) - グループごとの頂点数とオフセットを使用
		commandList->DrawInstanced(group.vertexCount, group.instanceCount, group.vertexOffset, 0);

		// インスタンスカウントをリセット
		group.instanceCount = 0;
	}
}

///=============================================================================
///						エミッター
void Particle::Emit(const std::string name, const MagMath::Vector3 &position, uint32_t count) {
	if (particleGroups.find(name) == particleGroups.end()) {
		// パーティクルグループが存在しない場合はエラーを出力して終了
		assert("Specified particle group does not exist!");
	}

	// 指定されたパーティクルグループが存在する場合、そのグループにパーティクルを追加
	ParticleGroup &group = particleGroups[name];

	// すでにkNumMaxInstanceに達している場合、新しいパーティクルの追加をスキップする
	if (group.particleList.size() >= count) {
		return;
	}

	// 指定された数のパーティクルを生成して追加
	for (uint32_t i = 0; i < count; ++i) {
		group.particleList.push_back(CreateNewParticle(randomEngine_, position));
	}
}

///=============================================================================
///						パーティクルグループ
void Particle::CreateParticleGroup(const std::string &name, const std::string &textureFilePath, ParticleShape shape) {
	// 登録済みの名前かチェックして assert
	bool nameExists = false;
	for (auto it = particleGroups.begin(); it != particleGroups.end(); ++it) {
		if (it->second.materialFilePath == name) {
			nameExists = true;
			break;
		}
	}
	if (nameExists) {
		assert(false && "Particle group with this name already exists!");
	}

	// 新たなパーティクルグループを作成
	ParticleGroup newGroup;
	newGroup.materialFilePath = textureFilePath;
	newGroup.shape = shape;

	// 形状に応じた頂点オフセットとカウントを設定
	const uint32_t kBoardVertexCount = 6;
	const uint32_t kRingDivide = 32;
	const uint32_t kRingVertexCount = kRingDivide * 6;
	const uint32_t kCylinderDivide = 32; // シリンダーの分割数
	// 上面: kCylinderDivide * 3, 底面: kCylinderDivide * 3, 側面: kCylinderDivide * 6
	const uint32_t kCylinderVertexCount = kCylinderDivide * 3 + kCylinderDivide * 3 + kCylinderDivide * 6;

	if (shape == ParticleShape::Board) {
		newGroup.vertexOffset = 0;
		newGroup.vertexCount = kBoardVertexCount;
	} else if (shape == ParticleShape::Ring) {
		newGroup.vertexOffset = kBoardVertexCount; // 板ポリゴンの頂点の後にリングの頂点が続く
		newGroup.vertexCount = kRingVertexCount;
	} else if (shape == ParticleShape::Cylinder) {
		newGroup.vertexOffset = kBoardVertexCount + kRingVertexCount; // 板とリングの後にシリンダー
		newGroup.vertexCount = kCylinderVertexCount;
		// assert(false && "Cylinder shape not yet fully implemented for vertex data."); // 実装したのでコメントアウト解除または削除
	}

	// テクスチャのSRVインデックスを取得して設定
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// テクスチャのSRVインデックスを取得して設定
	newGroup.srvIndex = TextureManager::GetInstance()->GetTextureIndex(textureFilePath);

	// テクスチャサイズを取得
	const DirectX::TexMetadata &metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath);
	MagMath::Vector2 textureSize = {static_cast<float>(metadata.width), static_cast<float>(metadata.height)};
	// カスタムサイズが指定されているかどうか
	// newGroup.textureSize = textureSize;

	// サイズを設定（指定があればそれを使用、なければテクスチャサイズを使用）
	if (customTextureSize.x > 0.0f && customTextureSize.y > 0.0f) {
		newGroup.textureSize = customTextureSize;
	} else {
		newGroup.textureSize = textureSize;
	}

	//// テクスチャサイズを設定
	// AdjustTextureSize(newGroup, textureFilePath);

	// インスタンシング用リソースの生成
	// newGroup.instancingResource =
	//	dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	newGroup.instancingResource =
		particleSetup_->GetDXManager()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);

	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void **>(&newGroup.instancingDataPtr));
	for (uint32_t index = 0; index < kNumMaxInstance; ++index) {
		newGroup.instancingDataPtr[index].WVP = MagMath::Identity4x4();
		newGroup.instancingDataPtr[index].World = MagMath::Identity4x4();
		// newGroup.instancingDataPtr[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// 最大インスタンシング用リソースの生成
	// InstancingMaxResource();

	// インスタンシング用SRVを確保してSRVインデックスを記録
	// newGroup.instancingSrvIndex =　srvManager_->Allocate() + 1;
	newGroup.instancingSrvIndex = particleSetup_->GetSrvSetup()->Allocate() + 1;
	// 作成したSRVをインスタンシング用リソースに設定
	// srvManager_->CreateSRVforStructuredBuffer(newGroup.instancingSrvIndex, newGroup.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));
	particleSetup_->GetSrvSetup()->CreateSRVStructuredBuffer(newGroup.instancingSrvIndex, newGroup.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

	// パーティクルグループをリストに追加
	particleGroups.emplace(name, newGroup);

	// マテリアルデータの初期化
	CreateMaterialData();

	// TODO:新しいブレンドモードを設定
	// blendMode_ = blendMode;
	// GraphicsPipelineState(blendMode);  // 再生成
}

///=============================================================================
///						静的メンバ関数
void Particle::CreateVertexData() {
	modelData_.vertices.clear(); // 既存の頂点データをクリア

	// 板ポリゴン（四角形）の頂点データ
	{
		MagMath::Vector3 normal = {0.0f, 0.0f, 1.0f};
		// 左上
		modelData_.vertices.push_back({{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, normal});
		// 右上
		modelData_.vertices.push_back({{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, normal});
		// 左下
		modelData_.vertices.push_back({{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, normal});

		// 左下
		modelData_.vertices.push_back({{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, normal});
		// 右上
		modelData_.vertices.push_back({{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, normal});
		// 右下
		modelData_.vertices.push_back({{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, normal});
	}

	// リング形状の頂点データ
	{
		const uint32_t kRingDivide = 32;				// この値は CreateParticleGroup の kRingDivide と一致させる
		const float kOuterRadius = ringRadius_;			// クラスメンバのringRadius_を使用 (または固定値)
		const float kInnerRadius = kOuterRadius * 0.5f; // 内径を外径の半分に (または固定値/別のパラメータ)
		const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

		for (uint32_t index = 0; index < kRingDivide; ++index) {
			float currentRadian = index * radianPerDivide;
			float nextRadian = (index + 1) * radianPerDivide;

			float sinCurrent = std::sin(currentRadian);
			float cosCurrent = std::cos(currentRadian);

			float sinNext = std::sin(nextRadian);
			float cosNext = std::cos(nextRadian);

			float uCurrent = float(index) / float(kRingDivide);
			float uNext = float(index + 1) / float(kRingDivide);

			MagMath::Vector4 outerCurrent = {cosCurrent * kOuterRadius, sinCurrent * kOuterRadius, 0.0f, 1.0f};
			MagMath::Vector4 outerNext = {cosNext * kOuterRadius, sinNext * kOuterRadius, 0.0f, 1.0f};

			MagMath::Vector4 innerCurrent = {cosCurrent * kInnerRadius, sinCurrent * kInnerRadius, 0.0f, 1.0f};
			MagMath::Vector4 innerNext = {cosNext * kInnerRadius, sinNext * kInnerRadius, 0.0f, 1.0f};

			MagMath::Vector3 normal = {0.0f, 0.0f, 1.0f};

			modelData_.vertices.push_back({outerCurrent, {uCurrent, 0.0f}, normal});
			modelData_.vertices.push_back({outerNext, {uNext, 0.0f}, normal});
			modelData_.vertices.push_back({innerCurrent, {uCurrent, 1.0f}, normal});

			modelData_.vertices.push_back({innerCurrent, {uCurrent, 1.0f}, normal});
			modelData_.vertices.push_back({outerNext, {uNext, 0.0f}, normal});
			modelData_.vertices.push_back({innerNext, {uNext, 1.0f}, normal});
		}
	}

	// シリンダー形状の頂点データ
	{
		const uint32_t kCylinderDivide = 32;  // CreateParticleGroup の kCylinderDivide と一致させる
		const float height = cylinderHeight_; // クラスメンバの cylinderHeight_ を使用
		const float radius = cylinderRadius_; // クラスメンバの cylinderRadius_ を使用
		const float halfHeight = height / 2.0f;
		const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

		// 上面の中心
		MagMath::Vector4 topCenter = {0.0f, halfHeight, 0.0f, 1.0f};
		// 底面の中心
		MagMath::Vector4 bottomCenter = {0.0f, -halfHeight, 0.0f, 1.0f};

		// 上面と底面の頂点
		for (uint32_t i = 0; i < kCylinderDivide; ++i) {
			float currentRadian = i * radianPerDivide;
			float nextRadian = (i + 1) * radianPerDivide;

			float cosCurrent = std::cos(currentRadian);
			float sinCurrent = std::sin(currentRadian);
			float cosNext = std::cos(nextRadian);
			float sinNext = std::sin(nextRadian);

			// 上面の円周上の点
			MagMath::Vector4 topP0 = {cosCurrent * radius, halfHeight, sinCurrent * radius, 1.0f};
			MagMath::Vector4 topP1 = {cosNext * radius, halfHeight, sinNext * radius, 1.0f};
			// 底面の円周上の点
			MagMath::Vector4 bottomP0 = {cosCurrent * radius, -halfHeight, sinCurrent * radius, 1.0f};
			MagMath::Vector4 bottomP1 = {cosNext * radius, -halfHeight, sinNext * radius, 1.0f};

			// UV座標 (仮。適切に設定する必要がある)
			MagMath::Vector2 uvTopCenter = {0.5f, 0.5f};											  // 上面中心
			MagMath::Vector2 uvTopP0 = {(cosCurrent + 1.0f) * 0.25f, (sinCurrent + 1.0f) * 0.25f}; // 上面周
			MagMath::Vector2 uvTopP1 = {(cosNext + 1.0f) * 0.25f, (sinNext + 1.0f) * 0.25f};

			MagMath::Vector2 uvBottomCenter = {0.5f, 0.5f};													// 底面中心
			MagMath::Vector2 uvBottomP0 = {(cosCurrent + 1.0f) * 0.25f + 0.5f, (sinCurrent + 1.0f) * 0.25f}; // 底面周
			MagMath::Vector2 uvBottomP1 = {(cosNext + 1.0f) * 0.25f + 0.5f, (sinNext + 1.0f) * 0.25f};

			float uSide0 = static_cast<float>(i) / kCylinderDivide;
			float uSide1 = static_cast<float>(i + 1) / kCylinderDivide;

			// 上面 (Y+)
			MagMath::Vector3 normalTop = {0.0f, 1.0f, 0.0f};
			modelData_.vertices.push_back({topCenter, uvTopCenter, normalTop});
			modelData_.vertices.push_back({topP1, uvTopP1, normalTop});
			modelData_.vertices.push_back({topP0, uvTopP0, normalTop});

			// 底面 (Y-)
			MagMath::Vector3 normalBottom = {0.0f, -1.0f, 0.0f};
			modelData_.vertices.push_back({bottomCenter, uvBottomCenter, normalBottom});
			modelData_.vertices.push_back({bottomP0, uvBottomP0, normalBottom});
			modelData_.vertices.push_back({bottomP1, uvBottomP1, normalBottom});

			// 側面
			MagMath::Vector3 sideNormalP0 = {cosCurrent, 0.0f, sinCurrent};
			MagMath::Vector3 sideNormalP1 = {cosNext, 0.0f, sinNext};
			MagMath::Normalize(sideNormalP0); // 正規化
			MagMath::Normalize(sideNormalP1); // 正規化

			// 側面 三角形1 (topP0, bottomP0, topP1)
			modelData_.vertices.push_back({topP0, {uSide0, 0.0f}, sideNormalP0});
			modelData_.vertices.push_back({bottomP0, {uSide0, 1.0f}, sideNormalP0});
			modelData_.vertices.push_back({topP1, {uSide1, 0.0f}, sideNormalP1});
			// 側面 三角形2 (topP1, bottomP0, bottomP1)
			modelData_.vertices.push_back({topP1, {uSide1, 0.0f}, sideNormalP1});
			modelData_.vertices.push_back({bottomP0, {uSide0, 1.0f}, sideNormalP0});
			modelData_.vertices.push_back({bottomP1, {uSide1, 1.0f}, sideNormalP1});
		}
	}
}

///--------------------------------------------------------------
///						 頂点バッファビューの作成
void Particle::CreateVertexBufferView() {
	//========================================
	// 1.頂点バッファの作成
	vertexBuffer_ = particleSetup_->GetDXManager()->CreateBufferResource(sizeof(MagMath::VertexData) * modelData_.vertices.size());

	//========================================
	// 2.頂点バッファビューの作成
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点のサイズ
	vertexBufferView_.SizeInBytes = UINT(sizeof(MagMath::VertexData) * modelData_.vertices.size());
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(MagMath::VertexData);
}

///=============================================================================
///						マテリアルデータの作成
void Particle::CreateMaterialData() {
	// マテリアル用のリソースを作成
	materialBuffer_ = particleSetup_->GetDXManager()->CreateBufferResource(sizeof(MagMath::Material));

	// 書き込むためのアドレスを取得
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
	materialData_->color = MagMath::Vector4(1.0f, 1.0f, 1.0f, 0.9f);
	// SpriteはLightingしないのfalseを設定する
	materialData_->enableLighting = false;
	materialData_->uvTransform = MagMath::Identity4x4();
}

///=============================================================================
///						新しいパーティクルを生成
ParticleStr Particle::CreateNewParticle(std::mt19937 &randomEngine, const MagMath::Vector3 &position) {
	// 範囲の安全性チェック（min > maxの場合はスワップ）
	auto safeRange = [](float &min, float &max) {
		if (min > max) {
			std::swap(min, max);
		}
		// 同じ値の場合は微小な差を追加
		if (std::abs(max - min) < 0.0001f) {
			max = min + 0.0001f;
		}
	};

	// 各範囲の安全性チェック
	MagMath::Vector3 safeTranslateMin = translateMin_;
	MagMath::Vector3 safeTranslateMax = translateMax_;
	safeRange(safeTranslateMin.x, safeTranslateMax.x);
	safeRange(safeTranslateMin.y, safeTranslateMax.y);
	safeRange(safeTranslateMin.z, safeTranslateMax.z);

	MagMath::Vector3 safeVelocityMin = velocityMin_;
	MagMath::Vector3 safeVelocityMax = velocityMax_;
	safeRange(safeVelocityMin.x, safeVelocityMax.x);
	safeRange(safeVelocityMin.y, safeVelocityMax.y);
	safeRange(safeVelocityMin.z, safeVelocityMax.z);

	MagMath::Vector4 safeColorMin = colorMin_;
	MagMath::Vector4 safeColorMax = colorMax_;
	safeRange(safeColorMin.x, safeColorMax.x);
	safeRange(safeColorMin.y, safeColorMax.y);
	safeRange(safeColorMin.z, safeColorMax.z);
	safeRange(safeColorMin.w, safeColorMax.w);

	float safeLifeMin = lifetimeRange_.min;
	float safeLifeMax = lifetimeRange_.max;
	safeRange(safeLifeMin, safeLifeMax);

	MagMath::Vector3 safeInitialScaleMin = initialScaleMin_;
	MagMath::Vector3 safeInitialScaleMax = initialScaleMax_;
	safeRange(safeInitialScaleMin.x, safeInitialScaleMax.x);
	safeRange(safeInitialScaleMin.y, safeInitialScaleMax.y);
	safeRange(safeInitialScaleMin.z, safeInitialScaleMax.z);

	MagMath::Vector3 safeEndScaleMin = endScaleMin_;
	MagMath::Vector3 safeEndScaleMax = endScaleMax_;
	safeRange(safeEndScaleMin.x, safeEndScaleMax.x);
	safeRange(safeEndScaleMin.y, safeEndScaleMax.y);
	safeRange(safeEndScaleMin.z, safeEndScaleMax.z);

	MagMath::Vector3 safeInitialRotationMin = initialRotationMin_;
	MagMath::Vector3 safeInitialRotationMax = initialRotationMax_;
	safeRange(safeInitialRotationMin.x, safeInitialRotationMax.x);
	safeRange(safeInitialRotationMin.y, safeInitialRotationMax.y);
	safeRange(safeInitialRotationMin.z, safeInitialRotationMax.z);

	MagMath::Vector3 safeEndRotationMin = endRotationMin_;
	MagMath::Vector3 safeEndRotationMax = endRotationMax_;
	safeRange(safeEndRotationMin.x, safeEndRotationMax.x);
	safeRange(safeEndRotationMin.y, safeEndRotationMax.y);
	safeRange(safeEndRotationMin.z, safeEndRotationMax.z);

	// カラーと寿命のランダム分布（安全な範囲を使用）
	std::uniform_real_distribution<float> distTranslateX(safeTranslateMin.x, safeTranslateMax.x);
	std::uniform_real_distribution<float> distTranslateY(safeTranslateMin.y, safeTranslateMax.y);
	std::uniform_real_distribution<float> distTranslateZ(safeTranslateMin.z, safeTranslateMax.z);
	std::uniform_real_distribution<float> distVelocityX(safeVelocityMin.x, safeVelocityMax.x);
	std::uniform_real_distribution<float> distVelocityY(safeVelocityMin.y, safeVelocityMax.y);
	std::uniform_real_distribution<float> distVelocityZ(safeVelocityMin.z, safeVelocityMax.z);
	std::uniform_real_distribution<float> distColorR(safeColorMin.x, safeColorMax.x);
	std::uniform_real_distribution<float> distColorG(safeColorMin.y, safeColorMax.y);
	std::uniform_real_distribution<float> distColorB(safeColorMin.z, safeColorMax.z);
	std::uniform_real_distribution<float> distColorA(safeColorMin.w, safeColorMax.w);
	std::uniform_real_distribution<float> distTime(safeLifeMin, safeLifeMax);
	std::uniform_real_distribution<float> distInitialScaleX(safeInitialScaleMin.x, safeInitialScaleMax.x);
	std::uniform_real_distribution<float> distInitialScaleY(safeInitialScaleMin.y, safeInitialScaleMax.y);
	std::uniform_real_distribution<float> distInitialScaleZ(safeInitialScaleMin.z, safeInitialScaleMax.z);
	std::uniform_real_distribution<float> distEndScaleX(safeEndScaleMin.x, safeEndScaleMax.x);
	std::uniform_real_distribution<float> distEndScaleY(safeEndScaleMin.y, safeEndScaleMax.y);
	std::uniform_real_distribution<float> distEndScaleZ(safeEndScaleMin.z, safeEndScaleMax.z);
	std::uniform_real_distribution<float> distInitialRotationX(safeInitialRotationMin.x, safeInitialRotationMax.x);
	std::uniform_real_distribution<float> distInitialRotationY(safeInitialRotationMin.y, safeInitialRotationMax.y);
	std::uniform_real_distribution<float> distInitialRotationZ(safeInitialRotationMin.z, safeInitialRotationMax.z);
	std::uniform_real_distribution<float> distEndRotationX(safeEndRotationMin.x, safeEndRotationMax.x);
	std::uniform_real_distribution<float> distEndRotationY(safeEndRotationMin.y, safeEndRotationMax.y);
	std::uniform_real_distribution<float> distEndRotationZ(safeEndRotationMin.z, safeEndRotationMax.z);

	// 新たなパーティクルの生成
	ParticleStr particle = {};
	particle.transform.translate = {
		position.x + distTranslateX(randomEngine),
		position.y + distTranslateY(randomEngine),
		position.z + distTranslateZ(randomEngine)};
	particle.velocity = {
		distVelocityX(randomEngine),
		distVelocityY(randomEngine),
		distVelocityZ(randomEngine)};
	particle.color = {
		distColorR(randomEngine),
		distColorG(randomEngine),
		distColorB(randomEngine),
		distColorA(randomEngine)};
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	particle.initialScale = {
		distInitialScaleX(randomEngine),
		distInitialScaleY(randomEngine),
		distInitialScaleZ(randomEngine)};
	particle.endScale = {
		distEndScaleX(randomEngine),
		distEndScaleY(randomEngine),
		distEndScaleZ(randomEngine)};
	particle.initialRotation = {
		distInitialRotationX(randomEngine),
		distInitialRotationY(randomEngine),
		distInitialRotationZ(randomEngine)};
	particle.endRotation = {
		distEndRotationX(randomEngine),
		distEndRotationY(randomEngine),
		distEndRotationZ(randomEngine)};

	// 初期回転をtransformに設定
	particle.transform.rotate = particle.initialRotation;
	// 初期スケールをtransformに設定
	particle.transform.scale = particle.initialScale;

	return particle;
}
