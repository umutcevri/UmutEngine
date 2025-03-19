#pragma once

#include "entt.hpp"

#include "SceneManager.h"
#include "Physics.h"

#include <iostream>

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

class ECS
{
	
public:
	entt::registry registry;

	static ECS& Get()
	{
		static ECS instance;
		return instance;
	}

	void UpdateEntityInstances(EntityInstance* entityInstanceBuffer, std::map<std::string, std::vector<EntityInstance>> &modelInstanceMap)
	{
		entt::basic_view view = ECS::Get().registry.view<ModelComponent, TransformComponent>();

		for (entt::entity entity : view) {
			const ModelComponent& modelComp = view.get<ModelComponent>(entity);
			const TransformComponent& transformComp = view.get<TransformComponent>(entity);

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, transformComp.position);
			model = glm::rotate(model, glm::radians(transformComp.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transformComp.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transformComp.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, transformComp.scale);

			EntityInstance data{};
			data.model = model;
			data.boneTransformBufferIndex = modelComp.boneTransformBufferIndex;

			modelInstanceMap[modelComp.modelName].push_back(data);
		}

		uint32_t instanceIndex = 0;

		for (const auto& pair : modelInstanceMap)
		{
			if (pair.second.size() == 0)
			{
				continue;
			}

			for (size_t i = 0; i < pair.second.size(); i++)
			{
				entityInstanceBuffer[instanceIndex] = pair.second[i];
				instanceIndex++;
			}
		}
	}

	void UpdatePhysicsActors()
	{
		entt::basic_view view = ECS::Get().registry.view<TransformComponent, PhysicsRigidBodyComponent>();

		for (entt::entity entity : view)
		{
			TransformComponent& transformComp = view.get<TransformComponent>(entity);
			const PhysicsRigidBodyComponent& physicsComp = view.get<PhysicsRigidBodyComponent>(entity);
			
			PxTransform transform = physicsComp.actor->getGlobalPose();

			transformComp.position = glm::vec3(transform.p.x, transform.p.y, transform.p.z);

			transformComp.rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
		}
	}

	void UpdateAnimationSystem(BoneTransformData* boneTransforms, float deltaTime)
	{
		int boneTransformBufferIndex = 0;

		entt::basic_view view = ECS::Get().registry.view<ModelComponent, AnimationComponent>();
		for (entt::entity entity : view)
		{
			ModelComponent& modelComp = view.get<ModelComponent>(entity);
			AnimationComponent& animComp = view.get<AnimationComponent>(entity);

			Model& model = SceneManager::Get().models[modelComp.modelName];
			Animation& anim = model.animations[animComp.currentAnimationIndex];

			modelComp.boneTransformBufferIndex = boneTransformBufferIndex;

			SceneManager::Get().UpdateAnimations(model, anim, boneTransforms[boneTransformBufferIndex], deltaTime);

			boneTransformBufferIndex++;
		}
	}
};