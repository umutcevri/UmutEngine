#pragma once

#include "SceneTypes.h"

const int MAX_BONES = 200;

struct EntityInstance
{
	glm::mat4 model;
	int boneTransformBufferIndex = -1;
	int padding[3];
};

struct BoneTransformData
{
	glm::mat4 boneTransforms[MAX_BONES];
};

class SceneManager
{
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<std::string> texturePaths;

	//map of model name to model
	std::unordered_map<std::string, Model> models;

	static SceneManager& Get()
	{
		static SceneManager instance;
		return instance;
	}

	void LoadModelFromFile(const std::string& path, const std::string& modelName);

	void LoadAnimationToModel(const std::string& path, const std::string& modelName);

	void UpdateAnimations(Model& model, Animation& anim, BoneTransformData& boneTransforms, float deltaTime);

	void UpdateBoneTransforms(Animation& anim, Model& model, SceneNode &sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform);

	glm::mat4 GetAnimationPositionMatrix(std::vector<PositionKey>& keys, double currentTime);

	glm::mat4 GetAnimationScalingMatrix(std::vector<ScalingKey>& keys, double currentTime);

	glm::mat4 GetAnimationRotationMatrix(std::vector<RotationKey>& keys, double currentTime);

	glm::mat4 GetBoneTransform(AnimationChannel &channel, double currentTime);

};
