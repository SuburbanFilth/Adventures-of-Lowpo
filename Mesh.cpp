#include "Mesh.hpp"
#include "OBJ_Loader.hpp"
#include <GL/glew.h>


Mesh::Mesh(const std::string& filePath)
{
    std::vector<float> data;
    objl::Loader loader;
    loader.LoadFile(filePath);
    
    glGenVertexArrays(1,&this->vertexArray);
    this->Bind();
    this->vertexCount = loader.LoadedMeshes[0].Vertices.size();

    glGenBuffers(1,&this->vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);

    for (int i = 0; i < this->vertexCount; i++)
    {
        // positions
        // std::cout << loader.LoadedMeshes[0].Vertices[i].Position.X/max << std::endl;
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Position.X);
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Position.Y);
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Position.Z);
        // normals - not sure if needed as of now.
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Normal.X);
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Normal.Y);
        data.push_back(loader.LoadedMeshes[0].Vertices[i].Normal.Z);
        // texture coords
        data.push_back(loader.LoadedMeshes[0].Vertices[i].TextureCoordinate.X);
        data.push_back(loader.LoadedMeshes[0].Vertices[i].TextureCoordinate.Y);
    }

    // send data to the video memory
    glBufferData(GL_ARRAY_BUFFER,loader.LoadedMeshes[0].Vertices.size() * 8 * sizeof(float), &data[0], GL_STATIC_DRAW);
    glGetError();
    // shader attributes
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0);
    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)(3 * sizeof(float)));
    // texture
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,32,(void*)(6 * sizeof(float)));

    this->Unbind();

}

void Mesh::Draw()
{
    glDrawArrays(GL_TRIANGLES,0,this->vertexCount);
}

void Mesh::Bind()
{
    glBindVertexArray(this->vertexArray);
}

void Mesh::Unbind()
{
    glBindVertexArray(0);
}