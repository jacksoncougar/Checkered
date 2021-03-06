//
// Created by root on 9/1/20.
//

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <Mesh.h>
#include <PhysicsActor.h>
#include <Vehicle.h>
#include <algorithm>
#include <iostream>

#include "Engine.h"
#include "Events/Events.h"
#include "FilterShader.h"
#include "PhysicsSystem.h"
#include "PxRigidStatic.h"
#include "VehicleFactory.hpp"
#include "Wheel.hpp"
#include "physicspacket.hpp"
#include "scenery.hpp"
#include "snippetvehiclecommon/SnippetVehicleTireFriction.h"
#include "tags.h"
#include <WorldTransform.h>

using namespace physx;
using namespace snippetvehicle;

const float GRAVITY = -12.81f;
const float STATIC_FRICTION = 0.9f;
const float DYNAMIC_FRICTION = 0.5f;
const float RESTITUTION = 0.3f;

Component::Passenger *activePassenger;

namespace {
  const char module[] = "Physics";
}

std::vector<PxVehicleDrive4W *> vehicles;

PxVehicleDrivableSurfaceToTireFrictionPairs *cFrictionPairs = NULL;

PxFoundation *cFoundation = NULL;

PxPvd *cPVD = NULL;
PxPvdTransport *cTransport = NULL;
PxCooking *cCooking = NULL;
PxCpuDispatcher *cDispatcher = NULL;
PxMaterial *cMaterial = NULL;
PxRigidStatic *cGroundPlane = NULL;
PxVehicleDrive4W *cVehicle4w = NULL;
VehicleSceneQueryData *cVehicleSceneQueryData = NULL;
PxBatchQuery *cBatchQuery = NULL;

std::set<int> keys;// we store key state between frames here...

bool cIsVehicleInAir = true;
bool cIsPassengerInVehicle = false;

static PxDefaultAllocator cDefaultAllocator;
static PxDefaultErrorCallback cErrorCallback;

std::map<physx::PxRigidDynamic *, std::shared_ptr<ComponentBase>> trackedComponents;

extern VehicleDesc initVehicleDesc();

VehicleDesc initVehicleDescription(bool is_player) {
  // Set up the chassis mass, dimensions, moment of inertia, and center of mass
  // offset. The moment of inertia is just the moment of inertia of a cuboid but
  // modified for easier steering. Center of mass offset is 0.65m above the base
  // of the chassis and 0.25m towards the front.
  const PxF32 chassisMass = 400.0f;
  const PxVec3 chassisDims(2.30239f, 2.17137f, 5.3818f);
  const PxVec3 chassisMOI(
      (chassisDims.y * chassisDims.y + chassisDims.z * chassisDims.z) * chassisMass / 12.0f,
      (chassisDims.x * chassisDims.x + chassisDims.z * chassisDims.z) * chassisMass / 12.0f,
      (chassisDims.x * chassisDims.x + chassisDims.y * chassisDims.y) * chassisMass / 12.0f);
  const PxVec3 chassisCMOffset(0.0f, -chassisDims.y * 0.5f, -0.25f);

  // Set up the wheel mass, radius, width, moment of inertia, and number of
  // wheels. Moment of inertia is just the moment of inertia of a cylinder.
  const PxF32 wheelMass = 30.0f;
  const PxF32 wheelRadius = 0.3231f;
  const PxF32 wheelWidth = 0.2234f;
  const PxF32 wheelMOI = 0.1f;
  const PxU32 nbWheels = 4;

  VehicleDesc vehicleDesc;

  vehicleDesc.chassisMass = chassisMass;
  vehicleDesc.chassisDims = chassisDims;
  vehicleDesc.chassisMOI = chassisMOI;
  vehicleDesc.chassisCMOffset = chassisCMOffset;
  vehicleDesc.chassisMaterial = cMaterial;
  if (is_player) {
    vehicleDesc.chassisSimFilterData = PxFilterData(Engine::FilterGroup::ePlayerVehicle,
                                                    Engine::FilterMask::ePlayerColliders, 0, 0);
  } else {
    vehicleDesc.chassisSimFilterData =
        PxFilterData(Engine::FilterGroup::eEnemyVehicle, Engine::FilterMask::eEnemyColliders, 0, 0);
  }

  vehicleDesc.wheelMass = wheelMass;
  vehicleDesc.wheelRadius = wheelRadius;
  vehicleDesc.wheelWidth = wheelWidth;
  vehicleDesc.wheelMOI = wheelMOI;
  vehicleDesc.numWheels = nbWheels;
  vehicleDesc.wheelMaterial = cMaterial;
  vehicleDesc.wheelSimFilterData =
      PxFilterData(Engine::FilterGroup::eWheel, Engine::FilterMask::eWheelColliders, 0, 0);

  return vehicleDesc;
}

