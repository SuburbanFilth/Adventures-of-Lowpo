#include <GL/glew.h>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

#include "Player.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ResourceManager.hpp"
#include "OBJ_Loader.hpp"
#include "ofbx.hpp"

ResourceManager::ResourceManager()
{
    
}

std::shared_ptr<Terrain> ResourceManager::LoadWorld(const std::string& filePath, std::unique_ptr<Scene>& scene)
{
    // texture loading
    std::shared_ptr<Texture> texture = std::make_shared<Texture>("/home/martin/Documents/Projects/Adventures-of-Lowpo/resources/color_palette.png");
    std::cout << texture->ID << std::endl;
    objl::Loader loader;
    loader.LoadFile(filePath);
    bool isHitbox = false;
    std::map<std::string,std::shared_ptr<Mesh>> hitboxMap;
    Material material = Material(glm::vec3(),glm::vec3(),glm::vec3(), 0.f);
    std::shared_ptr<Terrain> result;
    for (int j = 0; j < loader.LoadedMeshes.size(); j++)
    {
        if (loader.LoadedMeshes[j].MeshName == "Plane")
        {
            result = this->LoadTerrain(loader.LoadedMeshes[j].Vertices,33,8);
            result->texture = texture;
            std::cout << "terrain should be loaded" << std::endl;
        }
        else if (loader.LoadedMeshes[j].MeshName == "Player")
        {
            this->LoadPlayer(loader.LoadedMeshes[j].Vertices, scene, texture);
        }
        else
        {
            std::vector<float> data;
            isHitbox = false;
            std::size_t found = loader.LoadedMeshes[j].MeshName.find("Hitbox");
            if (found != std::string::npos)
            {
                isHitbox = true;
            }
            if (!isHitbox)
            {
                for (int i = 0; i < loader.LoadedMeshes[j].Vertices.size(); i++)
                {
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Position.X);
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Position.Y);
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Position.Z);
                    
                    // Normals
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Normal.X);
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Normal.Y);
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].Normal.Z);

                    // texture
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].TextureCoordinate.X);
                    data.push_back(loader.LoadedMeshes[j].Vertices[i].TextureCoordinate.Y);
                        
                    // std::cout << "X is " << loader.LoadedMeshes[j].Vertices[i].TextureCoordinate.X << std::endl;
                    // std::cout << "Y is " << loader.LoadedMeshes[j].Vertices[i].TextureCoordinate.Y << std::endl;
                }
                std::pair<unsigned int, unsigned int> buffers = this->SetupBuffers(data.data(), data.size() * sizeof(float));
                glm::vec3 ambient = glm::vec3(loader.LoadedMeshes[j].MeshMaterial.Ka.X, loader.LoadedMeshes[j].MeshMaterial.Ka.Y, loader.LoadedMeshes[j].MeshMaterial.Ka.Z);
                glm::vec3 diffuse = glm::vec3(loader.LoadedMeshes[j].MeshMaterial.Kd.X, loader.LoadedMeshes[j].MeshMaterial.Kd.Y, loader.LoadedMeshes[j].MeshMaterial.Kd.Z);
                glm::vec3 specular = glm::vec3(loader.LoadedMeshes[j].MeshMaterial.Ks.X, loader.LoadedMeshes[j].MeshMaterial.Ks.Y, loader.LoadedMeshes[j].MeshMaterial.Ks.Z);
                float shine = loader.LoadedMeshes[j].MeshMaterial.Ns;
                material.ambient = ambient;
                material.diffuse = diffuse;
                material.specular = specular;
                material.shine = shine;
                std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh(buffers.first, buffers.second, loader.LoadedMeshes[j].Vertices.size()));
                hitboxMap[loader.LoadedMeshes[j].MeshName] = mesh;
            }
        }
        
    }
    for (int j = 0; j < loader.LoadedMeshes.size(); j++)
    {
        if (loader.LoadedMeshes[j].MeshName == "Plane" || loader.LoadedMeshes[j].MeshName == "Player")
        {
            continue;
        }
        isHitbox = false;
        std::size_t found = loader.LoadedMeshes[j].MeshName.find("Hitbox");
        if (found != std::string::npos)
        {
            isHitbox = true;
        }

        glm::vec3 min, max;
        

        for (int i = 0; i < loader.LoadedMeshes[j].Vertices.size(); i++)
        {
            if ((i == 0) && isHitbox)
            {
                max.x = loader.LoadedMeshes[j].Vertices[i].Position.X;
                max.y = loader.LoadedMeshes[j].Vertices[i].Position.Y;
                max.z = loader.LoadedMeshes[j].Vertices[i].Position.Z;

                min.x = loader.LoadedMeshes[j].Vertices[i].Position.X;
                min.y = loader.LoadedMeshes[j].Vertices[i].Position.Y;
                min.z = loader.LoadedMeshes[j].Vertices[i].Position.Z;

            } else if ((i != 0) && isHitbox) {
                if (max.x < loader.LoadedMeshes[j].Vertices[i].Position.X)
                    max.x = loader.LoadedMeshes[j].Vertices[i].Position.X;
                if (max.y < loader.LoadedMeshes[j].Vertices[i].Position.Y)
                    max.y = loader.LoadedMeshes[j].Vertices[i].Position.Y;
                if (max.z > loader.LoadedMeshes[j].Vertices[i].Position.Z)
                    max.z = loader.LoadedMeshes[j].Vertices[i].Position.Z;

                if (min.x > loader.LoadedMeshes[j].Vertices[i].Position.X)
                    min.x = loader.LoadedMeshes[j].Vertices[i].Position.X;
                if (min.y > loader.LoadedMeshes[j].Vertices[i].Position.Y)
                    min.y = loader.LoadedMeshes[j].Vertices[i].Position.Y;
                if (min.z < loader.LoadedMeshes[j].Vertices[i].Position.Z)
                    min.z = loader.LoadedMeshes[j].Vertices[i].Position.Z;
            }

        }
        if (isHitbox)
        {
            std::string first = loader.LoadedMeshes[j].MeshName.substr(0,loader.LoadedMeshes[j].MeshName.find("_"));
            std::string second = loader.LoadedMeshes[j].MeshName.substr(loader.LoadedMeshes[j].MeshName.find("."), 4);
            std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(GameObject(Transform(), texture, hitboxMap[first+second], PhysicsComponent(min,max, glm::vec3(0.f,0.f,0.f),ObjectType::Static),material));
            scene->gameObjects.push_back(gameObject);
        }
    }
    return result;
}

