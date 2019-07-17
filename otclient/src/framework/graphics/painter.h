/*
* Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#ifndef PAINTER_H
#define PAINTER_H

#include <framework/graphics/declarations.h>
#include <framework/graphics/coordsbuffer.h>
#include <framework/graphics/paintershaderprogram.h>
#include <framework/graphics/texture.h>
#include <framework/graphics/colorarray.h>
#include <framework/graphics/drawqueue.h>

struct LightSource { // todo move it somewhere else
    Color color;
    int radius;
    float depth;
};

class Painter
{
public:
    enum BlendEquation {
        BlendEquation_Add,
        BlendEquation_Max,
        BlendEquation_Subtract
    };
    enum CompositionMode {
        CompositionMode_Normal,
        CompositionMode_Multiply,
        CompositionMode_Add,
        CompositionMode_Replace,
        CompositionMode_DestBlending,
        CompositionMode_Light,
        CompositionMode_AlphaZeroing,
        CompositionMode_AlphaRestoring,
        CompositionMode_ZeroAlphaOverrite

    };
    enum DepthFunc {
        DepthFunc_None,
        DepthFunc_LESS,
        DepthFunc_LESS_READ,
        DepthFunc_LEQUAL,
        DepthFunc_LEQUAL_READ,
        DepthFunc_EQUAL,
        DepthFunc_ALWAYS,
        DepthFunc_ALWAYS_READ
    };
    enum DrawMode {
        Triangles = GL_TRIANGLES,
        TriangleStrip = GL_TRIANGLE_STRIP
    };

    struct PainterState {
        Size resolution;
        Matrix3 transformMatrix;
        Matrix3 projectionMatrix;
        Matrix3 textureMatrix;
        Color color;
        float opacity;
        float depth;
        Painter::CompositionMode compositionMode;
        Painter::BlendEquation blendEquation;
        Painter::DepthFunc depthFunc;
        Rect clipRect;
        Texture *texture;
        PainterShaderProgram *shaderProgram;
        bool alphaWriting;
    };

    Painter();
    ~Painter() { }

    void bind();
    void unbind();

    void resetState();
    void refreshState();
    void saveState();
    void saveAndResetState();
    void restoreSavedState();

    void clear(const Color& color);
    void clearRect(const Color& color, const Rect& rect);

    void setTransformMatrix(const Matrix3& transformMatrix) { m_transformMatrix = transformMatrix; }
    void setProjectionMatrix(const Matrix3& projectionMatrix) { m_projectionMatrix = projectionMatrix; }
    void setTextureMatrix(const Matrix3& textureMatrix) { m_textureMatrix = textureMatrix; }
    void setCompositionMode(CompositionMode compositionMode);
    void setBlendEquation(BlendEquation blendEquation);
    void setDepthFunc(DepthFunc func);
    void setClipRect(const Rect& clipRect);
    void setShaderProgram(PainterShaderProgram *shaderProgram) { m_shaderProgram = shaderProgram; }
    void setTexture(Texture *texture);
    void setDepthTexture(Texture *texture);
    void setAlphaWriting(bool enable);

    void setTexture(const TexturePtr& texture) { setTexture(texture.get()); }
    void setDepthTexture(const TexturePtr& texture) { setDepthTexture(texture.get()); }
    void setResolution(const Size& resolution);

    void scale(float x, float y);
    void translate(float x, float y);
    void rotate(float angle);
    void rotate(float x, float y, float angle);

    void pushTransformMatrix();
    void popTransformMatrix();

    Matrix3 getTransformMatrix() { return m_transformMatrix; }
    Matrix3 getProjectionMatrix() { return m_projectionMatrix; }
    Matrix3 getTextureMatrix() { return m_textureMatrix; }
    BlendEquation getBlendEquation() { return m_blendEquation; }
    PainterShaderProgram *getShaderProgram() { return m_shaderProgram; }
    bool getAlphaWriting() { return m_alphaWriting; }

    void resetBlendEquation() { setBlendEquation(BlendEquation_Add); }
    void resetTexture() { setTexture(nullptr); }
    void resetAlphaWriting() { setAlphaWriting(false); }
    void resetTransformMatrix() { setTransformMatrix(Matrix3()); }

    /* org */
    void drawCoords(CoordsBuffer& coordsBuffer, DrawMode drawMode = Triangles, ColorArray* colorBuffer = nullptr);
    void drawFillCoords(CoordsBuffer& coordsBuffer);
    void drawTextureCoords(CoordsBuffer& coordsBuffer, const TexturePtr& texture);
    void drawTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src);
    void drawLights(const std::map<Point, LightSource>& lights, const TexturePtr& lightTexutre, const TexturePtr& depthTexture, int scaling);
    void drawLightDepthTexture(const Rect& dest, const TexturePtr& depthTexture, const Rect& src);
    void drawColorOnTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src);
    void drawUpsideDownTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src);
    void drawRepeatedTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src);
    void drawFilledRect(const Rect& dest);
    void drawFilledTriangle(const Point& a, const Point& b, const Point& c);
    void drawBoundingRect(const Rect& dest, int innerLineWidth = 1);

    void setDrawProgram(PainterShaderProgram *drawProgram) { m_drawProgram = drawProgram; }
    bool hasShaders() { return true; }

    void setAtlasTextures(const TexturePtr& atlas);
    void drawQueue(DrawQueue& drawqueue);

    //
    void drawTexturedRect(const Rect& dest, const TexturePtr& texture) { drawTexturedRect(dest, texture, Rect(Point(0,0), texture->getSize())); }

    void setColor(const Color& color) { m_color = color; }
    void setShaderProgram(const PainterShaderProgramPtr& shaderProgram) { setShaderProgram(shaderProgram.get()); }

    void scale(float factor) { scale(factor, factor); }
    void translate(const Point& p) { translate(p.x, p.y); }
    void rotate(const Point& p, float angle) { rotate(p.x, p.y, angle); }

    void setOpacity(float opacity) { m_opacity = opacity; }

    void setDepth(float depth) { m_depth = depth; }
    float getDepth() { return m_depth; }

    Size getResolution() { return m_resolution; }
    Color getColor() { return m_color; }
    float getOpacity() { return m_opacity; }
    Rect getClipRect() { return m_clipRect; }
    CompositionMode getCompositionMode() { return m_compositionMode; }

    DepthFunc getDepthFunc() { return m_depthFunc; }

    void resetClipRect() { setClipRect(Rect()); }
    void resetOpacity() { setOpacity(1.0f); }
    void resetDepth() { return setDepth(0.0f); }
    void resetCompositionMode() { setCompositionMode(CompositionMode_Normal); }
    void resetColor() { setColor(Color::white); }
    void resetShaderProgram() { setShaderProgram(nullptr); }
    void resetDepthFunc() { setDepthFunc(DepthFunc_None); }

    int draws() { return m_draws; }
    int calls() { return m_calls; }
    void resetDraws() { m_draws = m_calls = 0; }

    void setDrawColorOnTextureShaderProgram() {
        setShaderProgram(m_drawSolidColorOnTextureProgram);
    }

