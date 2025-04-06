#pragma once

#include "CommonTypes.h"

#include <vector>
#include <string>
#include <unordered_map>

struct PositionKey
{
	glm::vec3 value;
	double time;
};

struct RotationKey
{
	glm::quat value;
	double time;
};

struct ScalingKey
{
	glm::vec3 value;
	double time;
};

struct AnimationChannel
{
	std::string nodeName;
	std::vector<PositionKey> positionKeys;
	std::vector<RotationKey> rotationKeys;
	std::vector<ScalingKey> scalingKeys;
};

struct Animation
{
	std::string name;

	double duration = 0.0f;
	double ticksPerSecond = 0.0f;

	std::unordered_map<std::string, AnimationChannel> channels;
};

struct SceneNode
{
	std::string name;

	glm::mat4 localTransform;

	std::vector<SceneNode> children;
};

struct Bone 
{
	std::string name;

	//index in final bone buffer
	int boneIndex;

	glm::mat4 offsetMatrix;
};

struct Model
{
	std::vector<Mesh> meshes;

	//animation name to animation map
	std::unordered_map<std::string, Animation> animations;

	//bone name to bone map
	std::unordered_map<std::string, Bone> boneMap;

	SceneNode sceneRoot;
};
