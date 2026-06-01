#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================================================
// ZMIENNE GLOBALNE — kamera, czas, interakcja użytkownika
// ============================================================================
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
const float PI = 3.14159265359f;

// Transformacja całego modelu atomu (obrót i skalowanie obiektów)
float sceneRotationY = 0.0f;
float sceneScale = 1.0f;

// Tryby filtrowania tekstur (punkt 10 wytycznych)
enum FilterMode { FILTER_NEAREST = 0, FILTER_LINEAR = 1, FILTER_MIPMAP = 2 };
FilterMode filterMode = FILTER_MIPMAP;
unsigned int nucleusTexture = 0;
unsigned int floorTexture = 0;

// Stan klawiszy — zapobiega wielokrotnemu triggerowaniu przy trzymaniu
bool keyQ = false, keyE = false, keyR = false, keyF = false;
bool keyF1 = false, keyF2 = false, keyF3 = false;

// ============================================================================
// STRUKTURY I GENEROWANIE GEOMETRII (punkt 2 — min. 3 modele 3D)
// ============================================================================
struct MeshData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

// Model 1: Sfera — jądro atomu i elektrony
MeshData generateSphere(float radius, int sectors, int stacks) {
    MeshData data;
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2.0f - i * (PI / stacks);
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * PI / sectors);

            data.vertices.push_back(xy * cosf(sectorAngle));
            data.vertices.push_back(xy * sinf(sectorAngle));
            data.vertices.push_back(z);

            data.vertices.push_back(cosf(sectorAngle) * cosf(stackAngle));
            data.vertices.push_back(sinf(sectorAngle) * cosf(stackAngle));
            data.vertices.push_back(sinf(stackAngle));

            data.vertices.push_back((float)j / sectors);
            data.vertices.push_back((float)i / stacks);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                data.indices.push_back(k1);
                data.indices.push_back(k2);
                data.indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1)) {
                data.indices.push_back(k1 + 1);
                data.indices.push_back(k2);
                data.indices.push_back(k2 + 1);
            }
        }
    }
    return data;
}

// Model 2: Sześcian — podłoże sceny
std::vector<float> generateCube() {
    return {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
}

// Model 3: Okrąg — orbity elektronów (rysowane jako linie)
std::vector<float> generateOrbit(float radius, int segments) {
    std::vector<float> vertices;
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * PI * i / segments;
        vertices.push_back(radius * cosf(angle));
        vertices.push_back(0.0f);
        vertices.push_back(radius * sinf(angle));
    }
    return vertices;
}

// Konfiguracja atrybutów wierzchołków z UV (punkt 7 — teksturowanie)
void setupMeshAttributes(unsigned int vao) {
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

// ============================================================================
// TEKSTURY I FILTROWANIE (punkty 7 i 10)
// ============================================================================
void applyTextureFilter(unsigned int texture, FilterMode mode) {
    glBindTexture(GL_TEXTURE_2D, texture);
    switch (mode) {
        case FILTER_NEAREST:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case FILTER_LINEAR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case FILTER_MIPMAP:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        applyTextureFilter(textureID, filterMode);
        stbi_image_free(data);
        std::cout << "Zaladowano teksture: " << path << std::endl;
    } else {
        std::cout << "BLAD: Nie udalo sie zaladowac tekstury: " << path << std::endl;
    }
    return textureID;
}

void updateAllTextureFilters() {
    applyTextureFilter(nucleusTexture, filterMode);
    applyTextureFilter(floorTexture, filterMode);
    const char* modeName[] = { "NEAREST", "LINEAR", "MIPMAP" };
    std::cout << "Tryb filtrowania: " << modeName[filterMode] << std::endl;
}

// ============================================================================
// OBSŁUGA WEJŚCIA (punkt 4 — interakcja użytkownika)
// ============================================================================
void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

bool keyPressedOnce(GLFWwindow* window, int key, bool& state) {
    if (glfwGetKey(window, key) == GLFW_PRESS) {
        if (!state) {
            state = true;
            return true;
        }
    } else {
        state = false;
    }
    return false;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Sterowanie kamerą (WASD)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    // Obrót modelu atomu wokół osi Y (Q / E)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) sceneRotationY -= 1.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) sceneRotationY += 1.5f * deltaTime;

    // Skalowanie modelu atomu (R / F)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) sceneScale = std::min(sceneScale + 0.5f * deltaTime, 2.5f);
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) sceneScale = std::max(sceneScale - 0.5f * deltaTime, 0.3f);

    // Przełączanie trybu filtrowania tekstur (F1 / F2 / F3)
    if (keyPressedOnce(window, GLFW_KEY_F1, keyF1)) {
        filterMode = FILTER_NEAREST;
        updateAllTextureFilters();
    }
    if (keyPressedOnce(window, GLFW_KEY_F2, keyF2)) {
        filterMode = FILTER_LINEAR;
        updateAllTextureFilters();
    }
    if (keyPressedOnce(window, GLFW_KEY_F3, keyF3)) {
        filterMode = FILTER_MIPMAP;
        updateAllTextureFilters();
    }
}

