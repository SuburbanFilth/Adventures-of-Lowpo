#include "ObjectTransform.hpp"

Transform::Transform()
{

}

glm::mat4 Transform::getWorldMatrix()
{
    glm::mat4 scale = glm::mat4(
        this->scale.x, 0, 0, 0,
        0, this->scale.y, 0, 0,
        0, 0, this->scale.z, 0,
        0, 0, 0, 1
    );

    glm::mat4 translation = glm::mat4(
        1, 0, 0, this->position.x,
        0, 1, 0, this->position.y,
        0, 0, 1, this->position.z,
        0, 0, 0, 1
    );

    glm::mat4 result = glm::mat4_cast(this->rotation) * scale * translation;
    return result;
}

void Transform::SetPosition(glm::vec3 position)
{
    this->position = position;
}

glm::vec3 Transform::GetPosition()
{
    return this->position;
}

void Transform::Translate(glm::vec3 position)
{
    this->position += position;
}

void Transform::SetRotation(glm::quat rotation)
{
    this->rotation = rotation;
}

glm::quat Transform::GetRotation()
{
    return this->rotation;
}

void Transform::Rotate(glm::quat rotation)
{
    this->rotation = glm::normalize(this->rotation * rotation);
}

void Transform::SetScale(glm::vec3 scale)
{
    this->scale = scale;
}

glm::vec3 Transform::GetScale()
{
    return this->scale;
}

void Transform::Scale(glm::vec3 scale)
{
    this->scale += scale;
}

