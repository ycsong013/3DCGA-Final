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
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
        , m_day(RESOURCE_ROOT "resources/hdris/citrus_orchard_puresky_4k.hdr")
        , m_night(RESOURCE_ROOT "resources/hdris/NightSkyHDRI003_4K_HDR.hdr")
        , m_seaMesh(makeGrid(100, 200))
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
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder seaBuilder;
            seaBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/sea_vert.glsl");
            seaBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/sea_frag.glsl");
            m_seaShader = seaBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
            // Shadow texture initialization (This part was coded from my knowledge of assignment 4.)
            glGenTextures(1, &texShadow);
            glBindTexture(GL_TEXTURE_2D, texShadow);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glBindTexture(GL_TEXTURE_2D, 0);

            glGenFramebuffers(1, &shadowFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFrameBuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
            ImGui::Text("The current time is: %f", currentTime); // Use C printf formatting rules (%i is a signed integer)
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
                ImGui::SliderFloat("Speed", &m_speed, 0.0f, 1.0f);
            }

            ImGui::Checkbox("Use Normal Maps Of the Textures", &m_useNormalMaps);
            ImGui::Checkbox("Use Image Based Lighting", &m_useIBL);
            ImGui::Checkbox("Use Physically Based Rendering", &m_usePBR);
            ImGui::Checkbox("Use the Sun as a Directional Light", &m_useDirectionalLight);
            ImGui::Checkbox("Cast Shadows from the Sun", &m_useShadows);
            
            // In this mathematics forum, I saw that the sun's position is generally coded as an "azimuth" 
            // and an "elevation". Thus, instead of letting the user set the XYZ coordinates of the sun's direction,
            // it would be more in line for them to set the azimuth and the elevation and then
            // calculate the XYZ later. This forum shows how to do it: https://math.stackexchange.com/questions/4697069/calculate-cartesian-angle-from-suns-azimuth-and-elevation

            ImGui::SliderAngle("The Sun's Azimuth", &m_sunAzimuth, 0.0f, 360.0f);
            ImGui::SliderAngle("The Sun's Elevation", &m_sunElevation, 0.0f, 90.0f);
            ImGui::SliderFloat("The Sun's Intensity", &m_sunIntensity, 0.0f, 10.0f);
            ImGui::ColorEdit3("The Sun's Color", glm::value_ptr(m_lightColor));
            ImGui::SliderFloat("The Sun's Distance (Shadow Mapping)",&m_sunDistance, 5.0f, 30.0f);

            ImGui::Checkbox("Use a Day and Night System", &m_dayNightSystem);


            const char* cameraModes[] = { "Aerial view", "Top view", "Third person" };
            ImGui::Combo("Camera Mode", &m_cameraMode, cameraModes, IM_ARRAYSIZE(cameraModes));
            if (m_cameraMode == Aerial) {
                ImGui::SliderFloat("Aerial view X", &m_aerialX, 0.0f, 10.0f);
                ImGui::SliderFloat("Aerial view Y", &m_aerialY, 0.0f, 5.0f);
                ImGui::SliderFloat("Aerial view Z", &m_aerialZ, 0.0f, 10.0f);
            }

            // k = 2 * pi / wavelength, so wavelength cannot be 0
            ImGui::SliderFloat("Amplitude of wave +x+z", &m_amp1, 0.0f, 0.1f);
            ImGui::SliderFloat("Wavelength of wave +x+z", &m_wav1, 3.0f, 5.0f);
            ImGui::SliderFloat("Speed of wave +x+z", &m_spe1, 0.0f, 5.0f);
            ImGui::SliderFloat("Amplitude of wave -x+z", &m_amp2, 0.0f, 0.1f);
            ImGui::SliderFloat("Wavelength of wave -x+z", &m_wav2, 3.0f, 5.0f);
            ImGui::SliderFloat("Speed of wave -x+z", &m_spe2, 0.0f, 5.0f);
            ImGui::SliderFloat("Amplitude of wave -z", &m_amp3, 0.0f, 0.1f);
            ImGui::SliderFloat("Wavelength of wave -z", &m_wav3, 3.0f, 5.0f);
            ImGui::SliderFloat("Speed of wave -z", &m_spe3, 0.0f, 5.0f);

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
            currentTime += interval;
            if (currentTime > totalTime) {
                currentTime = 0.0;
            }

            if (currentTime < nightTime) {
                if (currentTime < sunsetTime) {
                    interpolationFactor = 0;
                }
                else {
                    interpolationFactor = (currentTime - sunsetTime) / (nightTime - sunsetTime);
                }
            }
            else if (currentTime >= nightTime) {
                if (currentTime < sunriseTime) {
                    interpolationFactor = 1;
                } else {
                interpolationFactor = 1 - ((currentTime - sunriseTime) / (totalTime - sunriseTime));
                }
            }
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

            // Set the sun's direction to be the user defined direction.
            sunDirection = glm::vec3(glm::cos(m_sunElevation) * glm::sin(m_sunAzimuth), glm::sin(m_sunElevation), glm::cos(m_sunElevation) * glm::cos(m_sunAzimuth));

            // Normally, directional lights do not have a position. However, for rendering shadows, LearnOpenGL
            // adds that we MUST still set a position. We assume that this is a position focused on the boat.
            glm::vec3 sunTarget = pos; // Set the target of the sun as the position of the boat.
            glm::vec3 sunPosition = sunTarget + sunDirection * m_sunDistance; 
            m_lightViewMatrix = glm::lookAt(sunPosition, sunTarget, glm::vec3(0, 1, 0));
            m_lightProjectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, m_sunDistance - 10.0f, m_sunDistance + 10.0f); // the 10 is a randomly set value, the near and far planes depend on the sun's distance though.
            m_lightMVPMatrix = m_lightProjectionMatrix * m_lightViewMatrix;


            // Render the shadow map:

            glBindFramebuffer(GL_FRAMEBUFFER, shadowFrameBuffer);
            glClearDepth(1.0);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            
            drawNodeShadow(m_boatRoot, m_modelMatrix); // Calculate the shadows for the boat.
            // Bind the shadow shader and send the required values
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glViewport(0, 0, 1024, 1024);


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
                    m_viewMatrix = glm::lookAt(pos - 5.0f * dir + glm::vec3(0, 1, 0), pos, glm::vec3(0, 1, 0));
                    cameraPosition = glm::vec3(pos - 5.0f * dir + glm::vec3(0, 1, 0));
                    break;
                default:
                    break;
            }

            for (GPUMesh& mesh : m_meshes) {
                drawMesh(mesh, m_modelMatrix);
            }

            drawSea(m_seaMesh, m_modelMatrix);
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
            m_environment->drawSkyBox(m_projectionMatrix, m_viewMatrix, &m_night, interpolationFactor, m_dayNightSystem);

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

    // Wave atributes for the shader: 1 for (+x, +z), 2 for (-x, +z), 3 for (0, -z)
    float m_amp1 = 0.05f;
    float m_wav1 = 4.0f;
    float m_spe1 = 1.0f;
    float m_amp2 = 0.05f;
    float m_wav2 = 4.0f;
    float m_spe2 = 1.0f;
    float m_amp3 = 0.05f;
    float m_wav3 = 4.0f;
    float m_spe3 = 1.0f;

    // Tools for debugging
    bool m_useNormalMaps = false;
    bool m_useIBL = false;
    bool m_usePBR = false;
    bool m_useDirectionalLight = false;
    bool m_wireframe = false;

    // Tools for the camera
    enum CameraMode { Aerial = 0, Top = 1, Third = 2 };
    int m_cameraMode = Aerial;
    float m_aerialX = 8;
    float m_aerialY = 2;
    float m_aerialZ = 8;

    // Tools for the Bezier path
    bezierPath m_path;
    bool m_showPath = false;
    bool m_animateBoat = false;
    double m_lastTime = 0.0;
    float m_speed = 0.3f; // unit: segments/second
    float m_curPosOnPath = 0.0f; // range: [0, 3), representing current position on the path

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_seaShader;

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    Environment m_day;
    Environment* m_environment = &m_day;
    Environment m_night;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_lightViewMatrix = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_lightProjectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f); // LearnOpenGL mentions that the projection matrix
    // for directional light sources must be different.
    glm::mat4 m_modelMatrix { 1.0f };
    glm::mat4 m_lightMVPMatrix;
    glm::vec3 cameraPosition;
    glm::vec3 sunDirection;

    // Meshes for the project
    GPUMesh m_seaMesh;
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

    // Tools for the sun
    float interpolationFactor = 0.0f;
    float m_sunElevation = 0.0f;
    float m_sunAzimuth = 0.0f;
    float m_sunIntensity = 0.0f; // Attenuation with the distance in the PBR shader makes the sun's color turn to zero.
    // Thus, we instead let the user define the intensity of the light instead of a realistic approach.
    glm::vec3 m_lightColor = glm::vec3(1.0f);
    float m_sunDistance = 15.0f;
    bool m_useShadows = false; // Set shadows to false at first.
  
    // Colors for the light of the sun for the "changing lighting conditions" feature
    // We used the color codes given in this forum question: https://community.khronos.org/t/day-night-cycle-light/57752
    glm::vec3 morningSun = glm::vec3(255.0, 255.0, 220.0) / 255.0f;
    glm::vec3 eveningSun = glm::vec3(255.0, 60.0, 60.0) / 255.0f;
    glm::vec3 nightSun = glm::vec3(20.0, 60.0, 180.0) / 255.0f;

    float currentTime = 0.0f;
    float sunsetTime = 10.0f;
    float nightTime = 20.0f;
    float sunriseTime = 30.0f;
    float totalTime = 40.0f;
    bool m_dayNightSystem = false;

    // Variables for shadow mapping
    GLuint texShadow;
    GLuint shadowFrameBuffer;
    const int SHADOWTEX_WIDTH = 1024;
    const int SHADOWTEX_HEIGHT = 1024;

    void drawMesh(GPUMesh& mesh, const glm::mat4& model, PBRMaterial* material = nullptr) {

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * model;
        // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
        // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(model));

        // I use the azimuth-elevation to cartesian conversion formula introduced in this 
        // math forum: https://math.stackexchange.com/questions/4697069/calculate-cartesian-angle-from-suns-azimuth-and-elevation
        // The answer in this forum is a bit different: OP's cartesian coordinate system puts z to be the "up" direction, whereas in OpenGL this is the "y" direction.
        // So I swapped those two directions here.

        m_defaultShader.bind();
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("lightMVPMatrix"), 1, GL_FALSE, glm::value_ptr(m_lightMVPMatrix));
        m_environment->bindIrradianceMap(GL_TEXTURE1);
        m_environment->bindPrefilteredMap(GL_TEXTURE7);
        m_environment->bindBRDFMap(GL_TEXTURE8);
        m_night.bindIrradianceMap(GL_TEXTURE9);
        m_night.bindPrefilteredMap(GL_TEXTURE10);
        // Set the shadow map texture.
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, texShadow);
        glUniform1i(m_defaultShader.getUniformLocation("irradianceMap"), 1);
        glUniform1i(m_defaultShader.getUniformLocation("prefilteredMap"), 7);
        glUniform1i(m_defaultShader.getUniformLocation("brdfMap"), 8);
        glUniform1i(m_defaultShader.getUniformLocation("irradianceMapNight"), 9);
        glUniform1i(m_defaultShader.getUniformLocation("prefilteredMapNight"), 10);
        glUniform1i(m_defaultShader.getUniformLocation("shadowMap"), 11);
        glUniform1i(m_defaultShader.getUniformLocation("useIBL"), m_useIBL);
        glUniform1i(m_defaultShader.getUniformLocation("dayNightSystem"), m_dayNightSystem);
        glUniform1i(m_defaultShader.getUniformLocation("useShadows"), m_useShadows);
        glUniform1f(m_defaultShader.getUniformLocation("interpolationFactor"), interpolationFactor);
        glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(cameraPosition)); // Assume there is a small point light by the camera.
        if (m_dayNightSystem && m_useDirectionalLight) {
            if (currentTime <= sunsetTime) {
                glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(glm::mix(morningSun, eveningSun, currentTime / sunsetTime))); // The final value is the evening sun.
            }
            else if (currentTime <= nightTime){
                glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(glm::mix(eveningSun, nightSun, (currentTime - sunsetTime) / (nightTime - sunsetTime))));
            }
            else {
                glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(nightSun));
            }
        } else {
            glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(m_lightColor));
        }
        glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform3fv(m_defaultShader.getUniformLocation("sunDirection"), 1, glm::value_ptr(sunDirection));
        glUniform1i(m_defaultShader.getUniformLocation("useNormalMap"), m_useNormalMaps);
        glUniform1i(m_defaultShader.getUniformLocation("useDirectionalLight"), m_useDirectionalLight);
        glUniform1f(m_defaultShader.getUniformLocation("sunIntensity"), m_sunIntensity);
        if (material != nullptr) {
            GLint textureSlots[] = {
                GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6
            };
            material->bind(textureSlots);
            glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
            glUniform1i(m_defaultShader.getUniformLocation("usePBR"), m_usePBR);
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

    void drawMeshShadow(GPUMesh& mesh, const glm::mat4& model) {
        glm::mat4 lightMVPMatrix = m_lightProjectionMatrix * m_lightViewMatrix * model;

        m_shadowShader.bind();
        glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
        glUniformMatrix4fv(m_shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(lightMVPMatrix));

        mesh.draw(m_shadowShader);
    }

    void drawSea(GPUMesh& mesh, const glm::mat4& model) {

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * model;
        // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
        // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(model));

        m_seaShader.bind();
        glUniformMatrix4fv(m_seaShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        //Uncomment this line when you use the modelMatrix (or fragmentPosition)
        glUniformMatrix4fv(m_seaShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(m_seaShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
        glUniform1f(m_seaShader.getUniformLocation("time"), static_cast<float>(glfwGetTime()));
        glUniform1f(m_seaShader.getUniformLocation("amp1"), m_amp1);
        glUniform1f(m_seaShader.getUniformLocation("wav1"), m_wav1);
        glUniform1f(m_seaShader.getUniformLocation("spe1"), m_spe1);
        glUniform1f(m_seaShader.getUniformLocation("amp2"), m_amp2);
        glUniform1f(m_seaShader.getUniformLocation("wav2"), m_wav2);
        glUniform1f(m_seaShader.getUniformLocation("spe2"), m_spe2);
        glUniform1f(m_seaShader.getUniformLocation("amp3"), m_amp3);
        glUniform1f(m_seaShader.getUniformLocation("wav3"), m_wav3);
        glUniform1f(m_seaShader.getUniformLocation("spe3"), m_spe3);
        m_environment->bindIrradianceMap(GL_TEXTURE1);
        m_environment->bindPrefilteredMap(GL_TEXTURE7);
        m_environment->bindBRDFMap(GL_TEXTURE8);
        glUniform1i(m_seaShader.getUniformLocation("irradianceMap"), 1);
        glUniform1i(m_seaShader.getUniformLocation("prefilteredMap"), 7);
        glUniform1i(m_seaShader.getUniformLocation("brdfMap"), 8);
        glUniform1i(m_seaShader.getUniformLocation("useIBL"), 1);
        glUniform3fv(m_seaShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(cameraPosition)); // Assume there is a small point light by the camera.
        glUniform3fv(m_seaShader.getUniformLocation("lightColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform3fv(m_seaShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameraPosition));
        mesh.draw(m_seaShader);
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

    void drawNodeShadow(const Node& node, const glm::mat4& parentWorld) {
        glm::mat4 world = parentWorld * node.transform;

        if (node.mesh) {
            drawMeshShadow(*node.mesh, world * node.meshTransform);
        }

        for (const auto& child : node.children) {
            drawNodeShadow(*child, world);
        }
    }
};

int main()
{
    Application app;
    app.update();

    return 0;
}
