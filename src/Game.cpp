#define GLEW_STATIC

#include <memory>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "Game.hpp"
#include "Loader.hpp"
#include "Entity.hpp"
#include "External/tinyxml2.hpp"
#include "Components/InputComponent.hpp"
#include "Components/PhysicsComponent.hpp"
#include "Components/RenderingComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Systems/Physics/ColliderBuilder.hpp"

#include "util.hpp"


void APIENTRY openglCallbackFunction(GLenum source,
                                           GLenum type,
                                           GLuint id,
                                           GLenum severity,
                                           GLsizei length,
                                           const GLchar* message,
                                           const void* userParam)
{
    using namespace std;
    cout << "---------------------opengl-callback-start------------" << endl;
    cout << "message: "<< message << endl;
    cout << "type: ";
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        cout << "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        cout << "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        cout << "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        cout << "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        cout << "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        cout << "OTHER";
        break;
    }
    cout << endl;
 
    cout << "id: " << id << endl;
    cout << "severity: ";
    switch (severity){
    case GL_DEBUG_SEVERITY_LOW:
        cout << "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        cout << "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        cout << "HIGH";
        break;
    }
    cout << endl;
    cout << "---------------------opengl-callback-end--------------" << endl;
}

Game::Game(int width, int height) : width(width), 
                                    height(height) , 
                                    physicsSystem(70.f, 5.f),
                                    renderingSystem(),
                                    currentID(1),
                                    messageToSystem(MessageType::MessageTypeEnd),
                                    systemToMessage(System::SystemEnd)
{
    this->Init();
    // adding shaders after the init as we need to initialize OPENGL before
    // we do anythin with it. It happens in the InitConfig method.
    this->renderingSystem.AddShaders(std::vector<std::string>{  "./src/Systems/Rendering/vertex.glsl", 
                                                                "./src/Systems/Rendering/fragment.glsl"}, 
                                    std::vector<std::string>{   "./src/Systems/Rendering/vertexShadow.glsl", 
                                                                "./src/Systems/Rendering/fragmentShadow.glsl"});
}

void Game::Init()
{
    this->InitConfig();
    std::string filename = "resources/test.dae";
    this->InitScene(filename, this->entities);
    // subscribe
    this->Subscribe(MessageType::MouseMove, System::RenderingSys);
    this->Subscribe(MessageType::Move, System::PhysicsSys);
    this->Subscribe(MessageType::MouseMove, System::PhysicsSys);
}

void Game::InitConfig()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // GLFW_OPENGL_CORE_PROFILE
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // Create window
    this->window = glfwCreateWindow(this->width, this->height, "Adventures of Lowpo", nullptr, nullptr);
    glfwMakeContextCurrent(this->window);
    // Disable mouse within window
    

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glEnable(GL_DEPTH_TEST);
    glewInit();
    glGetError();


    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openglCallbackFunction, nullptr);
    GLuint unusedIds = 0;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
}

