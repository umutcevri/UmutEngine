#pragma once

#include "CommonTypes.h"

#define MAX_BONE_INFLUENCE 4

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;

	to[0][0] = (float)from->a1; to[0][1] = (float)from->b1;  to[0][2] = (float)from->c1; to[0][3] = (float)from->d1;
	to[1][0] = (float)from->a2; to[1][1] = (float)from->b2;  to[1][2] = (float)from->c2; to[1][3] = (float)from->d2;
	to[2][0] = (float)from->a3; to[2][1] = (float)from->b3;  to[2][2] = (float)from->c3; to[2][3] = (float)from->d3;
	to[3][0] = (float)from->a4; to[3][1] = (float)from->b4;  to[3][2] = (float)from->c4; to[3][3] = (float)from->d4;

	return to;
}

struct SceneNode
{
	std::string name;
	glm::mat4 transformation;
	std::vector<SceneNode> children;
};

struct Bone
{
	std::string name;
	int id;
	glm::mat4 offset;
};

struct PositionKey
{
	glm::vec3 value;
	double time;
};

struct RotationKey
{
	glm::quat value;
	double time;
};

struct ScalingKey
{
	glm::vec3 value;
	double time;
};

struct AnimationChannel
{
	std::string nodeName;
	std::vector<PositionKey> positionKeys;
	std::vector<RotationKey> rotationKeys;
	std::vector<ScalingKey> scalingKeys;
};

struct Animation
{
	double duration;
	double ticksPerSecond;
	std::unordered_map<std::string, AnimationChannel> channels;
};

struct Object
{
	std::vector<ObjectInstance> instances;

	std::vector<Mesh> meshes;

	std::vector<Animation> animations;

	std::unordered_map<std::string, Bone> bones;

	SceneNode sceneRoot;
};

class AssetManager
{
public:
	Assimp::Importer importer;

	std::vector<std::string> texturePaths;

	std::vector<Vertex> vertices;

	std::vector<uint32_t> indices;

	std::vector<Object> objects;

	//map to hold object names and their corresponding index in the objects vector
	std::unordered_map<std::string, size_t> objectMap;

	AssetManager()
	{
	}

	void LoadAsset(const char* path, std::string objectName, unsigned int assimpFlags = 0)
	{
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}


		std::cout << scene->mNumAnimations << " animations" << std::endl;

		Object object{};

		ProcessNode(scene->mRootNode, object.sceneRoot, scene, object);

		if (object.meshes.size() > 0)
		{
			if (scene->HasAnimations())
			{
				for (int i = 0; i < scene->mNumAnimations; i++)
				{
					aiAnimation* animation = scene->mAnimations[i];
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

					object.animations.push_back(anim);
				}
			}

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

			ObjectInstance instance;
			instance.model = transform;

			objects[it->second].instances.push_back(instance);
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
				std::cout << "No instances!" << std::endl;
				return;
			}

			if (instanceIndex >= objects[it->second].instances.size())
			{
				std::cout << "Instance index out of range!" << std::endl;
				return;
			}

