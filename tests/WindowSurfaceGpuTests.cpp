/*********************************************************************
 * \file WindowSurfaceGpuTests.cpp
 * \brief 実デバイス上で反復リサイズとDescriptor再利用を検証する
 *********************************************************************/
#include "DirectXCore.h"
#include "SrvSetup.h"
#include "TextureManager.h"
#include "ImguiSetup.h"
#include <d3d12sdklayers.h>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace { void Check(bool value, const char *name) { if (!value) throw std::runtime_error(name); } }
int main() {
	try {
		using namespace MagEngine;
		WinApp window; window.CreateGameWindow(L"MagEngine Surface Validation");
		DirectXCore graphics; graphics.InitializeDirectX(&window);
		SrvSetup srv; srv.Initialize(&graphics);
		TextureManager textures; textures.Initialize(&graphics, "resources/", &srv);
		ImguiSetup ui; ui.Initialize(&window, &graphics, Style::EDITOR);
		const float baseline = ImGui::GetStyle().FramePadding.y;
		for (const float scale : {1.5f, 2.0f, 1.0f}) {
			ui.ApplyDpi(scale);
			Check(ImGui::GetStyle().FramePadding.y == baseline * scale, "GpuDpiStyleBaseline");
			const auto *font = ui.GetFont("firge_regular");
			Check(font && font->FontSize == 16 * scale, "GpuDpiFontSize");
		}
		const auto rtv = graphics.GetRtvAllocator().GetAllocatedCount();
		const auto dsv = graphics.GetDsvAllocator().GetAllocatedCount();
		const auto resources = graphics.GetResourceAllocator().GetAllocatedCount();
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> queue;
		graphics.GetDevice().As(&queue); if (queue) queue->ClearStoredMessages();
		std::string error;
		for (int iteration = 0; iteration < 8; ++iteration) {
			const SurfaceSize size = iteration % 2 ? SurfaceSize{1024, 768} : SurfaceSize{1600, 900};
			Check(graphics.ResizeSurface(size, error), error.c_str()); textures.RefreshRenderTextureViews();
			Check(graphics.GetRenderSize() == size, "AppliedGpuSize");
			for (uint32_t index = 0; index < 2; ++index) {
				const auto desc = graphics.GetRenderTextureResource(index)->GetDesc();
				Check(desc.Width == size.width && desc.Height == size.height, "ColorImageSize");
			}
			Check(graphics.GetRtvAllocator().GetAllocatedCount() == rtv && graphics.GetDsvAllocator().GetAllocatedCount() == dsv && graphics.GetResourceAllocator().GetAllocatedCount() == resources, "DescriptorsDoNotGrow");
		}
		Check(!graphics.ResizeSurface({0, 0}, error) && graphics.GetRenderSize() == SurfaceSize{1024, 768}, "InvalidResizeKeepsAppliedSize");
		if (queue) for (UINT64 i = 0; i < queue->GetNumStoredMessages(); ++i) {
			SIZE_T size = 0; queue->GetMessage(i, nullptr, &size); std::vector<unsigned char> storage(size);
			auto *message = reinterpret_cast<D3D12_MESSAGE *>(storage.data()); queue->GetMessage(i, message, &size);
			if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) { std::cerr << message->pDescription << '\n'; throw std::runtime_error("D3D12 resize warning"); }
		}
		queue.Reset(); ui.Finalize(); textures.Finalize(); graphics.ReleaseDirectX(); window.CloseWindow();
		std::cout << "WindowSurfaceGpu PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