std::shared_ptr<Terrain> ResourceManager::LoadTerrain(std::vector<objl::Vertex>& vertices, int gridSize, int cellSize)
{

    // TODO (Martin) : update terrain code to use planes in a quadtree

    // build the heightmap
    
    std::shared_ptr<std::vector<std::vector<float>>> map = std::make_shared<std::vector<std::vector<float>>>(gridSize);
    for (int i = 0; i < gridSize; i++)
    {
        (*map)[i] = std::vector<float>(gridSize);
    }

    std::vector<float>  data;
    
    for (int i = 0; i < vertices.size(); i++)
    {
        int z = -((int)vertices[i].Position.Z)/cellSize;
        int x = (int)vertices[i].Position.X/cellSize;
        (*map)[z][x] = vertices[i].Position.Y;
    }   

    for (int i = 0; i < vertices.size();i++)
    {
        data.push_back(vertices[i].Position.X);
        data.push_back(vertices[i].Position.Y);
        data.push_back(vertices[i].Position.Z);

        // Normals per face
        data.push_back(vertices[i].Normal.X);
        data.push_back(vertices[i].Normal.Y);
        data.push_back(vertices[i].Normal.Z);

        // texture
        data.push_back(vertices[i].TextureCoordinate.X);
        data.push_back(vertices[i].TextureCoordinate.Y);
    }
    std::pair<unsigned int, unsigned int> buffers = this->SetupBuffers(data.data(), data.size() * sizeof(float));
    glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f);
    std::shared_ptr<Terrain> terrain = std::make_shared<Terrain>(Terrain(buffers.first, buffers.second, data.size() / 6.0f, Transform(), nullptr ,map, cellSize,cellSize, position));
    return terrain;
}