void Game::InitScene(std::string filename, std::vector<std::unique_ptr<Entity>>& entities)
{
    unsigned int textureID = this->renderingSystem.CreateTexture("resources/DiffuseColor_Texture.png");
    tinyxml2::XMLDocument document;
    // TODO : check for the extension and report error if different from .dae
    tinyxml2::XMLError error = document.LoadFile(filename.c_str());
    if (error != 0)
        return;
    // the document represents the "whole" file so we need to qu
    // is always Collada
    tinyxml2::XMLElement* collada = document.FirstChildElement("COLLADA");
    
    // parse visual scenes for the world transforms.
    std::unordered_map<std::string, std::shared_ptr<InstanceGeometry>> instanceGeometries;
    tinyxml2::XMLElement* libraryVisualScenes = collada->FirstChildElement("library_visual_scenes");
    instanceGeometries = Loader::ParseVisualScenesStatic(libraryVisualScenes);

    // parse geometries
    // one object - > many colliders
    std::unordered_map<std::string, std::vector<std::shared_ptr<Collider>> > objectToColliders;
    std::unordered_map<std::string, std::shared_ptr<Geometry>> geometry;
    tinyxml2::XMLElement* libraryGeometries = collada->FirstChildElement("library_geometries");
    geometry = Loader::ParseGeometry(libraryGeometries);
    // first iteration to get hitboxes
    for (std::unordered_map<std::string, std::shared_ptr<Geometry>>::iterator it = geometry.begin(); it != geometry.end(); it++)
    {
        // Three scenarios
        // is hitbox
        // is not hitbox
        // is terrain
        bool isHitbox = it->first.find("_hitbox") != it->first.npos;
        if (isHitbox)
        {
            std::vector<glm::vec3> points;
            int hitboxIndex = it->first.find("_hitbox");
            std::string objectName = it->first.substr(0, hitboxIndex);
            // take thhe vertices and make a collider.
            for (int i = 0; i < it->second->vertices.size(); i += 3)
            {
                float x = it->second->vertices[i];
                float y = it->second->vertices[i + 1];
                float z = it->second->vertices[i + 2];
                glm::vec3 point = glm::vec3(x,y,z);
                points.push_back(point);
            }
            DynamicType type = DynamicType::Static;
            if (objectName == "player")
                type = DynamicType::Dynamic;
            std::shared_ptr<Collider> collider = ColliderBuilder::Build(0, type, points);
            objectToColliders[objectName].push_back(collider);
        }
    }
    std::vector<float> bufferData;
    glm::mat4 worldTransform;
    // second iteration to create game entities
    for (std::unordered_map<std::string, std::shared_ptr<Geometry>>::iterator it = geometry.begin(); it != geometry.end(); it++)
    {
        bool isHitbox = it->first.find("_hitbox") != it->first.npos;
        if (isHitbox)
            continue;

        std::shared_ptr<Geometry>   current = it->second;
        std::unique_ptr<Entity>     entity = std::make_unique<Entity>(this->CreateEntityID());

        bufferData = Loader::BuildBufferData(current);
        worldTransform = instanceGeometries[it->first]->matrix;
        std::pair<unsigned int, unsigned int> buffers = RenderingSystem::BufferData(bufferData.data(), bufferData.size() * sizeof(float), false);
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldTransform, scale, rotation, translation, skew, perspective);
        rotation = glm::conjugate(rotation);
        // RenderingComponent
        // we need to have
        // VAO, VBO and Texture loaded.
        std::unique_ptr<RenderingComponent> renderingComponent = std::make_unique<RenderingComponent>(buffers.first, buffers.second, bufferData.size() / 3, textureID, ShaderType::NormalShader);
        // PhysicsComponent
        DynamicType type = DynamicType::Static;
        float mass = 1.0f;
        if (it->first == "player")
            type = DynamicType::Dynamic;
        else
            mass = 1000.f;
        printVector(translation, "INIT TRANSLATION");
        std::unique_ptr<PhysicsComponent> physicsComponent = std::make_unique<PhysicsComponent>(1.f, translation, rotation, glm::mat3(1.f), type);
        // assign colliders to component and insert into grid
        physicsComponent->colliders = objectToColliders[it->first];
        for (int k = 0;k < physicsComponent->colliders.size(); k++)
        {
            physicsComponent->colliders[k]->entityID = entity->id;
        }
        this->physicsSystem.Insert(physicsComponent->colliders);
        std::unique_ptr<TransformComponent> transformComponent = std::make_unique<TransformComponent>(translation, rotation);
        
        // Entity
        entity->AddComponent(std::move(renderingComponent));
        entity->AddComponent(std::move(physicsComponent));
        entity->AddComponent(std::move(transformComponent));
        if (it->first == "player")
        {
            this->playerID = entity->id;
            std::unique_ptr<InputComponent> inputComponent = std::make_unique<InputComponent>();
            entity->AddComponent(std::move(inputComponent));
        }
        entities.push_back(std::move(entity));
    }
    // Push back an entity
}

    
void Game::Update(float deltaTime)
{
    // FIXME : this should not be here
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);
    // dispatch here
    this->Dispatch();
    // System Update
    this->inputSystem.Update(this->window, this->entities, this->systemToMessage[System::InputSys], this->globalQueue);
    this->physicsSystem.Update(deltaTime, this->entities, this->systemToMessage[System::PhysicsSys], this->globalQueue);
    this->animationSystem.Update(deltaTime, this->entities, this->systemToMessage[System::AnimationSys], this->globalQueue);
    this->renderingSystem.Update(this->entities, this->playerID, this->systemToMessage[System::RenderingSys], this->globalQueue);
}

void Game::Run()
{
    float deltaTime = 1.f/60.f;
    while (!glfwWindowShouldClose(window))
    {   
        glfwPollEvents();
        this->Update(deltaTime);
        glfwSwapBuffers(window);
        unsigned int error = glGetError();
        if (error != 0)
            std::cout << error << std::endl;
    }

    glfwTerminate();
}

int Game::CreateEntityID()
{
    return this->currentID++;
}

void Game::Subscribe(MessageType message, System system)
{
    this->messageToSystem[message].push_back(system);
}

void Game::Unsubscribe(MessageType message, System system)
{
    for (int i = 0; i < this->messageToSystem[message].size(); i++)
    {
        if (this->messageToSystem[message][i] == system)
            this->messageToSystem[message].erase(this->messageToSystem[message].begin() + i);
    }
}

void Game::Dispatch()
{
    // clear previous frame messages
    for (int i = 0; i < System::SystemEnd; i++)
    {
        this->systemToMessage[i].clear();
    }
    // dispatch messages
    for (int i = 0;i < this->globalQueue.size(); i++)
    {
        Message message = this->globalQueue[i];
        std::vector<System> interestedSystems = this->messageToSystem[message.type];
        for (int j = 0; j < interestedSystems.size(); j++)
        {
            System system = interestedSystems[j];
            this->systemToMessage[system].push_back(message);
        }
    }
    this->globalQueue.clear();
}