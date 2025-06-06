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

		bool customMaterialTextures = false;

		if (model.contains("customMaterialTextures"))
		{
			customMaterialTextures = true;
		}

		SceneManager::Get().LoadModelFromFile(model["file"], model["name"], customMaterialTextures);
	}
	for (auto& animation : scene["assets"]["animations"]) {
		SceneManager::Get().LoadAnimationToModel(animation["file"], animation["modelName"], animation["name"]);
	}

	// Create entities
	for (auto& entityData : scene["entities"]) {
		entt::entity entity = registry.create();

		entityMap[entityData["name"]] = entity;

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

			if (components.contains("MeshSocketComponent"))
			{
				auto meshSocket = components["MeshSocketComponent"];

				auto socketTransform = meshSocket["socketTransform"];

				registry.emplace<MeshSocketComponent>(
					entity,
					MeshSocketComponent{
						meshSocket["parentEntityName"],
						meshSocket["nodeName"],
						glm::vec3(socketTransform["position"][0], socketTransform["position"][1], socketTransform["position"][2]),
						glm::vec3(socketTransform["rotation"][0], socketTransform["rotation"][1], socketTransform["rotation"][2]),
						glm::vec3(socketTransform["scale"][0], socketTransform["scale"][1], socketTransform["scale"][2])
					}
				);
			}

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

void SceneManager::LoadModelFromFile(const std::string& path, const std::string& modelName, bool customMaterialTextures)
{
	Model& model = models[modelName];

	model.name = modelName;

	model.customMaterialTextures = customMaterialTextures;

	AssetImporter::Get().LoadModelFromFile(path.c_str(), model, vertices, indices, texturePaths);
}

void SceneManager::LoadAnimationToModel(const std::string& path, const std::string& modelName, const std::string& animName)
{
	AssetImporter::Get().LoadAnimatonToModel(path.c_str(), models[modelName], animName);
}

void SceneManager::UpdateBoneTransforms(std::vector<AnimationInstance> animations, Model& model, SceneNode** sceneNode, BoneTransformData& boneTransforms, glm::mat4 parentTransform, std::vector<float> blendFactors)
{
	std::string nodeName = (*sceneNode)->name;

	glm::mat4 nodeLocalTransform = (*sceneNode)->localTransform;

	glm::vec3 position(0.0f);
	glm::quat rotation(0.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scaling(0.0f);

	bool foundAnyNode = false;

	for (size_t i = 0; i < animations.size(); i++)
	{
		Animation& animAsset = model.animations[animations[i].name];

		if (animAsset.channels.find(nodeName) != animAsset.channels.end())
		{
			position += GetAnimationPosition(animAsset.channels[nodeName].positionKeys, animations[i].currentTime) * blendFactors[i];

			glm::quat animRotation = GetAnimationRotation(animAsset.channels[nodeName].rotationKeys, animations[i].currentTime);

			if (glm::dot(animRotation, rotation) < 0.0f)
				animRotation = -animRotation;

			rotation += animRotation * blendFactors[i];

			scaling += GetAnimationScaling(animAsset.channels[nodeName].scalingKeys, animations[i].currentTime) * blendFactors[i];

			foundAnyNode = true;
		}
	}

	if (foundAnyNode)
	{
		nodeLocalTransform = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scaling);
	}

	glm::mat4 globalTransform = parentTransform * nodeLocalTransform;

	(*sceneNode)->globalTransform = globalTransform;

	if (model.boneMap.find(nodeName) != model.boneMap.end())
	{
		int boneIndex = model.boneMap[nodeName].boneIndex;

		boneTransforms.boneTransforms[boneIndex] = globalTransform * model.boneMap[nodeName].offsetMatrix;
	}

	for (int i = 0; i < (*sceneNode)->children.size(); i++)
	{
		UpdateBoneTransforms(animations, model, &((*sceneNode)->children[i]), boneTransforms, globalTransform, blendFactors);
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

	if (currentTime > keys[keys.size() - 1].time)
	{
		return keys[keys.size() - 1].value;
	}
	else
	{
		return keys[0].value;
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

	if (currentTime > keys[keys.size() - 1].time)
	{
		return keys[keys.size() - 1].value;
	}
	else
	{
		return keys[0].value;
	}
}

glm::quat SceneManager::GetAnimationRotation(std::vector<RotationKey>& keys, double currentTime)
{
	for (size_t i = 0; i < keys.size() - 1; i++)
	{
		if (currentTime >= keys[i].time && currentTime <= keys[i + 1].time)
		{
			double deltaTime = keys[i + 1].time - keys[i].time;

			double factor = (currentTime - keys[i].time) / deltaTime;

			glm::quat rotation = glm::slerp(keys[i].value, keys[i + 1].value, static_cast<float>(factor));

			return rotation;
		}
	}

	if (currentTime > keys[keys.size() - 1].time)
	{
		return keys[keys.size() - 1].value;
	}
	else
	{
		return keys[0].value;
	}

}

void SceneManager::UpdateEntityInstances(EntityInstance* entityInstanceBuffer, std::map<std::string, std::vector<EntityInstance>>& modelInstanceMap)
{
	entt::basic_view view = registry.view<ModelComponent, TransformComponent>();

	//entities that are attached to a socket
	std::vector<entt::entity> socketEntities;

	for (entt::entity entity : view) {
		//check if entity has a socket component

		if (registry.all_of<MeshSocketComponent>(entity))
		{
			socketEntities.push_back(entity);
			continue;
		}

		ModelComponent& modelComp = view.get<ModelComponent>(entity);
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

		modelComp.modelMatrix = model;

		EntityInstance data{};
		data.model = model;
		data.boneTransformBufferIndex = modelComp.boneTransformBufferIndex;

		modelInstanceMap[modelComp.modelName].push_back(data);
	}

	for (entt::entity entity : socketEntities)
	{
		const MeshSocketComponent& socketComp = registry.get<MeshSocketComponent>(entity);

		const TransformComponent& transformComp = registry.get<TransformComponent>(entity);

		const ModelComponent& modelComp = registry.get<ModelComponent>(entity);

		const ModelComponent& parentModelComp = registry.get<ModelComponent>(entityMap[socketComp.parentEntityName]);

		Model& parentModel = models[parentModelComp.modelName];

		SceneNode* node = parentModel.sceneRoot;

		if (parentModel.nodeMap.find(socketComp.nodeName) != parentModel.nodeMap.end())
		{
			node = parentModel.nodeMap[socketComp.nodeName];
		}
		else
		{
			std::cout << "Node not found: " << socketComp.nodeName << std::endl;
			continue;
		}

		glm::mat4 model = parentModelComp.modelMatrix * node->globalTransform;

		model = glm::translate(model, socketComp.position);
		model = glm::rotate(model, glm::radians(socketComp.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(socketComp.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(socketComp.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, socketComp.scale);

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

		//set scale as 1

		model = glm::scale(model, glm::vec3(1 / parentModelComp.localScale.x, 1 / parentModelComp.localScale.y, 1 / parentModelComp.localScale.z));

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

		if (InputManager::Get().attack)
		{
			PlayAnimationMontage(entity, "Attack1");
		}

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

	if (!controller.activeMontage.empty())
	{
		if (controller.isMontageBlendingIn)
		{
			controller.activeMontageBlendTime += deltaTime;
			controller.montageBlendFactor = std::min(controller.activeMontageBlendTime / controller.montages[controller.activeMontage].blendInDuration, 1.0f);

			if (controller.activeMontageBlendTime >= controller.montages[controller.activeMontage].blendInDuration)
			{
				controller.isMontageBlendingIn = false;
				controller.activeMontageBlendTime = 0.0f;
			}
		}
		else if (!controller.isMontageBlendingOut)
		{
			AnimationMontage& montage = controller.montages[controller.activeMontage];

			AnimationInstance& montageAnim = controller.montages[controller.activeMontage].animation;

			Animation& montageAnimAsset = model.animations[montageAnim.name];

			if (montageAnim.currentTime >= montageAnimAsset.duration - (montage.blendOutDuration * montageAnimAsset.ticksPerSecond))
			{
				controller.isMontageBlendingOut = true;
				controller.isMontageBlendingIn = false;
				controller.activeMontageBlendTime = 0.0f;
			}
		}

		if (controller.isMontageBlendingOut)
		{
			controller.activeMontageBlendTime += deltaTime;
			controller.montageBlendFactor = std::max(1.0f - controller.activeMontageBlendTime / controller.montages[controller.activeMontage].blendOutDuration, 0.0f);

			

			if (controller.activeMontageBlendTime >= controller.montages[controller.activeMontage].blendOutDuration)
			{
				controller.isMontageBlendingOut = false;
				controller.activeMontageBlendTime = 0.0f;
				controller.activeMontage = "";
			}
		}
	}

	if (!controller.targetState.empty())
	{
		controller.transitionTime += deltaTime;
		controller.blendFactor = std::min(controller.transitionTime / controller.currentTransitionDuration, 1.0f);

		if (controller.blendFactor >= 1.0f)
		{
			controller.states.at(controller.currentState).animation.currentTime = 0;

			controller.currentState = controller.targetState;
			controller.targetState = "";
			controller.blendFactor = 0.0f;
			controller.transitionTime = 0.0f;
		}

		return;
	}

	// Check for transitions from current state
	const AnimationState& currentState = controller.states.at(controller.currentState);

	for (const auto& transition : currentState.transitions)
	{
		
		if (!transition.condition.empty() && animComp.parameters.count(transition.condition) && animComp.parameters[transition.condition] > 0.5f)
		{
			controller.targetState = transition.toState;

			std::cout << "Transitioning to: " << controller.currentState << " -> " << transition.toState << std::endl;

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

	std::vector<AnimationInstance> animations;
	std::vector<float> blendFactors;

	float montageBlendFactor = 0.0f;

	if (!controller.activeMontage.empty())
	{
		AnimationMontage& montage = controller.montages[controller.activeMontage];

		AnimationInstance& montageAnim = montage.animation;

		Animation& montageAnimAsset = model.animations[montageAnim.name];

		montageAnim.currentTime += montageAnimAsset.ticksPerSecond * deltaTime;
		montageAnim.currentTime = fmod(montageAnim.currentTime, montageAnimAsset.duration);

		montageBlendFactor = controller.montageBlendFactor;

		animations.push_back(montageAnim);
		blendFactors.push_back(montageBlendFactor);
	}

	AnimationInstance& currentAnim = controller.states.at(controller.currentState).animation;

	Animation& currentAnimAsset = model.animations[currentAnim.name];

	currentAnim.currentTime += currentAnimAsset.ticksPerSecond * deltaTime;
	currentAnim.currentTime = fmod(currentAnim.currentTime, currentAnimAsset.duration);

	animations.push_back(currentAnim);

	if (!controller.targetState.empty())
	{
		AnimationInstance& targetAnim = controller.states.at(controller.targetState).animation;

		Animation& targetAnimAsset = model.animations[targetAnim.name];

		targetAnim.currentTime += targetAnimAsset.ticksPerSecond * deltaTime;

		targetAnim.currentTime = fmod(targetAnim.currentTime, targetAnimAsset.duration);

		animations.push_back(targetAnim);

		float targetBlendFactor = controller.blendFactor * (1.0f - montageBlendFactor);
		float currentBlendFactor = (1.0f - controller.blendFactor) * (1.0f - montageBlendFactor);

		blendFactors.push_back(currentBlendFactor);
		blendFactors.push_back(targetBlendFactor);
	}
	else
	{
		blendFactors.push_back(1.0f - montageBlendFactor);
	}

	UpdateBoneTransforms(animations, model, &(model.sceneRoot), boneTransforms, glm::mat4(1.0f), blendFactors);
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

		if (stateJson["type"] == "State")
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
					transition.fromState = state.name;
					transition.toState = transitionJson["toState"];
					transition.transitionTime = transitionJson["transitionTime"];

					if (transitionJson.contains("condition")) {
						transition.condition = transitionJson["condition"];
					}

					state.transitions.push_back(transition);
				}
			}

			AnimationInstance animInstance;

			animInstance.name = stateJson["animationName"];

			state.animation = animInstance;

			controller.states[state.name] = state;
		}
		else if (stateJson["type"] == "Montage")
		{
			AnimationMontage montage;
			montage.name = stateJson["name"];
			montage.animation.name = stateJson["animationName"];
			montage.blendInDuration = stateJson["blendInDuration"];
			montage.blendOutDuration = stateJson["blendOutDuration"];

			controller.montages[montage.name] = montage;
		}
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

void SceneManager::PlayAnimationMontage(entt::entity entity, const std::string& montageName)
{
	//get anim comp if exists

	if (registry.all_of<AnimationComponent>(entity))
	{
		AnimationComponent& animComp = registry.get<AnimationComponent>(entity);
		
		AnimationController& controller = animComp.controller;

		//if controller contains montageName set current montage

		if (controller.montages.find(montageName) != controller.montages.end())
		{
			AnimationInstance& montageAnim = controller.montages[montageName].animation;

			montageAnim.currentTime = 0.0f;

			controller.activeMontage = montageName;
			controller.isMontageBlendingIn = true;
			controller.isMontageBlendingOut = false;
			controller.activeMontageBlendTime = 0.0f;
			controller.montageBlendFactor = 0.0f;
		}
		else
		{
			std::cout << "Montage not found: " << montageName << std::endl;
		}
	}
}

