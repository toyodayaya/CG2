#pragma once
#include "BaseScene.h"
#include "SceneFactory.h"

class SceneManager
{
private:
	// コンストラクタ
	SceneManager() = default;
	// デストラクタ
	~SceneManager() = default;
	// コピーコンストラクタとコピー代入演算子を削除
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	// インスタンス
	static SceneManager* instance;

public:
	// 次シーン予約
	void ChangeScene(const std::string& sceneName);
	// 更新
	void Update();
	// 描画
	void Draw();
	// 終了
	void Finalize();

	// インスタンス
	static SceneManager* GetInstance();

	// シーンファクトリーのセット
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }

private:
	// 実行中のシーン
	BaseScene* scene_ = nullptr;
	// 次シーン
	BaseScene* nextScene_ = nullptr;
	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;
};

