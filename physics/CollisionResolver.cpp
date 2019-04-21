#include "CollisionResolver.hpp"

CollisionResolver::CollisionResolver()
{

}

void CollisionResolver::Solve(std::vector<std::shared_ptr<Collision>>& collisions)
{
    float ELASTICITY = 0.4f;
    for (int i = 0; i < collisions.size(); i++)
    {
        std::shared_ptr<Collision> collision = collisions[i];
        std::shared_ptr<RigidBody> first = collision->first;
        std::shared_ptr<Collider> firstCollider = collision->firstCollider;
        std::shared_ptr<RigidBody> second = collision->second;
        std::shared_ptr<Collider> secondCollider = collision->secondCollider;
        for (int j = 0; j < collision->contacts.size(); j++)
        {
            Contact contact = collision->contacts[j];
            // pre - prefix used to indicate pre collision variable value
            // post - prefix used to indicate post collision variable value
            // find impulse
            glm::vec3 normal = contact.contactNormal;
            glm::vec3 rA = normal - firstCollider->center;
            glm::vec3 rB = normal - secondCollider->center;
            glm::vec3 vA = first->GetVelocity();
            glm::vec3 wA = first->GetAngularVelocity();
            glm::vec3 vB = second->GetVelocity();
            glm::vec3 wB = second->GetAngularVelocity();
            float invMassA = first->GetInverseMass();
            float invMassB = second->GetInverseMass();
            glm::mat3 invInertiaTensorA = first->GetInverseInertiaTensorWorld();
            glm::mat3 invInertiaTensorB = second->GetInverseInertiaTensorWorld();

            glm::vec3 preRelativeVelocity = vA + glm::cross(wA, rA) - vB - glm::cross(wB, rB);
            
            float denominator1 = glm::dot(normal, normal) * invMassA + invMassB;
            glm::vec3 denominator3 = glm::cross(invInertiaTensorA * glm::cross(rA, normal), rA);
            glm::vec3 denominator4 = glm::cross(invInertiaTensorB * glm::cross(rB, normal), rB);
            float denominator2 = glm::dot(denominator3 + denominator4, normal);

            float impulse = glm::dot((-1.0f - ELASTICITY) * preRelativeVelocity, normal) / (denominator1 + denominator2);
            
            // update linear velocity of objects based on impulse
            glm::vec3 vA2 = vA + impulse * normal * invMassA;
            glm::vec3 vB2 = vB - impulse * normal * invMassB;

            // update angular velocity of objects based on impulse.
            glm::vec3 wA2 = wA + invInertiaTensorA * glm::cross(rA, impulse * normal);
            glm::vec3 wB2 = wB - invInertiaTensorB * glm::cross(rB, impulse * normal);

            first->SetVelocity(vA2);
            first->SetAngularVelocity(wA2);
            
            second->SetVelocity(vB2);
            second->SetAngularVelocity(wB2);
        }
    }
}