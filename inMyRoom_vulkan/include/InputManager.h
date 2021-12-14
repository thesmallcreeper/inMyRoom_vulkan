#pragma once

#include <unordered_map>
#include <queue>
#include <mutex>

#include "configuru.hpp"

#include "ECS/ECStypes.h"

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

enum class eventInputIDenums
{
    /*Quit key bind*/
    SHOULD_CLOSE,
	/*Debug culling key bind*/
	TOGGLE_CULLING_DEBUG
};

class Engine;

class InputManager
{
public:
    InputManager(Engine* engine_ptr, configuru::Config& in_cfgFile);
    ~InputManager();

    std::vector<eventInputIDenums> GrabAndResetEventVector();

    void KeyPressed(int key);
    void KeyReleased(int key);

    void MouseMoved(long xOffset, long yOffset);

private:
    configuru::Config& cfgFile;

    void MoveForward();
    void MoveBackward();
    void MoveRight();
    void MoveLeft();
    void MoveUp();
    void MoveDown();

    void StopMovingForward();
    void StopMovingBackward();
    void StopMovingRight();
    void StopMovingLeft();
    void StopMovingUp();
    void StopMovingDown();

    void AddToQueue(eventInputIDenums event);

    bool forwardKeyIsPressed = false;
    bool backwardKeyIsPressed = false;
    bool rightKeyIsPressed = false;
    bool leftKeyIsPressed = false;
    bool upKeyIsPressed = false;
    bool downKeyIsPressed = false;

private:

    float mouseSensitivity;

    std::unordered_map<int, std::function<void()>> keyToFunction_onKeyPressed_umap;
    std::unordered_map<int, std::function<void()>> keyToFunction_onKeyReleased_umap;

    std::vector<eventInputIDenums> eventVector;

    std::map<std::string, std::pair<std::function<void()>, std::function<void()>>> functionNameToFunction_map =
    {
        {
            "MoveForward",
            std::make_pair(std::bind(&InputManager::MoveForward, this), std::bind(&InputManager::StopMovingForward, this))
        },
        {
            "MoveBackward",
            std::make_pair(std::bind(&InputManager::MoveBackward, this), std::bind(&InputManager::StopMovingBackward, this))
        },
        {
            "MoveLeft",
            std::make_pair(std::bind(&InputManager::MoveLeft, this), std::bind(&InputManager::StopMovingLeft, this))
        },
        {
            "MoveRight",
            std::make_pair(std::bind(&InputManager::MoveRight, this), std::bind(&InputManager::StopMovingRight, this))
        },
        {
            "MoveUp",
            std::make_pair(std::bind(&InputManager::MoveUp, this), std::bind(&InputManager::StopMovingUp, this))
        },
        {"Exit", std::make_pair(std::bind(&InputManager::AddToQueue, this, eventInputIDenums::SHOULD_CLOSE), nullptr)},
        {"CullingDebug", std::make_pair(std::bind(&InputManager::AddToQueue, this, eventInputIDenums::TOGGLE_CULLING_DEBUG), nullptr)}
    };

    std::unordered_map<std::string, int> buttomAliasToKey_map =
    {
        {"SPACE", GLFW_KEY_SPACE}, {"ESCAPE", GLFW_KEY_ESCAPE}, {"TAB", GLFW_KEY_TAB},
        {"LEFT_MOUSE", GLFW_MOUSE_BUTTON_LEFT}, {"MIDDLE_MOUSE", GLFW_MOUSE_BUTTON_MIDDLE}, {"RIGHT_MOUSE", GLFW_MOUSE_BUTTON_RIGHT},
        {"UP", GLFW_KEY_UP}, {"DOWN", GLFW_KEY_DOWN},
        {"LEFT", GLFW_KEY_LEFT}, {"RIGHT", GLFW_KEY_RIGHT}
    };

    Engine* engine_ptr;

    std::mutex controlMutex;
};
