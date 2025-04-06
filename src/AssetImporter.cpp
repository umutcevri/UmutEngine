#include "AssetImporter.h"

#include "SceneTypes.h"

#include <iostream>

#include "json.hpp"

using json = nlohmann::json;


void AssetImporter::LoadModelFromFile(const char* path, Model& model, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::string>& texturePaths)
{
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	ProcessNode(scene->mRootNode, scene, model, model.sceneRoot, vertices, indices, texturePaths);

	LoadAnimation(scene, model, "");
}

void AssetImporter::LoadAnimatonToModel(const char* path, Model& model, std::string name)
{
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

	if (!scene || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	LoadAnimation(scene, model, name);
}

void AssetImporter::LoadAnimation(const aiScene* scene, Model& model, std::string name)
{
	if (scene->HasAnimations())
	{
		for (int i = 0; i < scene->mNumAnimations; i++)
		{
			aiAnimation* animation = scene->mAnimations[i];
			std::string animationName = animation->mName.C_Str();
			
			if (name == "")
			{
				name = animationName;
			}

			Animation anim;

			anim.duration = animation->mDuration;
			anim.ticksPerSecond = animation->mTicksPerSecond;
			
			for (int j = 0; j < animation->mNumChannels; j++)
			{
				aiNodeAnim* channel = animation->mChannels[j];
				AnimationChannel animChannel;
				animChannel.nodeName = channel->mNodeName.C_Str();
				for (int k = 0; k < channel->mNumPositionKeys; k++)
				{
					PositionKey key;
					key.time = channel->mPositionKeys[k].mTime;
					key.value = glm::vec3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z);
					animChannel.positionKeys.push_back(key);
				}
				for (int k = 0; k < channel->mNumRotationKeys; k++)
				{
					RotationKey key;
					key.time = channel->mRotationKeys[k].mTime;
					key.value = glm::quat(channel->mRotationKeys[k].mValue.w, channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z);
					animChannel.rotationKeys.push_back(key);
				}
				for (int k = 0; k < channel->mNumScalingKeys; k++)
				{
					ScalingKey key;
					key.time = channel->mScalingKeys[k].mTime;
					key.value = glm::vec3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z);
					animChannel.scalingKeys.push_back(key);
				}
				anim.channels[animChannel.nodeName] = animChannel;
			}
			model.animations[name] = anim;
		}
	}
}

void AssetImporter::ProcessNode(aiNode* node, const aiScene* scene, Model& model, SceneNode& sceneNode, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::string>& texturePaths, glm::mat4 parentTransform)
{
	sceneNode.localTransform = aiMatrix4x4ToGlm(node->mTransformation);

	sceneNode.name = node->mName.C_Str();

	glm::mat4 globalTransform = parentTransform * sceneNode.localTransform;

	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		model.meshes.resize(model.meshes.size() + 1);

		ProcessMesh(model.meshes.back(), mesh, scene, model, vertices, indices, texturePaths, globalTransform);
	}

	sceneNode.children.resize(node->mNumChildren);

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, model, sceneNode.children[i], vertices, indices, texturePaths, globalTransform);
	}
}

void AssetImporter::ProcessMesh(Mesh& mesh, aiMesh* assimpMesh, const aiScene* scene, Model& model, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<std::string>& texturePaths, glm::mat4 globalTransform)
{
	uint32_t startIndex = static_cast<uint32_t>(indices.size());

	for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		aiFace face = assimpMesh->mFaces[i];

		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j] + static_cast<uint32_t>(vertices.size()));
		}
	}

	uint32_t indexCount = static_cast<uint32_t>(indices.size()) - startIndex;

	mesh.startIndex = startIndex;
	mesh.indexCount = indexCount;

	aiMaterial* material = scene->mMaterials[assimpMesh->mMaterialIndex];
	aiColor3D color(0.f, 0.f, 0.f);
	material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

	std::cout << "Mesh name:" << assimpMesh->mName.C_Str() << std::endl;
	std::cout << material->GetName().C_Str() << std::endl;


	int diffuseTextureID = -1;

	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		aiString _texturePath;

		material->GetTexture(aiTextureType_DIFFUSE, 0, &_texturePath);

		std::string texturePath = _texturePath.C_Str();

		//append textures/ to the beginning of the texture path

		texturePath = "textures/" + texturePath;

		std::cout << "Texture Path: " << texturePath << std::endl;

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
	else
	{
		std::ifstream f("config/CustomMaterialTextures.json");
		json data = json::parse(f);

		for (auto& entry : data["entries"])
		{
			if (entry["materialName"] == material->GetName().C_Str() && entry["textureType"] == "diffuse")
			{
				std::string texturePath = entry["texturePath"];

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
		}
	}


	std::vector<Vertex> meshVertices;

	meshVertices.resize(assimpMesh->mNumVertices);

	for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		aiVector3D& vertexPos = assimpMesh->mVertices[i];

		Vertex& vertex = meshVertices[i];

		glm::vec3 position = glm::vec3(vertexPos.x, vertexPos.y, vertexPos.z);

		glm::vec4 transformedVertexPos = globalTransform * glm::vec4(position, 1.0f);

		vertex.position = position;

		vertex.normal = glm::vec3(assimpMesh->mNormals[i].x, assimpMesh->mNormals[i].y, assimpMesh->mNormals[i].z);

		if (assimpMesh->HasTextureCoords(0))
		{
			vertex.uv_x = assimpMesh->mTextureCoords[0][i].x;
			vertex.uv_y = assimpMesh->mTextureCoords[0][i].y;
		}
		else
		{
			vertex.uv_x = 0.0f;
			vertex.uv_y = 0.0f;
		}

		vertex.diffuseTextureID = diffuseTextureID;

		vertex.color = glm::vec3(color.r, color.g, color.b);
	}

	ExtractBoneWeights(meshVertices, assimpMesh, model);

	vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
}

void AssetImporter::ExtractBoneWeights(std::vector<Vertex>& meshVertices, aiMesh* assimpMesh, Model& model)
{
	
	for (unsigned int i = 0; i < assimpMesh->mNumBones; i++)
	{
		//bone index in final bone buffer
		int boneIndex = -1;

		std::string boneName = assimpMesh->mBones[i]->mName.C_Str();

		if (model.boneMap.find(boneName) == model.boneMap.end())
		{
			boneIndex = static_cast<int>(model.boneMap.size());

			Bone bone;

			bone.name = boneName;

			bone.boneIndex = boneIndex;

			bone.offsetMatrix = aiMatrix4x4ToGlm(assimpMesh->mBones[i]->mOffsetMatrix);

			model.boneMap[boneName] = bone;
		}
		else
		{
			boneIndex = model.boneMap[boneName].boneIndex;
		}

		aiVertexWeight* weights = assimpMesh->mBones[i]->mWeights;
		unsigned int numWeights = assimpMesh->mBones[i]->mNumWeights;


		for (unsigned int j = 0; j < numWeights; j++)
		{
			unsigned int vertexID = weights[j].mVertexId;

			float weight = static_cast<float>(weights[j].mWeight);

			bool found = false;

			for (int i = 0; i < 4; i++)
			{
				if (meshVertices[vertexID].boneIndices[i] < 0)
				{
					meshVertices[vertexID].boneWeights[i] = weight;
					meshVertices[vertexID].boneIndices[i] = boneIndex;

					found = true;

					break;
				}
				
			}
		}
	}
}

