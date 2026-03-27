#pragma once
#include <string>
#include <MathManager.h>

class ParticleEmitter
{
private:

	struct Emitter
	{
		Transform transform;
		uint32_t count;
		float frequency;
		float frequencyTime;
	};

public:
	static ParticleEmitter* instance;

	ParticleEmitter(const std::string name, const Vector3& position, float frequency, uint32_t count);
	~ParticleEmitter() = default;
	ParticleEmitter(ParticleEmitter&) = delete;
	ParticleEmitter& operator=(ParticleEmitter&) = delete;

	// メンバ変数
	std::string name;
	Emitter emitter{};

public:

	// パーティクル発生
	void Emit();

	// 更新処理
	void Update();

};

