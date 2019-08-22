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

#include <framework/graphics/painter.h>
#include <framework/graphics/graphics.h>
#include <framework/graphics/colorarray.h>
#include <framework/graphics/deptharray.h>
#include <framework/platform/platformwindow.h>

#include <framework/graphics/ogl/shadersources.h>
#include <framework/graphics/ogl/newshader.h>
#include <framework/platform/platformwindow.h>
#include <framework/util/extras.h>

Painter *g_painter = nullptr;

Painter::Painter()
{
    m_glTextureId = 0;
    m_oldStateIndex = 0;
    m_color = Color::white;
    m_opacity = 1.0f;
    m_depth = 0;
    m_compositionMode = CompositionMode_Normal;
    m_blendEquation = BlendEquation_Add;
    m_depthFunc = DepthFunc_None;
    m_shaderProgram = nullptr;
    m_texture = nullptr;
    m_alphaWriting = false;
    setResolution(g_window.getSize());

    m_drawProgram = nullptr;
    resetState();

    m_drawTexturedProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawTexturedProgram);
    m_drawTexturedProgram->addShaderFromSourceCode(Shader::Vertex, glslMainWithTexCoordsVertexShader + glslPositionOnlyVertexShader);
    m_drawTexturedProgram->addShaderFromSourceCode(Shader::Fragment, glslMainFragmentShader + glslTextureSrcFragmentShader);
    m_drawTexturedProgram->link();

    m_drawSolidColorProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawSolidColorProgram);
    m_drawSolidColorProgram->addShaderFromSourceCode(Shader::Vertex, glslMainVertexShader + glslPositionOnlyVertexShader);
    m_drawSolidColorProgram->addShaderFromSourceCode(Shader::Fragment, glslMainFragmentShader + glslSolidColorFragmentShader);
    m_drawSolidColorProgram->link();


    m_drawSolidColorOnTextureProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawSolidColorOnTextureProgram);
    m_drawSolidColorOnTextureProgram->addShaderFromSourceCode(Shader::Vertex, glslMainWithTexCoordsVertexShader + glslPositionOnlyVertexShader);
    m_drawSolidColorOnTextureProgram->addShaderFromSourceCode(Shader::Fragment, glslMainFragmentShader + glslSolidColorOnTextureFragmentShader);
    m_drawSolidColorOnTextureProgram->link();    

    m_drawLightProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawLightProgram);
    m_drawLightProgram->addShaderFromSourceCode(Shader::Vertex, lightVertexShader);
    m_drawLightProgram->addShaderFromSourceCode(Shader::Fragment, lightFragmentShader);
    m_drawLightProgram->link();    

    m_drawNewProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawNewProgram);
    m_drawNewProgram->addShaderFromSourceCode(Shader::Vertex, newVertexShader);
    m_drawNewProgram->addShaderFromSourceCode(Shader::Fragment, newFragmentShader);
    m_drawNewProgram->link();    

    m_drawLightDepthProgram = PainterShaderProgramPtr(new PainterShaderProgram);
    assert(m_drawLightDepthProgram);
    m_drawLightDepthProgram->addShaderFromSourceCode(Shader::Vertex, lightDepthVertexShader);
    m_drawLightDepthProgram->addShaderFromSourceCode(Shader::Fragment, lightDepthFragmentShader);
    m_drawLightDepthProgram->link();    

    PainterShaderProgram::release();
}

void Painter::bind()
{
    refreshState();

    // vertex and texture coord attributes are always enabled
    // to avoid massive enable/disables, thus improving frame rate
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::VERTEX_ATTR);
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);
}

void Painter::unbind()
{
    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::VERTEX_ATTR);
    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);
    PainterShaderProgram::release();
}

void Painter::resetState()
{
    resetColor();
    resetOpacity();
    resetDepth();
    resetCompositionMode();
    resetBlendEquation();
    resetDepthFunc();
    resetClipRect();
    resetShaderProgram();
    resetTexture();
    resetAlphaWriting();
    resetTransformMatrix();
}

void Painter::refreshState()
{
    updateGlViewport();
    updateGlCompositionMode();
    updateGlBlendEquation();
    updateDepthFunc();
    updateGlClipRect();
    updateGlTexture();
    updateGlAlphaWriting();
}

