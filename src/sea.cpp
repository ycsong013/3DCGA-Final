#include "sea.h"
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

Mesh makeGrid(float size, int resolution) {
    // The complete grid is size * size
    // res * res small squares, each square is 2 triangles
    Mesh mesh;

    const auto idx = static_cast<glm::uint>(mesh.vertices.size());
    glm::vec3 normal = glm::vec3(0, 1, 0);
    
    // VERTICES
    for (int i = 0; i <= resolution; i++) {
        for (int j = 0; j <= resolution; j++) {
            float u = static_cast<float>(j) / static_cast<float>(resolution);
            float v = static_cast<float>(i) / static_cast<float>(resolution);

            mesh.vertices.push_back(Vertex{
                .position = glm::vec3(-size / 2 + u * size, 0, size / 2 - v * size),
                .normal = normal, 
                .texCoord = {u, v} 
                });
        }
    }

    // TRIANGLES
    for (int i = 0; i < resolution; i++) {
        for (int j = 0; j < resolution; j++) {
            mesh.triangles.push_back(glm::uvec3(
                idx + i * (resolution + 1) + j, 
                idx + i * (resolution + 1) + j + 1, 
                idx + (i + 1) * (resolution + 1) + j));
            mesh.triangles.push_back(glm::uvec3(
                idx + i * (resolution + 1) + j + 1,
                idx + (i + 1) * (resolution + 1) + j + 1,
                idx + (i + 1) * (resolution + 1) + j));
        }
    }

    // MATERIAL
    mesh.material.kd = glm::vec3(0.5f, 0.75f, 1.0f);

    return mesh;
}