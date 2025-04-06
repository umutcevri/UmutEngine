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
	glm::vec3 localPosition;
	glm::vec3 localRotation;
	glm::vec3 localScale;
};

struct AnimationInstance
{
	//animation asset name
	std::string name;
	float currentTime = 0.0f;
};

struct AnimationTransition 
{
	std::string fromState;
	std::string toState;
	float transitionTime;
	std::string condition = "";
};

struct AnimationMontage
{
	std::string name;
	AnimationInstance animation;
	float blendInDuration = 0.0f;
	float blendOutDuration = 0.0f;
	float currentBlendTime = 0.0f; // Tracks time for both blend-in and blend-out
	float playbackRate = 1.0f;
	float montageEndTime = 0.0f; // Calculated time when the animation clip ends

	bool isPlaying = false;
	bool isBlendingIn = false;
	bool isBlendingOut = false;
};


struct AnimationState 
{
	AnimationInstance animation;
	std::string name;
	bool isDefault = false;
	std::vector<AnimationTransition> transitions;
};

struct AnimationController 
{
	std::string name;

	std::map<std::string, AnimationState> states;
	std::map<std::string, AnimationMontage> montages;

	std::string activeMontage;

	std::string currentState;
	std::string targetState;

	float blendFactor = 0.0f;
	float transitionTime = 0.0f;
	float currentTransitionDuration = 0.0f;

	float activeMontageBlendTime = 0.0f;
	float montageBlendFactor = 0.0f; // Tracks the blending factor for montages

	float isMontageBlendingIn = false;
	float isMontageBlendingOut = false;


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

	void LoadAnimationToModel(const std::string& path, const std::string& modelName, const std::string& animName);

	void UpdateBoneTransforms(AnimationInstance& anim, Model& model, SceneNode &sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform);

	void UpdateBoneTransforms(AnimationInstance& anim, AnimationInstance& anim2, Model& model, SceneNode& sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform, float blendFactor);

	void UpdateBoneTransforms(std::vector<AnimationInstance> animations, Model& model, SceneNode& sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform, std::vector<float> blendFactors);

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

	void PlayAnimationMontage(entt::entity entity, const std::string& montageName);
};