void Painter::saveState()
{
    assert(m_oldStateIndex<10);
    m_olderStates[m_oldStateIndex].resolution = m_resolution;
    m_olderStates[m_oldStateIndex].transformMatrix = m_transformMatrix;
    m_olderStates[m_oldStateIndex].projectionMatrix = m_projectionMatrix;
    m_olderStates[m_oldStateIndex].textureMatrix = m_textureMatrix;
    m_olderStates[m_oldStateIndex].color = m_color;
    m_olderStates[m_oldStateIndex].opacity = m_opacity;
    m_olderStates[m_oldStateIndex].depth = m_depth;
    m_olderStates[m_oldStateIndex].compositionMode = m_compositionMode;
    m_olderStates[m_oldStateIndex].blendEquation = m_blendEquation;
    m_olderStates[m_oldStateIndex].depthFunc = m_depthFunc;
    m_olderStates[m_oldStateIndex].clipRect = m_clipRect;
    m_olderStates[m_oldStateIndex].shaderProgram = m_shaderProgram;
    m_olderStates[m_oldStateIndex].texture = m_texture;
    m_olderStates[m_oldStateIndex].alphaWriting = m_alphaWriting;
    m_oldStateIndex++;
}

void Painter::saveAndResetState()
{
    saveState();
    resetState();
}

void Painter::restoreSavedState()
{
    m_oldStateIndex--;
    setResolution(m_olderStates[m_oldStateIndex].resolution);
    setTransformMatrix(m_olderStates[m_oldStateIndex].transformMatrix);
    setProjectionMatrix(m_olderStates[m_oldStateIndex].projectionMatrix);
    setTextureMatrix(m_olderStates[m_oldStateIndex].textureMatrix);
    setColor(m_olderStates[m_oldStateIndex].color);
    setOpacity(m_olderStates[m_oldStateIndex].opacity);
    setDepth(m_olderStates[m_oldStateIndex].depth);
    setCompositionMode(m_olderStates[m_oldStateIndex].compositionMode);
    setBlendEquation(m_olderStates[m_oldStateIndex].blendEquation);
    setDepthFunc(m_olderStates[m_oldStateIndex].depthFunc);
    setClipRect(m_olderStates[m_oldStateIndex].clipRect);
    setShaderProgram(m_olderStates[m_oldStateIndex].shaderProgram);
    setTexture(m_olderStates[m_oldStateIndex].texture);
    setAlphaWriting(m_olderStates[m_oldStateIndex].alphaWriting);
}

