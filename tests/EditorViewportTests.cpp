/*********************************************************************
 * \file EditorViewportTests.cpp
 * \brief 画像の余白と選択ray、境界箱の交差を検証する
 *********************************************************************/
#include "engine/editor/EditorViewportMath.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace { void Check(bool v, const char *m) { if (!v) { throw std::runtime_error(m); } } }
int main() {
	try {
		using namespace MagEngine;
		const auto rect = FitViewport({100, 50, 800, 800}, 16.0f / 9.0f);
		Check(rect.x == 100 && rect.y == 225 && rect.width == 800 && rect.height == 450, "LetterboxCoordinatesPickCorrectly");
		MagMath::Transform camera{{1, 1, 1}, {}, {0, 0, -10}};
		Check(!ScreenToViewportRay({100, 50}, rect, camera, 0.45f), "IgnoreLetterbox");
		Check(!ScreenToViewportRay({0, 0}, {}, camera, 0.45f), "ZeroViewportReturnsNoRay");
		const auto ray = ScreenToViewportRay({500, 450}, rect, camera, 0.45f);
		Check(ray && std::abs(ray->direction.z - 1) < 0.0001f, "center forward");
		const auto hit = IntersectEditorBounds(*ray, {{-1, -1, -1}, {1, 1, 1}}, {{1, 1, 1}, {}, {}});
		Check(hit && std::abs(*hit - 9) < 0.001f, "bounds intersection distance");
		camera.rotate.y = 1.57079632679f;
		const auto rotated = ScreenToViewportRay({500, 450}, rect, camera, 0.45f);
		Check(rotated && std::abs(rotated->direction.x - 1) < 0.0001f, "TransformUsesRadians");
		std::cout << "EditorViewport PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
