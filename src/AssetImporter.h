#pragma once

#include "CommonTypes.h"

#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Model;
struct Mesh;
struct Bone;
struct SceneNode;

class AssetImporter
{
	Assimp::Importer importer;

public:
	static AssetImporter& Get()
	{
		static AssetImporter instance;
		return instance;
	}

	void LoadModelFromFile(const char* path, Model &model, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, std::vector<std::string>& texturePaths);

	void LoadAnimatonToModel(const char* path, Model& model, std::string name);

	void LoadAnimation(const aiScene* scene, Model& model, std::string name);

	void ProcessNode(aiNode* node, const aiScene* scene, Model& model, SceneNode &sceneNode, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::string>& texturePaths, glm::mat4 parentTransform = glm::mat4(1.0f));

	void ProcessMesh(Mesh &mesh, aiMesh* assimpMesh, const aiScene* scene, Model& model, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::string>& texturePaths, glm::mat4 globalTransform);

	void ExtractBoneWeights(std::vector<Vertex>& meshVertices, aiMesh* assimpMesh, Model& model);

	glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from)
	{
		glm::mat4 to;

		to[0][0] = (float)from.a1; to[0][1] = (float)from.b1;  to[0][2] = (float)from.c1; to[0][3] = (float)from.d1;
		to[1][0] = (float)from.a2; to[1][1] = (float)from.b2;  to[1][2] = (float)from.c2; to[1][3] = (float)from.d2;
		to[2][0] = (float)from.a3; to[2][1] = (float)from.b3;  to[2][2] = (float)from.c3; to[2][3] = (float)from.d3;
		to[3][0] = (float)from.a4; to[3][1] = (float)from.b4;  to[3][2] = (float)from.c4; to[3][3] = (float)from.d4;

		return to;
	}
};
