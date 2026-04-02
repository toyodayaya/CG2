#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ModelCommon.h"
#include "DirectXBasis.h"
class Model;

class ModelManager
{
private:
	static ModelManager* instance;

	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(ModelManager&) = delete;
	ModelManager& operator = (ModelManager&) = delete;

	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>> models;

	// モデル共通部のポインタ
	std::unique_ptr <ModelCommon> modelCommon;

public:
	// シングルトンインスタンスの取得
	static ModelManager* GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(DirectXBasis* dxBasis);

	// モデルファイル読み込み
	void LoadModel(const std::string& filePath);
	// モデルデータ取得
	Model* FindModel(const std::string& filePath);
};