// Macierz transformacji całej sceny atomu (obrót + skala)
glm::mat4 getSceneTransform() {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, sceneRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(sceneScale));
    return transform;
}

// ============================================================================
// FUNKCJA GŁÓWNA
// ============================================================================
int main() {
    // --- Punkt 1: Inicjalizacja OpenGL i okna (GLFW + GLAD) ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Model Atomu - Projekt GK", NULL, NULL);
    if (!window) {
        std::cout << "BLAD: Nie udalo sie utworzyc okna GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "BLAD: Nie udalo sie zainicjalizowac GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Shadery: oświetlenie Phonga + prosty shader linii dla orbit
    Shader lightingShader("shaders/vertex_shader.vs", "shaders/fragment_shader.fs");
    Shader lineShader("shaders/line_vertex.vs", "shaders/line_fragment.fs");

    // --- Ładowanie tekstur (punkt 7) ---
    nucleusTexture = loadTexture("textures/nucleus.png");
    floorTexture = loadTexture("textures/floor.png");

    // --- Przygotowanie buforów geometrii ---
    MeshData sphere = generateSphere(1.0f, 36, 18);

    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphere.vertices.size() * sizeof(float), sphere.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.indices.size() * sizeof(unsigned int), sphere.indices.data(), GL_STATIC_DRAW);
    setupMeshAttributes(sphereVAO);

    std::vector<float> cube = generateCube();
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, cube.size() * sizeof(float), cube.data(), GL_STATIC_DRAW);
    setupMeshAttributes(cubeVAO);

    std::vector<float> orbit = generateOrbit(1.0f, 100);
    unsigned int orbitVAO, orbitVBO;
    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);
    glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, orbit.size() * sizeof(float), orbit.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Pozycje 12 cząstek jądra (protony i neutrony)
    glm::vec3 nucleusPositions[] = {
        glm::vec3(0.00f,  0.00f,  0.00f), glm::vec3(0.25f,  0.10f, -0.10f),
        glm::vec3(-0.20f,  0.20f,  0.15f), glm::vec3(0.15f, -0.25f,  0.20f),
        glm::vec3(-0.15f, -0.15f, -0.20f), glm::vec3(0.30f, -0.10f,  0.10f),
        glm::vec3(-0.25f,  0.05f, -0.25f), glm::vec3(0.10f,  0.30f, -0.15f),
        glm::vec3(0.05f, -0.30f, -0.10f), glm::vec3(-0.30f, -0.20f,  0.05f),
        glm::vec3(0.20f,  0.25f,  0.25f), glm::vec3(-0.10f,  0.15f, -0.30f)
    };

    std::cout << "Sterowanie: WASD=kamera, mysz=obrot, scroll=zoom" << std::endl;
    std::cout << "Q/E=obrot atomu, R/F=skala, F1/F2/F3=filtrowanie tekstur" << std::endl;

    // --- Główna pętla renderowania ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Punkt 3: Rzut perspektywiczny ---
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 1280.0f / 720.0f, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 sceneTransform = getSceneTransform();

        // --- Punkt 5: Animacja źródła światła (orbita wokół sceny) ---
        glm::vec3 lightPos(
            5.0f * cosf(currentFrame * 0.4f),
            3.0f + sinf(currentFrame * 0.3f),
            5.0f * sinf(currentFrame * 0.4f)
        );

        // --- Rysowanie podłoża z teksturą (punkt 7) ---
        lightingShader.use();
        lightingShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setVec3("lightPos", lightPos);
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setBool("useTexture", true);
        lightingShader.setInt("diffuseMap", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);

        glBindVertexArray(cubeVAO);
        glm::mat4 modelCube = glm::mat4(1.0f);
        modelCube = glm::translate(modelCube, glm::vec3(0.0f, -6.0f, 0.0f));
        modelCube = glm::scale(modelCube, glm::vec3(20.0f, 0.2f, 20.0f));
        lightingShader.setMat4("model", modelCube);
        lightingShader.setVec3("objectColor", glm::vec3(1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- Rysowanie jądra atomu (12 sfer z teksturą na protonach) ---
        glBindVertexArray(sphereVAO);
        glBindTexture(GL_TEXTURE_2D, nucleusTexture);

        for (int i = 0; i < 12; i++) {
            glm::mat4 modelNucleus = sceneTransform;
            modelNucleus = glm::rotate(modelNucleus, currentFrame * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            modelNucleus = glm::translate(modelNucleus, nucleusPositions[i]);
            modelNucleus = glm::scale(modelNucleus, glm::vec3(0.25f));

            lightingShader.setMat4("model", modelNucleus);

            if (i % 2 == 0) {
                lightingShader.setBool("useTexture", true);
                lightingShader.setVec3("objectColor", glm::vec3(1.0f, 0.2f, 0.2f));
            } else {
                lightingShader.setBool("useTexture", false);
                lightingShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.6f));
            }

            glDrawElements(GL_TRIANGLES, (unsigned int)sphere.indices.size(), GL_UNSIGNED_INT, 0);
        }

        // --- Rysowanie orbit (shader linii) i elektronów (punkt 8 — animacja) ---
        for (int i = 0; i < 2; i++) {
            float orbitRadius = 2.0f + i * 0.8f;

            glm::mat4 baseOrbitModel = sceneTransform;

            float tiltX = (i == 0) ? 25.0f : -25.0f;
            float tiltZ = (i == 0) ? 15.0f : -15.0f;

            baseOrbitModel = glm::rotate(baseOrbitModel, glm::radians(tiltX), glm::vec3(1.0f, 0.0f, 0.0f));
            baseOrbitModel = glm::rotate(baseOrbitModel, glm::radians(tiltZ), glm::vec3(0.0f, 0.0f, 1.0f));


            // Orbita — prosty shader bez oświetlenia
            lineShader.use();
            lineShader.setMat4("projection", projection);
            lineShader.setMat4("view", view);
            glm::mat4 modelOrbitLine = baseOrbitModel;
            modelOrbitLine = glm::scale(modelOrbitLine, glm::vec3(orbitRadius));
            lineShader.setMat4("model", modelOrbitLine);
            lineShader.setVec3("lineColor", glm::vec3(0.4f, 0.4f, 0.4f));

            glBindVertexArray(orbitVAO);
            glDrawArrays(GL_LINE_LOOP, 0, 100);

            // Elektrony — animacja ruchu po orbicie
            lightingShader.use();
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);
            lightingShader.setBool("useTexture", false);

            glBindVertexArray(sphereVAO);
            float speed = 2.0f + i * 0.5f;

            int electronCount = (i == 1) ? 4 : 2;

            for (int e = 0; e < electronCount; e++) {
                glm::mat4 modelElectron = baseOrbitModel;
                float phaseShift = e * (2.0f * PI / electronCount);

                modelElectron = glm::rotate(modelElectron, currentFrame * speed + phaseShift, glm::vec3(0.0f, 1.0f, 0.0f));
                modelElectron = glm::translate(modelElectron, glm::vec3(orbitRadius, 0.0f, 0.0f));
                modelElectron = glm::scale(modelElectron, glm::vec3(0.12f));

                lightingShader.setMat4("model", modelElectron);
                lightingShader.setVec3("objectColor", glm::vec3(0.0f, 0.8f, 1.0f));
                glDrawElements(GL_TRIANGLES, (unsigned int)sphere.indices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Czyszczenie zasobów ---
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &orbitVAO);
    glDeleteBuffers(1, &orbitVBO);
    glDeleteTextures(1, &nucleusTexture);
    glDeleteTextures(1, &floorTexture);

    glfwTerminate();
    return 0;
}