protected:
    void updateGlTexture();
    void updateGlCompositionMode();
    void updateGlBlendEquation();
    void updateGlClipRect();
    void updateGlAlphaWriting();
    void updateGlViewport();
    void updateDepthFunc();

    CoordsBuffer m_coordsBuffer;
    CoordsBuffer m_coordsDepthBuffer;

    std::vector<Matrix3> m_transformMatrixStack;
    Matrix3 m_transformMatrix;
    Matrix3 m_projectionMatrix;
    Matrix3 m_textureMatrix;
    Matrix3 m_atlasTextureMatrix;

    BlendEquation m_blendEquation;
    Texture *m_texture;
    bool m_alphaWriting;

    PainterState m_olderStates[10];
    int m_oldStateIndex;

    uint m_glTextureId;

    PainterShaderProgram *m_shaderProgram;
    CompositionMode m_compositionMode;
    DepthFunc m_depthFunc;
    Color m_color;
    Size m_resolution;
    float m_opacity;
    float m_depth;
    Rect m_clipRect;
    int m_draws = 0;
    int m_calls = 0;

private:
    PainterShaderProgram *m_drawProgram;
    PainterShaderProgramPtr m_drawTexturedProgram;
    PainterShaderProgramPtr m_drawSolidColorProgram;
    PainterShaderProgramPtr m_drawSolidColorOnTextureProgram;

    PainterShaderProgramPtr m_drawNewProgram;    

    PainterShaderProgramPtr m_drawLightProgram;    
    PainterShaderProgramPtr m_drawLightDepthScalingProgram;    
};

extern Painter *g_painter;

#endif
