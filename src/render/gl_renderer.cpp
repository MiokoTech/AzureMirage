// Pengelola konteks EGL dan perenderan OpenGL ES 3.0.
#include "gl_renderer.h"
#include "debugger.h"
#include <vector>

namespace azure_mirage {

static const char* VERTEX_SHADER =
    "#version 300 es\n"
    "layout(location = 0) in vec2 aPosition;\n"
    "uniform mat4 uMVP;\n"
    "void main() {\n"
    "    gl_Position = uMVP * vec4(aPosition, 0.0, 1.0);\n"
    "}\n";

static const char* FRAGMENT_SHADER =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = uColor;\n"
    "}\n";

GLRenderer::GLRenderer()
    : m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
    , m_context(EGL_NO_CONTEXT)
    , m_window(nullptr)
    , m_screenW(1280)
    , m_screenH(720)
    , m_vpX(0), m_vpY(0), m_vpW(1280), m_vpH(720)
    , m_program(0), m_uMVP(-1), m_uColor(-1), m_vao(0), m_vbo(0)
{}

GLRenderer::~GLRenderer() {
    destroyEGL();
}

GLuint GLRenderer::compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint status;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        SB_LOGE("EGL", "Kompile shader gagal: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool GLRenderer::initEGL(ANativeWindow* window) {
    m_window = window;

    m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_display == EGL_NO_DISPLAY) {
        SB_LOGE("EGL", "Error gak dapat EGL default display");
        return false;
    }

    if (!eglInitialize(m_display, nullptr, nullptr)) {
        SB_LOGE("EGL", "Error eglInitialize");
        return false;
    }

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(m_display, attribs, &config, 1, &numConfigs) || numConfigs <= 0) {
        SB_LOGE("EGL", "Error memilih EGL config");
        return false;
    }

    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    m_context = eglCreateContext(m_display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (m_context == EGL_NO_CONTEXT) {
        SB_LOGE("EGL", "Error eglCreateContext (GLES 3.0)");
        return false;
    }

    m_surface = eglCreateWindowSurface(m_display, config, m_window, nullptr);
    if (m_surface == EGL_NO_SURFACE) {
        SB_LOGE("EGL", "Error eglCreateWindowSurface");
        return false;
    }

    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
        SB_LOGE("EGL", "Error eglMakeCurrent");
        return false;
    }

    eglSwapInterval(m_display, 1);

    m_screenW = ANativeWindow_getWidth(m_window);
    m_screenH = ANativeWindow_getHeight(m_window);
    resize(m_screenW, m_screenH);
    SB_LOGI("EGL", "EGL & GLES 3.0 siap: layar %dx%d (viewport %dx%d @ %d,%d)", m_screenW, m_screenH, m_vpW, m_vpH, m_vpX, m_vpY);

    GLuint vs = compileShader(GL_VERTEX_SHADER, VERTEX_SHADER);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    m_uMVP = glGetUniformLocation(m_program, "uMVP");
    m_uColor = glGetUniformLocation(m_program, "uColor");

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void GLRenderer::destroyEGL() {
    if (m_vao != 0) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo != 0) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_program != 0) { glDeleteProgram(m_program); m_program = 0; }

    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_surface != EGL_NO_SURFACE) eglDestroySurface(m_display, m_surface);
        if (m_context != EGL_NO_CONTEXT) eglDestroyContext(m_display, m_context);
        eglTerminate(m_display);
    }
    m_display = EGL_NO_DISPLAY;
    m_surface = EGL_NO_SURFACE;
    m_context = EGL_NO_CONTEXT;
}

void GLRenderer::resize(int width, int height) {
    m_screenW = width;
    m_screenH = height;

    float targetAspect = 1280.0f / 720.0f;
    float screenAspect = static_cast<float>(width) / static_cast<float>(height);

    if (screenAspect >= targetAspect) {
        m_vpH = height;
        m_vpW = static_cast<int>(height * targetAspect);
        m_vpX = (width - m_vpW) / 2;
        m_vpY = 0;
    } else {
        m_vpW = width;
        m_vpH = static_cast<int>(width / targetAspect);
        m_vpX = 0;
        m_vpY = (height - m_vpH) / 2;
    }
}

void GLRenderer::beginFrame() {
    glViewport(0, 0, m_screenW, m_screenH);
    glClearColor(0.04f, 0.06f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(m_vpX, m_vpY, m_vpW, m_vpH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLRenderer::endFrame() {
    if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE) {
        eglSwapBuffers(m_display, m_surface);
    }
}

void GLRenderer::drawQuad(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (m_program == 0 || m_vao == 0) return;

    glUseProgram(m_program);

    float mvp[16] = {
        2.0f / 1280.0f, 0.0f,           0.0f, 0.0f,
        0.0f,          -2.0f / 720.0f,  0.0f, 0.0f,
        0.0f,           0.0f,           1.0f, 0.0f,
       -1.0f,           1.0f,           0.0f, 1.0f
    };
    glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, mvp);
    glUniform4f(m_uColor, r, g, b, a);

    float vertices[8] = {
        x,     y,
        x + w, y,
        x,     y + h,
        x + w, y + h
    };

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}