void Painter::clear(const Color& color)
{
    glClearColor(color.rF(), color.gF(), color.bF(), color.aF());
#ifdef OPENGL_ES
    glClearDepthf(0.99f);
#else
    glClearDepth(0.99f);
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Painter::clearRect(const Color& color, const Rect& rect)
{
    Rect oldClipRect = m_clipRect;
    setClipRect(rect);
    glClearColor(color.rF(), color.gF(), color.bF(), color.aF());
#ifdef OPENGL_ES
    glClearDepthf(0.99f);
#else
    glClearDepth(0.99f);
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setClipRect(oldClipRect);
}

void Painter::setCompositionMode(Painter::CompositionMode compositionMode)
{
    if(m_compositionMode == compositionMode)
        return;
    m_compositionMode = compositionMode;
    updateGlCompositionMode();
}

void Painter::setBlendEquation(Painter::BlendEquation blendEquation)
{
    if(m_blendEquation == blendEquation)
        return;
    m_blendEquation = blendEquation;
    updateGlBlendEquation();
}

void Painter::setDepthFunc(DepthFunc func) 
{
    if (!g_graphics.canUseDepth())
        return;
    if (m_depthFunc == func)
        return;
    m_depthFunc = func;
    updateDepthFunc();
}

void Painter::setClipRect(const Rect& clipRect)
{
    if(m_clipRect == clipRect)
        return;
    m_clipRect = clipRect;
    updateGlClipRect();
}

void Painter::setTexture(Texture* texture)
{
    if(m_texture == texture)
        return;

    m_texture = texture;

    uint glTextureId;
    if(texture) {
        setTextureMatrix(texture->getTransformMatrix());
        glTextureId = texture->getId();
    } else
        glTextureId = 0;

    if(m_glTextureId != glTextureId) {
        m_glTextureId = glTextureId;
        updateGlTexture();
    }
}

void Painter::setAlphaWriting(bool enable)
{
    if(m_alphaWriting == enable)
        return;

    m_alphaWriting = enable;
    updateGlAlphaWriting();
}

void Painter::setResolution(const Size& resolution)
{
    // The projection matrix converts from Painter's coordinate system to GL's coordinate system
    //    * GL's viewport is 2x2, Painter's is width x height
    //    * GL has +y -> -y going from bottom -> top, Painter is the other way round
    //    * GL has [0,0] in the center, Painter has it in the top-left
    //
    // This results in the Projection matrix below.
    //
    //                                    Projection Matrix
    //   Painter Coord     ------------------------------------------------        GL Coord
    //   -------------     | 2.0 / width  |      0.0      |      0.0      |     ---------------
    //   |  x  y  1  |  *  |     0.0      | -2.0 / height |      0.0      |  =  |  x'  y'  1  |
    //   -------------     |    -1.0      |      1.0      |      1.0      |     ---------------

    Matrix3 projectionMatrix = { 2.0f/resolution.width(),  0.0f,                      0.0f,
        0.0f,                    -2.0f/resolution.height(),  0.0f,
        -1.0f,                     1.0f,                      1.0f };

    m_resolution = resolution;

    setProjectionMatrix(projectionMatrix);
    if(g_painter == this)
        updateGlViewport();
}

void Painter::scale(float x, float y)
{
    Matrix3 scaleMatrix = {
        x,  0.0f,  0.0f,
        0.0f,     y,  0.0f,
        0.0f,  0.0f,  1.0f
    };

    setTransformMatrix(m_transformMatrix * scaleMatrix.transposed());
}

void Painter::translate(float x, float y)
{
    Matrix3 translateMatrix = {
        1.0f,  0.0f,     x,
        0.0f,  1.0f,     y,
        0.0f,  0.0f,  1.0f
    };

    setTransformMatrix(m_transformMatrix * translateMatrix.transposed());
}

void Painter::rotate(float angle)
{
    Matrix3 rotationMatrix = {
        std::cos(angle), -std::sin(angle),  0.0f,
        std::sin(angle),  std::cos(angle),  0.0f,
        0.0f,             0.0f,  1.0f
    };

    setTransformMatrix(m_transformMatrix * rotationMatrix.transposed());
}

void Painter::rotate(float x, float y, float angle)
{
    translate(-x, -y);
    rotate(angle);
    translate(x, y);
}

void Painter::pushTransformMatrix()
{
    m_transformMatrixStack.push_back(m_transformMatrix);
    assert(m_transformMatrixStack.size() < 100);
}

void Painter::popTransformMatrix()
{
    assert(m_transformMatrixStack.size() > 0);
    setTransformMatrix(m_transformMatrixStack.back());
    m_transformMatrixStack.pop_back();
}

void Painter::updateGlTexture()
{
    if(m_glTextureId != 0)
        glBindTexture(GL_TEXTURE_2D, m_glTextureId);
}

void Painter::updateGlCompositionMode()
{
    switch(m_compositionMode) {
        case CompositionMode_Normal:
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
            break;
        case CompositionMode_Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case CompositionMode_Add:
            glBlendFunc(GL_ONE, GL_ONE);
            break;
        case CompositionMode_Replace:
            glBlendFunc(GL_ONE, GL_ZERO);
            break;
        case CompositionMode_DestBlending:
            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
            break;
        case CompositionMode_Light:
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            break;
        case CompositionMode_AlphaZeroing:
            glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
            break;
        case CompositionMode_AlphaRestoring:
            glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ONE);
            break;
        case CompositionMode_ZeroAlphaOverrite:
            glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE);
            break;
    }
}

void Painter::updateGlBlendEquation()
{
    if(!g_graphics.canUseBlendEquation())
        return;
    if(m_blendEquation == BlendEquation_Add)
        glBlendEquation(GL_FUNC_ADD); // GL_FUNC_ADD
    else if(m_blendEquation == BlendEquation_Max)
        glBlendEquation(0x8008); // GL_MAX
    else if(m_blendEquation == BlendEquation_Subtract)
        glBlendEquation(GL_FUNC_SUBTRACT); // GL_MAX
}

void Painter::updateDepthFunc()
{
    if (!g_graphics.canUseDepth())
        return;

    if (m_depthFunc != DepthFunc_None) {
        glEnable(GL_DEPTH_TEST);
    }

    switch (m_depthFunc) {
        case DepthFunc_None:
            glDisable(GL_DEPTH_TEST);
            break;
        case DepthFunc_ALWAYS:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_TRUE);  
            break;
        case DepthFunc_ALWAYS_READ:
            glDepthFunc(GL_ALWAYS);
            glDepthMask(GL_FALSE);  
            break;
        case DepthFunc_EQUAL:
            glDepthFunc(GL_EQUAL);
            glDepthMask(GL_TRUE);  
            break;
        case DepthFunc_LEQUAL:
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);  
            break;
        case DepthFunc_LEQUAL_READ:
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);  
            break;
        case DepthFunc_LESS:
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);  
            break;
        case DepthFunc_LESS_READ:
            glDepthFunc(GL_LESS);
            glDepthMask(GL_FALSE);  
            break;
    }
}