void ResourceManager::LoadPlayer(std::vector<objl::Vertex>& vertices, std::unique_ptr<Scene>& scene, std::shared_ptr<Texture> texture)
{
    std::vector<float> data;
    glm::vec3 min, max;
    for (int i = 0; i < vertices.size(); i++)
    {
        if (i == 0)
        {
            max.x = vertices[i].Position.X;
            max.y = vertices[i].Position.Y;
            max.z = vertices[i].Position.Z;

            min.x = vertices[i].Position.X;
            min.y = vertices[i].Position.Y;
            min.z = vertices[i].Position.Z;

        } else if (i != 0) {
            if (max.x < vertices[i].Position.X)
                max.x = vertices[i].Position.X;
            if (max.y < vertices[i].Position.Y)
                max.y = vertices[i].Position.Y;
            if (max.z > vertices[i].Position.Z)
                max.z = vertices[i].Position.Z;

            if (min.x > vertices[i].Position.X)
                min.x = vertices[i].Position.X;
            if (min.y > vertices[i].Position.Y)
                min.y = vertices[i].Position.Y;
            if (min.z < vertices[i].Position.Z)
                min.z = vertices[i].Position.Z;
        }

        data.push_back(vertices[i].Position.X);
        data.push_back(vertices[i].Position.Y);
        data.push_back(vertices[i].Position.Z);

        data.push_back(vertices[i].Normal.X);
        data.push_back(vertices[i].Normal.Y);
        data.push_back(vertices[i].Normal.Z);

        data.push_back(vertices[i].TextureCoordinate.X);
        data.push_back(vertices[i].TextureCoordinate.Y);
    }

    std::pair<unsigned int, unsigned int> buffers = this->SetupBuffers(data.data(), data.size() * sizeof(float));

    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh(buffers.first,buffers.second, vertices.size()));
    Material material = Material(glm::vec3(),glm::vec3(),glm::vec3(),0.f);
    std::shared_ptr<Player> player = std::make_shared<Player>(Player(Transform(), texture, mesh, PhysicsComponent(min,max, glm::vec3(0.f,-10.f,0.f) , ObjectType::Dynamic), material,(float)800, (float)600));
    std::shared_ptr<Camera> camera = player->GetCamera();
    scene->AddCamera(camera);
    scene->gameObjects.push_back(player);
    
}

std::pair<unsigned int,unsigned int> ResourceManager::SetupBuffers(float* data, int size)
{
    unsigned int VAO,VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),(void*)(sizeof(float)*3));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(sizeof(float)*6));

    return std::make_pair(VAO,VBO);
}

std::vector<float> ResourceManager::GenerateNormals(std::vector<float>& vertices, int width, int length)
{
    std::vector<float> normals;
    for (int z = 0; z < length; z++)
    {
        for (int x = 0; x < width; x++)
        {
            if (z == 0 || z == length-1 || x == 0 || x == width-1)
            {
                // handle outer cases
                // outer non corner
                // outer corner

                normals.push_back(0.f);
                normals.push_back(1.f);
                normals.push_back(0.f);
            }
            else
            {
                // handle inner cases
                // get current index
                // get 6 vectors touching the current vertex
                // compute their normals and sum them together
                int currentIndex = (z * width + x) * 3;

                // FIRST
                glm::vec3 first;
                first.x = vertices[currentIndex - width * 3] - vertices[currentIndex];
                first.y = vertices[currentIndex - width * 3 + 1] - vertices[currentIndex+1];
                first.z = vertices[currentIndex - width * 3 + 2] - vertices[currentIndex+2];
                
                // SECOND
                glm::vec3 second;
                second.x = vertices[currentIndex + 3] - vertices[currentIndex];
                second.y = vertices[currentIndex + 4] - vertices[currentIndex+1];
                second.z = vertices[currentIndex + 5] - vertices[currentIndex+2];

                // FOURTH
                glm::vec3 fourth;
                fourth.x = vertices[currentIndex + width*3] - vertices[currentIndex];
                fourth.x = vertices[currentIndex + width*3 + 1] - vertices[currentIndex + 1];
                fourth.x = vertices[currentIndex + width*3 + 2] - vertices[currentIndex + 2];

                // FIFTH
                glm::vec3 fifth;
                fifth.x = vertices[currentIndex - 3] - vertices[currentIndex];
                fifth.y = vertices[currentIndex - 2] - vertices[currentIndex + 1];
                fifth.z = vertices[currentIndex - 1] - vertices[currentIndex + 2];

                glm::vec3 result(0.0f,0.0f,0.0f);
                result += glm::cross(second,first);
                result += glm::cross(first,fifth);
                result += glm::cross(fifth,fourth);
                result += glm::cross(fourth,second);
                result = glm::normalize(result);
                normals.push_back(result.x);
                normals.push_back(result.y);
                normals.push_back(result.z);
            }
        }
    }
    return normals;
}

void ResourceManager::LoadAnimatedObject(const std::string& filePath)
{
    std::ifstream       file;
    std::stringstream   fileString;

    // open the file and load the content in the buffer.
    file.open(filePath,std::ios_base::binary);
    fileString << file.rdbuf();

    // get the size
    file.seekg(0,std::ios_base::end);
    int size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    char* cstring = new char[size];
    fileString.read(cstring,size);
    
    ofbx::IScene* scene = ofbx::load((ofbx::u8*)cstring,size);
    for (int i=0; i < scene->getAnimationStackCount() ; i++)
    {
        const ofbx::Object* stack = scene->getAnimationStack(i);
    }
}

unsigned int ResourceManager::LoadTexture(const std::string& filePath)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return texture;
}