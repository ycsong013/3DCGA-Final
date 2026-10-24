//#include "Image.h"
#include "mesh.h"
#include "texture.h"
#include "boat.h"
#include "sea.h"
#include "bezier.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <numbers>
#include "environment.h"

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
        , m_environment(RESOURCE_ROOT "resources/hdris/citrus_orchard_puresky_4k.hdr")
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
        , m_hullMesh(makeHull())
        , m_deckMesh(makeDeck())
        , m_sailMesh(makeSail())
        , m_boxMesh(makeUnitBox())
    {
        glDepthFunc(GL_LEQUAL); // Updated to ensure that the skybox is always outside.
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });

        //m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");
        m_meshes.push_back(makeGrid(100, 200));
        m_boat = buildBoat(m_boatRoot, m_hullMesh, m_deckMesh, m_boxMesh, m_sailMesh,
            m_hullMaterial, m_deckMaterial, m_mastMaterial, m_sailMaterial);

        m_path.build(glm::vec3(-5.0f, 0.0f, -5.0f),   // start of the linear bezier
                     glm::vec3(5.0f, 0.0f, -5.0f),    // end of the linear bezier / start of the quadratic bezier
                     glm::vec3(0.0f, 0.0f, 5.0f));    // end of the quadratic bezier
        m_lastTime = glfwGetTime();

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void update()
    {
        int dummyInteger = 0; // Initialized to 0
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

            // Use ImGui for easy input/output of ints, floats, strings, etc...
            ImGui::Begin("Window");
            // ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
            // ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
            ImGui::Checkbox("Use material if no texture", &m_useMaterial);
            ImGui::Checkbox("Wireframe", &m_wireframe);

            ImGui::Checkbox("Animate Boom", &m_animateBoom);
            if (m_animateBoom) {
                ImGui::SliderFloat("Boom speed (degrees / sec)", &m_boomSpeed, 0.0f, 60.0f);
            }

            ImGui::Checkbox("Pitch", &m_animatePitch);
            if (m_animatePitch) {
                ImGui::SliderFloat("Pitch speed (degrees / sec)", &m_pitchSpeed, 0.0f, 30.0f);
            }

            ImGui::Checkbox("Show Bezier Path", &m_showPath);

            ImGui::Checkbox("Animate Boat", &m_animateBoat);
            if (m_animateBoat) {
                ImGui::SliderFloat("Speed", &m_speed, 0.0f, 30.0f);
            }

            const char* cameraModes[] = { "Aerial view", "Top view", "Third person" };
            ImGui::Combo("Camera Mode", &m_cameraMode, cameraModes, IM_ARRAYSIZE(cameraModes));
            if (m_cameraMode == Aerial) {
                ImGui::SliderFloat("Aerial view X", &m_aerialX, 0.0f, 10.0f);
                ImGui::SliderFloat("Aerial view Y", &m_aerialY, 0.0f, 10.0f);
                ImGui::SliderFloat("Aerial view Z", &m_aerialZ, 0.0f, 10.0f);
            }

            ImGui::End();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

            if (m_wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // Set wireframe before draws
            }

            // Calculate time interval between last frame and this frame
            double now = glfwGetTime(); 
            float interval = static_cast<float>(now - m_lastTime);
            m_lastTime = now;

            // Calculate boat's current position on the Bezier path
            if (m_animateBoat) {
                m_curPosOnPath = std::fmod(m_curPosOnPath + m_speed * interval, 3.0f);
            }

            // Calculate boat's current position in the real world
            glm::vec3 pos, dir;
            m_path.calcPosAndDir(m_curPosOnPath, pos, dir);

            // Place the boat on the path, and make its bow point to the path.
            m_boatRoot.transform = glm::translate(glm::mat4(1.0f), pos)
                * glm::rotate(glm::mat4(1.0f), std::atan2(dir.x, dir.z) - glm::radians(180.0f), glm::vec3(0, 1, 0));

            // Make the boom sway
            if (m_animateBoom) {
                m_boomTime += interval;
            }
            setBoomAngle(*m_boat.boom, 30.0f * std::sin(m_boomSpeed / 30.0f * m_boomTime * static_cast<float>(std::numbers::pi) / 2.0f));

            // Make the boat pitch
            if (m_animatePitch) {
                m_pitchTime += interval;
            }
            setPitchAngle(*m_boat.hull, 15.0f * std::sin(m_pitchSpeed / 15.0f * m_pitchTime * static_cast<float>(std::numbers::pi) / 2.0f));


            switch (m_cameraMode) {
                case Aerial:
                    m_viewMatrix = glm::lookAt(glm::vec3(m_aerialX, m_aerialY, m_aerialZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
                    cameraPosition = glm::vec3(m_aerialX, m_aerialY, m_aerialZ);
                    break;
                case Top:
                    m_viewMatrix = glm::lookAt(glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
                    cameraPosition = glm::vec3(0, 10, 0);
                    break;
                case Third:
                    m_viewMatrix = glm::lookAt(pos - 4.0f * dir + glm::vec3(0, 4, 0), pos, glm::vec3(0, 1, 0));
                    cameraPosition = glm::vec3(pos - 4.0f * dir + glm::vec3(0, 4, 0));
                    break;
                default:
                    break;
            }

            for (GPUMesh& mesh : m_meshes) {
                drawMesh(mesh, m_modelMatrix);
            }

            drawNode(m_boatRoot, m_modelMatrix);

            if (m_showPath) {
                // small boxes along the path
                int numOfBoxes = 100;
                for (int i = 0; i < numOfBoxes; i++) {
                    glm::vec3 p, d;
                    m_path.calcPosAndDir(3.0f * i / numOfBoxes, p, d);
                    drawMesh(m_boxMesh,
                        glm::translate(glm::mat4(1.0f), p) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f)));
                }

                // big boxes for the control point of these Bezier paths
                const glm::vec3 ctrlPoints[] = { m_path.L0, m_path.L1, m_path.Q1, m_path.Q2, m_path.C1, m_path.C2 };
                for (const glm::vec3& ctrl : ctrlPoints) {
                    drawMesh(m_boxMesh,
                        glm::translate(glm::mat4(1.0f), ctrl) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f)));
                }
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   // Reset wireframe to off after all draws
            m_environment.drawSkyBox(m_projectionMatrix, m_viewMatrix);

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        std::cout << "Key pressed: " << key << std::endl;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    // Tools for debugging
    bezierPath m_path;
    bool m_showPath = false;
    bool m_wireframe = false;
    bool m_animateBoat = false;
    enum CameraMode { Aerial = 0, Top = 1, Third = 2 };
    int m_cameraMode = Aerial;
    float m_aerialX = 3;
    float m_aerialY = 3;
    float m_aerialZ = 3;
    double m_lastTime = 0.0;
    float m_speed = 0.3f; // unit: segments/second
    float m_curPosOnPath = 0.0f; // range: [0, 3), representing current position on the path

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    Environment m_environment;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix { 1.0f };
    glm::vec3 cameraPosition;

    // Meshes for building the boat
    GPUMesh m_hullMesh;
    GPUMesh m_deckMesh;
    GPUMesh m_sailMesh;
    GPUMesh m_boxMesh;
    Node m_boatRoot;

    // User for setting the angle of the boom and the pitch
    BoatParts m_boat;
    bool m_animateBoom = false;
    bool m_animatePitch = false;
    float m_boomSpeed = 30.0f;
    float m_boomTime = 0.0f;
    float m_pitchSpeed = 30.0f;
    float m_pitchTime = 0.0f;

    PBRMaterial m_hullMaterial = PBRMaterial(RESOURCE_ROOT "resources/materials/white-gold-bl/white-gold_");
    PBRMaterial m_deckMaterial = PBRMaterial(RESOURCE_ROOT "resources/materials/hungarian-point-flooring-bl/hungarian-point-flooring_");
    PBRMaterial m_sailMaterial = PBRMaterial(RESOURCE_ROOT "resources/materials/coarse-loose-fabric-bl/coarse-loose-fabric_");
    PBRMaterial m_mastMaterial = PBRMaterial(RESOURCE_ROOT "resources/materials/bamboo-wood-semigloss-bl/bamboo-wood-semigloss-");

    void drawMesh(GPUMesh& mesh, const glm::mat4& model, PBRMaterial* material = nullptr) {

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * model;
        // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
        // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(model));

        m_defaultShader.bind();
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
        m_environment.bindIrradianceMap(GL_TEXTURE1);
        m_environment.bindPrefilteredMap(GL_TEXTURE7);
        m_environment.bindBRDFMap(GL_TEXTURE8);
        glUniform1i(m_defaultShader.getUniformLocation("irradianceMap"), 1);
        glUniform1i(m_defaultShader.getUniformLocation("prefilteredMap"), 7);
        glUniform1i(m_defaultShader.getUniformLocation("brdfMap"), 8);
        glUniform1i(m_defaultShader.getUniformLocation("useIBL"), 1);
        glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(cameraPosition)); // Assume there is a small point light by the camera.
        glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameraPosition));
        if (material != nullptr) {
            GLint textureSlots[] = {
                GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6
            };
            material->bind(textureSlots);
            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
            // Is there an easier way of doing this??? Check.
            glUniform1i(m_defaultShader.getUniformLocation("usePBR"), 1);
            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
            glUniform1i(m_defaultShader.getUniformLocation("textureAlbedo"), 2);
            glUniform1i(m_defaultShader.getUniformLocation("textureAO"), 3);
            glUniform1i(m_defaultShader.getUniformLocation("textureMetallic"), 4);
            glUniform1i(m_defaultShader.getUniformLocation("textureNormal"), 5);
            glUniform1i(m_defaultShader.getUniformLocation("textureRoughness"), 6);

        }
        else if (mesh.hasTextureCoords()) {
            m_texture.bind(GL_TEXTURE0);
            glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
        }
        else {
            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
            glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
        }
        mesh.draw(m_defaultShader);
    }

    void drawNode(const Node& node, const glm::mat4& parentWorld) {
        glm::mat4 world = parentWorld * node.transform;

        if (node.mesh) {
            drawMesh(*node.mesh, world * node.meshTransform, node.m_material);
        }

        for (const auto& child : node.children) {
            drawNode(*child, world);
        }
    }
};

int main()
{
    Application app;
    app.update();

    return 0;
}
