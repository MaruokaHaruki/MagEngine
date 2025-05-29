///--------------------------------------------------------------
///						 パーティクル共通部
particleSetup_ = std::make_unique<ParticleSetup>();
// パーティクルセットアップの初期化
particleSetup_->Initialize(dxCore_.get(), srvSetup_.get());
// パーティクルのカメラ設定
particleSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
