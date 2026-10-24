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

void setBoomAngle(Node& boom, float degrees) {
    boom.transform = glm::translate(idt, glm::vec3(0.0f, 0.3f, 0.05f))
        * glm::rotate(idt, glm::radians(degrees), glm::vec3(0, 1, 0));
}

void setPitchAngle(Node& hull, float degrees) {
    hull.transform = glm::rotate(idt, glm::radians(degrees), glm::vec3(1, 0, 0));
}

// Corner points shared by the hull and the deck
namespace {
    struct HullPoints {
        glm::vec3 backUpperRight{ 0.5f, 0.5f,  1.5f };
        glm::vec3 backUpperLeft{ -0.5f, 0.5f,  1.5f };
        glm::vec3 backLowerLeft{ -0.4f, 0.0f,  1.4f };
        glm::vec3 backLowerRight{ 0.4f, 0.0f,  1.4f };
        glm::vec3 frontUpperRight{ 0.5f, 0.5f, -0.5f };
        glm::vec3 frontUpperLeft{ -0.5f, 0.5f, -0.5f };
        glm::vec3 frontLowerLeft{ -0.4f, 0.0f, -0.5f };
        glm::vec3 frontLowerRight{ 0.4f, 0.0f, -0.5f };
        glm::vec3 bowTip{ 0.0f, 0.6f, -1.5f };
    };
}

BoatParts buildBoat(Node& root, GPUMesh& hullMesh, GPUMesh& deckMesh, GPUMesh& boxMesh, GPUMesh& sailMesh,
    PBRMaterial& hullMaterial, PBRMaterial& deckMaterial, PBRMaterial& mastMaterial, PBRMaterial& sailMaterial) {
    root.transform = glm::translate(idt, glm::vec3(0.0f, 0.0f, 0.0f));

    Node* hull = root.addChild();
    setPitchAngle(*hull, 0.0f);
    hull->mesh = &hullMesh;
    hull->m_material = &hullMaterial;

    Node* deck = hull->addChild();
    deck->mesh = &deckMesh;
    deck->m_material = &deckMaterial;

    Node* mast = hull->addChild();
    mast->transform = glm::translate(idt, glm::vec3(0.0f, 0.5f, -0.25f));
    mast->mesh = &boxMesh;
    mast->meshTransform = glm::translate(idt, glm::vec3(0.0f, 1.5f, 0.0f))
        * glm::scale(idt, glm::vec3(0.1f, 3.0f, 0.1f));
    mast->m_material = &mastMaterial;

    Node* boom = mast->addChild();
    setBoomAngle(*boom, 0.0f);
    //boom->transform = glm::translate(idt, glm::vec3(0.0f, 0.3f, 0.05f))
    //    * glm::rotate(idt, glm::radians(30.0f), glm::vec3(0, 1, 0));
    boom->mesh = &boxMesh;
    boom->meshTransform = glm::translate(idt, glm::vec3(0.0f, 0.0f, 0.75f))
        * glm::scale(idt, glm::vec3(0.1f, 0.1f, 1.5f));
    boom->m_material = &mastMaterial;

    Node* mainsail = boom->addChild();
    //sail->transform = 
    mainsail->mesh = &sailMesh;
    mainsail->m_material = &sailMaterial;
    mainsail->meshTransform = glm::scale(idt, glm::vec3(1.0f, 2.7f, 1.3f));

    Node* jib = mast->addChild();
    jib->transform = glm::translate(idt, glm::vec3(0.0f, 0.1f, -0.05f))
        * glm::rotate(idt, glm::radians(180.0f), glm::vec3(0, 1, 0));
    jib->mesh = &sailMesh;
    jib->m_material = &sailMaterial;
    jib->meshTransform = glm::scale(idt, glm::vec3(1.0f, 2.8f, 1.0f));

    return BoatParts{ .hull = hull, .boom = boom };
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
    const HullPoints p;

    // Back side of the hull
    addQuadrilateral(mesh, p.backUpperRight, p.backUpperLeft, p.backLowerLeft, p.backLowerRight);
    // Right side of the hull
    addQuadrilateral(mesh, p.frontUpperRight, p.backUpperRight, p.backLowerRight, p.frontLowerRight);
    // Left side of the hull
    addQuadrilateral(mesh, p.backUpperLeft, p.frontUpperLeft, p.frontLowerLeft, p.backLowerLeft);
    // Bottom side of the hull
    addQuadrilateral(mesh, p.backLowerLeft, p.frontLowerLeft, p.frontLowerRight, p.backLowerRight);

    
    // Right side of the bow
    addTriangle(mesh, p.bowTip, p.frontUpperRight, p.frontLowerRight);
    // Left side of the bow
    addTriangle(mesh, p.bowTip, p.frontLowerLeft, p.frontUpperLeft);
    // Bottom side of the bow
    addTriangle(mesh, p.bowTip, p.frontLowerRight, p.frontLowerLeft);

    mesh.material.kd = glm::vec3(1.0f, 0.5f, 0.25f);

    return mesh;
}

Mesh makeDeck() {
    Mesh mesh;
    const HullPoints p;

    // Top side of the deck
    addQuadrilateral(mesh, p.backUpperRight, p.frontUpperRight, p.frontUpperLeft, p.backUpperLeft);

    // Top side of the bow
    addTriangle(mesh, p.bowTip, p.frontUpperLeft, p.frontUpperRight);

    mesh.material.kd = glm::vec3(1.0f, 0.5f, 0.25f);

    return mesh;
}

Mesh makeSail() {
    Mesh mesh;

    // Add a triangle with side length (1, 1, √2) with its normal pointing to +x axis
    addTriangle(mesh, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Add a triangle with side length (1, 1, √2) with its normal pointing to -x axis
    addTriangle(mesh, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // MATERIAL
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