void Physics::PhysicsSystem::initialize() {

  onKeyPressHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
      this, &Physics::PhysicsSystem::onKeyPress);
  onKeyDownHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
      this, &Physics::PhysicsSystem::onKeyDown);
  onKeyUpHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
      this, &Physics::PhysicsSystem::onKeyUp);
  onVehicleCreatedHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
      this, &Physics::PhysicsSystem::onVehicleCreated);
  onActorCreatedHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
      this, &Physics::PhysicsSystem::onActorCreated);

  createFoundation();
  createPVD();
  createPhysicsObject();
  createCooking();
  createScene();
  createGround();
  initVehicleSupport();
  createDrivablePlayerVehicle();
  createPhysicsCallbacks();

  std::cout << "Physics System Successfully Initialized" << std::endl;
}

using namespace Engine;

Engine::SimulationCallback scb;

void Physics::PhysicsSystem::createPhysicsCallbacks() {

  log<high>("Creating simulation callbacks");
  cScene->setSimulationEventCallback(&scb);
}

void Physics::PhysicsSystem::createFoundation() {

  cFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, cDefaultAllocator, cErrorCallback);
  if (!cFoundation) Engine::assertLog<module>(false, "PxCreateFoundation failed");
}

void Physics::PhysicsSystem::createPVD() {

  cPVD = PxCreatePvd(*cFoundation);
  cTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
  cPVD->connect(*cTransport, PxPvdInstrumentationFlag::eALL);
}

void Physics::PhysicsSystem::createPhysicsObject() {

  cPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *cFoundation, PxTolerancesScale(), true, cPVD);
  if (!cPhysics) Engine::assertLog<module>(false, "PxCreatePhysics Failed");
}

void Physics::PhysicsSystem::createCooking() {

  cCooking =
      PxCreateCooking(PX_PHYSICS_VERSION, *cFoundation, PxCookingParams(PxTolerancesScale()));
}

void Physics::PhysicsSystem::createScene() {

  PxSceneDesc sceneDesc(cPhysics->getTolerancesScale());
  sceneDesc.gravity = PxVec3(0.f, GRAVITY, 0.f);

  PxU32 numWorkers = 4;
  cDispatcher = PxDefaultCpuDispatcherCreate(numWorkers);
  sceneDesc.cpuDispatcher = cDispatcher;
  sceneDesc.filterShader = FilterShader::setupFilterShader;
  sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
  sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
  sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;
  cScene = cPhysics->createScene(sceneDesc);
}

void Physics::PhysicsSystem::createGround() {

  cMaterial = cPhysics->createMaterial(STATIC_FRICTION, DYNAMIC_FRICTION, RESTITUTION);
  // PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND,
  // COLLISION_FLAG_GROUND_AGAINST, 0, 0); cGroundPlane =
  // createDrivablePlane(groundPlaneSimFilterData, cMaterial, cPhysics);

  // cScene->addActor(*cGroundPlane);
}

void Physics::PhysicsSystem::initVehicleSupport() {

  cVehicleSceneQueryData = VehicleSceneQueryData::allocate(
      100, PX_MAX_NB_WHEELS, 1, 100, WheelSceneQueryPreFilterBlocking, nullptr, cDefaultAllocator);
  cBatchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *cVehicleSceneQueryData, cScene);

  PxInitVehicleSDK(*cPhysics);
  PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
  PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

  cFrictionPairs = createFrictionPairs(cMaterial);
}

