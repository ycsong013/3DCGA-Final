#include "boat.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

const glm::mat4 idt = glm::mat4(1.0f); // identity matrix

void buildBoat(Node& root, GPUMesh& hullMesh, GPUMesh& boxMesh) {
    root.transform = glm::translate(idt, glm::vec3(0.0f, 0.0f, 0.0f));

    Node* hull = root.addChild();
    hull->mesh = &hullMesh;

    Node* mast = hull->addChild();
    mast->transform = glm::translate(idt, glm::vec3(0.0f, 1.0f, -0.5f));
    mast->mesh = &boxMesh;
    mast->meshTransform = glm::translate(idt, glm::vec3(0.0f, 1.5f, 0.0f))
        * glm::scale(idt, glm::vec3(0.1f, 3.0f, 0.1f));

    Node* boom = mast->addChild();
    boom->transform = glm::translate(idt, glm::vec3(0.0f, 0.5f, 0.05f))
        * glm::rotate(idt, glm::radians(90.0f), glm::vec3(0, 1, 0));
    boom->mesh = &boxMesh;
    boom->meshTransform = glm::translate(idt, glm::vec3(0.0f, 0.0f, 0.75f))
        * glm::scale(idt, glm::vec3(0.1f, 0.1f, 1.5f));
}

// The verteces should follow a counter-clockwise order
static void addTriangle(Mesh& mesh, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
    glm::vec3 edge1 = p1 - p0;
    glm::vec3 edge2 = p2 - p0;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    const auto idx = static_cast<glm::uint>(mesh.vertices.size());

    // VERTICES
    mesh.vertices.push_back(Vertex{ .position = p0, .normal = normal, .texCoord = { 0.0f, 0.0f } });
    mesh.vertices.push_back(Vertex{ .position = p1, .normal = normal, .texCoord = { 1.0f, 0.0f } });
    mesh.vertices.push_back(Vertex{ .position = p2, .normal = normal, .texCoord = { 1.0f, 1.0f } });

    // TRIANGLES
    mesh.triangles.push_back(glm::uvec3(idx, idx + 1, idx + 2));
}

// The verteces should follow a counter-clockwise order
static void addQuadrilateral(Mesh& mesh, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    addTriangle(mesh, p0, p1, p2);
    addTriangle(mesh, p0, p2, p3);
}

Mesh makeHull() {
    Mesh mesh;

    // Points of the hull
    glm::vec3 backUpperRight = glm::vec3(0.5f, 1.0f, 1.5f);
    glm::vec3 backUpperLeft = glm::vec3(-0.5f, 1.0f, 1.5f);
    glm::vec3 backLowerLeft = glm::vec3(-0.5f, 0.0f, 1.5f);
    glm::vec3 backLowerRight = glm::vec3(0.5f, 0.0f, 1.5f);
    glm::vec3 frontUpperRight = glm::vec3(0.5f, 1.0f, -0.5f);
    glm::vec3 frontUpperLeft = glm::vec3(-0.5f, 1.0f, -0.5f);
    glm::vec3 frontLowerLeft = glm::vec3(-0.5f, 0.0f, -0.5f);
    glm::vec3 frontLowerRight = glm::vec3(0.5f, 0.0f, -0.5f);
    glm::vec3 bowTip = glm::vec3(0.0f, 1.0f, -1.5f);

    // Back side of the hull
    addQuadrilateral(mesh, backUpperRight, backUpperLeft, backLowerLeft, backLowerRight);
    // Right side of the hull
    addQuadrilateral(mesh, frontUpperRight, backUpperRight, backLowerRight, frontLowerRight);
    // Left side of the hull
    addQuadrilateral(mesh, backUpperLeft, frontUpperLeft, frontLowerLeft, backLowerLeft);
    // Top side of the hull
    addQuadrilateral(mesh, backUpperRight, frontUpperRight, frontUpperLeft, backUpperLeft);
    // Bottom side of the hull
    addQuadrilateral(mesh, backLowerLeft, frontLowerLeft, frontLowerRight, backLowerRight);

    // Top side of the bow
    addTriangle(mesh, bowTip, frontUpperLeft, frontUpperRight);
    // Right side of the bow
    addTriangle(mesh, bowTip, frontUpperRight, frontLowerRight);
    // Left side of the bow
    addTriangle(mesh, bowTip, frontLowerLeft, frontUpperLeft);

    // Bottom side of the bow
    addTriangle(mesh, bowTip, frontLowerRight, frontLowerLeft);

    mesh.material.kd = glm::vec3(1.0f, 0.5f, 0.25f);

    return mesh;
}

Mesh makeUnitBox() {
    Mesh mesh;

    // Front face
    addQuadrilateral(mesh, glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, 0.5f));

    // Back face
    addQuadrilateral(mesh, glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, -0.5f));

    // Top face
    addQuadrilateral(mesh, glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f));

    // Bottom face
    addQuadrilateral(mesh, glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, -0.5f));

    // Left face
    addQuadrilateral(mesh, glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, 0.5f));

    // Right face
    addQuadrilateral(mesh, glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.5f, -0.5f, -0.5f));

    // MATERIAL
    mesh.material.kd = glm::vec3(1.0f, 0.5f, 0.25f);

    return mesh;
}