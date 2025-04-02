#include "SceneManager.h"

#include "AssetImporter.h"

#include <iostream>

#include <fstream>

#include "json.hpp"

#include "InputManager.h"

#include "ThirdPersonCamera.h"

#include "FreeCamera.h"

using json = nlohmann::json;

void SceneManager::LoadScene(const std::string& path)
{
	std::ifstream file(path);
	json scene = json::parse(file);

	// Load assets
	for (auto& model : scene["assets"]["models"]) {
		SceneManager::Get().LoadModelFromFile(model["file"], model["name"]);
	}
	for (auto& animation : scene["assets"]["animations"]) {
		SceneManager::Get().LoadAnimationToModel(animation["file"], animation["modelName"]);
	}

	// Create entities
	for (auto& entityData : scene["entities"]) {
		entt::entity entity = registry.create();
		auto& components = entityData["components"];

		if (components.contains("PlayerInputComponent"))
		{
			registry.emplace<PlayerInputComponent>(entity, PlayerInputComponent{});
		}

		// Add TransformComponent
		if (components.contains("TransformComponent"))
		{
			auto transform = components["TransformComponent"];
			registry.emplace<TransformComponent>(
				entity,
				TransformComponent{
					glm::vec3(transform["position"][0], transform["position"][1], transform["position"][2]),
					glm::vec3(transform["rotation"][0], transform["rotation"][1], transform["rotation"][2]),
					glm::vec3(transform["scale"][0], transform["scale"][1], transform["scale"][2])
				}
			);

			if (components.contains("DynamicRigidBody"))
			{
				auto rigidBody = components["DynamicRigidBody"];

				PxTransform t = PxTransform(PxVec3(transform["position"][0], transform["position"][1], transform["position"][2]));

				std::unique_ptr<PxGeometry> geometry;

				if (rigidBody["shape"]["type"] == "box")
				{
					geometry = std::make_unique<PxBoxGeometry>(
						PxVec3(rigidBody["shape"]["halfExtents"][0],
							rigidBody["shape"]["halfExtents"][1],
							rigidBody["shape"]["halfExtents"][2]));
				}
				else if (rigidBody["shape"]["type"] == "sphere")
				{
					geometry = std::make_unique<PxSphereGeometry>(rigidBody["shape"]["radius"]);
				}

				PxVec3 velocity = PxVec3(rigidBody["velocity"][0], rigidBody["velocity"][1], rigidBody["velocity"][2]);

				registry.emplace<RigidBodyComponent>(entity, RigidBodyComponent{ UPhysics::Get().CreatePxRigidDynamicActor(t, *geometry, velocity)});
			}
			else if (components.contains("StaticRigidBody"))
			{
				auto rigidBody = components["StaticRigidBody"];

				PxTransform t = PxTransform(PxVec3(transform["position"][0], transform["position"][1], transform["position"][2]));

				std::unique_ptr<PxGeometry> geometry;

				if (rigidBody["shape"]["type"] == "box")
				{
					geometry = std::make_unique<PxBoxGeometry>(
						PxVec3(rigidBody["shape"]["halfExtents"][0],
							rigidBody["shape"]["halfExtents"][1],
							rigidBody["shape"]["halfExtents"][2]));
				}
				else if (rigidBody["shape"]["type"] == "sphere")
				{
					geometry = std::make_unique<PxSphereGeometry>(rigidBody["shape"]["radius"]);
				}

				registry.emplace<RigidBodyComponent>(entity, RigidBodyComponent{ UPhysics::Get().CreatePxRigidStaticActor(t, *geometry) });
			}
			else if (components.contains("CharacterController"))
			{
				auto controller = components["CharacterController"];

				PxExtendedVec3 pos = PxExtendedVec3(transform["position"][0], transform["position"][1], transform["position"][2]);
				
				registry.emplace<CharacterControllerComponent>(entity, CharacterControllerComponent{ UPhysics::Get().CreateCharacterController(pos), controller["speed"]});
			}

			if (components.contains("ModelComponent"))
			{
				auto modelComponent = components["ModelComponent"];

				ModelComponent modelComp;
				modelComp.modelName = modelComponent["modelName"];

				if (modelComponent.contains("localTransform"))
				{
					auto localTransform = modelComponent["localTransform"];
					modelComp.localPosition = glm::vec3(localTransform["position"][0], localTransform["position"][1], localTransform["position"][2]);
					modelComp.localRotation = glm::vec3(localTransform["rotation"][0], localTransform["rotation"][1], localTransform["rotation"][2]);
					modelComp.localScale = glm::vec3(localTransform["scale"][0], localTransform["scale"][1], localTransform["scale"][2]);
				}

				registry.emplace<ModelComponent>(entity, modelComp);

				if (components.contains("AnimationComponent")) {
					AnimationComponent animComp;

					if (components["AnimationComponent"].contains("animControllerPath"))
					{
						animComp.controller = LoadAnimationController(components["AnimationComponent"]["animControllerPath"]);
					}

					registry.emplace<AnimationComponent>(entity, animComp);
				}
			}
		}

		if (components.contains("CameraComponent"))
		{
			auto cameraComponent = components["CameraComponent"];
			if (cameraComponent["type"] == "thirdPerson")
			{
				float armLength = cameraComponent["armLength"];

				glm::vec3 offset = glm::vec3(cameraComponent["offset"][0], cameraComponent["offset"][1], cameraComponent["offset"][2]);

				registry.emplace<CameraComponent>(entity, CameraComponent{ new ThirdPersonCamera(armLength, offset) });
			}
			else if (cameraComponent["type"] == "free")
			{
				registry.emplace<CameraComponent>(entity, CameraComponent{ new FreeCamera() });
			}
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
		nodeLocalTransform = glm::translate(glm::mat4(1.0f), GetAnimationPosition(anim.channels[nodeName].positionKeys, anim.currentTime)) *
			glm::toMat4(GetAnimationRotation(anim.channels[nodeName].rotationKeys, anim.currentTime)) *
			glm::scale(glm::mat4(1.0f), GetAnimationScaling(anim.channels[nodeName].scalingKeys, anim.currentTime));
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

void SceneManager::UpdateBoneTransforms(Animation& anim, Animation& anim2, Model& model, SceneNode& sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform, float blendFactor)
{
	//blend position, rotation, and scaling individually

	std::string nodeName = sceneNode.name;

	glm::mat4 nodeLocalTransform = sceneNode.localTransform;

	if (anim.channels.find(nodeName) != anim.channels.end() && anim2.channels.find(nodeName) != anim2.channels.end())
	{
		glm::vec3 position = glm::mix(GetAnimationPosition(anim.channels[nodeName].positionKeys, anim.currentTime),
			GetAnimationPosition(anim2.channels[nodeName].positionKeys, anim2.currentTime), blendFactor);

		glm::quat rotation = glm::slerp(GetAnimationRotation(anim.channels[nodeName].rotationKeys, anim.currentTime), GetAnimationRotation(anim2.channels[nodeName].rotationKeys, anim2.currentTime), blendFactor);

		glm::vec3 scaling = glm::mix(GetAnimationScaling(anim.channels[nodeName].scalingKeys, anim.currentTime),
			GetAnimationScaling(anim2.channels[nodeName].scalingKeys, anim2.currentTime), blendFactor);

		nodeLocalTransform = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scaling);
	}

	glm::mat4 globalTransform = parentTransform * nodeLocalTransform;

	if (model.boneMap.find(nodeName) != model.boneMap.end())
	{
		int boneIndex = model.boneMap[nodeName].boneIndex;

		boneTransforms.boneTransforms[boneIndex] = globalTransform * model.boneMap[nodeName].offsetMatrix;
	}

	for (int i = 0; i < sceneNode.children.size(); i++)
	{
		UpdateBoneTransforms(anim, anim2, model, sceneNode.children[i], boneTransforms, globalTransform, blendFactor);
	}
}

glm::vec3 SceneManager::GetAnimationPosition(std::vector<PositionKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size() - 1; i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::vec3 position = glm::mix(keys[i].value, keys[i + 1].value, factor);

			return position;
		}
	}
}

