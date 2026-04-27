#include "Game.h"
#include "BaseScene.h"
#include "SceneFactory.h"

void Game::Initialize()
{
	// 汎用部の初期化処理
	Framework::Initialize();

	camera->SetRotate(Vector3{ 0.25f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-12.0f });

	// Imguiマネージャーの初期化
	imguiManager = std::make_unique <ImguiManager>();
	imguiManager->Initialize(winAPIManager.get(), dxBasis.get(), srvManager.get());

	// シーンファクトリーの生成とセット
	SceneFactory* sceneFactory = new SceneFactory;
	SceneManager::GetInstance()->SetSceneFactory(sceneFactory);
	// シーンマネージャーに最初のシーンをセット
	SceneManager::GetInstance()->ChangeScene("TitleScene");
}

void Game::Update()
{
#ifdef USE_IMGUI

	// 開発用UIの処理
	imguiManager->Begin();

#endif // USE_IMGUI

	// 汎用部の更新処理
	Framework::Update();

	// シーンの更新
	SceneManager::GetInstance()->Update();

#ifdef USE_IMGUI

	// ImGuiの受け付け終了
	imguiManager->End();

#endif // USE_IMGUI
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
	
	// ImGuiマネージャーの終了
	imguiManager->Finalize();

}

