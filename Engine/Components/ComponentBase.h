//
// Created by root on 15/1/20.
//
#pragma once

#ifndef ENGINE_COMPONENTBASE_H
#define ENGINE_COMPONENTBASE_H

#include <any>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>
#include "ComponentInterface.h"
#include "Node.hpp"
#include "Engine.h"


namespace Component {

    class ComponentBase : public ComponentInterface {
        friend class ::EngineStore;

    protected:
      virtual ~ComponentBase() = default;
        Node children;

        // this hack is here because this is the easiest way.
        Engine::EngineSystem *enginePtr = Engine::current;

    public:
        ComponentId id = ++ ++next_id;

        [[nodiscard]] struct Node &getChildren() override { return children; }

        [[nodiscard]] ComponentId getId() const override { return id; }

        Engine::EngineSystem *getEngine() { return enginePtr; }

        struct Node &getStore() { return children; }

        ComponentBase &operator=(const ComponentBase &other) {

            if (this == &other)return *this;

            children = other.children;
            //id  = other.id; // don't change it
            return *this;
        }
    };
}

#endif //ENGINE_COMPONENTBASE_H
