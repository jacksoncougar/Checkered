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

namespace Component {

	struct Node {
		std::unordered_map<std::type_index, std::any> components;

		template<typename T>
		std::vector<std::shared_ptr<T>> getComponentsOfType() {
			static_assert(std::is_base_of<Component::ComponentInterface, T>::value);

			if (!components[typeid(T)].has_value()) return {};
			return std::any_cast<std::vector<std::shared_ptr<T>>&>(components[typeid(T)]);
		}

		template<typename T>
		void addComponent(std::shared_ptr<T> component) {
			static_assert(std::is_base_of<Component::ComponentInterface, T>::value);

			if (!components[typeid(T)].has_value())
			{
				components[typeid(T)] = std::vector<std::shared_ptr<T>>();
			}

			auto& scene = std::any_cast<std::vector<std::shared_ptr<T>>&>(components[typeid(T)]);
			scene.emplace_back(std::move(component));
		}

		template<typename T>
		void eraseComponentsOfType() {
			static_assert(std::is_base_of<Component::ComponentInterface, T>::value);
			if (components[typeid(T)].has_value()) {
				std::any_cast<std::vector<std::shared_ptr<T>>&>(components[typeid(T)]).clear();
			}
		}
	};


	typedef int ComponentId;

	class ComponentInterface {
	public:
		[[nodiscard]] virtual Node& getChildren() = 0;
		[[nodiscard]] virtual ComponentId getId() const = 0;

	};

	class ComponentBase : public ComponentInterface {

	protected:
		Node children;

	public:
		const ComponentId id = 0;

		[[nodiscard]] Node& getChildren() override { return children; }
		[[nodiscard]] ComponentId getId() const override { return id; }

		template <typename T>
		void addChildComponent(std::shared_ptr<T> component) {
			children.addComponent(component);
		}

		template <typename T, typename... Args>
		void emplaceChildComponent(Args& ... args) {
			children.addComponent(Engine::getStore().create<T>(args...));
		}

		ComponentBase &operator=(const ComponentBase &other) {
			return *this;
		}
	};
}

#endif //ENGINE_COMPONENTBASE_H