void Painter::updateGlClipRect()
{
    if(m_clipRect.isValid()) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(m_clipRect.left(), m_resolution.height() - m_clipRect.bottom() - 1, m_clipRect.width(), m_clipRect.height());
    } else {
        glScissor(0, 0, m_resolution.width(), m_resolution.height());
        glDisable(GL_SCISSOR_TEST);
    }
}

void Painter::updateGlAlphaWriting()
{
    if(m_alphaWriting)
        glColorMask(1,1,1,1);
    else
        glColorMask(1,1,1,0);
}

void Painter::updateGlViewport()
{
    glViewport(0, 0, m_resolution.width(), m_resolution.height());
}

void Painter::drawCoords(CoordsBuffer& coordsBuffer, DrawMode drawMode, ColorArray* colorArray)
{
    int vertexCount = coordsBuffer.getVertexCount();
    if(vertexCount == 0)
        return;

    bool textured = coordsBuffer.getTextureCoordCount() > 0 && m_texture;

    // skip drawing of empty textures
    if(textured && m_texture->isEmpty())
        return;

    // update shader with the current painter state
    m_drawProgram->bind();
    m_drawProgram->setTransformMatrix(m_transformMatrix);
    m_drawProgram->setProjectionMatrix(m_projectionMatrix);
    if(textured) {
        m_drawProgram->setTextureMatrix(m_textureMatrix);
        m_drawProgram->bindMultiTextures();
    }

    m_drawProgram->setOpacity(m_opacity);
    m_drawProgram->setDepth(m_depth);
    m_drawProgram->setColor(m_color);
    m_drawProgram->setResolution(m_resolution);
    m_drawProgram->updateTime();

    // update coords buffer hardware caches if enabled
    coordsBuffer.updateCaches();
    bool hardwareCached = coordsBuffer.isHardwareCached();

    // only set texture coords arrays when needed
    if(textured) {
        if(hardwareCached) {
            coordsBuffer.getHardwareTextureCoordArray()->bind();
            m_drawProgram->setAttributeArray(PainterShaderProgram::TEXCOORD_ATTR, nullptr, 2);
        } else {
            m_drawProgram->setAttributeArray(PainterShaderProgram::TEXCOORD_ATTR, coordsBuffer.getTextureCoordArray(), 2);
        }
    } else
        PainterShaderProgram::disableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);

    if (colorArray) {
        PainterShaderProgram::enableAttributeArray(PainterShaderProgram::COLOR_ATTR);
        m_drawProgram->setAttributeArray(PainterShaderProgram::COLOR_ATTR, colorArray->colors(), 4);
    }

    // set vertex array
    if(hardwareCached) {
        coordsBuffer.getHardwareVertexArray()->bind();
        m_drawProgram->setAttributeArray(PainterShaderProgram::VERTEX_ATTR, nullptr, 2);
        HardwareBuffer::unbind(HardwareBuffer::VertexBuffer);
    } else
        m_drawProgram->setAttributeArray(PainterShaderProgram::VERTEX_ATTR, coordsBuffer.getVertexArray(), 2);

    if(drawMode == Triangles)
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    else if(drawMode == TriangleStrip)
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
    m_draws += vertexCount / 6;
    m_calls += 1;

    if(!textured)
        PainterShaderProgram::enableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);
    if (colorArray)
        PainterShaderProgram::disableAttributeArray(PainterShaderProgram::COLOR_ATTR);
}

void Painter::drawFillCoords(CoordsBuffer& coordsBuffer)
{
    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawSolidColorProgram.get());
    setTexture(nullptr);
    drawCoords(coordsBuffer);
}

void Painter::drawTextureCoords(CoordsBuffer& coordsBuffer, const TexturePtr& texture)
{
    if(texture && texture->isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawTexturedProgram.get());
    setTexture(texture);
    drawCoords(coordsBuffer);
}

