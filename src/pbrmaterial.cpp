
#include "pbrmaterial.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>
#include "texture.h"

#include <iostream>

PBRMaterial::PBRMaterial(std::string filePath) 
: m_albedo(filePath + "albedo.png"),
m_ao(filePath + "ao.png"),
m_metallic(filePath + "metallic.png"),
m_normal(filePath + "normal-ogl.png"),
m_roughness(filePath + "roughness.png")
{
    glm::vec3 hey = glm::vec3(1.0f);
}

PBRMaterial::PBRMaterial(PBRMaterial&& other) noexcept
    : m_albedo(std::move(other.m_albedo)),
    m_ao(std::move(other.m_ao)),
    m_metallic(std::move(other.m_metallic)),
    m_normal(std::move(other.m_normal)),
    m_roughness(std::move(other.m_roughness))
{
}

void PBRMaterial::bind(GLint textureSlots[])
{
    m_albedo.bind(textureSlots[0]);
    m_ao.bind(textureSlots[1]);
    m_metallic.bind(textureSlots[2]);
    m_normal.bind(textureSlots[3]);
    m_roughness.bind(textureSlots[4]);
}

PBRMaterial::~PBRMaterial() = default;
