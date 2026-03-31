#pragma once
#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include "Sprite.h"
#include "Object3d.h"
#include "Model.h"
#include <numbers>
#include "ParticleEmitter.h"
#include "Framework.h"

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
	
	// サウンドデータ
	Audio::SoundData soundData1;

	// スプライト
	std::vector<Sprite*> sprites;
	// スプライト切り替えフラグ
	bool useMonsterBall = true;

	// 3dオブジェクト
	std::vector<Object3d*> object3ds;

	// パーティクルエミッターのポインタ
	ParticleEmitter* emitter = nullptr;
};

