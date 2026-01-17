#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// 8x8 像素人数据（0=白，1=黑，2=红）
int pixelArt[64] = {
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,0,1,1,0,1,1,
    1,1,1,1,1,1,1,1,
    1,2,2,1,1,2,2,1,
    1,2,2,1,1,2,2,1,
    0,1,1,2,2,1,1,0,
    0,0,1,1,1,1,0,0
};

// 简单着色器加载工具
GLuint loadShader(const char* path, GLenum type) {
    FILE* file = fopen(path, "rb");
    if (!file) { std::cerr << "Failed to open shader " << path << std::endl; exit(-1); }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);
    char* source = new char[len + 1];
    fread(source, 1, len, file);
    source[len] = '\0';
    fclose(file);

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    delete[] source;

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error in " << path << ":\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vPath, const char* fPath) {
    GLuint vertex = loadShader(vPath, GL_VERTEX_SHADER);
    GLuint fragment = loadShader(fPath, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

int main() {
    // 初始化 GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pixel Art", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD" << std::endl;
        return -1;
    }

    // 全屏 Quad 顶点
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 加载 shader
    GLuint shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");

    // 调色板
    float palette[3][3] = {
        {1.0f, 1.0f, 1.0f}, // 白
        {0.0f, 0.0f, 0.0f}, // 黑
        {1.0f, 0.0f, 0.0f}  // 红
    };

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // 传 pixelArt & palette 给 shader
        glUniform1iv(glGetUniformLocation(shaderProgram, "pixelArt"), 64, pixelArt);
        glUniform3fv(glGetUniformLocation(shaderProgram, "palette"), 3, &palette[0][0]);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
