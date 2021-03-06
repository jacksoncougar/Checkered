//
// Created by root on 11/1/20.
//

#pragma once

#ifndef ENGINE_INPUTSYSTEM_H
#define ENGINE_INPUTSYSTEM_H

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "../../main.h"
#include "EventDelegate.h"
#include "SystemInterface.hpp"
#include <memory>
#include <functional>

namespace Input {

	namespace {
		char module[] = "Input";
	}

	class InputSystem : public Engine::SystemInterface {

		static void        keyHandler(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/);

		static void			gamepadHandler(int jid, int action);

	public:

		static Component::EventDelegate<int> onKeyPress;
		static Component::EventDelegate<int> onKeyDown;
		static Component::EventDelegate<int> onKeyUp;
		static Component::EventDelegate<GLFWgamepadstate, GLFWgamepadstate> onGamePadStateChanged;

		void initialize() override;

		void update(Engine::deltaTime /*elapsed*/) override;

	};

}


#endif //ENGINE_INPUTSYSTEM_H
