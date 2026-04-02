#pragma once
#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include "Framework.h"
#include "SceneManager.h"
#pragma warning(pop)

using namespace MathManager;
using namespace Logger;

class Game : public Framework
{
public:
	// 初期化
	void Initialize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	// 終了
	void Finalize() override;

	
private:
};

