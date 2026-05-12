#pragma once
#include "Audio.h"
#include "Sprite.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include <numbers>
#include "BaseScene.h"
#include <memory>
#include <random>

class GamePlayScene : public BaseScene
{
public:
	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	// サウンドデータ
	Audio::SoundData soundData1;

	// スプライト
	std::vector<std::unique_ptr<Sprite>> sprites;
	// スプライト切り替えフラグ
	bool useMonsterBall = true;

	// 3dオブジェクト
	std::vector< std::unique_ptr<Object3d>> object3ds;

	// パーティクルエミッターのポインタ
	std::unique_ptr <ParticleEmitter> emitter;
	Vector3 randomTranslate;
	// 乱数生成器
	std::random_device seedGenerator;
	std::mt19937 randomEngine;
};
