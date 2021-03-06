//
// Created by root on 17/1/20.
//

#ifndef ENGINE_VEHICLE_H
#define ENGINE_VEHICLE_H

#include "..\Systems\Navigation\astar.h"
#include "ComponentId.h"
#include "Model.h"
#include "damage.hpp"
#include "tags.h"
#include <Camera.h>
#include <EventDelegate.h>
#include <EventHandler.h>
#include <Events/Events.h>
#include <GLFW/glfw3.h>
#include <PxPhysicsAPI.h>
#include <scenery.hpp>
#include <vehicle/PxVehicleDrive4W.h>

#include "ComponentInterface.h"
#include "Engine.h"
#include "PhysicsActor.h"

#include <Physics/Wheel.hpp>
#include <Sound.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <al.h>
#include <alc.h>
#include <future>

namespace Component {

  struct CollisionEventArgs {
    struct Vehicle *vehicle;
    float momentum;
    std::string region_name;
  };

  struct CollisionEvent : public ComponentBase {
    class Vehicle *hit_actor;
    CollisionEvent(class Vehicle *hitActor);
  };

  inline physx::PxTransform convert_from(glm::mat4 T) {
    glm::vec3 scale, skew, translation;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(T, scale, orientation, translation, skew, perspective);
    return physx::PxTransform(
        translation.x, translation.y, translation.z,
        physx::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w));
  }

  inline glm::mat4 convert_from(physx::PxTransform T) {
    return glm::translate(glm::vec3{T.p.x, T.p.y, T.p.z}) *
           glm::mat4_cast(glm::quat(T.q.w, T.q.x, T.q.y, T.q.z));
  }

  inline glm::mat4 convert_from(physx::PxQuat q) {
    return glm::mat4_cast(glm::quat(q.w, q.x, q.y, q.z));
  }

  struct Vehicle : public ComponentBase {
public:
    bool is_outdated = true;
    std::shared_ptr<Model> model;
    ComponentId input{};
    std::shared_ptr<EventHandler<Engine::deltaTime>> onTickHandler;
    EventDelegate<Vehicle *> tickHandler{"tickHandler"};
    EventDelegate<CollisionEventArgs &> onHit{"onHit"};
    EventDelegate<physx::PxVehicleDrive4W *, std::string> onRegionDestroyed{"onRegionDestroyed"};
    EventDelegate<Vehicle *> onVehicleDestroyed{"onVehicleDestroyed"};

    ALuint aiSource;

    bool initialAccelerate = false;
    bool initialBreak = false;

    Model::Part *front_left_wheel = nullptr;
    Model::Part *front_right_wheel = nullptr;
    Model::Part *back_left_wheel = nullptr;
    Model::Part *back_right_wheel = nullptr;
    std::shared_ptr<Obstacle> last_garbage;

    enum Type { Player, Taxi } type = Taxi;
    AStar path;
    int framesStopped = 0;
    glm::vec3 scale = {1, 1, 1};
    glm::quat rotation = {1, 0, 0, 0};
    glm::quat local_rotation = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 position;
    glm::vec3 local_position = {0, 0, 0};

    glm::mat4 world_transform() {
      return glm::translate(position) * glm::mat4_cast(rotation) * glm::translate(local_position) *
             glm::mat4_cast(local_rotation) * glm::scale(scale);
    }

    glm::mat4 local_transform() {
      return glm::translate(local_position) * glm::mat4_cast(local_rotation);
    }

    glm::mat4 physx_transform() {

      return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);
    }

    bool pxIsVehicleInAir;
    physx::PxVehicleDrive4WRawInputData pxVehicleInputData;
    physx::PxVehicleDrive4W *pxVehicle = nullptr;
    physx::PxVehicleDrivableSurfaceToTireFrictionPairs *pxFrictionPairs = nullptr;
    physx::PxReal pxSteerVsForwardSpeedData[16];
    physx::PxFixedSizeLookupTable<8> pxSteerVsForwardSpeedTable;
    physx::PxVehicleKeySmoothingData pxKeySmoothingData;
    physx::PxVehiclePadSmoothingData pxPadSmoothingData;

    void onTick(const Component::EventArgs<Engine::deltaTime> &args) {
      auto collisions = getStore().getComponentsOfType<CollisionEvent>();
      if (!collisions.empty()) log<module, high>("Hit ", collisions.size());
      tickHandler(this);
    }

    void onHitHandler(CollisionEventArgs &args) {
      
      
      auto [vehicle, impulse, region] = args;
      
      auto damage = std::clamp<int>(static_cast<int>(impulse / 300), 0, 15);
      log<module, high>("Vehicle hit region ", region, " causing ", damage, " damage");
     
      model->getStore().emplaceComponent<Damage, 1>(damage, region);
      
      //if(!vehicle) return;
      if(damage>10 && type == Component::Vehicle::Type::Player)
      {
          getEngine()->createComponent<Component::Sound>("damageRecieved");
           std::cout <<"Collision sound should go off."<< std::endl; 
      }
    }


    void onRegionDestroyedHandler(std::string region_name) {
      log<module, high>("Region ", region_name, " has been destroyed.");
      onRegionDestroyed(pxVehicle, region_name);// mmm onion
      auto &&region = model->getRegionByName(region_name);
      auto garbage = getEngine()->createComponent<Obstacle>(
          world_transform(), region.getActiveVariation().mesh->mesh,
          region.getActiveVariation().mesh->material);
      last_garbage = garbage;
    }

    Vehicle()
        : pxSteerVsForwardSpeedData{0.0f, 0.75f, 5.0f, 0.25f, 30.0f, 0.25f, 120.0f, 0.15f},
          pxKeySmoothingData{{
                                 6.0f,// rise rate eANALOG_INPUT_ACCEL
                                 6.0f,// rise rate eANALOG_INPUT_BRAKE
                                 6.0f,// rise rate eANALOG_INPUT_HANDBRAKE
                                 2.5f,// rise rate eANALOG_INPUT_STEER_LEFT
                                 2.5f,// rise rate eANALOG_INPUT_STEER_RIGHT
                             },
                             {
                                 10.0f,// fall rate eANALOG_INPUT_ACCEL
                                 10.0f,// fall rate eANALOG_INPUT_BRAKE
                                 10.0f,// fall rate eANALOG_INPUT_HANDBRAKE
                                 5.0f, // fall rate eANALOG_INPUT_STEER_LEFT
                                 5.0f  // fall rate eANALOG_INPUT_STEER_RIGHT
                             }},
          pxPadSmoothingData{{
                                 6.0f,// rise rate eANALOG_INPUT_ACCEL
                                 6.0f,// rise rate eANALOG_INPUT_BRAKE
                                 6.0f,// rise rate eANALOG_INPUT_HANDBRAKE
                                 2.5f,// rise rate eANALOG_INPUT_STEER_LEFT
                                 2.5f,// rise rate eANALOG_INPUT_STEER_RIGHT
                             },
                             {
                                 10.0f,// fall rate eANALOG_INPUT_ACCEL
                                 10.0f,// fall rate eANALOG_INPUT_BRAKE
                                 10.0f,// fall rate eANALOG_INPUT_HANDBRAKE
                                 5.0f, // fall rate eANALOG_INPUT_STEER_LEFT
                                 5.0f  // fall rate eANALOG_INPUT_STEER_RIGHT
                             }} {

      pxSteerVsForwardSpeedTable = physx::PxFixedSizeLookupTable<8>(pxSteerVsForwardSpeedData, 4);
      onTickHandler =
          getEngine()->getSubSystem<EventSystem>()->createTickHandler(this, &Vehicle::onTick);
      onHit += std::bind(&Vehicle::onHitHandler, this, std::placeholders::_1);

      model = getEngine()->createComponent<Model>();
      model->onRegionDestroyed +=
          std::bind(&Vehicle::onRegionDestroyedHandler, this, std::placeholders::_1);
      model->onHealthChanged += [&](auto health) {
        if (health <= 0) { onVehicleDestroyed(this); }
      };
    }
  };

  struct ControlledVehicle : public ComponentBase {

    std::shared_ptr<Vehicle> vehicle;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<EventHandler<GLFWgamepadstate, GLFWgamepadstate>> onGamePadStateChangedHandler;
    std::shared_ptr<EventHandler<int>> onKeyDownHandler;
    std::shared_ptr<EventHandler<int>> onKeyUpHandler;

    bool isHonking = false;
    bool isCarryingPasssenger = false;

    ControlledVehicle()
        : vehicle(getEngine()->createComponent<Vehicle>()),
          camera(getEngine()->createComponent<Camera>()) {

      onGamePadStateChangedHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
          this, &ControlledVehicle::onGamePadStateChanged);
      onKeyDownHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
          this, &ControlledVehicle::onKeyDown);
      onKeyUpHandler = getEngine()->getSubSystem<EventSystem>()->createHandler(
          this, &ControlledVehicle::onKeyUp);
      vehicle->type = Vehicle::Type::Player;
      camera->target = vehicle;
      vehicle->onVehicleDestroyed +=
          [&](Vehicle *vehicle) { 
          camera->target = vehicle->last_garbage->mesh; };
    }

    template<typename T>
    int sgn(T val) {
      return (T(0) < val) - (val < T(0));
    }

    float filter_axis_data(const float input) {

      auto normalize = [](auto value, auto min, auto max) { return (value - min) / (max - min); };
      if (input >= 0) return std::clamp(input, 0.2f, 1.0f);
      else
        return std::clamp(input, -1.0f, -0.2f);
    }

    void onKeyDown(const EventArgs<int> &args) {

      auto v = vehicle->pxVehicle->getRigidDynamicActor()->getLinearVelocity();
      auto key = std::get<0>(args.values);


      if (key == GLFW_KEY_X) {
        vehicle->model->getStore().emplaceComponent<Damage, 1>(1000, "chassis");
      }
      if (key == GLFW_KEY_W) {
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eFIRST);
        vehicle->pxVehicleInputData.setAnalogAccel(1);
        getEngine()->createComponent<Component::Sound>("acceleration");
        getEngine()->createComponent<Component::Sound>("stopCarMoving");
      }

      if (key == GLFW_KEY_S) {
        if (v.z > 0.1) {// is moving forward
          vehicle->pxVehicleInputData.setAnalogBrake(1);
          getEngine()->createComponent<Component::Sound>("breaking");
        } else {
          vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eREVERSE);
          vehicle->pxVehicleInputData.setAnalogAccel(1);
          getEngine()->createComponent<Component::Sound>("acceleration");
        }
      }
      if (key == GLFW_KEY_D) { vehicle->pxVehicleInputData.setAnalogSteer(-1); }
      if (key == GLFW_KEY_A) { vehicle->pxVehicleInputData.setAnalogSteer(1); }
      if (key == GLFW_KEY_LEFT_SHIFT) {
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eREVERSE);

        vehicle->pxVehicleInputData.setAnalogAccel(1);
      }
    }

    void onKeyUp(const EventArgs<int> &args) {

      auto v = vehicle->pxVehicle->getRigidDynamicActor()->getLinearVelocity();
      auto key = std::get<0>(args.values);

      if (key == GLFW_KEY_W) {
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eNEUTRAL);
        vehicle->pxVehicleInputData.setAnalogAccel(0);
        getEngine()->createComponent<Component::Sound>("stopAcceleration");
        //	if (v.z > 0.5)
        //{
        getEngine()->createComponent<Component::Sound>("carMoving");
        //	}
      }

      if (key == GLFW_KEY_S) {
        vehicle->pxVehicleInputData.setAnalogBrake(0);
        getEngine()->createComponent<Component::Sound>("stopBreaking");
      }

      if (key == GLFW_KEY_D) { vehicle->pxVehicleInputData.setAnalogSteer(0); }

      if (key == GLFW_KEY_A) { vehicle->pxVehicleInputData.setAnalogSteer(0); }

      if (key == GLFW_KEY_S) {
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eNEUTRAL);
        vehicle->pxVehicleInputData.setAnalogBrake(0);
        getEngine()->createComponent<Component::Sound>("stopAcceleration");
        getEngine()->createComponent<Component::Sound>("stopBreaking");
      }
      if (key == GLFW_KEY_LEFT_SHIFT) {
        vehicle->pxVehicleInputData.setAnalogAccel(0);
        // vehicle->pxVehicleInputData.setAnalogBrake(1);
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eFIRST);
      }
    }

    bool reverse = false;
    void onGamePadStateChanged(const EventArgs<GLFWgamepadstate, GLFWgamepadstate> &args) {

      auto norm = [](auto value) { return 0.5 * value + 0.5; };

      auto previous = std::get<0>(args.values);
      auto current = std::get<1>(args.values);
      float control_deadzone = 0.3f;

      float reverse_throttle_value = norm(current.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
      float throttle_value = norm(current.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);

      if (reverse_throttle_value > 0.2f) {
        reverse = true;
        vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eREVERSE);

        vehicle->pxVehicleInputData.setAnalogAccel(reverse_throttle_value);
      } else if (throttle_value > 0.000001f) {

        if (reverse) {
          reverse = false;
          vehicle->pxVehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eNEUTRAL);
        }

        vehicle->pxVehicleInputData.setAnalogAccel(throttle_value);
      }

      vehicle->pxVehicleInputData.setAnalogBrake(current.buttons[GLFW_GAMEPAD_BUTTON_X]);

      if (current.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -control_deadzone ||
          current.axes[GLFW_GAMEPAD_AXIS_LEFT_X] >= control_deadzone) {
        vehicle->pxVehicleInputData.setAnalogSteer(-current.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
      } else {
        vehicle->pxVehicleInputData.setAnalogSteer(0);
      }

      const auto rotation_scale = static_cast<float>(3.14157) / 180.0f;
      auto camera_yaw = current.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * rotation_scale;
      auto delta = glm::quat(glm::vec3(0, glm::degrees(camera_yaw), 0));
      using namespace Engine;
      camera->local_rotation = delta;
    }
  };// namespace Component
}// namespace Component

#endif// ENGINE_VEHICLE_H
