#pragma once
#include "DirectXBasis.h"

class ModelCommon
{
public:
	// 初期化
	void Initialize(DirectXBasis* directXBasis);

	// getter
	DirectXBasis* GetDxBasis() const { return dxBasis_; }

private:
	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;
};