void Physics::PhysicsSystem::createDrivablePlayerVehicle() {
  //    createDrivableVehicle(PxTransform(0,0,0));
}

// TODO DIFFERENTIATE BETWEEN ENEMY VEHICLE AND PLAYER VEHICLE FOR COLLIDERS
PxVehicleDrive4W *
Physics::PhysicsSystem::createDrivableVehicle(const PxTransform &worldTransform, bool is_player,
                                              PxConvexMesh *chassis_mesh,
                                              const PxTransform &chassis_local_transform) {

  PxVehicleDrive4W *pxVehicle;
  VehicleDesc vehicleDesc = initVehicleDescription(is_player);
  vehicleDesc.chassis = chassis_mesh;


  vehicles.push_back(createVehicle4W(vehicleDesc, cPhysics, cCooking, chassis_local_transform));
  pxVehicle = vehicles.back();

  PxTransform startTransform(
      PxVec3(0, (vehicleDesc.chassisDims.y * 0.5f + vehicleDesc.wheelRadius + 2.0f), -10),
      PxQuat(PxIdentity));

  pxVehicle->getRigidDynamicActor()->setGlobalPose(startTransform * worldTransform);

  cScene->addActor(*pxVehicle->getRigidDynamicActor());

  pxVehicle->setToRestState();

  pxVehicle->mDriveDynData.setUseAutoGears(true);
  pxVehicle->mDriveDynData.mAutoBoxSwitchTime = 0.5;

  PxVehicleEngineData engine;

  engine.mMOI = .5;
  engine.mPeakTorque = 900.0f;
  engine.mMaxOmega = 600.0f;
  engine.mDampingRateFullThrottle = 0.195f;
  engine.mDampingRateZeroThrottleClutchEngaged = 0.40f;
  engine.mDampingRateZeroThrottleClutchDisengaged = 0.35f;

  pxVehicle->mDriveSimData.setEngineData(engine);

  PxVehicleDifferential4WData diff;
  diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
  pxVehicle->mDriveSimData.setDiffData(diff);

  PxVehicleTireData tireData;
  tireData.mFrictionVsSlipGraph[0][0] = 0.f;
  tireData.mFrictionVsSlipGraph[0][1] = 0.4f;
  tireData.mFrictionVsSlipGraph[1][0] = 0.5f;
  tireData.mFrictionVsSlipGraph[1][1] = 1.0f;
  tireData.mFrictionVsSlipGraph[2][0] = 1.f;
  tireData.mFrictionVsSlipGraph[2][1] = 0.6f;
  tireData.mLongitudinalStiffnessPerUnitGravity = 150.f;
  tireData.mCamberStiffnessPerUnitGravity = 150.f;
  tireData.mLatStiffX = 2;
  tireData.mLatStiffY = 15;

  for (int i = 0; i < 3; i++) { pxVehicle->mWheelsSimData.setTireData(0, tireData); }

  PxVehicleAckermannGeometryData acker;
  acker.mAccuracy = 1.0f;
  acker.mFrontWidth = 2.30f;
  acker.mRearWidth = 2.30f;
  acker.mAxleSeparation = 3.37f;
  pxVehicle->mDriveSimData.setAckermannGeometryData(acker);

  PxVehicleClutchData clutch;

  clutch.mStrength = 10.0f;
  clutch.mAccuracyMode = PxVehicleClutchAccuracyMode::eESTIMATE;
  clutch.mEstimateIterations = 5;

  pxVehicle->mDriveSimData.setClutchData(clutch);

  PxVehicleGearsData gears;
  pxVehicle->mDriveSimData.setGearsData(gears);

  return pxVehicle;
}

