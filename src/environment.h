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

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_environment { INVALID };
    GLuint m_cubemap {INVALID};
    Shader m_hdrToCubeConverterShader; // Shader used to convert the HDR map into a cube.
    void createCubeMap();
    void renderCube();
    GLuint cubeVertexBuffer = 0;
    GLuint cubeVertexArray = 0;
};
