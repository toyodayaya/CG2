#include "ParticleEmitter.h"
#include "ParticleManager.h"

ParticleEmitter::ParticleEmitter(const std::string name, const Vector3& position, float frequency, uint32_t count)

// 引数で受け取ってメンバ変数として記録する
	: name(name)
{
	emitter.transform.scale = { 1.0f, 1.0f, 1.0f }; 
	emitter.transform.rotate = { 0.0f, 0.0f, 0.0f };
	emitter.transform.translate = position;

	emitter.frequency = frequency;
	emitter.count = count;
	emitter.frequencyTime = 0.0f;
}

void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count);
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