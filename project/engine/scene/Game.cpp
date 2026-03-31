#include "Game.h"

void Game::Initialize()
{
	// 汎用部の初期化処理
	Framework::Initialize();

	camera->SetRotate({ std::numbers::pi_v<float> / 3.0f,std::numbers::pi_v<float> ,0.0f });
	camera->SetTranslate({ 0.0f,23.0f,10.0f });

	// 音声読み込み
	soundData1 = audio->SoundLoadFile("resources/fanfare.mp3");

	// スプライトの初期化
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/circle.png");
	for (uint32_t i = 0; i < 5; ++i)
	{
		Sprite* sprite = new Sprite();
		sprite->Initialize(spriteCommon, "resources/uvChecker.png");
		sprites.push_back(sprite);
	}

	// objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	// 3Dオブジェクトの初期化
	for (uint32_t i = 0; i < 2; ++i)
	{
		Object3d* object3d = new Object3d();
		object3d->Initialize(object3dCommon);
		object3d->SetModel("plane.obj");
		Vector3 pos = object3d->GetTranslate();
		pos.x += (1.0f * (i + 1));
		object3d->SetTranslate(pos);
		object3ds.push_back(object3d);
	}

	object3ds[1]->SetModel("axis.obj");

	// パーティクルグループの作成
	ParticleManager::GetInstance()->CreateParticleGroup("Particle", "resources/circle.png");

	// パーティクルエミッターの宣言
	Transform translate;
	translate.translate = { 0.0f,0.0f,0.0f };
	translate.rotate = { 0.0f,0.0f,0.0f };
	translate.scale = { 1.0f,1.0f,1.0f };
	emitter = new ParticleEmitter("Particle", translate.translate, 0.5f, 2);

	// 音声再生
	audio->SoundPlayWave(audio->GetXAudio2().Get(), soundData1);
}

void Game::Update()
{

	// 汎用部の更新処理
	Framework::Update();

	// 3Dモデルの更新処理
	for (Object3d* object3d : object3ds)
	{
		object3d->Update();

	}

	// スプライトの更新処理
	for (Sprite* sprite : sprites)
	{
		sprite->Update();
	}

	// パーティクルの更新処理
	emitter->Update();

}

void Game::Draw()
{

	// 描画前処理
	dxBasis->PreDraw();
	srvManager->PreDraw();

	// 3dモデルの描画準備
	object3dCommon->DrawSettingCommon();

	// Spriteの描画準備
	spriteCommon->DrawSettingCommon();

	// 3dモデルの描画
	for (Object3d* object3d : object3ds)
	{
		object3d->Draw();

	}


	// Spriteの描画
	for (Sprite* sprite : sprites)
	{
		sprite->Draw();
	}

	// パーティクルの描画
	ParticleManager::GetInstance()->Draw();

#ifdef USE_IMGUI

	// ImGuiの描画
	imguiManager->Draw();

#endif // USE_IMGUI

	// 描画後処理
	dxBasis->PostDraw();

	TextureManager::GetInstance()->ReleaseIntermediateResources();
}

void Game::Finalize()
{
	
	audio->SoundUnload(&soundData1);
	
	delete emitter;

	for (Sprite* sprite : sprites)
	{
		delete sprite;
	}

	for (Object3d* object3d : object3ds)
	{
		delete object3d;
	}

	// 汎用部の終了処理
	Framework::Finalize();
	
}

