#include "TitleScene.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "SceneManager.h"

void TitleScene::Initialize()
{
	// 音声読み込み
	soundData1 = Audio::GetInstance()->SoundLoadFile("resources/Alarm01.wav");

	// スプライトの初期化
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
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

	// 音声再生
	Audio::GetInstance()->SoundPlayWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);

}

void TitleScene::Finalize()
{
	Audio::GetInstance()->SoundStopWave(Audio::GetInstance()->GetXAudio2().Get(), soundData1);
	Audio::GetInstance()->SoundUnload(&soundData1);

	for (Sprite* sprite : sprites)
	{
		delete sprite;
	}

	for (Object3d* object3d : object3ds)
	{
		delete object3d;
	}
}

void TitleScene::Update()
{
	// Enterキーを押したら
	if (Input::GetInstance()->TriggerKey(DIK_RETURN))
	{
		SceneManager::GetInstance()->ChangeScene("GamePlayScene");
	}

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


}

void TitleScene::Draw()
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
}