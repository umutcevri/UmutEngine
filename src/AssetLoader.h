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

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

class Bone
{
private:
	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;
	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	glm::mat4 m_LocalTransform;
	std::string m_Name;
	int m_ID;

public:
	Bone(const std::string& name, int ID, const aiNodeAnim* channel)
		:
		m_Name(name),
		m_ID(ID),
		m_LocalTransform(1.0f)
	{
		m_NumPositions = channel->mNumPositionKeys;

		std::cout << "Num of positions" << m_NumPositions << std::endl;

		for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data;
			data.position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
			data.timeStamp = timeStamp;
			m_Positions.push_back(data);
		}

		m_NumRotations = channel->mNumRotationKeys;
		for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
			data.timeStamp = timeStamp;
			m_Rotations.push_back(data);
		}

		m_NumScalings = channel->mNumScalingKeys;
		for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			float timeStamp = channel->mScalingKeys[keyIndex].mTime;
			KeyScale data;
			data.scale = glm::vec3(scale.x, scale.y, scale.z);
			data.timeStamp = timeStamp;
			m_Scales.push_back(data);
		}
	}

	Bone(){}

	void Update(float animationTime)
	{
		glm::mat4 translation = InterpolatePosition(animationTime);
		glm::mat4 rotation = InterpolateRotation(animationTime);
		glm::mat4 scale = InterpolateScaling(animationTime);
		m_LocalTransform = translation * rotation * scale;
	}
	glm::mat4 GetLocalTransform() { return m_LocalTransform; }
	std::string GetBoneName() const { return m_Name; }
	int GetBoneID() { return m_ID; }

	int GetPositionIndex(float animationTime)
	{
		for (int index = 0; index < m_NumPositions - 1; ++index)
		{
			if (animationTime < m_Positions[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}

	int GetRotationIndex(float animationTime)
	{
		for (int index = 0; index < m_NumRotations - 1; ++index)
		{
			if (animationTime < m_Rotations[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}

	int GetScaleIndex(float animationTime)
	{
		for (int index = 0; index < m_NumScalings - 1; ++index)
		{
			if (animationTime < m_Scales[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}


private:

	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	glm::mat4 InterpolatePosition(float animationTime)
	{
		if (1 == m_NumPositions)
			return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

		int p0Index = GetPositionIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
			m_Positions[p1Index].timeStamp, animationTime);
		glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position
			, scaleFactor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	glm::mat4 InterpolateRotation(float animationTime)
	{
		if (1 == m_NumRotations)
		{
			auto rotation = glm::normalize(m_Rotations[0].orientation);
			return glm::toMat4(rotation);
		}

		int p0Index = GetRotationIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
			m_Rotations[p1Index].timeStamp, animationTime);
		glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation
			, scaleFactor);
		finalRotation = glm::normalize(finalRotation);
		return glm::toMat4(finalRotation);

	}

	glm::mat4 InterpolateScaling(float animationTime)
	{
		if (1 == m_NumScalings)
			return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

		int p0Index = GetScaleIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
			m_Scales[p1Index].timeStamp, animationTime);
		glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale
			, scaleFactor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}
};

struct BoneInfo
{
	int id;
	glm::mat4 offset;
};

struct Object
{
	std::vector<ObjectInstance> instances;
	std::vector<Mesh> meshes;
	const aiScene* objectAsset = nullptr;
	std::unordered_map<std::string, BoneInfo> boneInfoMap;
	std::vector<Bone> bones;
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
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | assimpFlags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		Object object{};

		object.objectAsset = scene;

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

	void ProcessNode(aiNode* node, const aiScene* scene, Object& object, const aiMatrix4x4& parentTransform = aiMatrix4x4())
	{
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
			ProcessNode(node->mChildren[i], scene, object, nodeTransform);
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

	void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		for (int i = 0; i < 4; i++)
		{
			if (vertex.boneIDs[i] < 0)
			{
				vertex.boneWeights[i] = weight;
				vertex.boneIDs[i] = boneID;
				break;
			}
		}
	}

	void ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh* mesh, Object &object)
	{
		for (int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (object.boneInfoMap.find(boneName) == object.boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = object.boneInfoMap.size();
				boneID = object.boneInfoMap.size();

				newBoneInfo.offset = aiMatrix4x4ToGlm(&(mesh->mBones[boneIndex]->mOffsetMatrix));
				object.boneInfoMap[boneName] = newBoneInfo;

				
			}
			else
			{
				boneID = object.boneInfoMap[boneName].id;
			}

			assert(boneID != -1);

			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;



			for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;



				assert(vertexId <= vertices.size());

				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}

	void PlayAnimation(std::string objectName, size_t instanceIndex, size_t animationIndex)
	{
		ObjectInstance* instance = GetObjectInstance(objectName, instanceIndex);

		if (instance != nullptr)
		{
			Object* object = GetObject(objectName);

			if (object->objectAsset->mNumAnimations == 0)
			{
				std::cout << "Object has no animations!" << std::endl;
				return;
			}

			if (animationIndex >= object->objectAsset->mNumAnimations)
			{
				std::cout << "Animation index out of range!" << std::endl;
				return;
			}

			const aiAnimation* animation = object->objectAsset->mAnimations[animationIndex];

			int numOfChannels = animation->mNumChannels;

			std::cout << "Num of channels: " << numOfChannels << std::endl;

			for (int i = 0; i < numOfChannels; i++)
			{
				const aiNodeAnim* channel = animation->mChannels[i];
				std::string channelName = channel->mNodeName.C_Str();

				int boneIndex = -1;

				if (object->boneInfoMap.find(channelName) == object->boneInfoMap.end())
				{
					boneIndex = object->boneInfoMap.size();
					object->boneInfoMap[channelName].id = object->boneInfoMap.size();
				}
				else
				{
					boneIndex = object->boneInfoMap[channelName].id;
				}

				std::cout << "Channel name" << channelName << std::endl;

				if (boneIndex >= object->bones.size())
				{
					object->bones.resize(boneIndex + 1);
				}

				object->bones[boneIndex] = Bone(channelName, boneIndex, channel);

			}

			instance->currentAnimation = animationIndex;
		}
	}

	void CalculateBoneTransform(Object* object, ObjectInstance* instance, aiNode* node, glm::mat4 parentTransform)
	{
		std::string nodeName = node->mName.C_Str();

		glm::mat4 nodeTransform = aiMatrix4x4ToGlm(&node->mTransformation);

		Bone* bone = nullptr;

		if (object->boneInfoMap.find(nodeName) != object->boneInfoMap.end())
		{
			bone = &object->bones[object->boneInfoMap[nodeName].id];
			bone->Update(instance->currentAnimationTime);
		}

		if (bone != nullptr)
		{
			nodeTransform = bone->GetLocalTransform();
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;

		if (object->boneInfoMap.find(nodeName) != object->boneInfoMap.end())
		{
			int boneIndex = object->boneInfoMap[nodeName].id;
			instance->boneTransforms[boneIndex] = globalTransform * object->boneInfoMap[nodeName].offset;
		}

		for (int i = 0; i < node->mNumChildren; i++)
		{
			CalculateBoneTransform(object, instance, node->mChildren[i], globalTransform);
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

				const aiAnimation* animation = object.objectAsset->mAnimations[instance.currentAnimation];

				instance.currentAnimationTime += animation->mTicksPerSecond * deltaTime;

				instance.currentAnimationTime = fmod(instance.currentAnimationTime, animation->mDuration);

				CalculateBoneTransform(&object, &instance, object.objectAsset->mRootNode, glm::mat4(1.0f));
			}
		}
	}


};