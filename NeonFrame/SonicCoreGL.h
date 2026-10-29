#pragma once
#include <JuceHeader.h>
#include "VisualFifo.h"
#include <cmath>
#include <vector>

/**
 * SALEK SONIC CORE — OpenGL waveform + spectrum + cyber rings.
 * Feed audio via VisualFifo; setPulse / setAccent from editor.
 */
class SonicCoreGL : public juce::Component,
                    private juce::OpenGLRenderer,
                    private juce::Timer
{
public:
    explicit SonicCoreGL (VisualFifo& fifoIn) : fifo (fifoIn)
    {
        setOpaque (false);
        setInterceptsMouseClicks (false, false);
        openGLContext.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
        openGLContext.setRenderer (this);
        // Continuous GL = massive GPU load; drive frames manually at low rate
        openGLContext.setContinuousRepainting (false);
        openGLContext.setComponentPaintingEnabled (false);
        wave.resize (waveN, 0.f);
        spectrum.resize (specN, 0.f);
        fft = std::make_unique<juce::dsp::FFT> (fftOrder);
        fftBuf.resize ((size_t) (2 * fft->getSize()), 0.f);
        window.resize ((size_t) fft->getSize());
        for (int i = 0; i < fft->getSize(); ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fft->getSize() - 1));
        startTimerHz (12); // ~12 GL frames/sec max
    }

    void timerCallback()
    {
        if (isShowing()) openGLContext.triggerRepaint();
    }

    ~SonicCoreGL() override
    {
        stopTimer();
        openGLContext.detach();
    }

    void setPulse (float p) { pulse.store (juce::jlimit (0.f, 1.f, p)); }
    void setAccent (juce::Colour c) { accent.store (c.getARGB()); }
    void setAccent2 (juce::Colour c) { accent2.store (c.getARGB()); }

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
        const char* vs = R"(
            #version 150
            in vec2 position;
            in vec4 colour;
            out vec4 vCol;
            uniform vec2 scale;
            uniform vec2 offset;
            void main() {
                vec2 p = position * scale + offset;
                gl_Position = vec4(p, 0.0, 1.0);
                vCol = colour;
            }
        )";
        const char* fs = R"(
            #version 150
            in vec4 vCol;
            out vec4 fragColor;
            void main() { fragColor = vCol; }
        )";
        shader = std::make_unique<juce::OpenGLShaderProgram> (openGLContext);
        if (! shader->addVertexShader (vs) || ! shader->addFragmentShader (fs) || ! shader->link())
        {
            shader.reset();
            return;
        }
        // Correct JUCE OpenGL API (no getAttributeID / getUniformID)
        posAttr  = (GLint) glGetAttribLocation (shader->getProgramID(), "position");
        colAttr  = (GLint) glGetAttribLocation (shader->getProgramID(), "colour");
        scaleUni  = shader->getUniformIDFromName ("scale");
        offsetUni = shader->getUniformIDFromName ("offset");
        ready = true;
        time0 = juce::Time::getMillisecondCounterHiRes() * 0.001;
    }

    void openGLContextClosing() override
    {
        shader.reset();
        ready = false;
    }

    void renderOpenGL() override
    {
        using namespace ::juce::gl;
        if (! ready || shader == nullptr) return;

        const float desk = (float) openGLContext.getRenderingScale();
        const int w = juce::jmax (1, juce::roundToInt (desk * (float) getWidth()));
        const int h = juce::jmax (1, juce::roundToInt (desk * (float) getHeight()));
        glViewport (0, 0, w, h);
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor (0.f, 0.f, 0.f, 0.f);
        glClear (GL_COLOR_BUFFER_BIT);

        fifo.readLatest (wave.data(), waveN);
        updateSpectrum();

        const float t = (float) (juce::Time::getMillisecondCounterHiRes() * 0.001 - time0);
        const float pul = pulse.load();
        juce::Colour a1 = juce::Colour (accent.load());
        juce::Colour a2 = juce::Colour (accent2.load());

        shader->use();
        if (scaleUni >= 0)  glUniform2f (scaleUni, 1.f, 1.f);
        if (offsetUni >= 0) glUniform2f (offsetUni, 0.f, 0.f);

        drawRings (t, pul, a1, a2);
        drawWave (pul, a1);
        drawSpectrum (pul, a1, a2);
    }

