#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <exception>
#include <framework/shader.h>
#include <framework/opengl_includes.h>
#include <filesystem>

class Environment {
public:
    Environment(std::filesystem::path filePath);
    Environment(const Environment&) = delete;
    Environment(Environment&&);
    ~Environment();

    Environment& operator=(const Environment&) = delete;
    Environment& operator=(Environment&&) = default;

    void bind(GLint environmentSlot);
    void bindCubeMap(GLint cubeMapSlot);
    void bindIrradianceMap(GLint irradianceMapSlot);
    void bindPrefilteredMap(GLint prefilteredMapSlot);
    void bindBRDFMap(GLint brdfMapSlot);
    void drawSkyBox(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_environment { INVALID };
    GLuint m_cubemap {INVALID};
    GLuint m_irradiencemap {INVALID};
    GLuint m_prefilteredEnvMap {INVALID};
    GLuint m_brdfmap {INVALID};
    Shader m_hdrToCubeConverterShader; // Shader used to convert the HDR map into a cube.
    Shader m_skyboxShader; // Shader to render the environment map
    Shader m_convolutionShader;
    Shader m_prefilterShader;
    Shader m_brdfShader;
    void createCubeMap(GLuint frameBuffer, GLuint renderBuffer, glm::mat4 projectionMatrix, glm::mat4 viewMatrices[]);
    void createIrradianceMap(GLuint frameBuffer, GLuint renderBuffer, glm::mat4 projectionMatrix, glm::mat4 viewMatrices[]);
    void createPrefilteredMap(GLuint frameBuffer, GLuint renderBuffer, glm::mat4 projectionMatrix, glm::mat4 viewMatrices[]);
    void createBRDFMap(GLuint frameBuffer, GLuint renderBuffer);
    void renderCube();
    void renderQuad();
    GLuint cubeVertexBuffer = 0;
    GLuint cubeVertexArray = 0;
    GLuint quadVertexBuffer = 0;
    GLuint quadVertexArray = 0;
};
