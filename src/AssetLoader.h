#pragma once

#include "CommonTypes.h"

class AssetManager
{
public:
	std::vector<std::string> transparentTextureSet;

	std::vector<std::string> texturePaths;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<Object> objects;

	//map to hold object names and their corresponding index in the objects vector
	std::unordered_map<std::string, size_t> objectMap;

	AssetManager()
	{
		std::ifstream infile("assets/TransparentTextures.txt");
		std::string line;
		while (std::getline(infile, line))
		{
			line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
			if (!line.empty())
				transparentTextureSet.push_back(line);
		}
	}

	void LoadAsset(const char* path, std::string objectName, unsigned int assimpFlags = 0)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | assimpFlags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		Object object{};

		ProcessNode(scene->mRootNode, scene, object);

		if (object.meshes.size() > 0)
		{
			objectMap[objectName] = objects.size();
			objects.push_back(object);
		}
	}

	void CreateObjectInstance(std::string objectName, glm::vec3 position = glm::vec3(0,0,0), glm::vec3 rotation = glm::vec3(0,0,0), glm::vec3 scale = glm::vec3(1,1,1))
	{
		// Check if the object exists in the objectMap
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			glm::mat4 transform = glm::mat4(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
			transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
			transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
			transform = glm::scale(transform, scale);

			objects[it->second].instances.push_back(transform);
		}
		else
		{
			std::cout << "Object not found" << std::endl;
		}
	}

	void DeleteObjectInstance(std::string objectName, size_t instanceIndex)
	{
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			if (objects[it->second].instances.size() == 0)
			{
				std::cout << "No instances to delete" << std::endl;
				return;
			}

			if (instanceIndex >= objects[it->second].instances.size())
			{
				std::cout << "Instance index out of range" << std::endl;
				return;
			}

			objects[it->second].instances.erase(objects[it->second].instances.begin() + instanceIndex);
		}
		else
		{
			std::cout << "Object not found" << std::endl;
		}
	}

	Object& GetObject(std::string objectName)
	{
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			return objects[it->second];
		}
		else
		{
			std::cout << "Object not found" << std::endl;
			return objects[0];
		}
	}

	void SetObjectInstanceTransform(std::string objectName, size_t instanceIndex, glm::vec3 position = glm::vec3(0, 0, 0), glm::vec3 rotation = glm::vec3(0, 0, 0), glm::vec3 scale = glm::vec3(1, 1, 1))
	{
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			if (objects[it->second].instances.size() == 0)
			{
				std::cout << "No instances to transform" << std::endl;
				return;
			}

			if (instanceIndex >= objects[it->second].instances.size())
			{
				std::cout << "Instance index out of range" << std::endl;
				return;
			}

			glm::mat4 transform = glm::mat4(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
			transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
			transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
			transform = glm::scale(transform, scale);

			objects[it->second].instances[instanceIndex] = transform;
		}
		else
		{
			std::cout << "Object not found" << std::endl;
		}
	}


	void ProcessNode(aiNode* node, const aiScene* scene, Object& object, const aiMatrix4x4& parentTransform = aiMatrix4x4())
	{
		aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			Mesh meshData = ProcessMesh(mesh, scene, nodeTransform);

			if (meshData.indexCount > 0)
			{
				object.meshes.push_back(meshData);
			}
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, object, nodeTransform);
		}
	}

	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& nodeTransform)
	{
		Mesh meshData;

		//print name of mesh
		std::cout << "Mesh name: " << mesh->mName.C_Str() << std::endl;
		
		//if mesh name has "Plane" in it, skip it
		if (std::string(mesh->mName.C_Str()).find("Plane") != std::string::npos) {
			return meshData;
		}
		
		bool hasTransparentTexture = false;

		int diffuseTextureID = -1;

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

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

		meshData.isTransparent = hasTransparentTexture;

		uint32_t startIndex = static_cast<uint32_t>(indices.size());

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j] + static_cast<uint32_t>(vertices.size()));
			}
		}

		uint32_t indexCount = static_cast<uint32_t>(indices.size()) - startIndex;

		meshData.startIndex = startIndex;
		meshData.indexCount = indexCount;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D& vertexPos = mesh->mVertices[i];

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

			vertices.push_back(vertex);
		}

		return meshData;

	}
};