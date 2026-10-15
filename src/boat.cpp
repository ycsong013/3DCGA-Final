#include "boat.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <numeric>
#include <span>
#include <stack>
#include <string>
#include <tuple>
#include <map>

static void addFaceOfUnitBox(Mesh& mesh, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 normal) {
    const auto idx = static_cast<glm::uint>(mesh.vertices.size());

    // VERTICES
    mesh.vertices.push_back(Vertex{ .position = p0, .normal = normal, .texCoord = { 0.0f, 0.0f } });
    mesh.vertices.push_back(Vertex{ .position = p1, .normal = normal, .texCoord = { 1.0f, 0.0f } });
    mesh.vertices.push_back(Vertex{ .position = p2, .normal = normal, .texCoord = { 1.0f, 1.0f } });
    mesh.vertices.push_back(Vertex{ .position = p3, .normal = normal, .texCoord = { 0.0f, 1.0f } });

    // TRIANGLES
    mesh.triangles.push_back(glm::uvec3(idx, idx + 1, idx + 2));
    mesh.triangles.push_back(glm::uvec3(idx, idx + 2, idx + 3));
}

Mesh makeUnitBox() {
    Mesh mesh;

    // Front face
    addFaceOfUnitBox(mesh, glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0, 0, 1));

    // Back face
    addFaceOfUnitBox(mesh, glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0, 0, -1));

    // Top face
    addFaceOfUnitBox(mesh, glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 1, 0));

    // Bottom face
    addFaceOfUnitBox(mesh, glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0, -1, 0));

    // Left face
    addFaceOfUnitBox(mesh, glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1, 0, 0));

    // Right face
    addFaceOfUnitBox(mesh, glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1, 0, 0));

    // MATERIAL
    mesh.material.kd = glm::vec3(1.0f, 0.5f, 0.25f);

    return mesh;
}