#pragma once
#include <vector>
#include "MaterialData.h"
#include "VertexData.h"
#include "Matrix4x4.h"


/// @brief ノードデータ
/// @note モデルの階層構造を表すためのデータ構造
struct Node{
	Matrix4x4 localMatrix;	//ローカル行列
	std::string name;
	std::vector<Node> children;	//子ノード
};

/// @brief モデルデータ
struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
	Node rootNode;	//ルートノード
};