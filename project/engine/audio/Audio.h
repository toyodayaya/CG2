#pragma once
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include <cstdint>
#include <wrl.h>
#include <vector>
#include <string>

struct SoundData
{
	// 波形フォーマット
	WAVEFORMATEX wfex;
	// バッファ
	std::vector<BYTE> buffer;
};

class Audio
{
private:
	// チャンクヘッダ
	struct ChunkHeader
	{
		char id[4];
		int32_t size;
	};

	// RIFFヘッダチャンク
	struct RiffHeader
	{
		ChunkHeader chunk;
		char type[4];
	};

	// FMTチャンク
	struct FormatChunk
	{
		ChunkHeader chunk;
		WAVEFORMATEX fmt;
	};


	
public:
	// 初期化
	void Initialize();

	// 終了
	void Finalize();

	// 音声データ読み込み関数
	SoundData SoundLoadFile(const std::string& filename);

	// 音声データの解放関数
	void SoundUnload(SoundData* soundData);

	// 音声データの再生関数
	void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);

	// getter
	Microsoft::WRL::ComPtr<IXAudio2> GetXAudio2() const { return xAudio2; }

private:
	
	//　音声データ用の変数宣言
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
	
};

