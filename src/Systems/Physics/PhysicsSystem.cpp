#include "PhysicsSystem.hpp"
#include <iostream>
#include "../../util.hpp"

#include "../../Components/TransformComponent.hpp"
#include "../Messaging/MoveData.hpp"
#include "../Messaging/MouseMoveData.hpp"

#include <GL/glew.h>

PhysicsSystem::PhysicsSystem(float gridLength, float cellHalfWidth) : grid(gridLength, cellHalfWidth)
{
    this->primaryBitset = ComponentType::Physics | ComponentType::Transform;
}

PhysicsSystem::~PhysicsSystem()
{

}

void PhysicsSystem::Insert(std::vector<std::shared_ptr<Collider>>& colliders)
{
    for (int i = 0; i < colliders.size(); i++)
    {
        this->grid.Insert(colliders[i]);
    }
}

void PhysicsSystem::Update(float dt, std::vector<std::unique_ptr<Entity>>& entities, std::vector<Message>& messages, std::vector<Message>& globalQueue)
{
    // build entity -> messages map
    std::unordered_map<int, std::vector<Message>> idToMessage;
    for (int i = 0; i < messages.size(); i++)
    {
        idToMessage[messages[i].senderID].push_back(messages[i]);
    }
    std::unordered_map<int, int> idToIndexMap;
    // Integration step
    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i]->IsEligibleForSystem(this->primaryBitset))
        {
            idToIndexMap[entities[i]->id] = i;

            PhysicsComponent* component = entities[i]->GetComponent<PhysicsComponent>(ComponentType::Physics);
            TransformComponent* transformComponent = entities[i]->GetComponent<TransformComponent>(ComponentType::Transform);

            if (component->dynamicType != DynamicType::Static)
            {
                // handle messages for the current entity
                if (idToMessage.find(entities[i]->id) != idToMessage.end())
                    this->HandleMessages(idToMessage[entities[i]->id], component);

                this->Integrate(dt, component);

                // Transform Component Update
                transformComponent->position = component->position;
                transformComponent->orientation = component->orientation;

                // clear accumulators
                component->forceAccumulator = glm::vec3(0.f, 0.f, 0.f);
                component->torqueAccumulator = glm::vec3(0.f, 0.f, 0.f);
            }
        }
    } 
    // 2. Check for collision
    std::vector<std::shared_ptr<Collision>> collisions = this->grid.CheckCollisions();
    // 3. Resolve Collisions
    this->Solve(entities, collisions, idToIndexMap);
    // 4. Resolve Interpenetration
    // TO DO

    this->DebugDraw(entities, collisions);
}

void PhysicsSystem::Integrate(float dt, PhysicsComponent* component)
{
    // Component integration

    // acceleration update
    component->acceleration += component->forceAccumulator * component->inverseMass;
    component->angularAcc += component->torqueAccumulator * component->invInertiaTensor;
    // velocity update
    component->velocity += component->acceleration * dt;
    component->angularVel += component->angularAcc * dt;
    // position update
    component->position += component->velocity * dt;
    component->orientation.x += 0.5f * component->orientation.x * component->angularVel.x * dt;
    component->orientation.y += 0.5f * component->orientation.y * component->angularVel.y * dt;
    component->orientation.z += 0.5f * component->orientation.z * component->angularVel.z * dt;

    // Colliders Integration
    printVector(component->position, "component position");
    // check if we need to move the object accross grid spaces
    glm::vec3 velocityChange = component->velocity * dt;
    for (int j = 0; j < component->colliders.size(); j++)
    {
        component->colliders[j]->Update(velocityChange);
        int newRow = this->grid.GetInsertRow(component->colliders[j]->center);
        int newCol = this->grid.GetInsertCol(component->colliders[j]->center);
        int oldRow = component->colliders[j]->row;
        int oldCol = component->colliders[j]->col;
        if (oldRow != newRow || oldCol != newCol)
        {
            this->grid.Remove(component->colliders[j]);
            this->grid.Insert(component->colliders[j]);
        }
    }
}

