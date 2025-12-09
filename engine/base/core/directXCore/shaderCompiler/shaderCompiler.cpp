#include "shaderCompiler.h"
#include "Logger.h"
#include "WstringUtility.h"
#include <cassert>
#include <format>
#include <fstream>

using namespace Logger;
using namespace WstringUtility;

// エラー内容をファイルに書き出す補助関数
static void WriteToFile(const std::string &fileName, const std::string &text) {
	std::ofstream outputFile(fileName);
	if (!outputFile) {
		return;
	}
	outputFile << text;
	outputFile.close();
}

///=============================================================================
///						初期化
void ShaderCompiler::Initialize() {
	// dxcCompilerを初期化
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));
	// 現時点でincludeはしないが、includeに対応するために設定を行う
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						終了処理
void ShaderCompiler::Finalize() {
	if (includeHandler_) {
		includeHandler_->Release();
		includeHandler_ = nullptr;
	}
	if (dxcCompiler_) {
		dxcCompiler_->Release();
		dxcCompiler_ = nullptr;
	}
	if (dxcUtils_) {
		dxcUtils_->Release();
		dxcUtils_ = nullptr;
	}
}

///=============================================================================
///						シェーダーのコンパイル
IDxcBlob *ShaderCompiler::CompileShader(
	const std::wstring &filePath,
	const wchar_t *profile) {

	// これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin Compiler, path:{}, profile:{}", filePath, profile)), LogLevel::Info);

	// hlslファイルを読む
	IDxcBlobEncoding *shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer = {};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイルする
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"-Qembed_debug",
		L"-Od", L"-Zpr",
		L"-I", L"resources/shader/"};

	// 実際にShaderをコンパイルする
	IDxcResult *shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler_,
		IID_PPV_ARGS(&shaderResult));
	assert(SUCCEEDED(hr));

	// 警告・エラーがでてないか確認する
	IDxcBlobUtf8 *shaderError = nullptr;
	if (shaderResult->HasOutput(DXC_OUT_ERRORS)) {
		shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	}

	// エラーがある場合はエラーを出力して終了
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer(), LogLevel::Error);
		WriteToFile("shaderError.txt", shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob *shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}", filePath, profile)), LogLevel::Success);

	// もう使わないリソースを開放
	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}
