#pragma once

#include <glm/glm.hpp>

#include "AABB.hpp"

class PhysicsComponent
{
    public:
        PhysicsComponent(glm::vec3 min, glm::vec3 max, glm::vec3 velocity);
        void        Integrate(float deltaTime);
        const AABB  GetBoundingBox();

        void        SetVelocity(glm::vec3 velocity);
        glm::vec3   GetVelocity();
        glm::vec3   GetPosition();

        glm::vec3   position;
        glm::vec3   velocity;

    private:
        
        AABB        boundingBox;
};