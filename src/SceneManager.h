#pragma once

#include "SceneTypes.h"

#include "entt.hpp"

#include "Physics.h"

const int MAX_BONES = 200;

struct TransformComponent
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

struct ModelComponent
{
	std::string modelName;
	int boneTransformBufferIndex = -1;
};

struct AnimationComponent
{
	uint16_t currentAnimationIndex = 0;

	float currentAnimationTime = 0;
};

struct PhysicsRigidBodyComponent
{
	PxRigidActor* actor;
};

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
	entt::registry registry;


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

	void LoadScene(const std::string& path);

	void LoadModelFromFile(const std::string& path, const std::string& modelName);

	void LoadAnimationToModel(const std::string& path, const std::string& modelName);

	void UpdateAnimations(Model& model, Animation& anim, BoneTransformData& boneTransforms, float deltaTime);

	void UpdateBoneTransforms(Animation& anim, Model& model, SceneNode &sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform);

	glm::mat4 GetAnimationPositionMatrix(std::vector<PositionKey>& keys, double currentTime);

	glm::mat4 GetAnimationScalingMatrix(std::vector<ScalingKey>& keys, double currentTime);

	glm::mat4 GetAnimationRotationMatrix(std::vector<RotationKey>& keys, double currentTime);

	glm::mat4 GetBoneTransform(AnimationChannel &channel, double currentTime);

	void UpdateEntityInstances(EntityInstance* entityInstanceBuffer, std::map<std::string, std::vector<EntityInstance>>& modelInstanceMap);

	void UpdatePhysicsActors();

	void UpdateAnimationSystem(BoneTransformData* boneTransforms, float deltaTime);

};
