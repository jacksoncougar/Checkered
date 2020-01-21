//
// Created by root on 11/1/20.
//

#include "../../Components/Component.h"
#include "InputSystem.h"

Component::ComponentEvent<int> Input::InputSystem::onKeyPress;
Component::ComponentEvent<int> Input::InputSystem::onKeyDown;
Component::ComponentEvent<int> Input::InputSystem::onKeyUp;

void Input::InputSystem::initialize(GLFWwindow *window) {
    glfwSetKeyCallback(window, keyHandler);
}

void Input::InputSystem::keyHandler(GLFWwindow *, int key, int, int action, int) {

    if (action == GLFW_RELEASE) {
        Engine::log<module, Engine::Importance::low>("onKeyPress: ", key);
        onKeyPress(key);
        onKeyUp(key);
    }
    if (action == GLFW_PRESS)
    {
        Engine::log<module, Engine::Importance::low>("onKeyDown: ", key);
        onKeyDown(key);
    }
}