private:
    static constexpr int waveN = 128;   // was 256
    static constexpr int specN = 32;    // was 64
    static constexpr int fftOrder = 8;  // 256-pt FFT (was 512)

    VisualFifo& fifo;
    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    bool ready = false;
    double time0 = 0.0;
    std::atomic<float> pulse { 0.f };
    std::atomic<uint32_t> accent { 0xff00e8ff };
    std::atomic<uint32_t> accent2 { 0xffff2d9b };

    std::vector<float> wave, spectrum, fftBuf, window;
    std::unique_ptr<juce::dsp::FFT> fft;
    GLint posAttr = -1, colAttr = -1, scaleUni = -1, offsetUni = -1;

    struct Vtx { float x, y, r, g, b, a; };

    void updateSpectrum()
    {
        if (fft == nullptr) return;
        const int N = fft->getSize();
        std::fill (fftBuf.begin(), fftBuf.end(), 0.f);
        for (int i = 0; i < N; ++i)
        {
            float src = (float) i / (float) N * (float) (waveN - 1);
            int i0 = (int) src;
            float f = src - (float) i0;
            float s = wave[(size_t) i0] * (1.f - f) + wave[(size_t) juce::jmin (waveN - 1, i0 + 1)] * f;
            fftBuf[(size_t) i] = s * window[(size_t) i];
        }
        fft->performFrequencyOnlyForwardTransform (fftBuf.data());
        for (int i = 0; i < specN; ++i)
        {
            float mag = fftBuf[(size_t) i];
            spectrum[(size_t) i] = juce::jlimit (0.f, 1.f, std::log10 (1.f + mag * 12.f) / 2.f);
        }
    }

    void drawLineStrip (const std::vector<Vtx>& v)
    {
        using namespace ::juce::gl;
        if (v.size() < 2 || posAttr < 0) return;
        glEnableVertexAttribArray ((GLuint) posAttr);
        glVertexAttribPointer ((GLuint) posAttr, 2, GL_FLOAT, GL_FALSE, sizeof (Vtx), &v[0].x);
        if (colAttr >= 0)
        {
            glEnableVertexAttribArray ((GLuint) colAttr);
            glVertexAttribPointer ((GLuint) colAttr, 4, GL_FLOAT, GL_FALSE, sizeof (Vtx), &v[0].r);
        }
        glDrawArrays (GL_LINE_STRIP, 0, (GLsizei) v.size());
        glDisableVertexAttribArray ((GLuint) posAttr);
        if (colAttr >= 0) glDisableVertexAttribArray ((GLuint) colAttr);
    }

    void drawTriangles (const std::vector<Vtx>& v)
    {
        using namespace ::juce::gl;
        if (v.size() < 3 || posAttr < 0) return;
        glEnableVertexAttribArray ((GLuint) posAttr);
        glVertexAttribPointer ((GLuint) posAttr, 2, GL_FLOAT, GL_FALSE, sizeof (Vtx), &v[0].x);
        if (colAttr >= 0)
        {
            glEnableVertexAttribArray ((GLuint) colAttr);
            glVertexAttribPointer ((GLuint) colAttr, 4, GL_FLOAT, GL_FALSE, sizeof (Vtx), &v[0].r);
        }
        glDrawArrays (GL_TRIANGLES, 0, (GLsizei) v.size());
        glDisableVertexAttribArray ((GLuint) posAttr);
        if (colAttr >= 0) glDisableVertexAttribArray ((GLuint) colAttr);
    }

    void drawRings (float t, float pul, juce::Colour a1, juce::Colour a2)
    {
        // 2 rings × 24 segs (was 5 × 64) — large GPU vertex win
        for (int ring = 0; ring < 2; ++ring)
        {
            std::vector<Vtx> v;
            v.reserve (25);
            const int segs = 24;
            const float rad = 0.22f + ring * 0.12f + pul * 0.05f;
            auto c = a1.interpolatedWith (a2, (float) ring * 0.5f);
            const float alpha = 0.25f + pul * 0.2f;
            for (int i = 0; i <= segs; ++i)
            {
                const float ang = juce::MathConstants<float>::twoPi * (float) i / (float) segs + t * 0.25f;
                v.push_back ({ std::cos (ang) * rad, std::sin (ang) * rad * 0.75f + 0.08f,
                               c.getFloatRed(), c.getFloatGreen(), c.getFloatBlue(), alpha });
            }
            drawLineStrip (v);
        }
    }

    void drawWave (float pul, juce::Colour a1)
    {
        std::vector<Vtx> v;
        v.reserve ((size_t) waveN);
        for (int i = 0; i < waveN; ++i)
        {
            float x = -0.85f + 1.7f * (float) i / (float) (waveN - 1);
            float y = wave[(size_t) i] * (0.22f + pul * 0.15f) - 0.35f;
            float a = 0.45f + pul * 0.4f;
            v.push_back ({ x, y, a1.getFloatRed(), a1.getFloatGreen(), a1.getFloatBlue(), a });
        }
        drawLineStrip (v);
    }

    void drawSpectrum (float pul, juce::Colour a1, juce::Colour a2)
    {
        std::vector<Vtx> tris;
        const float base = -0.92f;
        const float maxH = 0.28f + pul * 0.12f;
        const float left = -0.9f;
        const float width = 1.8f;
        float bw = width / (float) specN;
        for (int i = 0; i < specN; ++i)
        {
            float h = spectrum[(size_t) i] * maxH;
            float x0 = left + (float) i * bw + bw * 0.1f;
            float x1 = x0 + bw * 0.7f;
            auto c = a1.interpolatedWith (a2, (float) i / (float) specN);
            float r = c.getFloatRed(), g = c.getFloatGreen(), b = c.getFloatBlue();
            float a = 0.35f + spectrum[(size_t) i] * 0.5f;
            tris.push_back ({ x0, base, r, g, b, a });
            tris.push_back ({ x1, base, r, g, b, a });
            tris.push_back ({ x1, base + h, r, g, b, a });
            tris.push_back ({ x0, base, r, g, b, a });
            tris.push_back ({ x1, base + h, r, g, b, a });
            tris.push_back ({ x0, base + h, r, g, b, a });
        }
        drawTriangles (tris);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicCoreGL)
};
