#pragma once

#include <unordered_map>
#include <queue>
#include <mutex>

#include "configuru.hpp"

#include "misc/io.h"

#include "CameraBaseClass.h"

enum eventInputIDenums
{
    /*Quit key bind*/
    SHOULD_CLOSE,
	/*Debug culling key bind*/
	TOGGLE_CULLING_DEBUG
};

class InputManager
{
public:
    InputManager(configuru::Config& in_cfgFile);
    ~InputManager();

    std::vector<eventInputIDenums> GrabAndResetEventVector();

    void BindPlayerInputFreezeOldUnfreezeNew(CameraBaseClass* in_camera_ptr);

    void KeyPressed(Anvil::KeyID in_key);
    void KeyReleased(Anvil::KeyID in_key);

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

    std::unordered_map<Anvil::KeyID, std::function<void()>> keyToFunction_onKeyPressed_umap;
    std::unordered_map<Anvil::KeyID, std::function<void()>> keyToFunction_onKeyReleased_umap;

    CameraBaseClass* volatile camera_ptr = nullptr;

    std::vector<eventInputIDenums> eventVector;

    std::mutex controlMutex;

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
        {"Exit", std::make_pair(std::bind(&InputManager::AddToQueue, this, SHOULD_CLOSE), nullptr)},
        {"CullingDebug", std::make_pair(std::bind(&InputManager::AddToQueue, this, TOGGLE_CULLING_DEBUG), nullptr)}
    };

    std::unordered_map<std::string, Anvil::KeyID> buttomAliasToKey_map =
    {
        {"SPACE", Anvil::KEY_ID_SPACE}, {"ESCAPE", Anvil::KEY_ID_ESCAPE}, {"TAB", Anvil::KEY_ID_TAB},
        {"LEFT_MOUSE", Anvil::KEY_ID_LBUTTON}, {"MIDDLE_MOUSE", Anvil::KEY_ID_MBUTTON}, {"RIGHT_MOUSE", Anvil::KEY_ID_RBUTTON}, 
        {"UP", Anvil::KEY_ID_UP}, {"DOWN", Anvil::KEY_ID_DOWN},
        {"LEFT", Anvil::KEY_ID_LEFT}, {"RIGHT", Anvil::KEY_ID_RIGHT}
    };
};
