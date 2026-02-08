#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace gps {

    static glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;

        this->cameraUpDirection = glm::normalize(cameraUp);

        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);

        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
        this->cameraUpDirection = glm::normalize(glm::cross(this->cameraRightDirection, this->cameraFrontDirection));

        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
        
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition,
            cameraPosition + cameraFrontDirection,
            cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        glm::vec3 delta(0.0f);

        switch (direction) {
        case MOVE_FORWARD:
            delta = cameraFrontDirection * speed;
            break;
        case MOVE_BACKWARD:
            delta = -cameraFrontDirection * speed;
            break;
        case MOVE_RIGHT:
            delta = cameraRightDirection * speed;
            break;
        case MOVE_LEFT:
            delta = -cameraRightDirection * speed;
            break;
        }

        cameraPosition += delta;
        cameraTarget = cameraPosition + cameraFrontDirection;
    }


    void Camera::rotate(float pitch, float yaw) {
        float p = glm::radians(pitch);
        float y = glm::radians(yaw);

        glm::vec3 front;
        front.x = cos(p) * cos(y);
        front.y = sin(p);
        front.z = cos(p) * sin(y);
        cameraFrontDirection = glm::normalize(front);

        glm::vec3 upRef = WORLD_UP; 
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, upRef));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));

        cameraTarget = cameraPosition + cameraFrontDirection;
    }
    
}
