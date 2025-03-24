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

struct AnimationTransition 
{
	std::string fromAnim;
	std::string toAnim;
	float transitionTime;
	bool onAnimationEnd = false;
	std::string condition = "";
};

struct AnimationState 
{
	std::string name;
	bool isDefault = false;
	std::vector<AnimationTransition> transitions;
};

struct AnimationController 
{
	std::string name;
	std::map<std::string, AnimationState> states;
	std::string currentState;
	std::string targetState;
	float blendFactor = 0.0f;
	float transitionTime = 0.0f;
	float currentTransitionDuration = 0.0f;
};

struct AnimationComponent
{
	AnimationController controller;
	std::map<std::string, float> parameters;

	uint16_t currentAnimationIndex = 0;

	float currentAnimationTime = 0;
};

struct RigidBodyComponent
{
	PxRigidActor* actor;
};

struct CharacterControllerComponent
{
	PxController* controller;
	float speed = 5.f;
	float targetYaw = 0;
	float rotationSpeed = 5.0f;
	bool jump = false;
	float jumpStrength = 6.0f;
	float gravity = 10.0f;
	float verticalVelocity = 0.0f;
	float terminalVelocity = 20.0f;
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

struct PlayerInputComponent
{
};

struct CameraComponent
{
	class Camera* camera;
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

	void UpdateBoneTransforms(Animation& anim, Animation& anim2, Model& model, SceneNode& sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform, float blendFactor);

	glm::vec3 GetAnimationPosition(std::vector<PositionKey>& keys, double currentTime);

	glm::vec3 GetAnimationScaling(std::vector<ScalingKey>& keys, double currentTime);

	glm::quat GetAnimationRotation(std::vector<RotationKey>& keys, double currentTime);

	void UpdateEntityInstances(EntityInstance* entityInstanceBuffer, std::map<std::string, std::vector<EntityInstance>>& modelInstanceMap);

	void UpdatePhysicsActors(float deltaTime);

	void UpdateAnimationSystem(BoneTransformData* boneTransforms, float deltaTime);

	void UpdateCameraSystem(float deltaTime, std::vector<class Camera*> &cameras);

	void ProcessAnimationController(Model& model, AnimationComponent& animComp, float deltaTime);

	void UpdateAnimationsWithBlending(Model& model, AnimationComponent& animComp, BoneTransformData& boneTransforms, float deltaTime);

	AnimationController LoadAnimationController(const std::string& filepath);

	void SetAnimationParameter(entt::entity entity, const std::string& paramName, float value);
};
