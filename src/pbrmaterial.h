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
#include "texture.h"

class PBRMaterial {
public:
    PBRMaterial(std::string filePath);
    PBRMaterial(const PBRMaterial&) = delete;
    PBRMaterial(PBRMaterial&&) noexcept;
    ~PBRMaterial();

    PBRMaterial& operator=(const PBRMaterial&) = delete;
    PBRMaterial& operator=(PBRMaterial&&) = default;
    void bind(GLint textureSlots[]);

private:
    Texture m_albedo;
    Texture m_ao;
    Texture m_metallic;
    Texture m_normal;
    Texture m_roughness;
};
