
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

// شيفرات shader (vertex + fragment)
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos,1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;

void main()
{
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * objectColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result,1.0);
}
)";

// دوال shader
unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << "\n";
    }
    return shader;
}

unsigned int createShaderProgram(const char* vSource, const char* fSource)
{
    unsigned int vShader = compileShader(GL_VERTEX_SHADER, vSource);
    unsigned int fShader = compileShader(GL_FRAGMENT_SHADER, fSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << "\n";
    }
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    return program;
}

// صندوق الساعة (متوازي مستطيلات 3D)
float boxVertices[] = {
    // pos                // normal
    -0.5f, -1.0f,  0.3f,   0.0f, 0.0f, 1.0f,
     0.5f, -1.0f,  0.3f,   0.0f, 0.0f, 1.0f,
     0.5f,  0.8f,  0.3f,   0.0f, 0.0f, 1.0f,
    -0.5f,  0.8f,  0.3f,   0.0f, 0.0f, 1.0f,

    -0.5f, -1.0f, -0.3f,   0.0f, 0.0f,-1.0f,
     0.5f, -1.0f, -0.3f,   0.0f, 0.0f,-1.0f,
     0.5f,  0.8f, -0.3f,   0.0f, 0.0f,-1.0f,
    -0.5f,  0.8f, -0.3f,   0.0f, 0.0f,-1.0f,

    -0.5f,  0.8f, -0.3f,  -1.0f, 0.0f, 0.0f,
    -0.5f,  0.8f,  0.3f,  -1.0f, 0.0f, 0.0f,
    -0.5f, -1.0f,  0.3f,  -1.0f, 0.0f, 0.0f,
    -0.5f, -1.0f, -0.3f,  -1.0f, 0.0f, 0.0f,

     0.5f,  0.8f, -0.3f,   1.0f, 0.0f, 0.0f,
     0.5f,  0.8f,  0.3f,   1.0f, 0.0f, 0.0f,
     0.5f, -1.0f,  0.3f,   1.0f, 0.0f, 0.0f,
     0.5f, -1.0f, -0.3f,   1.0f, 0.0f, 0.0f,

    -0.5f,  0.8f,  0.3f,   0.0f, 1.0f, 0.0f,
     0.5f,  0.8f,  0.3f,   0.0f, 1.0f, 0.0f,
     0.5f,  0.8f, -0.3f,   0.0f, 1.0f, 0.0f,
    -0.5f,  0.8f, -0.3f,   0.0f, 1.0f, 0.0f,

    -0.5f, -1.0f,  0.3f,   0.0f,-1.0f, 0.0f,
     0.5f, -1.0f,  0.3f,   0.0f,-1.0f, 0.0f,
     0.5f, -1.0f, -0.3f,   0.0f,-1.0f, 0.0f,
    -0.5f, -1.0f, -0.3f,   0.0f,-1.0f, 0.0f,
};
unsigned int boxIndices[] = {
    0,1,2, 2,3,0,
    4,5,6, 6,7,4,
    8,9,10, 10,11,8,
    12,13,14, 14,15,12,
    16,17,18, 18,19,16,
    20,21,22, 22,23,20
};

// دوال إعدادات المجسمات الأساسية مع الأقراص و المستطيلات

std::vector<float> diskVertices;
std::vector<unsigned int> diskIndices;
unsigned int diskVAO = 0, diskVBO = 0, diskEBO = 0;

