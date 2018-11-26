#include "InputManager.h"

InputManager::InputManager(configuru::Config& in_cfgFile)
	:cfgFile(in_cfgFile)
{

}

InputManager::~InputManager()
{

}

void InputManager::init()
{
	std::lock_guard<std::mutex> lock(control_mutex);

	for (auto const& bind : functionNameToFunction_map)
	{
		for (const configuru::Config& arrayIterator : cfgFile["inputSettings"]["keybinds"][bind.first.c_str()].as_array())
		{
			Anvil::KeyID keyID;

			std::string keyName = arrayIterator.as_string();
			auto search = buttomAliasToKey_umap.find(keyName);

			if (search != buttomAliasToKey_umap.end())
				keyID = search->second;
			else
				keyID = static_cast<Anvil::KeyID>(keyName[0]);

			if(std::get<0>(bind.second) != nullptr)
				keyToFunction_onKeyPressed_umap.try_emplace(keyID, std::get<0>(bind.second));
			if (std::get<1>(bind.second) != nullptr)
				keyToFunction_onKeyReleased_umap.try_emplace(keyID, std::get<1>(bind.second));
		}
	}

	mouseSensitivity = cfgFile["inputSettings"]["mouseSensitivity"].as_float();
}

void InputManager::bindCameraFreezeOldUnfreezeNew(MovementBaseClass* in_camera_ptr)
{
	std::lock_guard<std::mutex> lock(control_mutex);

	if (camera_ptr)
		camera_ptr->freeze();

	camera_ptr = in_camera_ptr;

	ForwardKeyIsPressed = false;
	BackwardKeyIsPressed = false;
	RightKeyIsPressed = false;
	LeftKeyIsPressed = false;
	UpKeyIsPressed = false;
	DownKeyIsPressed = false;

	if (camera_ptr)
		camera_ptr->unfreeze();
}

std::vector<eventInputID> InputManager::grabAndResetEventVector()
{
	std::lock_guard<std::mutex> lock(control_mutex);

	std::vector<eventInputID> returnVector;
	returnVector.swap(eventVector);
	return returnVector;
}

void InputManager::mouseMoved(const long xOffset, const long yOffset)
{
	std::lock_guard<std::mutex> lock(control_mutex);

	float xRotation_rads = - xOffset * mouseSensitivity;
	float yRotation_rads = yOffset * mouseSensitivity;

	camera_ptr->moveCamera(xRotation_rads, yRotation_rads);
}

void InputManager::keyPressed(const Anvil::KeyID in_key)
{
	std::lock_guard<std::mutex> lock(control_mutex);

	auto search = keyToFunction_onKeyPressed_umap.find(in_key);
	if (search != keyToFunction_onKeyPressed_umap.end())
		search->second();
}


void InputManager::keyReleased(const Anvil::KeyID in_key)
{
	std::lock_guard<std::mutex> lock(control_mutex);

	auto search = keyToFunction_onKeyReleased_umap.find(in_key);
	if (search != keyToFunction_onKeyReleased_umap.end())
		search->second();
}

void InputManager::addToQueue(eventInputID event)
{
	// keyPressed or keyReleased access this function that are have locked the mutex

	eventVector.push_back(event);
}

void InputManager::moveForward()
{
	if (camera_ptr && !ForwardKeyIsPressed)
	{
		camera_ptr->moveForward();
		ForwardKeyIsPressed = true;
	}		
}

void InputManager::moveBackward()
{
	if (camera_ptr && !BackwardKeyIsPressed)
	{
		camera_ptr->moveBackward();
		BackwardKeyIsPressed = true;
	}
}

void InputManager::moveLeft()
{
	if (camera_ptr && !LeftKeyIsPressed)
	{
		camera_ptr->moveLeft();
		LeftKeyIsPressed = true;
	}	
}

void InputManager::moveRight()
{
	if (camera_ptr && !RightKeyIsPressed)
	{
		camera_ptr->moveRight();
		RightKeyIsPressed = true;
	}
}

void InputManager::moveUp()
{
	if (camera_ptr && !UpKeyIsPressed)
	{
		camera_ptr->moveUp();
		UpKeyIsPressed = true;
	}
}

void InputManager::moveDown()
{
	if (camera_ptr && !DownKeyIsPressed)
	{
		camera_ptr->moveDown();
		DownKeyIsPressed = true;
	}
}


void InputManager::stopMovingForward()
{
	if (camera_ptr && ForwardKeyIsPressed)
	{
		camera_ptr->stopMovingForward();
		ForwardKeyIsPressed = false;
	}
}

void InputManager::stopMovingBackward()
{
	if (camera_ptr && BackwardKeyIsPressed)
	{
		camera_ptr->stopMovingBackward();
		BackwardKeyIsPressed = false;
	}
}

void InputManager::stopMovingLeft()
{
	if (camera_ptr && LeftKeyIsPressed)
	{
		camera_ptr->stopMovingLeft();
		LeftKeyIsPressed = false;
	}
}

void InputManager::stopMovingRight()
{
	if (camera_ptr && RightKeyIsPressed)
	{
		camera_ptr->stopMovingRight();
		RightKeyIsPressed = false;
	}
}

void InputManager::stopMovingUp()
{
	if (camera_ptr && UpKeyIsPressed)
	{
		camera_ptr->stopMovingUp();
		UpKeyIsPressed = false;
	}
}

void InputManager::stopMovingDown()
{
	if (camera_ptr && DownKeyIsPressed)
	{
		camera_ptr->stopMovingDown();
		DownKeyIsPressed = false;
	}
}

void InputManager::shouldClose()
{
	addToQueue(SHOULD_CLOSE);
}