void PhysicsSystem::Solve(std::vector<std::unique_ptr<Entity>>& entities, std::vector<std::shared_ptr<Collision>>& collisions, std::unordered_map<int, int>& idToIndexMap)
{
    float ELASTICITY = .1f;
    for (int i = 0; i < collisions.size(); i++)
    {
        std::shared_ptr<Collision> collision = collisions[i];
        int firstEntityIndex = idToIndexMap[collision->first];
        int secondEntityIndex = idToIndexMap[collision->second];

        PhysicsComponent* first = entities[firstEntityIndex]->GetComponent<PhysicsComponent>(ComponentType::Physics);
        PhysicsComponent* second = entities[secondEntityIndex]->GetComponent<PhysicsComponent>(ComponentType::Physics);

        std::shared_ptr<Collider> firstCollider = collision->firstCollider;
        std::shared_ptr<Collider> secondCollider = collision->secondCollider;

        for (int j = 0; j < collision->contacts.size(); j++)
        {
            Contact contact = collision->contacts[j];
            glm::vec3 normal = contact.contactNormal;
            glm::vec3 rA = contact.contactPoint - firstCollider->center;
            glm::vec3 rB = contact.contactPoint - secondCollider->center;
            glm::vec3 vA = first->velocity;
            glm::vec3 wA = first->angularVel;
            glm::vec3 vB = second->velocity;
            glm::vec3 wB = second->angularVel;
            float invMassA = first->inverseMass;
            float invMassB = second->inverseMass;
            glm::mat3 invInertiaTensorA = first->invInertiaTensor;
            glm::mat3 invInertiaTensorB = second->invInertiaTensor;

            /*
            j = nominator / denominator
            nominator = -(1 + e) * n * ( (vA + wA x rA) - (vB + wB x rB) )
            denominator = 1/mA + 1/mB + n * ( (invIA (rA x n) x rA) + (invIB (rB x n) x rB) )
                          -----------         ---------------------   ---------------------
                                /                       /                       /
                               /                       /                       /
                            invMassSum            angularPart1           angularPart2
            */
            glm::vec3 preRelVelocity = vA + glm::cross(wA, rA) - vB - glm::cross(wB, rB);
            float nominator = glm::dot((-1.0f - ELASTICITY) * preRelVelocity, normal);
            float invMassSum = invMassA + invMassB;
            glm::vec3 angularPart1 = glm::cross(invInertiaTensorA * glm::cross(rA, normal), rA);
            glm::vec3 angularPart2 = glm::cross(invInertiaTensorB * glm::cross(rB, normal), rB);
            float angularFinal = glm::dot(angularPart1 + angularPart2, normal);
            float impulse = nominator / (invMassSum + angularFinal);
            
            if (firstCollider->dynamicType != DynamicType::Static)
            {
                first->velocity = vA + impulse * normal * invMassA;
                first->angularVel = wA + invInertiaTensorA * glm::cross(rA, impulse * normal);
            }
            if (secondCollider->dynamicType != DynamicType::Static)
            {
                second->velocity = vB - impulse * normal * invMassB;
                second->angularVel = wB - invInertiaTensorB * glm::cross(rB, impulse * normal);
            }
        }
    }
}

void PhysicsSystem::HandleMessages(std::vector<Message>& messages, PhysicsComponent* component)
{
    for (int i = 0; i < messages.size(); i++)
    {
        Message message = messages[i];
        if (message.type == MessageType::Move)
        {
            glm::vec3 result = glm::vec3(0.f, 0.f, 0.f);
            std::shared_ptr<MoveData> moveData = std::static_pointer_cast<MoveData>(message.data);
            if (moveData->forward)
                result += glm::vec3(0.f,0.f,-1.f);
            else if (moveData->backward)
                result += glm::vec3(0.f,0.f, 1.f);
            else if (moveData->left)
                result += glm::vec3(-1.f,0.f, 0.f);
            else if (moveData->right)
                result += glm::vec3(1.f,0.f, 0.f);
            printVector(result, "Setting vel to ");
            component->velocity = result;
        }
        else if (message.type == MessageType::MouseMove)
        {
            std::shared_ptr<MouseMoveData> mouseMoveData = std::static_pointer_cast<MouseMoveData>(message.data);
        }
    }
}

void PhysicsSystem::DebugDraw( std::vector<std::unique_ptr<Entity>>& entities, std::vector<std::shared_ptr<Collision>>& collisions)
{
    /*
    1. iterate over entities and render physics
    2. iterate over collisions and render contacts + normals
    */

    for (int i = 0; i < entities.size(); i++)
    {
        if (entities[i]->IsEligibleForSystem(this->primaryBitset))
        {
            PhysicsComponent* component = entities[i]->GetComponent<PhysicsComponent>(ComponentType::Physics);
            for (int j = 0; j < component->colliders.size(); j++)
            {
                const std::vector<glm::vec3>& points = component->colliders[j]->GetPoints();
                const std::vector<std::pair<int, int>>& edges = component->colliders[j]->GetEdges();

                for (int k = 0; k < edges.size(); k++)
                {
                    glBegin(GL_LINES);
                    glColor3f(1.f,0.f,0.f);
                    glVertex3d(points[edges[k].first].x, points[edges[k].first].y, points[edges[k].first].z);
                    glVertex3d(points[edges[k].second].x, points[edges[k].second].y, points[edges[k].second].z);
                    glEnd();
                }
            }
        }
    }
    for (int i = 0; i < collisions.size(); i++)
    {
        std::shared_ptr<Collision> collision = collisions[i];
        for (int j = 0; j < collision->contacts.size(); j++)
        {
            Contact contact = collision->contacts[j];
            printVector(contact.contactPoint, "contact point is ");
            glBegin(GL_POINTS);
            glColor3f(0.f,1.f,0.f);
            glVertex3d(contact.contactPoint.x, contact.contactPoint.y, contact.contactPoint.z);
            glEnd();
        }
    }
}