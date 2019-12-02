#include "InputManager.h"

#include "Engine.h"

InputManager::InputManager(Engine* engine_ptr, configuru::Config& in_cfgFile)
    :cfgFile(in_cfgFile),
     engine_ptr(engine_ptr)
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

std::vector<eventInputIDenums> InputManager::GrabAndResetEventVector()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    std::vector<eventInputIDenums> returnVector;
    returnVector.swap(eventVector);
    return returnVector;
}

void InputManager::MouseMoved(const long xOffset, const long yOffset)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    float xRotation_rads = - xOffset * mouseSensitivity;
    float yRotation_rads = - yOffset * mouseSensitivity;

    InputMouse this_input_data;
    this_input_data.x_axis = xRotation_rads;
    this_input_data.y_axis = yRotation_rads;

    engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MouseMove, reinterpret_cast<void*>(&this_input_data));
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

void InputManager::AddToQueue(eventInputIDenums event)
{
    // KeyPressed or keyReleased access this function that are have locked the mutex 2. wtf i sayy

    eventVector.push_back(event);
}

void InputManager::MoveForward()
{
    if (!forwardKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveForward, nullptr);
        forwardKeyIsPressed = true;
    }
}

void InputManager::MoveBackward()
{
    if (!backwardKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveBackward, nullptr);
        backwardKeyIsPressed = true;
    }
}

void InputManager::MoveLeft()
{
    if (!leftKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveLeft, nullptr);
        leftKeyIsPressed = true;
    }
}

void InputManager::MoveRight()
{
    if (!rightKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveRight, nullptr);
        rightKeyIsPressed = true;
    }
}

void InputManager::MoveUp()
{
    if (!upKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveUp, nullptr);
        upKeyIsPressed = true;
    }
}

void InputManager::MoveDown()
{
    if (!downKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::MoveDown, nullptr);
        downKeyIsPressed = true;
    }
}


void InputManager::StopMovingForward()
{
    if (forwardKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingForward, nullptr);
        forwardKeyIsPressed = false;
    }
}

void InputManager::StopMovingBackward()
{
    if (backwardKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingBackward, nullptr);
        backwardKeyIsPressed = false;
    }
}

void InputManager::StopMovingLeft()
{
    if (leftKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingLeft, nullptr);
        leftKeyIsPressed = false;
    }
}

void InputManager::StopMovingRight()
{
    if (rightKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingRight, nullptr);
        rightKeyIsPressed = false;
    }
}

void InputManager::StopMovingUp()
{
    if (upKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingUp, nullptr);
        upKeyIsPressed = false;
    }
}

void InputManager::StopMovingDown()
{
    if (downKeyIsPressed)
    {
        engine_ptr->GetECSwrapperPtr()->AsyncInput(InputType::StopMovingDown, nullptr);
        downKeyIsPressed = false;
    }
}