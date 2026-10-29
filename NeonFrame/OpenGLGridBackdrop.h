#pragma once
#include <JuceHeader.h>
#include <cmath>

/** Lightweight OpenGL grid backdrop — continuous repaint OFF, cached uniforms, sparse mesh. */
class OpenGLGridBackdrop : public juce::Component,
                           private juce::OpenGLRenderer,
                           private juce::Timer
{
public:
    OpenGLGridBackdrop()
    {
        openGLContext.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
        openGLContext.setRenderer (this);
        // CRITICAL: continuous repaint = 60fps GPU burn. Drive via timer only.
        openGLContext.setContinuousRepainting (false);
        openGLContext.setComponentPaintingEnabled (false);
        startTimerHz (10); // low-rate swap
        setOpaque (false);
        setInterceptsMouseClicks (false, false);
    }

    ~OpenGLGridBackdrop() override
    {
        stopTimer();
        openGLContext.detach();
    }

    void setPulse (float p) { pulse = juce::jlimit (0.f, 1.f, p); }
    void setAccent (juce::Colour c) { accent = c; }

    // GPU context only while on screen: hidden tabs and closed windows hold no GL memory
    void visibilityChanged() override { syncGlContext(); }
    void parentHierarchyChanged() override { syncGlContext(); }
    void syncGlContext()
    {
        const bool want = isShowing();
        if (want == glAttached) return;
        if (want) openGLContext.attachTo (*this); else openGLContext.detach();
        glAttached = want;
    }
    bool glAttached = false;

    void newOpenGLContextCreated() override
    {
        using namespace ::juce::gl;
        glReady = false;
        shader.reset();
        // Cheap shaders — no per-vertex sin wave (was expensive on dense mesh)
        const char* vs = R"(
            #version 150
            in vec3 position;
            uniform mat4 projectionMatrix;
            uniform mat4 viewMatrix;
            out float vFade;
            void main() {
                vec4 world = viewMatrix * vec4(position, 1.0);
                gl_Position = projectionMatrix * world;
                vFade = clamp(1.0 - length(world.xyz) * 0.12, 0.0, 1.0);
            }
        )";
        const char* fs = R"(
            #version 150
            uniform vec3 accentColor;
            uniform float pulse;
            in float vFade;
            out vec4 fragColor;
            void main() {
                float a = vFade * (0.18 + pulse * 0.25);
                fragColor = vec4(accentColor, a);
            }
        )";
        shader = std::make_unique<juce::OpenGLShaderProgram> (openGLContext);
        if (! shader->addVertexShader (vs) || ! shader->addFragmentShader (fs) || ! shader->link())
        {
            shader.reset();
            return;
        }
        // Cache uniform locations once
        uProj  = shader->getUniformIDFromName ("projectionMatrix");
        uView  = shader->getUniformIDFromName ("viewMatrix");
        uPulse = shader->getUniformIDFromName ("pulse");
        uAcc   = shader->getUniformIDFromName ("accentColor");
        positionAttribute = glGetAttribLocation (shader->getProgramID(), "position");
        if (positionAttribute < 0)
        {
            shader.reset();
            return;
        }
        buildMesh();
        glReady = true;
    }

    void openGLContextClosing() override
    {
        shader.reset();
        mesh.release();
        glReady = false;
    }

    void renderOpenGL() override
    {
        using namespace ::juce::gl;
        if (! glReady || shader == nullptr) return;

        // Transparent clear — no full opaque fill cost when layered under UI
        glClearColor (0.f, 0.f, 0.f, 0.f);
        glClear (GL_COLOR_BUFFER_BIT);

        const float w = (float) juce::jmax (1, getWidth());
        const float h = (float) juce::jmax (1, getHeight());
        if (w < 2.f || h < 2.f) return;

        const float aspect = w / h;
        const float fov = 55.0f * juce::MathConstants<float>::pi / 180.f;
        const float nearP = 0.1f, farP = 40.0f;
        const float f = 1.0f / std::tan (fov * 0.5f);
        const float pm[16] = {
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (farP + nearP) / (nearP - farP), -1,
            0, 0, (2 * farP * nearP) / (nearP - farP), 0
        };

        // Static-ish camera (no sin every frame)
        const juce::Vector3D<float> eye (0.4f, 2.3f + pulse * 0.15f, 5.5f);
        const juce::Vector3D<float> center (0, 0, 0);
        const juce::Vector3D<float> up (0, 1, 0);
        auto z = (eye - center);
        const float zl = std::sqrt (z.x * z.x + z.y * z.y + z.z * z.z) + 1e-6f;
        z = { z.x / zl, z.y / zl, z.z / zl };
        auto x = up ^ z;
        const float xl = std::sqrt (x.x * x.x + x.y * x.y + x.z * x.z) + 1e-6f;
        x = { x.x / xl, x.y / xl, x.z / xl };
        auto y = z ^ x;
        const float vm[16] = {
            x.x, y.x, z.x, 0,
            x.y, y.y, z.y, 0,
            x.z, y.z, z.z, 0,
            -(x.x * eye.x + x.y * eye.y + x.z * eye.z),
            -(y.x * eye.x + y.y * eye.y + y.z * eye.z),
            -(z.x * eye.x + z.y * eye.y + z.z * eye.z),
            1
        };

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth (1.0f);

        shader->use();
        if (uProj  >= 0) glUniformMatrix4fv (uProj, 1, GL_FALSE, pm);
        if (uView  >= 0) glUniformMatrix4fv (uView, 1, GL_FALSE, vm);
        if (uPulse >= 0) glUniform1f (uPulse, pulse);
        if (uAcc   >= 0) glUniform3f (uAcc, accent.getFloatRed(), accent.getFloatGreen(), accent.getFloatBlue());

        mesh.draw (openGLContext, GL_LINES, positionAttribute);
    }

    void paint (juce::Graphics&) override {} // GL only — no software paint cost

    void timerCallback() override
    {
        if (! isShowing()) return;
        // Trigger one GL frame at low rate (not continuous)
        openGLContext.triggerRepaint();
    }

