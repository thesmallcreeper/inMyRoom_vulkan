#include "InputManager.h"

InputManager::InputManager(configuru::Config& in_cfgFile)
    :cfgFile(in_cfgFile)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    for (auto const& bind : functionNameToFunction_map)
    {
        for (const configuru::Config& arrayIterator : cfgFile["inputSettings"]["keybinds"][bind.first.c_str()].as_array())
        {
            Anvil::KeyID keyID;

            std::string keyName = arrayIterator.as_string();
            auto search = buttomAliasToKey_map.find(keyName);

            if (search != buttomAliasToKey_map.end())
                keyID = search->second;
            else
                keyID = static_cast<Anvil::KeyID>(keyName[0]);

            if (std::get<0>(bind.second) != nullptr)
                keyToFunction_onKeyPressed_umap.try_emplace(keyID, std::get<0>(bind.second));
            if (std::get<1>(bind.second) != nullptr)
                keyToFunction_onKeyReleased_umap.try_emplace(keyID, std::get<1>(bind.second));
        }
    }

    mouseSensitivity = cfgFile["inputSettings"]["mouseSensitivity"].as_float();
}

InputManager::~InputManager()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    keyToFunction_onKeyReleased_umap.clear();
    eventVector.clear();
}

void InputManager::BindCameraFreezeOldUnfreezeNew(CameraBaseClass* in_camera_ptr)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (camera_ptr)
        camera_ptr->Freeze();

    camera_ptr = in_camera_ptr;

    forwardKeyIsPressed = false;
    backwardKeyIsPressed = false;
    rightKeyIsPressed = false;
    leftKeyIsPressed = false;
    upKeyIsPressed = false;
    downKeyIsPressed = false;

    if (camera_ptr)
        camera_ptr->Unfreeze();
}

std::vector<eventInputID> InputManager::GrabAndResetEventVector()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    std::vector<eventInputID> returnVector;
    returnVector.swap(eventVector);
    return returnVector;
}

void InputManager::MouseMoved(const long xOffset, const long yOffset)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    float xRotation_rads = - xOffset * mouseSensitivity;
    float yRotation_rads = - yOffset * mouseSensitivity;

    if (camera_ptr)
        camera_ptr->MoveCamera(xRotation_rads, yRotation_rads);
}

void InputManager::KeyPressed(const Anvil::KeyID in_key)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    auto search = keyToFunction_onKeyPressed_umap.find(in_key);
    if (search != keyToFunction_onKeyPressed_umap.end())
        search->second();
}


void InputManager::KeyReleased(const Anvil::KeyID in_key)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    auto search = keyToFunction_onKeyReleased_umap.find(in_key);
    if (search != keyToFunction_onKeyReleased_umap.end())
        search->second();
}

void InputManager::AddToQueue(eventInputID event)
{
    // KeyPressed or keyReleased access this function that are have locked the mutex 2. wtf i sayy

    eventVector.push_back(event);
}

void InputManager::MoveForward()
{
    if (camera_ptr && !forwardKeyIsPressed)
    {
        camera_ptr->MoveForward();
        forwardKeyIsPressed = true;
    }
}

void InputManager::MoveBackward()
{
    if (camera_ptr && !backwardKeyIsPressed)
    {
        camera_ptr->MoveBackward();
        backwardKeyIsPressed = true;
    }
}

void InputManager::MoveLeft()
{
    if (camera_ptr && !leftKeyIsPressed)
    {
        camera_ptr->MoveLeft();
        leftKeyIsPressed = true;
    }
}

void InputManager::MoveRight()
{
    if (camera_ptr && !rightKeyIsPressed)
    {
        camera_ptr->MoveRight();
        rightKeyIsPressed = true;
    }
}

void InputManager::MoveUp()
{
    if (camera_ptr && !upKeyIsPressed)
    {
        camera_ptr->MoveUp();
        upKeyIsPressed = true;
    }
}

void InputManager::MoveDown()
{
    if (camera_ptr && !downKeyIsPressed)
    {
        camera_ptr->MoveDown();
        downKeyIsPressed = true;
    }
}


void InputManager::StopMovingForward()
{
    if (camera_ptr && forwardKeyIsPressed)
    {
        camera_ptr->StopMovingForward();
        forwardKeyIsPressed = false;
    }
}

void InputManager::StopMovingBackward()
{
    if (camera_ptr && backwardKeyIsPressed)
    {
        camera_ptr->StopMovingBackward();
        backwardKeyIsPressed = false;
    }
}

void InputManager::StopMovingLeft()
{
    if (camera_ptr && leftKeyIsPressed)
    {
        camera_ptr->StopMovingLeft();
        leftKeyIsPressed = false;
    }
}

void InputManager::StopMovingRight()
{
    if (camera_ptr && rightKeyIsPressed)
    {
        camera_ptr->StopMovingRight();
        rightKeyIsPressed = false;
    }
}

void InputManager::StopMovingUp()
{
    if (camera_ptr && upKeyIsPressed)
    {
        camera_ptr->StopMovingUp();
        upKeyIsPressed = false;
    }
}

void InputManager::StopMovingDown()
{
    if (camera_ptr && downKeyIsPressed)
    {
        camera_ptr->StopMovingDown();
        downKeyIsPressed = false;
    }
}

void InputManager::ShouldClose()
{
    AddToQueue(SHOULD_CLOSE);
}