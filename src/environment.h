#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <exception>
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

    void bind(GLint textureSlot);

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_environment { INVALID };
};