glm::vec3 SceneManager::GetAnimationScaling(std::vector<ScalingKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size() - 1; i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::vec3 scaling = glm::mix(keys[i].value, keys[i + 1].value, factor);

			return scaling;
		}
	}
}

glm::quat SceneManager::GetAnimationRotation(std::vector<RotationKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size(); i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::quat rotation = glm::slerp(keys[i].value, keys[i + 1].value, static_cast<float>(factor));

			return rotation;
		}
	}

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

		model = glm::translate(model, modelComp.localPosition);
		model = glm::rotate(model, glm::radians(modelComp.localRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(modelComp.localRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(modelComp.localRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, modelComp.localScale);


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

void SceneManager::UpdatePhysicsActors(float deltaTime)
{
	entt::basic_view view = registry.view<TransformComponent, RigidBodyComponent>();

	for (entt::entity entity : view)
	{
		TransformComponent& transformComp = view.get<TransformComponent>(entity);
		const RigidBodyComponent& rigidBodyComp = view.get<RigidBodyComponent>(entity);

		PxTransform transform = rigidBodyComp.actor->getGlobalPose();

		transformComp.position = glm::vec3(transform.p.x, transform.p.y, transform.p.z);

		transformComp.rotation = glm::eulerAngles(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
	}

	entt::basic_view view2 = registry.view<TransformComponent, CharacterControllerComponent, PlayerInputComponent>();
	for (entt::entity entity : view2)
	{
		CharacterControllerComponent& controllerComp = view2.get<CharacterControllerComponent>(entity);

		PxControllerState state;
		controllerComp.controller->getState(state);

		// Check if the controller is grounded
		bool isGrounded = (state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) != 0;

		if (isGrounded)
		{
			if (InputManager::Get().jump)
			{
				controllerComp.verticalVelocity = controllerComp.jumpStrength;
				SetAnimationParameter(entity, "jump", 1.0f);
				SetAnimationParameter(entity, "isGrounded", 0.0f);
			}
			else
			{
				SetAnimationParameter(entity, "jump", 0.0f);
				SetAnimationParameter(entity, "isGrounded", 1.0f);
			}
		}
		else
		{
			SetAnimationParameter(entity, "jump", 0.0f);
			SetAnimationParameter(entity, "isGrounded", 0.0f);
		}

		controllerComp.verticalVelocity -= controllerComp.gravity * deltaTime;

		if (controllerComp.verticalVelocity < -controllerComp.terminalVelocity)
		{
			controllerComp.verticalVelocity = -controllerComp.terminalVelocity;
		}

		//get forward and right vector of the camera

		glm::vec3 forward(0, 0, 1);
		glm::vec3 right(1, 0, 0);

		if (Camera* camera = registry.get<CameraComponent>(entity).camera)
		{
			forward = camera->Front;
			right = camera->Right;
			forward.y = 0;
			right.y = 0;
			forward = glm::normalize(forward);
			right = glm::normalize(right);
		}

		glm::vec2 moveInput = InputManager::Get().moveInput;

		glm::vec3 moveDirection = forward * moveInput.y + right * moveInput.x;

		if (glm::length(moveDirection) > 0.0001f)
		{
			SetAnimationParameter(entity, "isMoving", 1.0f);
			SetAnimationParameter(entity, "notIsMoving", 0.0f);

			moveDirection = glm::normalize(moveDirection);
		}
		else
		{
			SetAnimationParameter(entity, "isMoving", 0.0f);
			SetAnimationParameter(entity, "notIsMoving", 1.0f);
		}

		glm::vec3 displacement = (glm::vec3(0, controllerComp.verticalVelocity, 0) + moveDirection * view2.get<CharacterControllerComponent>(entity).speed) * deltaTime;

		PxVec3 disp = PxVec3(displacement.x, displacement.y, displacement.z);

		TransformComponent& transformComp = view2.get<TransformComponent>(entity);

		

		controllerComp.controller->move(disp, 0.01f, deltaTime, PxControllerFilters());

		PxExtendedVec3 pos = controllerComp.controller->getFootPosition();
		transformComp.position = glm::vec3(pos.x, pos.y, pos.z);

		if (glm::length(moveDirection) > 0.0001f)
		{
			float yaw = atan2(moveDirection.x, moveDirection.z);

			controllerComp.targetYaw = glm::degrees(yaw);
		}

		float currentYaw = transformComp.rotation.y;

		float deltaYaw = glm::mod((controllerComp.targetYaw - currentYaw + 540.0f), 360.0f) - 180.0f;

		if (glm::abs(deltaYaw) > 0.0001f)
		{
			float smoothedYaw = currentYaw + deltaYaw * glm::clamp(controllerComp.rotationSpeed * deltaTime, 0.0f, 1.0f);

			transformComp.rotation.y = smoothedYaw;
		}

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

		// Process animation controller
		ProcessAnimationController(model, animComp, deltaTime);

		// Update bone transforms
		modelComp.boneTransformBufferIndex = boneTransformBufferIndex;
		UpdateAnimationsWithBlending(model, animComp, boneTransforms[boneTransformBufferIndex], deltaTime);

		boneTransformBufferIndex++;
	}
}

void SceneManager::UpdateCameraSystem(float deltaTime, std::vector<Camera*>& cameras)
{
	entt::basic_view view = registry.view<CameraComponent>();
	for (entt::entity entity : view)
	{
		CameraComponent& cameraComp = view.get<CameraComponent>(entity);

		if (dynamic_cast<ThirdPersonCamera*>(cameraComp.camera))
		{
			cameraComp.camera->Position = registry.get<TransformComponent>(entity).position;
		}

		cameraComp.camera->Update(deltaTime);

		cameras.push_back(cameraComp.camera);
	}
}

void SceneManager::ProcessAnimationController(Model& model, AnimationComponent& animComp, float deltaTime)
{
	AnimationController& controller = animComp.controller;

	if (!controller.targetState.empty())
	{
		controller.transitionTime += deltaTime;
		controller.blendFactor = std::min(controller.transitionTime / controller.currentTransitionDuration, 1.0f);

		if (controller.blendFactor >= 1.0f)
		{
			Animation& currentAnim = model.animations.at(controller.currentState);

			currentAnim.currentTime = 0.0f;

			controller.currentState = controller.targetState;
			controller.targetState = "";
			controller.blendFactor = 0.0f;
			controller.transitionTime = 0.0f;
		}

		return;
	}

	Animation& currentAnim = model.animations.at(controller.currentState);

	float nextFrameTime = currentAnim.currentTime + (currentAnim.ticksPerSecond * deltaTime);

	// Check for transitions from current state
	const AnimationState& currentState = controller.states.at(controller.currentState);
	for (const auto& transition : currentState.transitions)
	{
		// End of animation transition
		if (transition.onAnimationEnd)
		{
			if (nextFrameTime >= (currentAnim.duration / currentAnim.ticksPerSecond) - transition.transitionTime)
			{
				std::cout << (currentAnim.duration / currentAnim.ticksPerSecond) << " " << transition.transitionTime << std::endl;


				std::cout << "Transitioning to: " << controller.currentState << " -> " << transition.toAnim << std::endl;

				controller.targetState = transition.toAnim;
				controller.currentTransitionDuration = transition.transitionTime;
				controller.transitionTime = 0.0f;
				controller.blendFactor = 0.0f;
				break;
			}
		}
		// Parameter transition
		else if (!transition.condition.empty() && animComp.parameters.count(transition.condition) && animComp.parameters[transition.condition] > 0.5f)
		{
			controller.targetState = transition.toAnim;

			std::cout << "Transitioning to: " << controller.currentState << " -> " << transition.toAnim << std::endl;

			controller.currentTransitionDuration = transition.transitionTime;
			controller.transitionTime = 0.0f;
			controller.blendFactor = 0.0f;
			break;
		}
	}
}

void SceneManager::UpdateAnimationsWithBlending(Model& model, AnimationComponent& animComp, BoneTransformData& boneTransforms, float deltaTime)
{
	AnimationController& controller = animComp.controller;

	Animation& currentAnim = model.animations.at(controller.currentState);

	currentAnim.currentTime += currentAnim.ticksPerSecond * deltaTime;
	currentAnim.currentTime = fmod(currentAnim.currentTime, currentAnim.duration);

	if (!controller.targetState.empty())
	{
		Animation& targetAnim = model.animations.at(controller.targetState);
		targetAnim.currentTime += targetAnim.ticksPerSecond * deltaTime;
		targetAnim.currentTime = fmod(targetAnim.currentTime, targetAnim.duration);

		// Calculate bone transforms for both animations
		UpdateBoneTransforms(currentAnim, targetAnim, model, model.sceneRoot, boneTransforms, glm::mat4(1.0f), controller.blendFactor);
	}
	else
	{
		// Just update bone transforms for current animation
		UpdateBoneTransforms(currentAnim, model, model.sceneRoot, boneTransforms, glm::mat4(1.0f));
	}
}

AnimationController SceneManager::LoadAnimationController(const std::string& filepath)
{
	AnimationController controller;

	std::ifstream file(filepath);
	if (!file.is_open())
	{
		// Handle error
		return controller;
	}

	json jsonData;
	file >> jsonData;

	controller.name = jsonData["name"];

	// Load states
	for (const auto& stateJson : jsonData["states"])
	{
		AnimationState state;
		state.name = stateJson["name"];

		if (stateJson.contains("isDefault"))
			state.isDefault = stateJson["isDefault"];

		// If this is the default state, set it as current
		if (state.isDefault)
			controller.currentState = state.name;

		// Load transitions
		if (stateJson.contains("transitions"))
		{
			for (const auto& transitionJson : stateJson["transitions"])
			{
				AnimationTransition transition;
				transition.fromAnim = state.name;
				transition.toAnim = transitionJson["toState"];
				transition.transitionTime = transitionJson["transitionTime"];

				if (transitionJson.contains("onAnimationEnd") && transitionJson["onAnimationEnd"]) {
					transition.onAnimationEnd = true;
				}
				else if (transitionJson.contains("condition")) {
					transition.condition = transitionJson["condition"];
				}

				state.transitions.push_back(transition);
			}
		}

		controller.states[state.name] = state;
	}

	// Ensure we have a current state
	if (controller.currentState.empty() && !controller.states.empty())
		controller.currentState = controller.states.begin()->first;

	return controller;
}

void SceneManager::SetAnimationParameter(entt::entity entity, const std::string& paramName, float value)
{
	//get anim comp if exists
	if (registry.all_of<AnimationComponent>(entity))
	{
		AnimationComponent& animComp = registry.get<AnimationComponent>(entity);
		animComp.parameters[paramName] = value;
	}
}