void Painter::drawTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src)
{
    if(dest.isEmpty() || src.isEmpty() || texture->isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawTexturedProgram.get());
    setTexture(texture);

    m_coordsBuffer.clear();
    m_coordsBuffer.addQuad(dest, src);
    drawCoords(m_coordsBuffer, TriangleStrip);
}

void Painter::drawColorOnTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src)     
{
    if(dest.isEmpty() || src.isEmpty() || texture->isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawSolidColorOnTextureProgram.get());
    setTexture(texture);

    m_coordsBuffer.clear();
    m_coordsBuffer.addQuad(dest, src);
    drawCoords(m_coordsBuffer, TriangleStrip);
}

void Painter::drawUpsideDownTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src)
{
    if(dest.isEmpty() || src.isEmpty() || texture->isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawTexturedProgram.get());
    setTexture(texture);

    m_coordsBuffer.clear();
    m_coordsBuffer.addUpsideDownQuad(dest, src);
    drawCoords(m_coordsBuffer, TriangleStrip);
}

void Painter::drawRepeatedTexturedRect(const Rect& dest, const TexturePtr& texture, const Rect& src)
{
    if(dest.isEmpty() || src.isEmpty() || texture->isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawTexturedProgram.get());
    setTexture(texture);

    m_coordsBuffer.clear();
    m_coordsBuffer.addRepeatedRects(dest, src);
    drawCoords(m_coordsBuffer);
}

void Painter::drawFilledRect(const Rect& dest)
{
    if(dest.isEmpty())
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawSolidColorProgram.get());

    m_coordsBuffer.clear();
    m_coordsBuffer.addRect(dest);
    drawCoords(m_coordsBuffer);
}

void Painter::drawFilledTriangle(const Point& a, const Point& b, const Point& c)
{
    if(a == b || a == c || b == c)
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawSolidColorProgram.get());

    m_coordsBuffer.clear();
    m_coordsBuffer.addTriangle(a, b, c);
    drawCoords(m_coordsBuffer);
}

void Painter::drawBoundingRect(const Rect& dest, int innerLineWidth)
{
    if(dest.isEmpty() || innerLineWidth == 0)
        return;

    setDrawProgram(m_shaderProgram ? m_shaderProgram : m_drawSolidColorProgram.get());

    m_coordsBuffer.clear();
    m_coordsBuffer.addBoudingRect(dest, innerLineWidth);
    drawCoords(m_coordsBuffer);
}

// new render
void Painter::setAtlasTextures(const TexturePtr& atlas) {
    static uint activeTexture = 0;
    if (activeTexture == atlas->getId())
        return;
    activeTexture = atlas ? atlas->getId() : 0;
    glActiveTexture(GL_TEXTURE6); // atlas
    glBindTexture(GL_TEXTURE_2D, activeTexture);
    if (atlas)
        m_atlasTextureMatrix = atlas->getTransformMatrix();
    glActiveTexture(GL_TEXTURE0);
}

inline void addRect(float* dest, const Rect& rect) {
    dest[0] = rect.left();
    dest[1] = rect.top();
    dest[2] = rect.right()+1;
    dest[3] = rect.top();
    dest[4] = rect.left();
    dest[5] = rect.bottom()+1;
    dest[6] = rect.left();
    dest[7] = rect.bottom()+1;
    dest[8] = rect.right()+1;
    dest[9] = rect.top();
    dest[10] = rect.right()+1;
    dest[11] = rect.bottom()+1;
}
inline void addRect(float* dest, const RectF& rect) {
    dest[0] = rect.left();
    dest[1] = rect.top();
    dest[2] = rect.right()+1;
    dest[3] = rect.top();
    dest[4] = rect.left();
    dest[5] = rect.bottom()+1;
    dest[6] = rect.left();
    dest[7] = rect.bottom()+1;
    dest[8] = rect.right()+1;
    dest[9] = rect.top();
    dest[10] = rect.right()+1;
    dest[11] = rect.bottom()+1;
}

