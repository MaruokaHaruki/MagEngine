/*********************************************************************
 * \file WindowSurfaceTests.cpp
 * \brief リサイズ要求の集約・最小化・適用済み状態をCPUで検証する
 *********************************************************************/
#include "engine/base/core/WindowSurfaceState.h"
#include <iostream>
#include <stdexcept>
namespace { void Check(bool value, const char *name) { if (!value) throw std::runtime_error(name); } }
int main() {
	try {
		using namespace MagEngine;
		WindowSurfaceState state;
		state.Request({1600, 900}); state.Request({1024, 768});
		Check(state.Pending() == SurfaceSize{1024, 768}, "LatestResizeWins");
		state.Commit({1024, 768}); state.Request({1024, 768});
		Check(!state.Pending(), "SameAppliedSizeDoesNotResize");
		state.Request({0, 0});
		Check(state.IsMinimized() && !state.Pending(), "MinimizedNeverProducesGpuSize");
		state.Request({800, 600});
		Check(!state.IsMinimized() && state.Pending() == SurfaceSize{800, 600}, "RestoreAppliesLatestPositiveSize");
		Check(state.Applied() == SurfaceSize{1024, 768}, "FailedApplyDoesNotCommitSize");
		std::cout << "WindowSurface PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
