#pragma once
#include "MaterialData.h"
#include "Matrix4x4.h"
#include "VertexData.h"
#include <vector>

namespace MagMath {

	/// @brief ノードデータ
	/// @note モデルの階層構造を表すためのデータ構造
	struct Node {
		Matrix4x4 localMatrix; // ローカル行列
		std::string name;
		std::vector<Node> children; // 子ノード
	};

	/// @brief モデルデータ
	struct ModelData {
		std::vector<VertexData> vertices;
		MaterialData material;
		Node rootNode; // ルートノード
	};

} // namespace MagMath