void Physics::PhysicsSystem::stepPhysics(Engine::deltaTime timestep) {

  // update all vehicles in the scene.
  auto vehicles =
      getEngine()->getSubSystem<EngineStore>()->getRoot().getComponentsOfType<Component::Vehicle>();
  std::vector<Component::Vehicle *> active;

  std::copy_if(vehicles.begin(), vehicles.end(), std::back_inserter(active),
               [](Component::Vehicle *vehicle) {
                 return vehicle->pxVehicle;// vehicles might not be initialized yet...
               });

  for (auto &meta : active) {

    PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
        meta->pxPadSmoothingData, meta->pxSteerVsForwardSpeedTable, meta->pxVehicleInputData,
        timestep, meta->pxIsVehicleInAir, *meta->pxVehicle);
  }

  std::vector<PxVehicleWheels *> wheels;
  std::transform(active.begin(), active.end(), std::back_inserter(wheels),
                 [](auto meta) { return meta->pxVehicle; });

  if (!wheels.empty()) {
    // raycasts
    PxRaycastQueryResult *raycastResults = cVehicleSceneQueryData->getRaycastQueryResultBuffer(0);
    const PxU32 raycastResultsSize = cVehicleSceneQueryData->getQueryResultBufferSize();
    PxVehicleSuspensionRaycasts(cBatchQuery, static_cast<PxU32>(wheels.size()), wheels.data(),
                                raycastResultsSize, raycastResults);

    // vehicle update
    const PxVec3 gravity = cScene->getGravity();
    PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];

    std::vector<PxVehicleWheelQueryResult> vehicleQueryResults;
    for (auto w : wheels) {

      vehicleQueryResults.push_back({wheelQueryResults, wheels[0]->mWheelsSimData.getNbWheels()});
    }
    PxVehicleUpdates(0.0001f + timestep / 1000.0f, gravity, *cFrictionPairs, wheels.size(),
                     wheels.data(), vehicleQueryResults.data());
  }

  cScene->simulate(0.0001f + timestep / 1000.0f);
  cScene->fetchResults(true);

  PxU32 nbActors;
  nbActors = cScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
  std::vector<PxActor *> actors(nbActors);
  cScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), nbActors);

  // update each render object with the new transform
  for (auto &actor : actors) {
    auto *component = static_cast<ComponentBase *>(actor->userData);
    if (component && actor->getType() == PxActorType::eRIGID_DYNAMIC) {
      auto A = static_cast<PxRigidActor *>(static_cast<PxBase *>(actor));
      if (A) {
        auto T = A->getGlobalPose();
        component->getStore().eraseComponentsOfType<WorldTransform>();
        component->getStore().emplaceComponent<WorldTransform>(T);
      }
    }
  }
}

void Physics::PhysicsSystem::update(Engine::deltaTime deltaTime) {

  deltaTime = std::min(deltaTime, 32.0f);
  stepPhysics(deltaTime);
}

std::ostream &physx::operator<<(std::ostream &out, const physx::PxTransform &transform) {

  out << transform.p.x << ", " << transform.p.y << ", " << transform.p.z;
  return out;
}

void Physics::PhysicsSystem::onKeyDown(const Component::EventArgs<int> &args) {

  auto key = std::get<0>(args.values);

  Engine::log<module, Engine::low>("onKeyDown=", key);

  keys.emplace(key);
}

void Physics::PhysicsSystem::onKeyUp(const Component::EventArgs<int> &args) {

  auto key = std::get<0>(args.values);

  Engine::log<module, Engine::low>("onKeyUp=", key);

  keys.erase(key);
}

void Physics::PhysicsSystem::onKeyPress(const Component::EventArgs<int> &args) { /* do nothing */
}

void Physics::PhysicsSystem::onVehicleRegionDestroyed(PxVehicleDrive4W *vehicle,
                                                      std::string region_name) {
  std::vector<PxShape *> shapes(vehicle->getRigidDynamicActor()->getNbShapes());
  vehicle->getRigidDynamicActor()->getShapes(shapes.data(), shapes.size(), 1);

  for (auto &&shape : shapes) {
    if (shape && shape->getName() && shape->getName() == region_name) {
      vehicle->getRigidDynamicActor()->detachShape(*shape);
      break;
    }
  }
}

