#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <mutex>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



class MovementBaseClass
{
public:
	MovementBaseClass(glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up);
	~MovementBaseClass();

	void freeze();
	void unfreeze();

	void moveCamera(float xRotation_rads, float yRotation_rads);

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

	glm::mat4x4 getLookAtMatrix();

private:

	virtual std::pair<glm::vec3, glm::vec3> calculate_snap(const std::chrono::duration<float> durationOfCurrentState) = 0;

protected:

	struct
	{
		bool movingForward;
		bool movingBackward;
		bool movingRight;
		bool movingLeft;
		bool movingUp;
		bool movingDown;
	} movementState;

	glm::vec3 position;
	glm::vec3 lookingDirection;

	glm::vec3 upVector;

private:

	std::chrono::steady_clock::time_point last_snap_timePoint;

	volatile bool freezed = true;
	std::mutex control_mutex;
	
};