#pragma once
#include "IRenderPass.h"
namespace MagEngine { class TextRenderPass final : public IRenderPass { public: void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override; }; }
