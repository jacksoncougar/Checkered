//
// Created by Jackson Cougar Wiebe on 2/13/2020.
//

#ifndef ENGINE_VEHICLESYSTEM_HPP
#define ENGINE_VEHICLESYSTEM_HPP

#include <systeminterface.hpp>
#include <ComponentId.h>

namespace Engine {
    class vehicleSystem : public Engine::SystemInterface {
    public:
        Component::ComponentEvent<Component::ComponentId> onVehicleCreated;

        vehicleSystem() : onVehicleCreated("onVehicleCreated") {}

        void initialize() override;
        void update(deltaTime /*deltaTime*/) override;
    };

}

#endif //ENGINE_VEHICLESYSTEM_HPP