void Physics::PhysicsSystem::onVehicleCreated(
    const Component::EventArgs<Component::Vehicle *> &args) {

  const auto &vehicleComponent = std::get<0>(args.values);

  // grab the vehicle world position and set the physx actors transform
  // accordingly.

  auto T = convert_from(vehicleComponent->world_transform());
  auto local_T = convert_from(vehicleComponent->local_transform());

  auto is_player = vehicleComponent->type == Vehicle::Type::Player;
  auto pxVehicle = createDrivableVehicle(
      T, is_player,
      createConvexMesh(vehicleComponent->model->parts[4].variations[0].mesh->mesh.get()), local_T);

  // Go through the list of regions on the vehicle and create a corresponding shape in physx;

  for (int i = 5; i < vehicleComponent->model->parts.size(); ++i) {

    auto &&region = vehicleComponent->model->parts[i];
    if (region.is_wheel) continue;
    auto default_variation = region.variations[0].mesh;
    auto shape = PxRigidActorExt::createExclusiveShape(
        *pxVehicle->getRigidDynamicActor(),
        PxConvexMeshGeometry(createConvexMesh(default_variation->mesh.get())), *cMaterial);
    shape->setName(region.region_name.c_str());
    shape->setLocalPose(local_T * convert_from(region.transform));


    if (is_player) {
      shape->setSimulationFilterData(PxFilterData(Engine::FilterGroup::ePlayerVehicle,
                                                  Engine::FilterMask::ePlayerColliders, 0, 0));
    } else {
      shape->setSimulationFilterData(PxFilterData(Engine::FilterGroup::eEnemyVehicle,
                                                  Engine::FilterMask::eEnemyColliders, 0, 0));
    }
  }

  vehicleComponent->onRegionDestroyed += std::bind(&PhysicsSystem::onVehicleRegionDestroyed, this,
                                                   std::placeholders::_1, std::placeholders::_2);

  Engine::log<module, Engine::high>("onVehicleCreated #", vehicleComponent);

  // link the component with the physx actor so we can replicate updates.
  vehicleComponent->pxVehicle = pxVehicle;
  pxVehicle->getRigidDynamicActor()->userData = vehicleComponent;
}

PxTriangleMesh *Physics::PhysicsSystem::createTriMesh(Mesh *mesh) {

  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count = static_cast<PxU32>(mesh->vertices.size());
  meshDesc.points.stride = sizeof(Vertex);
  meshDesc.points.data = mesh->vertices.data();

  meshDesc.triangles.count =
      static_cast<physx::PxU32>(mesh->indices.size() / 3);// assumption tri-mesh
  meshDesc.triangles.stride = static_cast<physx::PxU32>(3 * sizeof(PxU32));
  meshDesc.triangles.data = mesh->indices.data();

  PxDefaultMemoryOutputStream writeBuffer;
  PxTriangleMeshCookingResult::Enum result;
  bool status = cCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);

  Engine::assertLog(status, "Cooking triangle mesh");

  PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());

  return cPhysics->createTriangleMesh(readBuffer);
}

PxConvexMesh *Physics::PhysicsSystem::createConvexMesh(const Mesh *mesh) {

  PxConvexMeshDesc convexDesc;
  convexDesc.points.count = static_cast<PxU32>(mesh->vertices.size());
  convexDesc.points.stride = sizeof(Vertex);
  convexDesc.points.data = mesh->vertices.data();
  convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

  PxDefaultMemoryOutputStream buf;
  PxConvexMeshCookingResult::Enum result;
  auto status = cCooking->cookConvexMesh(convexDesc, buf, &result);

  Engine::assertLog(status, "Cooking convex mesh");

  PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
  return cPhysics->createConvexMesh(input);
}

