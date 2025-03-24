#include "Physics.h"

UPhysics::UPhysics()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	manager = PxCreateControllerManager(*gScene);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
}

PxRigidActor* UPhysics::CreatePxRigidDynamicActor(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}

PxRigidActor* UPhysics::CreatePxRigidStaticActor(const PxTransform& t, const PxGeometry& geometry)
{
	PxRigidStatic* actor = PxCreateStatic(*gPhysics, t, geometry, *gMaterial);
	gScene->addActor(*actor);
	return actor;
}

PxController* UPhysics::CreateCharacterController(PxExtendedVec3 pos)
{
	PxCapsuleControllerDesc desc;

	desc.height = 1.5f;
	desc.radius = 0.25f;
	desc.position = pos + PxExtendedVec3(0,1,0);
	desc.contactOffset = 0.1f;
	desc.stepOffset = 0.5f;
	desc.slopeLimit = cosf(PxPi / 4); // 45 degrees
	desc.material = gPhysics->createMaterial(
    0.5f, // static friction
    0.5f, // dynamic friction
    0.5f  // restitution
	);

	return manager->createController(desc);
}




