#pragma once
#include <dxcapi.h>
#include <string>
#include <wrl/client.h>

/// @brief シェーダーコンパイラクラス
class ShaderCompiler {
public:
	void Initialize();
	void Finalize();

	IDxcBlob *CompileShader(
		const std::wstring &filePath,
		const wchar_t *profile);

private:
	IDxcCompiler3 *dxcCompiler_ = nullptr;
	IDxcUtils *dxcUtils_ = nullptr;
	IDxcIncludeHandler *includeHandler_ = nullptr;
};