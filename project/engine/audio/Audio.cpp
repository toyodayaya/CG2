#include <Windows.h>

#pragma warning(push)
#pragma warning(disable : 4229)
#include <mfapi.h>
#include <mfidl.h> 
#include <mfreadwrite.h>
#pragma warning(pop)

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")

#include <fstream>
#include <cassert>
#include "Audio.h"
#include "StringUtility.h"

Audio* Audio::instance = nullptr;

Audio* Audio::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new Audio;
	}

	return instance;
}

void Audio::Initialize()
{
	// MF全体の初期化
	HRESULT result;
	result = MFStartup(MF_VERSION,MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(result));

	// xAudio2エンジンのインスタンスを生成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	// マスターボイスを生成
	result = xAudio2->CreateMasteringVoice(&masterVoice);
}

Audio::SoundData Audio::SoundLoadFile(const std::string& filename)
{
	// フルパスをワイド文字列に変換
	std::wstring filePathW = StringUtility::ConvertString(filename);
	HRESULT result;

	// SourcesReaderを生成
	Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
	result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
	assert(SUCCEEDED(result));

	// PCM形式にフォーマット指定する
	Microsoft::WRL::ComPtr<IMFMediaType> pPCMType;
	MFCreateMediaType(&pPCMType);
	pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	result = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
	assert(SUCCEEDED(result));

	// 実際にセットされたメディアタイプを取得する
	Microsoft::WRL::ComPtr<IMFMediaType> pOutType;
	pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);

	// waveフォーマットを取得する
	WAVEFORMATEX* waveFormat = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);

	// 読み込んだ音声データを返す
	// 返すための音声データ
	SoundData soundData = {};
	soundData.wfex = *waveFormat;

	// 生成したWaveフォーマットを解放
	CoTaskMemFree(waveFormat);

	// PCMデータのバッファを構築
	while (true)
	{
		Microsoft::WRL::ComPtr<IMFSample> pSample;
		DWORD streamIndex = 0, flags = 0;
		LONGLONG llTimeStamp = 0;
		// サンプルを読み込む
		result = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &pSample);
		// ストリームの末尾に達したらループを抜ける
		if(flags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			break;
		}

		if (pSample)
		{
			Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
			// サンプルに含まれるサウンドデータのバッファをひとつなぎにして取得
			pSample->ConvertToContiguousBuffer(&pBuffer);

			BYTE* pData = nullptr;
			DWORD maxLength = 0, currentLength = 0;
			// バッファ読み込み用にロック
			pBuffer->Lock(&pData, &maxLength, &currentLength);
			// バッファの末尾にデータを追加
			soundData.buffer.insert(soundData.buffer.end(), pData, pData + currentLength);
			pBuffer->Unlock();
		}
	}
	
	return soundData;
}

void Audio::SoundUnload(SoundData* soundData)
{
	// バッファのメモリを解放
	soundData->buffer.clear();
	soundData->wfex = {};
}

void Audio::SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	// 波形フォーマットを基にSoundVoiceを生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = (UINT32)soundData.buffer.size();
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 管理リストに追加
	activeVoices.insert(pSourceVoice);

	// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

void Audio::SoundStopWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	// 管理リストから削除
	for (auto voice : activeVoices)
	{
		voice->Stop();
		voice->FlushSourceBuffers();
		voice->DestroyVoice();
	}
	activeVoices.clear();
}

void Audio::Finalize()
{
	// xAudio2解放
	xAudio2.Reset();

	// MF全体の終了
	HRESULT result = MFShutdown();
	assert(SUCCEEDED(result));

	delete instance;
	instance = nullptr;

}