void Painter::drawQueue(DrawQueue& drawqueue) {
    static std::vector<float> destCoords(8192 * 2 * 6);
    static std::vector<float> texCoords(8192 * 2 * 6);
    static std::vector<float> depthBuffer(8192 * 6);
    static std::vector<float> colorBuffer(8192 * 4 * 6);

    if (!drawqueue.getAtlas())
        return;

    setAtlasTextures(drawqueue.getAtlas());

    size_t size = 0;

    for(auto& item : drawqueue.items()) {        
        if (!item->cached)
            continue;
        addRect(&destCoords[size * 12], item->dest);
        addRect(&texCoords[size * 12], item->src);
        for (int j = 0; j < 6; ++j) {
            depthBuffer[size * 6 + j] = item->depth;
            colorBuffer[size * 24 + j * 4] = item->color.rF();
            colorBuffer[size * 24 + j * 4 + 1] = item->color.gF();
            colorBuffer[size * 24 + j * 4 + 2] = item->color.bF();
            colorBuffer[size * 24 + j * 4 + 3] = item->color.aF();
        }
        if (++size >= 8192)
            break;
    }

    // update shader with the current painter state
    m_drawNewProgram->bind();
    m_drawNewProgram->setTransformMatrix(m_transformMatrix);
    m_drawNewProgram->setProjectionMatrix(m_projectionMatrix);
    m_drawNewProgram->setTextureMatrix(m_atlasTextureMatrix);

    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::DEPTH_ATTR);
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::COLOR_ATTR);

    m_drawNewProgram->setAttributeArray(PainterShaderProgram::VERTEX_ATTR, destCoords.data(), 2);
    m_drawNewProgram->setAttributeArray(PainterShaderProgram::TEXCOORD_ATTR, texCoords.data(), 2);
    m_drawNewProgram->setAttributeArray(PainterShaderProgram::DEPTH_ATTR, depthBuffer.data(), 1);
    m_drawNewProgram->setAttributeArray(PainterShaderProgram::COLOR_ATTR, colorBuffer.data(), 4);

    glDrawArrays(GL_TRIANGLES, 0, size * 6);
    m_draws += size;
    m_calls += 1;

    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::DEPTH_ATTR);
    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::COLOR_ATTR);
}

void Painter::drawLights(const LightBuffer& buffer)
{
    if (!buffer.texture)
        return;

    setTexture(buffer.texture);

    // update shader with the current painter state
    m_drawLightProgram->bind();
    m_drawLightProgram->setTransformMatrix(m_transformMatrix);
    m_drawLightProgram->setProjectionMatrix(m_projectionMatrix);
    m_drawLightProgram->setTextureMatrix(m_textureMatrix);

    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::DEPTH_ATTR);
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::COLOR_ATTR);

    m_drawLightProgram->setAttributeArray(PainterShaderProgram::VERTEX_ATTR, buffer.destCoords, 2);
    m_drawLightProgram->setAttributeArray(PainterShaderProgram::TEXCOORD_ATTR, buffer.texCoords, 2);
    m_drawLightProgram->setAttributeArray(PainterShaderProgram::DEPTH_ATTR, buffer.depthBuffer, 1);
    m_drawLightProgram->setAttributeArray(PainterShaderProgram::COLOR_ATTR, buffer.colorBuffer, 4);

    glDrawArrays(GL_TRIANGLES, 0, buffer.size * 6);
    m_draws += buffer.size;
    m_calls += 1;

    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::DEPTH_ATTR);
    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::COLOR_ATTR);
}

void Painter::drawLightDepth(const std::map<PointF, float> depth, float tileSize)
{
    static std::vector<float> destCoords(1024 * 2 * 6);
    static std::vector<float> depthBuffer(1024 * 6);

    size_t size = std::min<size_t>(1024, depth.size());

    size_t i = 0;
    for (auto& it : depth) {
        addRect(&destCoords[i * 12], RectF(it.first, 1 + 31.f / tileSize, 1 + 31.f / tileSize));
        for (int j = 0; j < 6; ++j) {
            depthBuffer[i * 6 + j] = it.second;
        }
        if (++i >= size)
            break;
    }

    // update shader with the current painter state
    m_drawLightDepthProgram->bind();
    m_drawLightDepthProgram->setTransformMatrix(m_transformMatrix);
    m_drawLightDepthProgram->setProjectionMatrix(m_projectionMatrix);
    
    
    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::DEPTH_ATTR);

    m_drawLightDepthProgram->setAttributeArray(PainterShaderProgram::VERTEX_ATTR, destCoords.data(), 2);
    m_drawLightDepthProgram->setAttributeArray(PainterShaderProgram::DEPTH_ATTR, depthBuffer.data(), 1);

    glDrawArrays(GL_TRIANGLES, 0, size * 6);
    m_draws += size;
    m_calls += 1;

    PainterShaderProgram::disableAttributeArray(PainterShaderProgram::DEPTH_ATTR);
    PainterShaderProgram::enableAttributeArray(PainterShaderProgram::TEXCOORD_ATTR);
}
