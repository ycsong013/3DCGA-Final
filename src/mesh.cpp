#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>

GPUMaterial::GPUMaterial(const Material& material) :
    kd(material.kd),
    ks(material.ks),
    shininess(material.shininess),
    transparency(material.transparency)
{}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
{
    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);

    // Figure out if this mesh has texture coordinates
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);


    // TBN creation for normal mapping -> I was able to find that this code is where the (layout = 0, 1, 2) indices
    // are pushed to the main shader of the program, as I "ctrl + f"'d for the "glVertexAttribPointer" function.
    // In LearnOpenGL's normal mapping tutorial, we can further push the tangent and bitangent of the vertex to
    // the vertex shader. This piece of code is where we also find the TBN, according to the instructions
    // of LearnOpenGL's normal mapping tutorial (https://learnopengl.com/Advanced-Lighting/Normal-Mapping).

    std::vector<glm::vec3> tangents(cpuMesh.vertices.size());
    std::vector<glm::vec3> bitangents(cpuMesh.vertices.size());

    // Accessing "cpuMesh.triangles", the description showed that it is a vector
    // of "uvec3" variables, representing 3 vertices. The description further mentions that each
    // value is used to access the "vertices" array of the mesh, to access the actual vertices.
    // This description helped us find each vertex easily.

    // For this part of the code, we followed LearnOpenGL's tutorial to understand how normal mapping works.
    // After understanding the main points, we tried to implement it on our own without looking at the tutorial's code.
    for (glm::uvec3 triangle : cpuMesh.triangles) { 
        Vertex v1 = cpuMesh.vertices[triangle[0]];
        Vertex v2 = cpuMesh.vertices[triangle[1]];
        Vertex v3 = cpuMesh.vertices[triangle[2]];

        glm::vec3 position1 = v1.position;
        glm::vec2 uv1 = v1.texCoord;

        glm::vec3 position2 = v2.position;
        glm::vec2 uv2 = v2.texCoord;

        glm::vec3 position3 = v3.position;
        glm::vec2 uv3 = v3.texCoord;

        glm::vec3 edge1 = position2 - position1;
        glm::vec3 edge2 = position3 - position1;

        float deltaU1 = (uv2 - uv1).x;
        float deltaV1 = (uv2 - uv1).y;
        float deltaU2 = (uv3 - uv1).x;
        float deltaV2 = (uv3 - uv1).y;

        // Calculate the determinant of the 2x2 matrix of deltas
        float determinant = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

        float tangentX = deltaV2 * edge1.x - deltaV1 * edge2.x;
        float tangentY = deltaV2 * edge1.y - deltaV1 * edge2.y;
        float tangentZ = deltaV2 * edge1.z - deltaV1 * edge2.z;

        glm::vec3 tangent = determinant * glm::vec3(tangentX, tangentY, tangentZ);
        // Triangles might share vertices. And thus, this loop might run for the same vertex multiple times.
        // For this issue, I found that on the internet, a user gave the following answer:
        // https://computergraphics.stackexchange.com/questions/9702/calculate-normals-from-vertices.
        // A user, namely "wychmaster", suggests that a good approach is to sum the values we receive
        // per vertex, weighing them by the angle of the vertex in the current triangle, and normalizing
        // in the end. We thought that this was a nice idea to incorporate for the tangent and bitangent calculations too
        // as the logic seems similar.

        float angle1 = glm::acos(glm::dot(glm::normalize(edge1), glm::normalize(edge2))); // Using the simple math trick, the arc cosin of the dot product of the normalized vectors will give the angle.
        float angle2 = glm::acos(glm::dot(glm::normalize(position1 - position2), glm::normalize(position3 - position2)));
        float angle3 = glm::acos(glm::dot(glm::normalize(position1 - position3), glm::normalize(position2 - position3)));
        tangents[triangle[0]] += angle1 * tangent;
        tangents[triangle[1]] += angle2 * tangent;
        tangents[triangle[2]] += angle3 * tangent;

        float bitangentX = -deltaU2 * edge1.x + deltaU1 * edge2.x;
        float bitangentY = -deltaU2 * edge1.y + deltaU1 * edge2.y;
        float bitangentZ = -deltaU2 * edge1.z + deltaU1 * edge2.z;

        glm::vec3 bitangent = determinant * glm::vec3(bitangentX, bitangentY, bitangentZ);

        bitangents[triangle[0]] += angle1 * bitangent;
        bitangents[triangle[1]] += angle2 * bitangent;
        bitangents[triangle[2]] += angle3 * bitangent;

    }

    for (int i = 0; i < tangents.size();  i++) {
        tangents[i] = glm::normalize(tangents[i]);
    }

    for (int j = 0; j < bitangents.size(); j++) {
        bitangents[j] = glm::normalize(bitangents[j]);
    }

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.vertices.size() * sizeof(decltype(cpuMesh.vertices)::value_type)), cpuMesh.vertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Create two other vertex buffer objects for the tangents and bitangents.

    glGenBuffers(1, &m_tangentvbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_tangentvbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(tangents.size() * sizeof(decltype(tangents)::value_type)), tangents.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_bitangentvbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_bitangentvbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bitangents.size() * sizeof(decltype(bitangents)::value_type)), bitangents.data(), GL_STATIC_DRAW);


    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glBindBuffer(GL_ARRAY_BUFFER, m_tangentvbo);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_bitangentvbo);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
    glVertexAttribDivisor(3, 0);
    glVertexAttribDivisor(4, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());
}

GPUMesh::GPUMesh(GPUMesh&& other)
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other)
{
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, { .normalizeVertexPositions = normalize });
    std::vector<GPUMesh> gpuMeshes;
    for (const Mesh& mesh : subMeshes) { gpuMeshes.emplace_back(mesh); }
    
    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const
{
    return m_hasTextureCoords;
}

void GPUMesh::draw(const Shader& drawingShader)
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);
    
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

void GPUMesh::moveInto(GPUMesh&& other)
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_bitangentvbo = other.m_bitangentvbo;
    m_tangentvbo = other.m_tangentvbo;
    m_uboMaterial = other.m_uboMaterial;

    other.m_numIndices = 0;
    other.m_hasTextureCoords = other.m_hasTextureCoords;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_bitangentvbo = INVALID;
    other.m_tangentvbo = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
    if (m_bitangentvbo)
        glDeleteBuffers(1, &m_bitangentvbo);
    if (m_tangentvbo)
        glDeleteBuffers(1, &m_tangentvbo);
}
