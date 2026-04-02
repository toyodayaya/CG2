#include "GamePlayScene.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ParticleManager.h"

void GamePlayScene::Initialize()
{
	// 音声読み込み
	soundData1 = Audio::GetInstance()->SoundLoadFile("resources/fanfare.mp3");

	// スプライトの初期化
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/circle.png");
	for (uint32_t i = 0; i < 5; ++i)
	{
		Sprite* sprite = new Sprite();
		sprite->Initialize(SpriteCommon::GetInstance(), "resources/uvChecker.png");
		sprites.push_back(sprite);
	}

	// objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	// 3Dオブジェクトの初期化
	for (uint32_t i = 0; i < 2; ++i)
	{
		Object3d* object3d = new Object3d();
		object3d->Initialize(Object3dCommon::GetInstance());
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
	Audio::GetInstance()->SoundPlayWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);
}

void GamePlayScene::Finalize()
{
	Audio::GetInstance()->SoundStopWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);
	Audio::GetInstance()->SoundUnload(&soundData1);

	delete emitter;

	for (Sprite* sprite : sprites)
	{
		delete sprite;
	}

	for (Object3d* object3d : object3ds)
	{
		delete object3d;
	}
}

void GamePlayScene::Update()
{
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

void GamePlayScene::Draw()
{
	
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
}