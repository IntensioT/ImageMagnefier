﻿#include <Windows.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

unsigned int loadGeometry();
unsigned int loadShader();
unsigned int loadTexture(unsigned char *data, int channels);
void framebuffer_resize(GLFWwindow *window, int w, int h);
void processInput(GLFWwindow* window);

float zoom = 1.0f;
float offsetX = 0.0f;
float offsetY = 0.0f;
const float zoomStep = 0.1f;
const float moveStep = 0.05f;

float megnefierSpeed = 10.f;


int offsetx, offsety, width, height;

void receiveDataFromUSART() {
    HANDLE hSerial = CreateFileA("COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening COM port" << std::endl;
        return;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting COM port state" << std::endl;
        CloseHandle(hSerial);
        return;
    }
    dcbSerialParams.BaudRate = CBR_9600; 
    dcbSerialParams.ByteSize = 8;          
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;    
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting COM port state" << std::endl;
        CloseHandle(hSerial);
        return;
    }

    int8_t buffer[1];
    DWORD bytesRead;
    if (ReadFile(hSerial, buffer, sizeof(buffer), &bytesRead, NULL)) {
        // Обработка данных в buffer[0]
        std::cout << "Received byte: " << static_cast<int>(buffer[0]) << std::endl;
        int8_t byteValue = buffer[0];
        bool decreaseScale = byteValue & (1 << 0);  // 0-й бит - уменьшить масштаб
        bool increaseScale = byteValue & (1 << 1);  // 1-й бит - увеличить масштаб
        bool decreaseX = byteValue & (1 << 2);      // 2-й бит - уменьшаем по x
        bool increaseX = byteValue & (1 << 3);      // 3-й бит - увеличиваем по x
        bool decreaseY = byteValue & (1 << 4);      // 4-й бит - уменьшаем по y
        bool increaseY = byteValue & (1 << 5);      // 5-й бит - увеличиваем по y

        if (decreaseScale) {
            zoom = std::min(zoom - zoomStep, 10.0f);
        }

        if (increaseScale) {
            zoom = std::min(zoom + zoomStep, 10.0f);
        }

        if (decreaseX) {
            offsetY += moveStep / zoom;
        }

        if (increaseX) {
            offsetY -= moveStep / zoom;
        }

        if (decreaseY) {
            offsetX -= moveStep / zoom;
        }

        if (increaseY) {
            offsetX += moveStep / zoom;
        }
    }
    else {
        std::cerr << "Error reading from COM port" << std::endl;
    }

    CloseHandle(hSerial);
}


int main(int argc, char **argv) {
    /*if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <imagename>" << std::endl;
        return EXIT_FAILURE;
    }*/
    offsetx = 0;
    offsety = 0;
    //const char *filename = argv[1];
    const char *filename = "D:\\Media\\photo\\avatars\\hayasaka6.png";
    int channels;
    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);
    if(data == nullptr) {
        perror(filename);
        return EXIT_FAILURE;
    }

    if(glfwInit() == GLFW_FALSE) {
        std::cerr << "failed to init glfw" << std::endl;
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow * window = glfwCreateWindow(width, height, "Image", NULL, NULL);
    if(window == nullptr) {
        std::cerr << "failed to make window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize);
    glfwSwapInterval(1);

    if(gladLoadGL() == GLFW_FALSE) {
        std::cerr << "failed to init glad" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glViewport(offsetx, offsety, width, height);

    unsigned int VAO = loadGeometry();
    unsigned int program = loadShader();
    unsigned int texture = loadTexture(data, channels);

    while(!glfwWindowShouldClose(window)) {
        receiveDataFromUSART();

        processInput(window);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(program, "tex"), 0);

        glUniform1f(glGetUniformLocation(program, "zoom"), zoom);
        glUniform2f(glGetUniformLocation(program, "offset"), offsetX, offsetY);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        //glfwWaitEvents();
        glfwPollEvents();
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}

unsigned int loadGeometry() {
    float vertices[] = {-1, -1, -1, 1, 1, 1, -1, -1, 1, 1, 1, -1};
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

unsigned int loadShader() {
    int success;
    char infoLog[512];


    const char *vCode = "#version 450 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "out vec2 uv;\n"
    "void main() {\n"
        "gl_Position = vec4(aPos, 0, 1);\n"
        "uv = (aPos + 1) / 2;\n"
    "}\n\0";

    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vCode, NULL);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vShader, 512, NULL, infoLog);
        std::cerr << "SHADER::VERTEX::COMPILE ERROR\n" << infoLog << std::endl;
    }

    const char *fCode = "#version 450 core\n"
    "in vec2 uv;\n"
    "out vec4 FragColor;"
    "uniform sampler2D tex;"
    "uniform vec2 offset;\n"
    "uniform float zoom;\n"
    "void main() {\n"
    "    vec2 scaledUV = (uv - 0.5) / zoom + 0.5 + offset;\n"
        "FragColor = texture(tex, scaledUV);\n"
    "}\n\0";
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fCode, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        std::cerr << "SHADER::FRAGMENT::COMPILE ERROR\n" << infoLog << std::endl;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "SHADER::PROGRAM::LINKING ERROR\n" << infoLog << std::endl;
    }
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    return program;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        offsetY += moveStep / zoom;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        offsetY -= moveStep / zoom;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        offsetX -= moveStep / zoom;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        offsetX += moveStep / zoom;
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) { // Key '+'z
        zoom = std::min(zoom + zoomStep, 10.0f); 
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) { // Key '-'
        zoom = std::max(zoom - zoomStep, 0.1f);
    }
}


unsigned int loadTexture(unsigned char *data, int channels) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int format = channels == 3 ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return texture;
}

void framebuffer_resize(GLFWwindow * window, int w, int h) {
    glViewport(offsetx, offsety, w, h);
    width = w;
    height = h;
}