
#pragma once
#include <string>
#include <MathManager.h>

class ParticleEmitter
{
private:

	struct Emitter
	{
		Transform transform;
		Vector3 velocity;
		Vector4 color;
		float lifeTime;
		float currentTime;
		uint32_t count;
		float frequency;
		float frequencyTime;
	};

public:
	static ParticleEmitter* instance;

	ParticleEmitter(const std::string name, const Transform& transform,
		const Vector3& velocity, const Vector4& color, const float lifeTime, const float currentTime, float frequency, uint32_t count);
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