void createDisk(float radius, float thickness, int segments)
{
    diskVertices.clear();
    diskIndices.clear();

    diskVertices.push_back(0.f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(thickness * 0.5f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(1.f);

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.f * glm::pi<float>() * float(i) / float(segments);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        diskVertices.push_back(x);
        diskVertices.push_back(y);
        diskVertices.push_back(thickness * 0.5f);
        diskVertices.push_back(0.f);
        diskVertices.push_back(0.f);
        diskVertices.push_back(1.f);
    }

    unsigned int baseCenterIndex = (unsigned int)(diskVertices.size() / 6);
    diskVertices.push_back(0.f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(-thickness * 0.5f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(0.f);
    diskVertices.push_back(-1.f);

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.f * glm::pi<float>() * float(i) / float(segments);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        diskVertices.push_back(x);
        diskVertices.push_back(y);
        diskVertices.push_back(-thickness * 0.5f);
        diskVertices.push_back(0.f);
        diskVertices.push_back(0.f);
        diskVertices.push_back(-1.f);
    }

    for (int i = 1; i <= segments; i++)
    {
        diskIndices.push_back(0);
        diskIndices.push_back(i);
        diskIndices.push_back(i + 1);
    }

    for (int i = 1; i <= segments; i++)
    {
        diskIndices.push_back(baseCenterIndex);
        diskIndices.push_back(baseCenterIndex + i + 1);
        diskIndices.push_back(baseCenterIndex + i);
    }

    int topStart = 1;
    int bottomStart = baseCenterIndex + 1;

    for (int i = 0; i < segments; i++)
    {
        int next = (i + 1) % segments;
        diskIndices.push_back(topStart + i);
        diskIndices.push_back(bottomStart + i);
        diskIndices.push_back(bottomStart + next);

        diskIndices.push_back(topStart + i);
        diskIndices.push_back(bottomStart + next);
        diskIndices.push_back(topStart + next);
    }
}

void setupDisk()
{
    glGenVertexArrays(1, &diskVAO);
    glGenBuffers(1, &diskVBO);
    glGenBuffers(1, &diskEBO);

    glBindVertexArray(diskVAO);
    glBindBuffer(GL_ARRAY_BUFFER, diskVBO);
    glBufferData(GL_ARRAY_BUFFER, diskVertices.size() * sizeof(float), diskVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, diskEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, diskIndices.size() * sizeof(unsigned int), diskIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// مستطيل للعقارب والرقاص
float rectVertices[] = {
    // pos               // normal
    -0.04f,  0.2f, 0.f,  0.f,0.f,1.f,
     0.04f,  0.2f, 0.f,  0.f,0.f,1.f,
     0.04f, -0.2f, 0.f,  0.f,0.f,1.f,
    -0.04f, -0.2f, 0.f,  0.f,0.f,1.f,
};

unsigned int rectIndices[] = { 0,1,2, 2,3,0 };

unsigned int boxVAO, boxVBO, boxEBO;
unsigned int rectVAO, rectVBO, rectEBO;

auto startTime = std::chrono::high_resolution_clock::now();

// حركة العقارب (تعتمد على الوقت الحقيقي)
float getHourAngle()
{
    auto now = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(now - startTime).count();
    return glm::radians(t * 1.0f); // عقرب الساعة يتحرك ببطء شديد
}

float getMinuteAngle()
{
    auto now = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(now - startTime).count();
    return glm::radians(t * 6.0f); // عقرب الدقائق يتحرك ببطء
}

// حركة النواس - حركة جيبية حقيقية
float getPendulumAngle()
{
    auto now = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(now - startTime).count();

    // حركة بندول حقيقية: تردد 0.5 هرتز (دورة كاملة كل ثانيتين)
    float frequency = 0.5f; // هرتز
    float amplitude = glm::radians(30.0f); // سعة 30 درجة

    // حركة جيبية بسيطة
    return amplitude * sin(2.0f * glm::pi<float>() * frequency * t);
}

// تحكم الدوران بالماوس
bool mousePressed = false;
double lastMouseX, lastMouseY;
float rotationX = 0.f, rotationY = 0.f;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mousePressed)
    {
        float xoffset = float(xpos - lastMouseX);
        float yoffset = float(ypos - lastMouseY);
        lastMouseX = xpos;
        lastMouseY = ypos;

        float sensitivity = 0.3f;
        rotationY += xoffset * sensitivity;
        rotationX += yoffset * sensitivity;

        if (rotationX > 89.0f) rotationX = 89.0f;
        if (rotationX < -89.0f) rotationX = -89.0f;
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "3D Pendulum Clock - Top Position", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // إعداد الصندوق
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);

    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // إعداد المستطيل
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glGenBuffers(1, &rectEBO);

    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // إعداد القرص
    createDisk(0.55f, 0.05f, 64);
    setupDisk();

    glm::vec3 lightPos(2.f, 3.f, 2.f);
    glm::vec3 viewPos(0.f, 1.5f, 5.f); // كاميرا مرتفعة

    glViewport(0, 0, 800, 800);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.95f, 0.95f, 0.95f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(45.f), 1.f, 0.1f, 100.f);
        glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));

        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

        // دوران الساعة مع الماوس
        glm::mat4 rotation = glm::mat4(1.f);
        rotation = glm::rotate(rotation, glm::radians(rotationX), glm::vec3(1.f, 0.f, 0.f));
        rotation = glm::rotate(rotation, glm::radians(rotationY), glm::vec3(0.f, 1.f, 0.f));

        // رفع كامل للمجسم لأعلى الصفحة
        float globalElevation = 1.5f; // ارتفاع كامل للمجسم

        // 1) صندوق الساعة خمري داكن - مرفوع كامل
        glm::mat4 boxModel = glm::translate(rotation, glm::vec3(0.f, globalElevation, 0.f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(boxModel));
        glUniform3f(colorLoc, 0.402f, 0.0f, 0.0f);

        glBindVertexArray(boxVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // 2) قرص الساعة خمري فاتح - مرفوع كامل
        glm::mat4 clockFace = glm::translate(rotation, glm::vec3(0.f, globalElevation, 0.3f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(clockFace));
        glUniform3f(colorLoc, 0.686f, 0.274f, 0.274f);

        glBindVertexArray(diskVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)diskIndices.size(), GL_UNSIGNED_INT, 0);

        // 3) علامات الساعة 12 علامة - مرفوعة كامل
        glBindVertexArray(rectVAO);
        glUniform3f(colorLoc, 0.4f, 0.0f, 0.0f);

        for (int i = 0; i < 12; i++)
        {
            float angle = glm::radians(i * 30.f);
            glm::mat4 markModel = glm::translate(rotation, glm::vec3(glm::cos(angle) * 0.45f, glm::sin(angle) * 0.45f + globalElevation, 0.35f));
            markModel = glm::rotate(markModel, angle, glm::vec3(0.f, 0.f, 1.f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(markModel));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // الحصول على زوايا الحركة
        float pendulumAngle = getPendulumAngle();
        float hourAngle = getHourAngle();
        float minuteAngle = getMinuteAngle();

        // 4) عقرب الساعة (أقصر وأسمك) - أسود - مرفوع كامل
        glm::mat4 hourHand = glm::translate(rotation, glm::vec3(0.f, globalElevation, 0.45f));
        hourHand = glm::rotate(hourHand, hourAngle, glm::vec3(0.f, 0.f, 1.f));
        hourHand = glm::scale(hourHand, glm::vec3(0.2f, 0.5f, 0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(hourHand));
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 5) عقرب الدقائق (طويل) - أسود - مرفوع كامل
        glm::mat4 minuteHand = glm::translate(rotation, glm::vec3(0.f, globalElevation, 0.4f));
        minuteHand = glm::rotate(minuteHand, minuteAngle, glm::vec3(0.f, 0.f, 1.f));
        minuteHand = glm::scale(minuteHand, glm::vec3(0.2f, 0.8f, 0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(minuteHand));
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 6) نقطة تعليق الرقاص (منتصف أسفل الصندوق المرفوع)
        glm::mat4 pendulumPivot = glm::translate(rotation, glm::vec3(0.f, globalElevation - 1.0f, 0.f));
        pendulumPivot = glm::scale(pendulumPivot, glm::vec3(0.05f, 0.05f, 0.05f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pendulumPivot));
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(diskVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)diskIndices.size(), GL_UNSIGNED_INT, 0);

        // 7) قضيب الرقاص (طويل ورفيع) - يتحرك من نقطة التعليق المرفوعة
        glm::mat4 pendulumRod = glm::translate(rotation, glm::vec3(0.f, globalElevation - 1.0f, 0.f));
        pendulumRod = glm::rotate(pendulumRod, pendulumAngle, glm::vec3(0.f, 0.f, 1.f));
        pendulumRod = glm::translate(pendulumRod, glm::vec3(0.f, -0.3f, 0.f));
        pendulumRod = glm::scale(pendulumRod, glm::vec3(0.2f, 2.6f, 0.2f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pendulumRod));
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

        glBindVertexArray(rectVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 8) كرة الرقاص (كبيرة وواضحة) - متصلة بنهاية القضيب
        glm::mat4 pendulumBall = glm::translate(rotation, glm::vec3(0.f, globalElevation - 1.0f, 0.f));
        pendulumBall = glm::rotate(pendulumBall, pendulumAngle, glm::vec3(0.f, 0.f, 1.f));
        pendulumBall = glm::translate(pendulumBall, glm::vec3(0.f, -0.8f, 0.f));
        pendulumBall = glm::scale(pendulumBall, glm::vec3(0.20f, 0.20f, 0.06f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pendulumBall));
        glUniform3f(colorLoc, 0.9f, 0.8f, 0.1f);

        glBindVertexArray(diskVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)diskIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // تنظيف
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO);

    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteBuffers(1, &rectEBO);

    glDeleteVertexArrays(1, &diskVAO);
    glDeleteBuffers(1, &diskVBO);
    glDeleteBuffers(1, &diskEBO);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}