#include "environment.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>
#include <stb/stb_image.h>

#include <iostream>
#include <filesystem>
#include <framework/shader.h>
#include <glm/gtc/type_ptr.hpp>

// Like the "Texture" class used to load textures, we use this environment class
// to load environment maps, including the HDR loading logic. 
// The class creates a skybox from a given environment map -> This conversion is done
// only once in the constructor. Later, the skybox has to be updated whenever the view
// changes, which is why there is an available "drawSkybox" function
// for the main application code to call.
// To understand how environment maps work, we used the LearnOpenGL tutorial:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance.

// In this code, in the "createCube" function, the vertices were taken directly from LearnOpenGL. We
// did not think that we would be able to think of these numbers on our own from scratch.
//  The view matrix values are also taken from LearnOpenGL.

// As mentioned, the rest of it was written on our own, taking notes from LearnOpenGL's tutorial
// and then trying to write the code without looking at the website.

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
        stbi_set_flip_vertically_on_load(false);
        createCubeMap();
        ShaderBuilder skyboxShaderBuilder;

        // Prepare the skybox shader for whenever we need to create the skybox.
        skyboxShaderBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
        skyboxShaderBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
        m_skyboxShader = skyboxShaderBuilder.build();
    }
    else
    {
        std::cout << "There was an error when loading the HDR image located at " << filePath << std::endl;
    }
}

void Environment::createCubeMap()
{
    ShaderBuilder hdrToCubeConverterBuilder;
    hdrToCubeConverterBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/cube_vert.glsl");
    hdrToCubeConverterBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/cube_frag.glsl");
    m_hdrToCubeConverterShader = hdrToCubeConverterBuilder.build();

    // Create the frame and render bufferers
    GLuint frameBuffer;
    GLuint renderBuffer;

    glGenFramebuffers(1, &frameBuffer);
    glGenRenderbuffers(1, &renderBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);

    // Bind the render buffer to the frame buffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512); // 512 x 512 is the resolution of the cubemap
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

    glGenTextures(1, &m_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);

    for (int i = 0; i < 6; i++) { // Generate a texture for each of the 6 faces of the cubemap from the HDR.
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // These values below, as they are specific to the cube creation, have been copied and pasted from LearnOpenGL's tutorial above.

    glm::mat4 projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 viewMatrices[] = 
    {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // The rest of this code is again written on our own without looking at the tutorial.

    m_hdrToCubeConverterShader.bind();
    glUniformMatrix4fv(m_hdrToCubeConverterShader.getUniformLocation("projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    bind(GL_TEXTURE0); // Bind the HDR image texture to slot 0
    glUniform1i(m_hdrToCubeConverterShader.getUniformLocation("environmentMap"), 0); // Pass slot 0 as sampler2D.
    
    glViewport(0, 0, 512, 512); // Configure the viewport

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    for (int i = 0; i < 6; i++) {
        glUniformMatrix4fv(m_hdrToCubeConverterShader.getUniformLocation("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrices[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_cubemap, 0); // Load the cubemap texture to the frame buffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color buffer and the depth buffer, so empty the frame buffer's content.

        renderCube();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // From this point on (as we initialized the environment, the HDR turned into the cube map, and "m_cubemap" has this information.)

    glViewport(0, 0, 1024, 1024); // Reset back to the previous size of the viewport.
    glDeleteFramebuffers(1, &frameBuffer);
    glDeleteRenderbuffers(1, &renderBuffer);
}

// The matrices here need to come from the MVP! There is no model matrix needed because the cube is all around the scene.
void Environment::drawSkyBox(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
    m_skyboxShader.bind();
    glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(m_skyboxShader.getUniformLocation("projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    bindCubeMap(GL_TEXTURE0);
    glUniform1i(m_skyboxShader.getUniformLocation("skyboxMap"), 0);
    renderCube();
}

Environment::Environment(Environment&& other)
    : m_environment(other.m_environment), m_cubemap(other.m_cubemap)
{
    other.m_environment = INVALID;
    other.m_cubemap = INVALID;
}

Environment::~Environment()
{
    if (m_environment != INVALID) {
        glDeleteTextures(1, &m_environment);
    }
    
    if (m_cubemap != INVALID) {
        glDeleteTextures(1, &m_cubemap);
    }
}

void Environment::bind(GLint environmentSlot)
{
    glActiveTexture(environmentSlot);
    glBindTexture(GL_TEXTURE_2D, m_environment);
}

void Environment::bindCubeMap(GLint cubeMapSlot)
{
    glActiveTexture(cubeMapSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
}

void Environment::renderCube() 

{
    if (cubeVertexArray == 0) {
        // These vertices are taken from the LearnOpenGL tutorial directly. As 
        // mentioned, we did not think it was useful to write these down from scratch.
        float cubeVertices[] = // back face
             { -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left       
        };

        glGenVertexArrays(1, &cubeVertexArray);
        glGenBuffers(1, &cubeVertexBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVertexBuffer);

        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW); // Static draw means: We load these cube vertices once and they will never change.

        glBindVertexArray(cubeVertexArray);

        // Add in the information of each float per vertex -> There are 8 floats per vertex,
        // so do 3 + 3 + 2
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0); // Stride: The number of bytes from one vertex to the next, offset: where to start reading
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3* sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

    }

    glBindVertexArray(cubeVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 36); 
    glBindVertexArray(0);
}