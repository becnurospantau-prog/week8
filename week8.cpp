// 8-апта: тереңдік буфері, back-face culling, Z-fighting, бірнеше объект
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

int gWidth = 800, gHeight = 600;

// Басқару (тапсырмалар үшін)
bool  gDepth = true;       // T : тереңдік тесті қосу/өшіру (1-тапсырма)
bool  gZFight = false;     // Z : Z-fighting жасау/жою (2-тапсырма)
bool  gCull = true;        // C : culling қосу/өшіру
bool  gCullFront = false;  // F : GL_BACK <-> GL_FRONT (3-тапсырма)
bool  gWire = false;       // Tab : wireframe
float gNear = 0.1f;
float gFar = 100.0f;

const char* vsSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vColor;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fsSrc = R"(#version 330 core
in vec3 vColor;
uniform vec3 uTint;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor * uTint, 1.0);
}
)";

// 36 вершина, барлық қыр CCW (сырттан қарағанда сағат тіліне қарсы)
float vertices[] = {
    // артқы қыр (-z) қызыл
    -0.5f,-0.5f,-0.5f, 1.0f,0.0f,0.0f,
    -0.5f, 0.5f,-0.5f, 1.0f,0.0f,0.0f,
     0.5f, 0.5f,-0.5f, 1.0f,0.0f,0.0f,
     0.5f, 0.5f,-0.5f, 1.0f,0.0f,0.0f,
     0.5f,-0.5f,-0.5f, 1.0f,0.0f,0.0f,
    -0.5f,-0.5f,-0.5f, 1.0f,0.0f,0.0f,
    // алдыңғы қыр (+z) жасыл
    -0.5f,-0.5f, 0.5f, 0.0f,1.0f,0.0f,
     0.5f,-0.5f, 0.5f, 0.0f,1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 0.0f,1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 0.0f,1.0f,0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,1.0f,0.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,1.0f,0.0f,
    // сол қыр (-x) көк
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,0.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,0.0f,1.0f,
    -0.5f, 0.5f,-0.5f, 0.0f,0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,1.0f,
    // оң қыр (+x) сары
     0.5f,-0.5f,-0.5f, 1.0f,1.0f,0.0f,
     0.5f, 0.5f,-0.5f, 1.0f,1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 1.0f,1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 1.0f,1.0f,0.0f,
     0.5f,-0.5f, 0.5f, 1.0f,1.0f,0.0f,
     0.5f,-0.5f,-0.5f, 1.0f,1.0f,0.0f,
    // төменгі қыр (-y) көгілдір
    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,1.0f,
     0.5f,-0.5f,-0.5f, 0.0f,1.0f,1.0f,
     0.5f,-0.5f, 0.5f, 0.0f,1.0f,1.0f,
     0.5f,-0.5f, 0.5f, 0.0f,1.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,1.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,1.0f,
    // жоғарғы қыр (+y) қызғылт
    -0.5f, 0.5f,-0.5f, 1.0f,0.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 1.0f,0.0f,1.0f,
     0.5f, 0.5f, 0.5f, 1.0f,0.0f,1.0f,
     0.5f, 0.5f, 0.5f, 1.0f,0.0f,1.0f,
     0.5f, 0.5f,-0.5f, 1.0f,0.0f,1.0f,
    -0.5f, 0.5f,-0.5f, 1.0f,0.0f,1.0f,
};

void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    gWidth = w;
    gHeight = h;
    glViewport(0, 0, w, h);
}

void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        case GLFW_KEY_T: gDepth = !gDepth; std::cout << "depth test: " << gDepth << "\n"; break;
        case GLFW_KEY_C: gCull = !gCull; std::cout << "cull: " << gCull << "\n"; break;
        case GLFW_KEY_F: gCullFront = !gCullFront; std::cout << (gCullFront ? "cull FRONT\n" : "cull BACK\n"); break;
        case GLFW_KEY_Z:
            gZFight = !gZFight;
            gNear = gZFight ? 0.0001f : 0.1f;
            gFar  = gZFight ? 1000.0f : 100.0f;
            std::cout << "z-fight: " << gZFight << " near=" << gNear << " far=" << gFar << "\n";
            break;
        case GLFW_KEY_B: gFar = (gFar == 1000.0f) ? 100.0f : 1000.0f; std::cout << "far=" << gFar << "\n"; break;
        case GLFW_KEY_TAB: gWire = !gWire; break;
    }
}

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cerr << log << "\n"; }
    return s;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 8: Depth + Culling", nullptr, nullptr);
    if (!window) { std::cerr << "Window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n"; return -1;
    }

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLint locModel = glGetUniformLocation(prog, "uModel");
    GLint locView  = glGetUniformLocation(prog, "uView");
    GLint locProj  = glGetUniformLocation(prog, "uProjection");
    GLint locTint  = glGetUniformLocation(prog, "uTint");

    // Тереңдік буфері бар ма екенін тексеру
    int bits = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    std::cout << "depth bits: " << bits << "\n";
    std::cout << "T depth | Z z-fight | B far 100<->1000 | C cull | F cull front/back | Tab wireframe\n";

    // Циклге дейін, бір рет
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glm::vec3 positions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  0.5f, -2.0f),
        glm::vec3(-1.8f, -0.3f, -1.0f),
        glm::vec3( 1.2f, -0.8f, -3.5f),
        glm::vec3(-2.2f,  1.0f, -4.0f)
    };

    while (!glfwWindowShouldClose(window)) {
        // Пернемен ауыстырылатын күйлер
        if (gDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (gCull)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
        glCullFace(gCullFront ? GL_FRONT : GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, gWire ? GL_LINE : GL_FILL);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DEPTH ұмытпа!

        glUseProgram(prog);

        float angle = (float)glfwGetTime();
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, gNear, gFar);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glBindVertexArray(vao);

        if (!gZFight) {
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            for (int i = 0; i < 5; i++) {
                glm::mat4 model = glm::mat4(1.0f); // цикл ішінде жарияланады
                model = glm::translate(model, positions[i]);
                model = glm::rotate(model, angle * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        } else {
            // Z-fighting: екі куб бетпе-бет, z айырмасы тек 0.001
            glm::mat4 base = glm::rotate(glm::mat4(1.0f), 0.4f * sinf(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniform3f(locTint, 1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(base));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 second = glm::translate(base, glm::vec3(0.0f, 0.0f, 0.001f));
            glUniform3f(locTint, 0.4f, 0.4f, 0.4f); // қараңғырақ, жыпылықтау көрінсін
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(second));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}