#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Mesh
{
    public:
    
        Mesh(unsigned int vertexArray, unsigned int vertexBuffer, unsigned int vertexCount);
        void Draw();

        void Bind();
        void Unbind();

    private:
        GLuint vertexArray;
        GLuint vertexBuffer;
        GLuint vertexCount;
};