void Physics::PhysicsSystem::onActorCreated(
    const Component::EventArgs<Component::PhysicsActor *> &args) {

  Engine::log<module>("Running onActorCreated");

  auto &aPhysicsActor = std::get<0>(args.values);
  auto &aMesh = aPhysicsActor->mesh;

  auto position = physx::PxVec3{aPhysicsActor->position.x, aPhysicsActor->position.y,
                                aPhysicsActor->position.z};
  auto rotation = physx::PxQuat{aPhysicsActor->rotation.x, aPhysicsActor->rotation.y,
                                aPhysicsActor->rotation.z, aPhysicsActor->rotation.w};

  if (aPhysicsActor->type == PhysicsActor::Type::StaticObject) {
    aPhysicsActor->actor = cPhysics->createRigidStatic(PxTransform(position, rotation));
    PxShape *aConvexShape = PxRigidActorExt::createExclusiveShape(
        *aPhysicsActor->actor, PxTriangleMeshGeometry(createTriMesh(aMesh.get())), *cMaterial);
  }
  if (aPhysicsActor->type == PhysicsActor::Type::Ground) {
    aPhysicsActor->actor = cPhysics->createRigidStatic(PxTransform(position, rotation));
    PxShape *aConvexShape = PxRigidActorExt::createExclusiveShape(
        *aPhysicsActor->actor, PxTriangleMeshGeometry(createTriMesh(aMesh.get())), *cMaterial);
  }

  if (aPhysicsActor->type == PhysicsActor::Type::TriggerVolume) {
    aPhysicsActor->actor = cPhysics->createRigidStatic(PxTransform(position, rotation));
    PxShape *aConvexShape = PxRigidActorExt::createExclusiveShape(
        *aPhysicsActor->actor, PxConvexMeshGeometry(createConvexMesh(aMesh.get())), *cMaterial,
        (PxShapeFlag::eTRIGGER_SHAPE | PxShapeFlag::eVISUALIZATION));
  }

  if (aPhysicsActor->type == PhysicsActor::Type::DynamicObject) {
    auto rigid = cPhysics->createRigidDynamic(PxTransform(position, rotation));
    rigid->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    aPhysicsActor->actor = rigid;
    PxShape *aConvexShape = PxRigidActorExt::createExclusiveShape(
        *aPhysicsActor->actor, PxConvexMeshGeometry(createConvexMesh(aMesh.get())), *cMaterial);
  }

  aPhysicsActor->actor->userData = aPhysicsActor;

  switch (aPhysicsActor->type) {
    case PhysicsActor::Type::StaticObject:
      FilterShader::setupFiltering(aPhysicsActor->actor, FilterGroup::eGround,
                                   FilterMask::eGroundColliders);
      FilterShader::setupQueryFiltering(aPhysicsActor->actor, 0, QueryFilterMask::eDrivable);
      break;
    case PhysicsActor::Type::DynamicObject:
      FilterShader::setupFiltering(aPhysicsActor->actor, FilterGroup::eObstacle,
                                   FilterMask::eEverything);
      break;
    case PhysicsActor::Type::Ground:
      FilterShader::setupFiltering(aPhysicsActor->actor, FilterGroup::eGround,
                                   FilterMask::eGroundColliders);
      FilterShader::setupQueryFiltering(aPhysicsActor->actor, 0, QueryFilterMask::eDrivable);
      break;
    case PhysicsActor::Type::TriggerVolume:
      FilterShader::setupFiltering(aPhysicsActor->actor, FilterGroup::eTrigger,
                                   FilterMask::eTriggerColliders);
      break;
  }
  cScene->addActor(*aPhysicsActor->actor);
}

void Physics::PhysicsSystem::onPassengerCreated(Component::Passenger *passenger) {

  auto passengers = getEngine()
                        ->getSubSystem<EngineStore>()
                        ->getRoot()
                        .getComponentsOfType<Component::Passenger>();
  for (auto passenger : passengers) {

    if (!passenger->pickup_actor->actor->actor || !passenger->dropoff_actor->actor->actor)
      continue;// todo, not this....

    PxRigidStatic *temp_rigstat_dropoff = cPhysics->createRigidStatic(passenger->dropOffTransform);

    passenger->pass_material = cPhysics->createMaterial(100.0f, 100.f, 100.f);

    PxShapeFlags pass_flags = (PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eVISUALIZATION);

    activePassenger = passenger;

    break;// only make one at a time?
  }
}

Component::Passenger *Physics::PhysicsSystem::createPassenger(const PxTransform &pickupTrans,
                                                              const PxTransform &dropOffTrans) {
  // create temp passenger component
  Component::Passenger *temp_pass = nullptr;

  return (temp_pass);
}

void Physics::PhysicsSystem::link(Vehicle *sceneComponent, physx::PxRigidDynamic *actor) {

  trackedComponents.emplace(actor, sceneComponent);
}
