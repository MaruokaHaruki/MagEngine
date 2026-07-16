#include "TextRenderPass.h"
#include "RenderWorld.h"
#include "engine/graphics/text/TextRenderer.h"
namespace MagEngine { void TextRenderPass::Execute(RenderContext &, const RenderWorld &world) { for (const TextRenderItem &item : world.GetTextItems()) { if (item.visible && item.renderer) { item.renderer->Draw(); } } } }
