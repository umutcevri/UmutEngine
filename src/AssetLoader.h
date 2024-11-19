#pragma once

#include "CommonTypes.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class AssetLoader
{
public:
	static void LoadAsset(const char* path, AssetData* assets, std::vector<std::string> &texturePaths)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		ProcessNode(scene->mRootNode, scene, assets, texturePaths);

		return;
	}

	static void ProcessNode(aiNode* node, const aiScene* scene, AssetData* assets, std::vector<std::string>& texturePaths, const aiMatrix4x4& parentTransform = aiMatrix4x4())
	{
		aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			ProcessMesh(mesh, scene, assets, texturePaths, nodeTransform);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, assets, texturePaths, nodeTransform);
		}
	}

	static void ProcessMesh(aiMesh* mesh, const aiScene* scene, AssetData* assets, std::vector<std::string>& texturePaths, const aiMatrix4x4& nodeTransform)
	{
		int diffuseTextureID = -1;

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		aiString name;
		material->Get(AI_MATKEY_NAME, name);

		aiColor3D color(0.f, 0.f, 0.f);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

		aiString _texturePath;

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			material->GetTexture(aiTextureType_DIFFUSE, 0, &_texturePath);

			std::string texturePath = _texturePath.C_Str();

			if (texturePath.find("textures/") != 0) {
				texturePath = "textures/" + texturePath;
			}

			auto it = std::find(texturePaths.begin(), texturePaths.end(), texturePath);

			if (it != texturePaths.end()) 
			{
				diffuseTextureID = std::distance(texturePaths.begin(), it);
			}
			else 
			{
				texturePaths.push_back(texturePath);

				diffuseTextureID = static_cast<int>(texturePaths.size()) - 1;
			}
		}


		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				assets->indices.push_back(face.mIndices[j] + static_cast<uint32_t>(assets->vertices.size()));
			}
		}

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D& vertexPos = mesh->mVertices[i];
			vertexPos = nodeTransform * vertexPos;

			Vertex vertex;
			vertex.position = glm::vec3(vertexPos.x, vertexPos.y, vertexPos.z);
			vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertex.uv_x = mesh->mTextureCoords[0][i].x;
			vertex.uv_y = mesh->mTextureCoords[0][i].y;

			vertex.diffuseTextureID = diffuseTextureID;
			
			vertex.color = glm::vec3(color.r, color.g, color.b);

			assets->vertices.push_back(vertex);
		}

	}
};