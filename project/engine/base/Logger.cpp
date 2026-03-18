#include "Logger.h"
#include <windows.h>
#include <string>

// ログ出力
namespace Logger
{
	// 出力ウインドウに文字を出す
	void Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}
}