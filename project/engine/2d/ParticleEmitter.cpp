#include "ParticleEmitter.h"
#include "ParticleManager.h"

ParticleEmitter::ParticleEmitter(const std::string name, const Transform& transform,
	const Vector3& velocity, const Vector4& color, const float lifeTime, const float currentTime, float frequency, uint32_t count,Type type)

	// 引数で受け取ってメンバ変数として記録する
	: name(name)
{
	emitter.transform.scale = transform.scale;
	emitter.transform.rotate = transform.rotate;
	emitter.transform.translate = transform.translate;

	emitter.velocity = velocity;
	emitter.color = color;
	emitter.lifeTime = lifeTime;
	emitter.currentTime = currentTime;

	emitter.frequency = frequency;
	emitter.count = count;
	emitter.type = type;
	emitter.frequencyTime = 0.0f;
}

void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.transform.scale, emitter.transform.rotate,
		emitter.velocity, emitter.color, emitter.lifeTime, emitter.currentTime, emitter.count,emitter.type);
}

void ParticleEmitter::Update()
{
	const float kDeltaTime = 1.0f / 60.0f;
	emitter.frequencyTime += kDeltaTime;

	if (emitter.frequencyTime >= emitter.frequency)
	{
		// 発生頻度より大きいなら発生
		Emit();

		// 余計に過ぎた時間も加味
		emitter.frequencyTime -= emitter.frequency;
	}

}