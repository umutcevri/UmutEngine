#pragma once

#include "CommonTypes.h"

class AssetLoader
{
public:
	static std::vector<std::string> transparentTextureSet;
	static bool isTransparentTexturesLoaded;

	static void LoadAsset(const char* path, AssetData* assets, std::vector<std::string> &texturePaths, unsigned int assimpFlags = 0)
	{
		if (!isTransparentTexturesLoaded)
		{
			std::ifstream infile("assets/TransparentTextures.txt");
			std::string line;
			while (std::getline(infile, line))
			{
				line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
				if (!line.empty())
					transparentTextureSet.push_back(line);
			}

			isTransparentTexturesLoaded = true;
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | assimpFlags);

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
		//print name of mesh
		std::cout << "Mesh name: " << mesh->mName.C_Str() << std::endl;
		
		//if mesh name has "Plane" in it, skip it
		if (std::string(mesh->mName.C_Str()).find("Plane") != std::string::npos) {
			return;
		}
		
		bool hasTransparentTexture = false;

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

			std::cout << "Texture path: " << texturePath << std::endl;

			if (texturePath.find("textures/") != 0) {
				texturePath = "textures/" + texturePath;
			}

			hasTransparentTexture = std::find(transparentTextureSet.begin(), transparentTextureSet.end(), texturePath) != transparentTextureSet.end();

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

			if (texturePath.find("claws_diffuse") != std::string::npos)
			{
				diffuseTextureID = -1;
			}
		}

		//print if has transparent texture
		std::cout << "Has transparent texture: " << hasTransparentTexture << std::endl;

		//hasTransparentTexture = true;

		auto& outVertices = hasTransparentTexture ? assets->verticesTransparent : assets->vertices;
		auto& outIndices = hasTransparentTexture ? assets->indicesTransparent : assets->indices;


		if (hasTransparentTexture)
		{
			std::cout << "num of faces: " << mesh->mNumFaces << std::endl;
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				outIndices.push_back(face.mIndices[j] + static_cast<uint32_t>(outVertices.size()));
			}
		}

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D& vertexPos = mesh->mVertices[i];

			//scale the node transform
			aiMatrix4x4 scaleMatrix;
			aiMatrix4x4::Scaling(aiVector3D(10, 10, 10), scaleMatrix);
			//nodeTransform = scaleMatrix * nodeTransform;

			//vertexPos = scaleMatrix * vertexPos;

			Vertex vertex;
			vertex.position = glm::vec3(vertexPos.x, vertexPos.y, vertexPos.z);
			vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

			if (mesh->HasTextureCoords(0))
			{
				vertex.uv_x = mesh->mTextureCoords[0][i].x;
				vertex.uv_y = mesh->mTextureCoords[0][i].y;
			}
			else
			{
				vertex.uv_x = 0.0f;
				vertex.uv_y = 0.0f;
			}

			vertex.diffuseTextureID = diffuseTextureID;
			
			vertex.color = glm::vec3(color.r, color.g, color.b);

			outVertices.push_back(vertex);
		}

	}
};