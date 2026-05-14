
#include "GamePlayScene.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ParticleManager.h"
#include "MathManager.h"
#include "ParticleEmitter.h"
#include <random>

using namespace MathManager;

void GamePlayScene::Initialize()
{
	// 音声読み込み
	soundData1 = Audio::GetInstance()->SoundLoadFile("resources/fanfare.mp3");

	// スプライトの初期化
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/circle.png");
	for (uint32_t i = 0; i < 5; ++i)
	{
		std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();
		sprite->Initialize(SpriteCommon::GetInstance(), "resources/uvChecker.png");
		sprites.push_back(std::move(sprite));
	}

	// objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");
	ModelManager::GetInstance()->LoadModel("fence.obj");

	//// 3Dオブジェクトの初期化
	//for (uint32_t i = 0; i < 1; ++i)
	//{
	//	std::unique_ptr<Object3d> object3d = std::make_unique<Object3d>();
	//	object3d->Initialize(Object3dCommon::GetInstance());
	//	object3d->SetModel("fence.obj");
	//	Vector3 pos = object3d->GetTranslate();
	//	pos.x += (1.0f * (i + 1));
	//	object3d->SetTranslate(pos);
	//	object3ds.push_back(std::move(object3d));
	//}

	

	// パーティクルグループの作成
	ParticleManager::GetInstance()->CreateParticleGroup("Particle", "resources/circle.png");

	// パーティクルエミッターの宣言
	Transform transform;
	transform.translate = { 1.0f,1.0f,1.0f };
	transform.rotate = { 0.0f,0.0f,0.0f };
	transform.scale = { 1.0f,1.0f,1.0f };
	transform.translate = Vector3Add(transform.translate, randomTranslate);
	Vector3 velocity = { 0.0f,0.0f,0.0f };
	Vector4 color = { 0.0f,0.0f,0.0f,0.0f };
	float lifeTime = 0.0f;
	float currentTime = 0;
	emitter = std::make_unique <ParticleEmitter>("Particle", transform,velocity,color,lifeTime,currentTime,0.5f,2,ParticleEmitter::Type::kNormal);

	// 音声再生
	Audio::GetInstance()->SoundPlayWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);
}

void GamePlayScene::Finalize()
{
	Audio::GetInstance()->SoundStopWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);
	Audio::GetInstance()->SoundUnload(&soundData1);
}

void GamePlayScene::Update()
{
	// 3Dモデルの更新処理
	/*for (const std::unique_ptr<Object3d>& object3d : object3ds)
	{
		object3d->Update();

	}*/

	// スプライトの更新処理
	for (const std::unique_ptr <Sprite>& sprite : sprites)
	{
		sprite->Update();
	}

	// パーティクルの更新処理
	emitter->Update();
}

void GamePlayScene::Draw()
{

	// 3dモデルの描画
	//for (const std::unique_ptr <Object3d>& object3d : object3ds)
	//{
	//	//object3d->Draw();

	//}


	// Spriteの描画
	for (const std::unique_ptr <Sprite>& sprite : sprites)
	{
		//sprite->Draw();
	}

	// パーティクルの描画
	ParticleManager::GetInstance()->Draw();
}