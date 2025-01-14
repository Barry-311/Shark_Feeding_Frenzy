#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "FBX.hpp"
#include "TexFBX.hpp" // Include Texture for Shark texture handling

#include <iostream>
#include <vector>

//// Window Parameters ////
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1000;

Camera camera(glm::vec3(0.0f, 4.0f, 15.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float quadVertices[] = 
{
    // positions   // texCoords
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};

//// Model parameters ////
struct Fish 
{
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    float speed = 0.3f;
    float angle = 0.0f;
    float angularSpeed = 0.1f;
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius;

    glm::vec3 boundingSphereCenter;
    float boundingSphereRadius;

    bool isActive = true; // To see if the fish is alive
};

struct BoundingSphere 
{
    glm::vec3 center;
    float radius;
};

BoundingSphere sharkBoundingSphere1;
BoundingSphere sharkBoundingSphere2;

bool checkCollision(const BoundingSphere& sphere1, const BoundingSphere& sphere2) 
{
    float distance = glm::length(sphere1.center - sphere2.center);
    float radiusSum = sphere1.radius + sphere2.radius;
    return distance <= radiusSum;
}

float sharkDirectionAngle = -60.0f; // Default direction of shark
float turnSpeed = 10.0f;          // Turning speed
float sharkPitchAngle = 10.0f;    // Angle of up & down
float verticalTurnSpeed = 5.0f;   // Up & Down speed

float speedBoostTimer = 0.0f;
bool isSpeedBoostActive = false;
bool hunted = false;

int main() 
{
    // GLFW initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Shark_Feeding_Frenzy", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD initialization
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }
    stbi_set_flip_vertically_on_load(false);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for smooth fog transition (fragment alpha blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //// Quad ////
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    //// Shaders ////
    Shader backgroundShader("shader/background.vert", "shader/background.frag");
    Shader modelShader("shader/model.vert", "shader/model.frag");

    //// Models ////
    Model landModel("model/terrian/ShangGu.obj");
    Model fishModel("model/fish/fish.obj");

    FBXModel sharkModel;
    if (!sharkModel.loadFromFile("model/fish/shark.fbx")) 
    {
        std::cerr << "Failed to load Shark model!" << std::endl;
        return -1;
    }
    FBXModel::MeshData sharkMeshData;
    sharkModel.generateObjectBufferMesh(sharkModel.getModelData(), sharkMeshData);

    // Shark texture
    TexFBX sharkTexture("model/fish/shark.jpg");
    glm::vec3 sharkPosition(-10.0f, 2.0f, 10.0f);
    sharkBoundingSphere1.center = sharkPosition;
    sharkBoundingSphere1.radius = 12.0f;

    sharkBoundingSphere2.center = sharkPosition;
    sharkBoundingSphere2.radius = 2.0f;

    float sharkSpeed = 0.4f; // Default shark speed

    // School of fish (10 fishes)
    std::vector<Fish> fishes =
    {
        {{-2.0f, 3.0f, -16.0f}, {0.3f, 0.2f, 0.2f}, {0.0f, 1.0f, 0.0f}},
        {{ 1.0f, 4.0f, -12.0f}, {0.3f, 0.2f, 0.2f}, {0.0f, 1.0f, 0.0f}},
        {{-2.0f, 1.0f, -11.0f}, {0.35f, 0.15f, 0.15f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, 2.5f, -13.5f}, {0.4f, 0.2f, 0.2f}, {0.0f, 1.0f, 0.0f}},
        {{-1.5f, 2.4f, -12.5f}, {0.4f, 0.3f, 0.3f}, {0.0f, 1.0f, 0.0f}},
        {{-1.5f, 5.0f, -14.5f}, {0.5f, 0.4f, 0.4f}, {0.0f, 1.0f, 0.0f}},
        {{-2.5f, 1.7f, -10.0f}, {0.3f, 0.2f, 0.2f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 3.5f, -13.0f}, {0.4f, 0.3f, 0.3f}, {0.0f, 1.0f, 0.0f}},
        {{ 2.0f, 4.2f, -15.5f}, {0.3f, 0.3f, 0.3f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 2.0f, -10.5f}, {0.3f, 0.3f, 0.3f}, {0.0f, 1.0f, 0.0f}}
    };

    for (auto& fish : fishes)
    {
        // Default radius & angle
        fish.radius = glm::length(glm::vec2(fish.position.x - fish.center.x, fish.position.z - fish.center.z));
        fish.angle = atan2(fish.position.z - fish.center.z, fish.position.x - fish.center.x);
    }

    //// RENDER LOOP ////
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear and draw background
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        backgroundShader.use();
        backgroundShader.setFloat("time", glfwGetTime());
        backgroundShader.setVec3("sunPosition", glm::vec3(0.5f, 0.8f, 0.3f));
        backgroundShader.setVec3("topColor", glm::vec3(0.0f, 0.3f, 0.5f));
        backgroundShader.setVec3("bottomColor", glm::vec3(0.0f, 0.1f, 0.2f));

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // Camera and projection
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        if (isSpeedBoostActive)
        {
            speedBoostTimer -= deltaTime;
            if (speedBoostTimer <= 0.0f) {
                isSpeedBoostActive = false;
                speedBoostTimer = 0.0f;
                hunted = false;
            }
        }

        // Use modelShader once for all 3D models
        modelShader.use();
        modelShader.setVec3("ambientLight", glm::vec3(0.0f, 0.3f, 0.5f));
        modelShader.setVec3("lightColor", glm::vec3(0.8f, 0.9f, 1.0f));
        modelShader.setVec3("lightDir", glm::vec3(0.0f, -1.0f, 0.0f));
        modelShader.setVec3("viewPos", camera.position());
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setFloat("fogDensity", 0.025f);
        modelShader.setVec3("fogColor", glm::vec3(0.0f, 0.2f, 0.4f));

        //// Terrain ////
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.08f, 0.08f, 0.08f));
            modelShader.setMat4("model", model);
            modelShader.setBool("isShark", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, landModel.textures_loaded[0].id);
            modelShader.setInt("texture_diffuse", 0);
            landModel.Draw(modelShader);
        }

        //// Fish ////
        for (auto& fish : fishes)
        {
            if (!fish.isActive) continue;
            float currentSpeed = fish.speed;
            float currentAngularSpeed = fish.angularSpeed;

            if (isSpeedBoostActive)
            {
                currentSpeed *= 3.5f;
                currentAngularSpeed *= 3.5f;
            }

            // Circular motion
            fish.angle += currentAngularSpeed * deltaTime;
            fish.position.x = fish.center.x + fish.radius * cos(fish.angle);
            fish.position.z = fish.center.z + fish.radius * sin(fish.angle);

            glm::vec3 direction = glm::vec3(
                sin(fish.angle),
                0.0f,
                -cos(fish.angle)
            );

            fish.rotation.y = glm::degrees(atan2(direction.x, direction.z));
            fish.boundingSphereCenter = fish.position;
            fish.boundingSphereRadius = 0.8f * glm::max(fish.scale.x, glm::max(fish.scale.y, fish.scale.z));
        }

        for (const auto& fish : fishes)
        {
            if (!fish.isActive) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, fish.position);
            model = glm::rotate(model, glm::radians(fish.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, fish.scale);
            modelShader.setMat4("model", model);
            modelShader.setBool("isShark", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fishModel.textures_loaded[0].id);
            modelShader.setInt("texture_diffuse", 0);

            fishModel.Draw(modelShader);
        }

        //// Shark ////
        bool isCollision = false;
        for (auto& fish : fishes)
        {
            if (!fish.isActive) continue;

            BoundingSphere fishSphere = { fish.boundingSphereCenter, fish.boundingSphereRadius };

            // Check if the shark should start hunting
            if (checkCollision(sharkBoundingSphere1, fishSphere))
            {
                isCollision = true;
            }

            if (hunted) continue;

            // Check if the shark can eat the fish
            if (checkCollision(sharkBoundingSphere2, fishSphere))
            {
                fish.isActive = false;
                hunted = true;
                isSpeedBoostActive = true;
                speedBoostTimer = 4.0f;
            }
        }

        // In hunting mode, the shark will get more speed
        if (isCollision)
        {
            sharkSpeed = 1.0f;
            turnSpeed = 20.0f;
            modelShader.setFloat("swayMultiplier", 3.0f);
        }
        else {
            sharkSpeed = 0.4f;
            turnSpeed = 10.0f;
        }

        // Update shark direction and position
        glm::vec3 sharkDirection = glm::normalize(glm::vec3(
            cos(glm::radians(sharkDirectionAngle)) * cos(glm::radians(sharkPitchAngle)),
            glm::clamp(sin(glm::radians(sharkPitchAngle)), -0.5f, 0.5f),
            sin(glm::radians(sharkDirectionAngle)) * cos(glm::radians(sharkPitchAngle))
        ));

        // In case that the shark goes below the land
        if (sharkPosition.y < -1.0f)
        {
            sharkPitchAngle = glm::clamp(sharkPitchAngle + verticalTurnSpeed * deltaTime, 0.0f, 30.0f);
        }

        sharkPosition += sharkDirection * sharkSpeed * deltaTime;

        if (sharkPosition.y < -1.0f)
        {
            sharkPosition.y = -1.0f;
        }

        // Update shark bounding spheres
        sharkBoundingSphere1.center = sharkPosition;
        sharkBoundingSphere2.center = sharkPosition;

        // Shark drawing
        modelShader.setFloat("time", currentFrame);
        for (int meshID = 0; meshID < 3; ++meshID) 
        {
            modelShader.setBool("isShark", true);
            modelShader.setInt("meshID", meshID);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, sharkPosition);
            model = glm::rotate(model, glm::radians(-sharkDirectionAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(sharkPitchAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
            modelShader.setMat4("model", model);

            sharkTexture.Bind(0);
            modelShader.setInt("texture_diffuse", 0);

            glBindVertexArray(sharkMeshData.vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(sharkMeshData.vertexCount));
            glBindVertexArray(0);
        }

        // Swap and poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD - forward, backward, left and right
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(kForward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(kBackward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(kLeft, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(kRight, deltaTime);

    // O - camera up; K - camera down
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        camera.ProcessKeyboard(kUp, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        camera.ProcessKeyboard(kDown, deltaTime);

    // Arrow Keys - Camera rotation
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_RIGHT, deltaTime);

    //// Shark Control ////
    // Q - left
    // E - right
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        sharkDirectionAngle -= turnSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        sharkDirectionAngle += turnSpeed * deltaTime;

    // Z - up
    // C - down
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        sharkPitchAngle += verticalTurnSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        sharkPitchAngle -= verticalTurnSpeed * deltaTime;

    // Avoid flip upside down
    sharkPitchAngle = glm::clamp(sharkPitchAngle, -30.0f, 30.0f);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    glViewport(0, 0, width, height);
}