			objects[it->second].instances.erase(objects[it->second].instances.begin() + instanceIndex);
		}
		else
		{
			std::cout << "Object not found!" << std::endl;
		}
	}

	Object* GetObject(std::string objectName)
	{
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			return &objects[it->second];
		}
		else
		{
			std::cout << "Object not found!" << std::endl;
			return nullptr;
		}
	}

	ObjectInstance* GetObjectInstance(std::string objectName, size_t instanceIndex)
	{
		auto it = objectMap.find(objectName);
		if (it != objectMap.end())
		{
			if (objects[it->second].instances.size() == 0)
			{
				std::cout << "No instances!" << std::endl;
				return nullptr;
			}

			if (instanceIndex >= objects[it->second].instances.size())
			{
				std::cout << "Instance index out of range!" << std::endl;
				return nullptr;
			}

			return &objects[it->second].instances[instanceIndex];
		}
		else
		{
			std::cout << "Object not found!" << std::endl;
			return nullptr;
		}
	}

	void SetObjectInstanceTransform(std::string objectName, size_t instanceIndex, glm::vec3 position = glm::vec3(0, 0, 0), glm::vec3 rotation = glm::vec3(0, 0, 0), glm::vec3 scale = glm::vec3(1, 1, 1))
	{
		ObjectInstance* instance = GetObjectInstance(objectName, instanceIndex);

		if (instance != nullptr)
		{
			glm::mat4 transform = glm::mat4(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
			transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
			transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
			transform = glm::scale(transform, scale);

			instance->model = transform;
		}
	}

	void ProcessNode(aiNode* node, SceneNode &sceneNode, const aiScene* scene, Object& object, const aiMatrix4x4& parentTransform = aiMatrix4x4())
	{
		sceneNode.name = node->mName.C_Str();
		sceneNode.transformation = aiMatrix4x4ToGlm(&node->mTransformation);

		aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			Mesh meshData = ProcessMesh(mesh, scene, nodeTransform, object);

			if (meshData.indexCount > 0)
			{
				object.meshes.push_back(meshData);
			}
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			sceneNode.children.push_back(SceneNode());

			ProcessNode(node->mChildren[i], sceneNode.children[i], scene, object, nodeTransform);
		}
	}

	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& nodeTransform, Object& object)
	{
		bool isRendered = true;

		Mesh meshData;

		meshData.isTransparent = false;

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
			// If no diffuse texture, check "CustomMaterialTextures.txt"
			std::ifstream file("assets/CustomMaterialTextures.txt");
			if (file.is_open())
			{
				std::string line;
				std::string _meshName = mesh->mName.C_Str();
				std::string _materialName = material->GetName().C_Str();

				while (std::getline(file, line))
				{
					std::istringstream iss(line);
					std::string meshName, matName, texType, texPath, isTransparent, renderMesh;
					if (!(iss >> meshName >> matName >> texType >> texPath >> isTransparent >> renderMesh))
						continue;

					if (meshName == _meshName && renderMesh == "No")
					{
						isRendered = false;
						break;
					}

					if (meshName == _meshName && (matName == _materialName || matName == "None") && texType == "Diffuse")
					{
						if (texPath.find("textures/") != 0) {
							texPath = "textures/" + texPath;
						}

						auto it = std::find(texturePaths.begin(), texturePaths.end(), texPath);

						if (it != texturePaths.end())
						{
							diffuseTextureID = std::distance(texturePaths.begin(), it);
						}
						else
						{
							texturePaths.push_back(texPath);
							diffuseTextureID = static_cast<int>(texturePaths.size()) - 1;
						}

						if (isTransparent == "Yes")
						{
							meshData.isTransparent = true;
						}

						break;
					}
				}
				file.close();
			}
			else
			{
				std::cerr << "Could not open CustomMaterialTextures.txt" << std::endl;
			}
		}

		if (!isRendered)
		{
			meshData.indexCount = 0;
			return meshData;
		}

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

		std::vector<Vertex> meshVertices;

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

			meshVertices.push_back(vertex);
		}

		ExtractBoneWeightForVertices(meshVertices, mesh, object);

		vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());

		return meshData;

	}

	void ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh* mesh, Object &object)
	{
		for (int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
		{
			//bone id in final scene bone array
			int boneID = -1;

			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

			//if bone does not exists
			if(object.bones.find(boneName) == object.bones.end())
			{
				Bone newBone;

				newBone.id = object.bones.size();
				newBone.name = boneName;
				newBone.offset = aiMatrix4x4ToGlm(&(mesh->mBones[boneIndex]->mOffsetMatrix));

				object.bones[boneName] = newBone;

				boneID = newBone.id;
			}
			else
			{
				boneID = object.bones[boneName].id;
			}

			aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
			{
				int vertexID = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				
				for (int i = 0; i < 4; i++)
				{
					//find empty bone slot and assign values
					if (vertices[vertexID].boneIDs[i] < 0)
					{
						vertices[vertexID].boneWeights[i] = weight;
						vertices[vertexID].boneIDs[i] = boneID;

						break;
					}
				}
			}
		}
	}

	void PlayAnimation(std::string objectName, size_t instanceIndex, size_t animationIndex)
	{
		ObjectInstance* instance = GetObjectInstance(objectName, instanceIndex);

		if (instance != nullptr)
		{
			Object* object = GetObject(objectName);

			if (object->animations.size() == 0)
			{
				std::cout << "Object has no animations!" << std::endl;
				return;
			}

			if (animationIndex >= object->animations.size())
			{
				std::cout << "Animation index out of range!" << std::endl;
				return;
			}

			instance->currentAnimation = animationIndex;
		}
	}

	glm::mat4 GetAnimationScalingMatrix(std::vector<ScalingKey>& keys, double currentAnimationTime)
	{
		int previousKeyIndex;
		int nextKeyIndex;

		//get indexes for previous and next keys relative to current time
		for (int index = 0; index < keys.size() - 1; index++)
		{
			if (currentAnimationTime < keys[index + 1].time)
			{
				previousKeyIndex = index;
				nextKeyIndex = index + 1;
				break;
			}
		}

		float mixFactor = (currentAnimationTime - keys[previousKeyIndex].time) / (keys[nextKeyIndex].time - keys[previousKeyIndex].time);

		glm::vec3 scaling = glm::mix(keys[previousKeyIndex].value, keys[nextKeyIndex].value, mixFactor);

		return glm::scale(glm::mat4(1.0f), scaling);
	}

	glm::mat4 GetAnimationRotationMatrix(std::vector<RotationKey>& keys, double currentAnimationTime)
	{
		int previousKeyIndex;
		int nextKeyIndex;
		//get indexes for previous and next keys relative to current time
		for (int index = 0; index < keys.size() - 1; index++)
		{
			if (currentAnimationTime < keys[index + 1].time)
			{
				previousKeyIndex = index;
				nextKeyIndex = index + 1;
				break;
			}
		}

		float mixFactor = (currentAnimationTime - keys[previousKeyIndex].time) / (keys[nextKeyIndex].time - keys[previousKeyIndex].time);

		glm::quat rotation = glm::slerp(keys[previousKeyIndex].value, keys[nextKeyIndex].value, mixFactor);

		return glm::toMat4(rotation);
	}

	glm::mat4 GetAnimationPositionMatrix(std::vector<PositionKey>& keys, double currentAnimationTime)
	{
		int previousKeyIndex;
		int nextKeyIndex;
		//get indexes for previous and next keys relative to current time
		for (int index = 0; index < keys.size() - 1; index++)
		{
			if (currentAnimationTime < keys[index + 1].time)
			{
				previousKeyIndex = index;
				nextKeyIndex = index + 1;
				break;
			}
		}
		float mixFactor = (currentAnimationTime - keys[previousKeyIndex].time) / (keys[nextKeyIndex].time - keys[previousKeyIndex].time);
		glm::vec3 position = glm::mix(keys[previousKeyIndex].value, keys[nextKeyIndex].value, mixFactor);
		return glm::translate(glm::mat4(1.0f), position);
	}

	glm::mat4 GetAnimationNodeTransform(AnimationChannel& channel, double currentAnimationTime)
	{
		glm::mat4 scaling = GetAnimationScalingMatrix(channel.scalingKeys, currentAnimationTime);
		glm::mat4 rotation = GetAnimationRotationMatrix(channel.rotationKeys, currentAnimationTime);
		glm::mat4 position = GetAnimationPositionMatrix(channel.positionKeys, currentAnimationTime);

		return position * rotation * scaling;
	}

	void UpdateBoneTransforms(Object* object, ObjectInstance* instance, SceneNode& sceneNode, glm::mat4 parentTransform)
	{
		std::string nodeName = sceneNode.name;
		glm::mat4 nodeTransform = sceneNode.transformation;

		if (object->animations[instance->currentAnimation].channels.find(nodeName) != object->animations[instance->currentAnimation].channels.end())
		{
			nodeTransform = GetAnimationNodeTransform(object->animations[instance->currentAnimation].channels[nodeName], instance->currentAnimationTime);
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;

		if (object->bones.find(nodeName) != object->bones.end())
		{
			int boneIndex = object->bones[nodeName].id;
			instance->boneTransforms[boneIndex] = globalTransform * object->bones[nodeName].offset;
		}

		for (int i = 0; i < sceneNode.children.size(); i++)
		{
			UpdateBoneTransforms(object, instance, sceneNode.children[i], globalTransform);
		}
	}

	void UpdateAnimationSystem(float deltaTime)
	{
		for (auto& object : objects)
		{
			for (auto& instance : object.instances)
			{
				if (instance.currentAnimation == -1)
				{
					continue;
				}

				instance.currentAnimationTime += object.animations[instance.currentAnimation].ticksPerSecond * deltaTime;

				instance.currentAnimationTime = fmod(instance.currentAnimationTime, object.animations[instance.currentAnimation].duration);

				UpdateBoneTransforms(&object, &instance, object.sceneRoot, glm::mat4(1.0f));
			}
		}
	}
};