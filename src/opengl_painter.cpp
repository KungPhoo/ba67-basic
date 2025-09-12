#include "opengl_painter.h"

#include <glad/glad.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cmath>
#include "os.h"

// Globals
static GLuint texID = 0;
static GLuint vao = 0, vbo = 0;
static GLuint shaderProgram = 0;

// Simple shader sources
static const char* vertexSrc = R"(
#version 330 core
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
out vec2 TexCoord;
void main() {
    TexCoord = uv;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static const char* fragmentSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex;
uniform vec2 uWinSize;   // (winWidth, winHeight)
uniform vec2 uDrawSize;  // (drawW, drawH)
uniform vec2 uOffset;    // (offsetX, offsetY)
uniform vec4 uBorderColor;  // RGBA color for out-of-range areas
uniform bool crt_emulation;

// Hard-coded 4x4 RGB multipliers
const vec3 blockMatrix[16] = vec3[16]( // vertically flipped
    0.7*vec3(1.0, 0.7, 0.7), 0.7*vec3(0.7, 1.0, 0.7), 0.7*vec3(0.7, 0.7, 1.0), 0.7*vec3(0.7, 0.7, 0.7),
    vec3(1.0, 0.7, 0.7),vec3(0.7, 1.0, 0.7),vec3(0.7, 0.7, 1.0),vec3(0.7, 0.7, 0.7),
    vec3(1.0, 0.7, 0.7),vec3(0.7, 1.0, 0.7),vec3(0.7, 0.7, 1.0),vec3(0.7, 0.7, 0.7),
    vec3(1.0, 0.7, 0.7),vec3(0.7, 1.0, 0.7),vec3(0.7, 0.7, 1.0),vec3(0.7, 0.7, 0.7)
);


void main() {
    // Convert current screen fragment position (TexCoord is [0,1]) to pixel coords
    vec2 fragPixel = TexCoord * uWinSize;

    // Compute normalized UV relative to draw area
    vec2 rel = (fragPixel - uOffset) / uDrawSize;

    if (rel.x < 0.0 || rel.x > 1.0 ||
        rel.y < 0.0 || rel.y > 1.0) {
        FragColor = uBorderColor;
    } else {
        // Flip Y coordinate (top-left window origin -> bottom-left texture origin)
        rel.y = 1.0 - rel.y;
        FragColor = texture(tex, rel);
    }

    if(crt_emulation){
        vec2 resolution=uWinSize;

        // Convert TexCoord to pixel coordinates
        vec2 pixelPos = TexCoord * resolution;

        // Get position within 4x4 block
        ivec2 blockPos = ivec2(mod(pixelPos.xy, 4.0));

        // Compute index into the 16-element matrix
        int index = blockPos.y * 4 + blockPos.x;

        // Apply the hardcoded RGB multiplier
        FragColor.rgb *= blockMatrix[index];




        // float scan = 0.75; // simulate darkness between scanlines
        // float apply = abs(sin(gl_FragCoord.y)*0.5*scan);

        // FragColor = vec4( mix( FragColor.rgb, vec3(0.0), apply ), 1.0);
    }
}
)";


// Helper: compile shader and link program
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compile error: " << info << "\n";
    }
    return shader;
}

GLuint createShaderProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        std::cerr << "Shader link error: " << info << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}


void OpenGLPainter::initGL() {
    // (GLADloadproc)glfwGetProcAddress)
    if (!gladLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

#if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
    // I get 4.x, so we can use shaders etc.
    std::cout << "OpenGL version " << glGetString(GL_VERSION) << "\n";
#endif


    // 1. Compile shaders and create program
    shaderProgram = createShaderProgram(vertexSrc, fragmentSrc);

    // 2. Setup fullscreen quad
    float vertices[] = {
        // pos.x, pos.y,  u, v
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,

        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}


// winWidth, winHeight: OpenGL context size. Can be resized.
// pixWidth, pixHeight: texture bitmap size.
// pixelsBGRA: pixel data (pixWidth * pixHeight)
// frameColorBGRA: glClearColor
void OpenGLPainter::draw(size_t winWidth, size_t winHeight, size_t pixWidth, size_t pixHeight, uint32_t* pixelsBGRA, uint32_t borderColorBGRA) {
    if (pixWidth == 0 || pixHeight == 0) {
        return;
    }

    static size_t oldW = 0, oldH = 0;
    if (oldW != pixWidth || oldH != pixHeight) { // switched to 80 column mode e.g.
        oldW = pixWidth;
        oldH = pixHeight;


        if (texID != 0) {
            glDeleteTextures(1, &texID);
            texID = 0;
        }
        // 3. Create texture
        glGenTextures(1, &texID);

        glBindTexture(GL_TEXTURE_2D, texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // create new texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GLsizei(pixWidth), GLsizei(pixHeight), 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    }


    // center image and keep proportions
    float scaleX = static_cast<float>(winWidth) / pixWidth;
    float scaleY = static_cast<float>(winHeight) / pixHeight;

    if (pixWidth / pixHeight == 1 /*320x200*/) {
        float scale = std::max(0.25f, std::min(scaleX, scaleY)); // Keep aspect ratio
        scaleX = scaleY = scale;
    } else { // 640x200
        float scale = std::max(0.25f, std::min(scaleX, scaleY / 2.0f)); // Keep aspect ratio
        scaleX      = scale;
        scaleY      = scale * 2;
    }

    // scale to full pixels
    if (scaleX > 1.0) {
        scaleX = floor(scaleX);
    }
    if (scaleY > 1.0) {
        scaleY = floor(scaleY);
    }

    // image width to be drawn on the screen
    size_t drawW = static_cast<size_t>(pixWidth * scaleX);
    size_t drawH = static_cast<size_t>(pixHeight * scaleY);
    // offset on window to center the image
    float offsetX = float(winWidth - drawW) / 2.0f;
    float offsetY = float(winHeight - drawH) / 2.0f;


    glViewport(0, 0, GLsizei(winWidth), GLsizei(winHeight));

    // pass to pixel shader
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "crt_emulation"), 1);
    glUniform2f(glGetUniformLocation(shaderProgram, "uWinSize"), (float)winWidth, (float)winHeight);
    glUniform2f(glGetUniformLocation(shaderProgram, "uDrawSize"), (float)drawW, (float)drawH);
    glUniform2f(glGetUniformLocation(shaderProgram, "uOffset"), (float)offsetX, (float)offsetY);

    const float one_over_255 = 1.0f / 255.0f;
    auto red                 = [one_over_255](uint32_t c) { return float((c >> 0) & 0xFF) * one_over_255; };
    auto green               = [one_over_255](uint32_t c) { return float((c >> 8) & 0xFF) * one_over_255; };
    auto blue                = [one_over_255](uint32_t c) { return float((c >> 16) & 0xFF) * one_over_255; };

    // Assuming fully opaque border
    glUseProgram(shaderProgram);
    GLint locBorder = glGetUniformLocation(shaderProgram, "uBorderColor");
    glUniform4f(locBorder,
                red(borderColorBGRA),
                green(borderColorBGRA),
                blue(borderColorBGRA),
                1.0f);

    // Upload new data to texture
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GLsizei(pixWidth), GLsizei(pixHeight), GL_BGRA, GL_UNSIGNED_BYTE, pixelsBGRA);

    // Render
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}


void OpenGLPainter::shutdownGL() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
}
