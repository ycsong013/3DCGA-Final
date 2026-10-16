#include "environment.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>
#include <stb_image.h> // For HDR environment loading

#include <iostream>
#include <filesystem>

// Like the "Texture" class used to load textures, we use this environment class
// to load environment maps, including the HDR loading logic. 
// To understand how environment maps work, we used the LearnOpenGL tutorial:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance

Environment::Environment(std::filesystem::path filePath) 
{
    // This "loading an HDR logic" was taken from the LearnOpenGL tutorial. 
    stbi_set_flip_vertically_on_load(true);
    int width, height, componentNumber;
    float *data = stbi_loadf(filePath.string().c_str(), &width, &height, &componentNumber, 0);

    if (data) {
        glGenTextures(1, &m_environment);
        glBindTexture(GL_TEXTURE_2D, m_environment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "There was an error when loading the HDR image located at " << filePath << std::endl;
    }
}

Environment::Environment(Environment&& other)
    : m_environment(other.m_environment)
{
    other.m_environment = INVALID;
}

Environment::~Environment()
{
    if (m_environment != INVALID)
        glDeleteTextures(1, &m_environment);
}

void Environment::bind(GLint environmentSlot)
{
    glActiveTexture(environmentSlot);
    glBindTexture(GL_TEXTURE_2D, m_environment);
}