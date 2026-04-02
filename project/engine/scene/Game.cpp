#include "Game.h"
#include "BaseScene.h"
#include "SceneFactory.h"

void Game::Initialize()
{
	// 汎用部の初期化処理
	Framework::Initialize();

	camera->SetRotate({ std::numbers::pi_v<float> / 3.0f,std::numbers::pi_v<float> ,0.0f });
	camera->SetTranslate({ 0.0f,23.0f,10.0f });

	// シーンファクトリーの生成とセット
	SceneFactory* sceneFactory = new SceneFactory;
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory);
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("TitleScene");
}

void Game::Update()
{

	// 汎用部の更新処理
	Framework::Update();

	// シーンの更新
	SceneManager::GetInstance()->Update();

}

void Game::Draw()
{

	// 描画前処理
	dxBasis->PreDraw();
	srvManager->PreDraw();

	// 3dモデルの描画準備
	Object3dCommon::GetInstance()->DrawSettingCommon();

	// Spriteの描画準備
	SpriteCommon::GetInstance()->DrawSettingCommon();

	// シーンの描画
	SceneManager::GetInstance()->Draw();
	

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
	// シーンマネージャーの終了処理
	SceneManager::GetInstance()->Finalize();

	// シーンファクトリーの終了処理
	delete sceneFactory;

	// 汎用部の終了処理
	Framework::Finalize();
	
}

