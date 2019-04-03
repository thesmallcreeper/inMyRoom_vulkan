#pragma once

#include <unordered_map>
#include <queue>
#include <mutex>

#include "configuru.hpp"

#include "misc/io.h"

#include "CameraBaseClass.h"

enum eventInputID
{
    /*Quit key bind*/
    SHOULD_CLOSE
};

class InputManager
{
public:
    InputManager(configuru::Config& in_cfgFile);
    ~InputManager();

    void init();

    std::vector<eventInputID> grabAndResetEventVector();

    void bindCameraFreezeOldUnfreezeNew(CameraBaseClass* in_camera_ptr);

    void keyPressed(const Anvil::KeyID in_key);
    void keyReleased(const Anvil::KeyID in_key);

    void mouseMoved(const long xOffset, const long yOffset);

private:
    configuru::Config& cfgFile;

    void moveForward();
    void moveBackward();
    void moveRight();
    void moveLeft();
    void moveUp();
    void moveDown();

    void stopMovingForward();
    void stopMovingBackward();
    void stopMovingRight();
    void stopMovingLeft();
    void stopMovingUp();
    void stopMovingDown();

    void shouldClose();

    void addToQueue(eventInputID event);

    bool ForwardKeyIsPressed = false;
    bool BackwardKeyIsPressed = false;
    bool RightKeyIsPressed = false;
    bool LeftKeyIsPressed = false;
    bool UpKeyIsPressed = false;
    bool DownKeyIsPressed = false;

private:

    float mouseSensitivity;

    std::map<std::string, std::pair<std::function<void()>, std::function<void()>>> functionNameToFunction_map =
    {
        {"MoveForward",std::make_pair(std::bind(&InputManager::moveForward, this), std::bind(&InputManager::stopMovingForward, this))},
        {"MoveBackward",std::make_pair(std::bind(&InputManager::moveBackward, this), std::bind(&InputManager::stopMovingBackward, this))},
        {"MoveLeft",std::make_pair(std::bind(&InputManager::moveLeft, this), std::bind(&InputManager::stopMovingLeft, this))},
        {"MoveRight",std::make_pair(std::bind(&InputManager::moveRight, this), std::bind(&InputManager::stopMovingRight, this))},
        {"Exit",std::make_pair(std::bind(&InputManager::shouldClose, this), nullptr)}
    };

    std::unordered_map<std::string, Anvil::KeyID> buttomAliasToKey_umap =
    {
        {"SPACE", Anvil::KEY_ID_SPACE}, {"ESCAPE", Anvil::KEY_ID_ESCAPE}, {"LEFT_MOUSE", Anvil::KEY_ID_LBUTTON}, {"MIDDLE_MOUSE", Anvil::KEY_ID_MBUTTON},
        {"RIGHT_MOUSE", Anvil::KEY_ID_RBUTTON}, {"UP", Anvil::KEY_ID_UP}, {"DOWN", Anvil::KEY_ID_DOWN}, {"LEFT", Anvil::KEY_ID_LEFT},
        {"RIGHT", Anvil::KEY_ID_RIGHT}
    };

    std::unordered_map<Anvil::KeyID, std::function<void()>> keyToFunction_onKeyPressed_umap;
    std::unordered_map<Anvil::KeyID, std::function<void()>> keyToFunction_onKeyReleased_umap;

    CameraBaseClass* volatile camera_ptr = nullptr;

    std::vector<eventInputID> eventVector;

    std::mutex control_mutex;
};