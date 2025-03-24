#pragma once

#include "physx/PxPhysicsAPI.h"

using namespace physx;

class UPhysics
{
	PxDefaultAllocator gAllocator;
	PxDefaultErrorCallback	gErrorCallback;
	PxFoundation *gFoundation;
	PxPhysics *gPhysics;
	PxDefaultCpuDispatcher *gDispatcher;
	PxScene *gScene;
	PxMaterial *gMaterial;
	PxPvd *gPvd;
	PxControllerManager *manager;

public:
	static UPhysics& Get()
	{
		static UPhysics instance;
		return instance;
	}

	UPhysics();

	PxRigidActor* CreatePxRigidDynamicActor(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0));

	PxRigidActor* CreatePxRigidStaticActor(const PxTransform& t, const PxGeometry& geometry);

	PxController* CreateCharacterController(PxExtendedVec3 pos);

	void Update(float deltaTime)
	{
		gScene->simulate(deltaTime);
		gScene->fetchResults(true);
	}
};

