#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include "Collider.hpp"

/* 
Triangle Collider
 */
class Triangle : public Collider
{
    public:
        Triangle(   int                     entityID,
                    glm::vec3               center, 
                    std::string             name,
                    glm::vec3               normal, 
                    std::vector<glm::vec3>  points, 
                    ColliderType            colliderType,
                    DynamicType             dynamicType);

        ~Triangle();

        virtual void ComputeDerivedData();

        // DATA
        glm::vec3               normal;
        std::vector<glm::vec3>  points;
    private:
};