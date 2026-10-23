// Manager context OpenGL ES 3.0 dan EGL display.
#ifndef AZURE_MIRAGE_GL_RENDERER_H
#define AZURE_MIRAGE_GL_RENDERER_H

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

namespace azure_mirage {

class GLRenderer {
public:
    GLRenderer();
    ~GLRenderer();

    bool initEGL(ANativeWindow* window);
    void destroyEGL();
    void resize(int width, int height);

    void beginFrame();
    void endFrame();

    void drawQuad(float x, float y, float w, float h, float r, float g, float b, float a);

    int getViewportW() const { return m_vpW; }
    int getViewportH() const { return m_vpH; }

private:
    GLuint compileShader(GLenum type, const char* src);

    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
    ANativeWindow* m_window;

    int m_screenW;
    int m_screenH;
    int m_vpX;
    int m_vpY;
    int m_vpW;
    int m_vpH;

    GLuint m_program;
    GLint m_uMVP;
    GLint m_uColor;
    GLuint m_vao;
    GLuint m_vbo;
};

}

#endif
