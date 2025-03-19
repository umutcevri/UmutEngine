#include "SceneManager.h"

#include "AssetImporter.h"

#include <iostream>

void SceneManager::LoadModelFromFile(const std::string& path, const std::string& modelName)
{
	AssetImporter::Get().LoadModelFromFile(path.c_str(), models[modelName], vertices, indices);
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

