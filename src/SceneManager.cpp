#include "SceneManager.h"

#include "AssetImporter.h"

#include <iostream>

#include <fstream>

#include "json.hpp"

using json = nlohmann::json;

void SceneManager::LoadScene(const std::string& path)
{
	std::ifstream file(path);
	json scene;
	file >> scene;

	// Load assets
	for (auto& model : scene["assets"]["models"]) {
		SceneManager::Get().LoadModelFromFile(model["file"], model["id"]);
	}
	for (auto& animation : scene["assets"]["animations"]) {
		SceneManager::Get().LoadAnimationToModel(animation["file"], animation["modelId"]);
	}

	// Create entities
	for (auto& entityData : scene["entities"]) {
		entt::entity entity = registry.create();
		auto& components = entityData["components"];

		// Add TransformComponent
		auto transform = components["TransformComponent"];
		registry.emplace<TransformComponent>(
			entity,
			TransformComponent{
				glm::vec3(transform["position"][0], transform["position"][1], transform["position"][2]),
				glm::vec3(transform["rotation"][0], transform["rotation"][1], transform["rotation"][2]),
				glm::vec3(transform["scale"][0], transform["scale"][1], transform["scale"][2])
			}
		);

		// Add ModelComponent
		auto modelComponent = components["ModelComponent"];
		registry.emplace<ModelComponent>(entity, ModelComponent{ modelComponent["modelId"] });

		// Optionally add AnimationComponent if present
		if (components.contains("AnimationComponent")) {
			registry.emplace<AnimationComponent>(entity, AnimationComponent{});
		}
	}
}

void SceneManager::LoadModelFromFile(const std::string& path, const std::string& modelName)
{
	AssetImporter::Get().LoadModelFromFile(path.c_str(), models[modelName], vertices, indices, texturePaths);
}

void SceneManager::LoadAnimationToModel(const std::string& path, const std::string& modelName)
{
	AssetImporter::Get().LoadAnimatonToModel(path.c_str(), models[modelName]);
}

void SceneManager::UpdateAnimations(Model& model, Animation& anim, BoneTransformData& boneTransforms, float deltaTime)
{
	anim.currentTime += anim.ticksPerSecond * deltaTime;

	anim.currentTime = fmod(anim.currentTime, anim.duration);

	UpdateBoneTransforms(anim, model, model.sceneRoot, boneTransforms, glm::mat4(1.0f));
}

void SceneManager::UpdateBoneTransforms(Animation& anim, Model& model, SceneNode& sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform)
{
	std::string nodeName = sceneNode.name;

	glm::mat4 nodeLocalTransform = sceneNode.localTransform;

	if (anim.channels.find(nodeName) != anim.channels.end())
	{
		nodeLocalTransform = GetBoneTransform(anim.channels[nodeName], anim.currentTime);
	}

	glm::mat4 globalTransform = parentTransform * nodeLocalTransform;

	if (model.boneMap.find(nodeName) != model.boneMap.end())
	{
		int boneIndex = model.boneMap[nodeName].boneIndex;

		boneTransforms.boneTransforms[boneIndex] = globalTransform * model.boneMap[nodeName].offsetMatrix;
	}

	for (int i = 0; i < sceneNode.children.size(); i++)
	{
		UpdateBoneTransforms(anim, model, sceneNode.children[i], boneTransforms, globalTransform);
	}
}

glm::mat4 SceneManager::GetAnimationPositionMatrix(std::vector<PositionKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size() - 1; i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::vec3 position = glm::mix(keys[i].value, keys[i + 1].value, factor);

			return glm::translate(glm::mat4(1.0f), position);
		}
	}
}

glm::mat4 SceneManager::GetAnimationScalingMatrix(std::vector<ScalingKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size() - 1; i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::vec3 scaling = glm::mix(keys[i].value, keys[i + 1].value, factor);

			return glm::scale(glm::mat4(1.0f), scaling);
		}
	}
}

glm::mat4 SceneManager::GetAnimationRotationMatrix(std::vector<RotationKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size(); i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::quat rotation = glm::slerp(keys[i].value, keys[i + 1].value, static_cast<float>(factor));

			return glm::toMat4(rotation);
		}
	}

}

glm::mat4 SceneManager::GetBoneTransform(AnimationChannel& channel, double currentTime)
{
	return GetAnimationPositionMatrix(channel.positionKeys, currentTime) * GetAnimationRotationMatrix(channel.rotationKeys, currentTime) * GetAnimationScalingMatrix(channel.scalingKeys, currentTime);
}

void SceneManager::UpdateEntityInstances(EntityInstance* entityInstanceBuffer, std::map<std::string, std::vector<EntityInstance>>& modelInstanceMap)
{
	entt::basic_view view = registry.view<ModelComponent, TransformComponent>();

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

void SceneManager::UpdatePhysicsActors()
{
	entt::basic_view view = registry.view<TransformComponent, PhysicsRigidBodyComponent>();

	for (entt::entity entity : view)
	{
		TransformComponent& transformComp = view.get<TransformComponent>(entity);
		const PhysicsRigidBodyComponent& physicsComp = view.get<PhysicsRigidBodyComponent>(entity);

		PxTransform transform = physicsComp.actor->getGlobalPose();

		transformComp.position = glm::vec3(transform.p.x, transform.p.y, transform.p.z);

		transformComp.rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
	}
}

void SceneManager::UpdateAnimationSystem(BoneTransformData* boneTransforms, float deltaTime)
{
	int boneTransformBufferIndex = 0;

	entt::basic_view view = registry.view<ModelComponent, AnimationComponent>();
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

