#pragma once

#include "mesh.h"                       // GPUMesh
#include <framework/disable_all_warnings.h>
#include <framework/mesh.h>
#include <framework/shader.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>

struct Node {
    GPUMesh* mesh = nullptr;

    // mesh transform, in mesh's space, NOT inherited by children
    glm::mat4 meshTransform = glm::mat4(1.0f); // default

    // transform, in own space, inherited by children
    glm::mat4 transform = glm::mat4(1.0f); // default

    std::vector<std::unique_ptr<Node>> children;

    Node* addChild() {
        children.push_back(std::make_unique<Node>());
        return children.back().get();
    }
};

void buildBoat(Node& root, GPUMesh& hullMesh, GPUMesh& boxMesh);

Mesh makeHull();

Mesh makeUnitBox();