private:
    void buildMesh()
    {
        mesh.reset();
        juce::Array<juce::Vector3D<float>> verts;
        // Sparse grid (was N=24 → N=10) — far fewer lines
        const int N = 10;
        const float span = 8.0f;
        for (int i = -N; i <= N; ++i)
        {
            const float t = (float) i / (float) N * span;
            verts.add ({ t, 0.f, -span }); verts.add ({ t, 0.f,  span });
            verts.add ({ -span, 0.f, t }); verts.add ({  span, 0.f, t });
        }
        mesh.load (verts);
    }

    struct Mesh
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        int numVerts = 0;

        void reset() { release(); }
        void release()
        {
            using namespace ::juce::gl;
            if (vbo != 0) { glDeleteBuffers (1, &vbo); vbo = 0; }
            if (vao != 0) { glDeleteVertexArrays (1, &vao); vao = 0; }
            numVerts = 0;
        }
        void load (const juce::Array<juce::Vector3D<float>>& verts)
        {
            using namespace ::juce::gl;
            release();
            numVerts = verts.size();
            if (numVerts <= 0) return;
            glGenVertexArrays (1, &vao);
            glBindVertexArray (vao);
            glGenBuffers (1, &vbo);
            glBindBuffer (GL_ARRAY_BUFFER, vbo);
            glBufferData (GL_ARRAY_BUFFER, (GLsizeiptr) (sizeof (float) * 3 * (size_t) numVerts),
                          verts.getRawDataPointer(), GL_STATIC_DRAW);
            glBindBuffer (GL_ARRAY_BUFFER, 0);
            glBindVertexArray (0);
        }
        void draw (juce::OpenGLContext&, GLenum mode, GLint positionAttribute)
        {
            using namespace ::juce::gl;
            if (vao == 0 || vbo == 0 || numVerts <= 0 || positionAttribute < 0) return;
            glBindVertexArray (vao);
            glBindBuffer (GL_ARRAY_BUFFER, vbo);
            glEnableVertexAttribArray ((GLuint) positionAttribute);
            glVertexAttribPointer ((GLuint) positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays (mode, 0, numVerts);
            glDisableVertexAttribArray ((GLuint) positionAttribute);
            glBindBuffer (GL_ARRAY_BUFFER, 0);
            glBindVertexArray (0);
        }
    };

    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    Mesh mesh;
    bool glReady = false;
    GLint positionAttribute = -1;
    float pulse = 0.f;
    float timeSec = 0.f;
    juce::Colour accent { 0xff00e8ff };
    GLint uProj = -1, uView = -1, uPulse = -1, uAcc = -1;
};
