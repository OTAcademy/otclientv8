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

#include "fontmanager.h"

#include "painter.h"

#include <framework/graphics/graphics.h>
#include <framework/graphics/texture.h>
#include "texturemanager.h"
#include "framebuffermanager.h"
#include <framework/platform/platformwindow.h>

Graphics g_graphics;

Graphics::Graphics()
{
    m_maxTextureSize = -1;
    m_selectedPainterEngine = Painter_Any;
}

void Graphics::init()
{
    g_logger.info(stdext::format("GPU %s", glGetString(GL_RENDERER)));
    g_logger.info(stdext::format("OpenGL %s", glGetString(GL_VERSION)));

    // init GL extensions
#ifndef OPENGL_ES
    GLenum err = glewInit();
    if(err != GLEW_OK)
        g_logger.fatal(stdext::format("Unable to init GLEW: %s", glewGetErrorString(err)));

    // overwrite framebuffer API if needed
    if(GLEW_EXT_framebuffer_object && !GLEW_ARB_framebuffer_object) {
        glGenFramebuffers = glGenFramebuffersEXT;
        glDeleteFramebuffers = glDeleteFramebuffersEXT;
        glBindFramebuffer = glBindFramebufferEXT;
        glFramebufferTexture2D = glFramebufferTexture2DEXT;
        glCheckFramebufferStatus = glCheckFramebufferStatusEXT;
        glGenerateMipmap = glGenerateMipmapEXT;
    }
#endif
    m_painter = new Painter;

    // blending is always enabled
    glEnable(GL_BLEND);
    // depth test

    // hints, 
#ifndef OPENGL_ES
    glHint(GL_FOG_HINT, GL_FASTEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
#endif

    // determine max texture size
    int maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if(m_maxTextureSize == -1 || m_maxTextureSize > maxTextureSize)
        m_maxTextureSize = maxTextureSize;

    if(Size(m_maxTextureSize,m_maxTextureSize) < g_window.getDisplaySize())
        m_cacheBackbuffer = false;

    m_alphaBits = 0;
    glGetIntegerv(GL_ALPHA_BITS, &m_alphaBits);

    m_ok = true;

    selectPainterEngine(m_prefferedPainterEngine);

    g_textures.init();
    g_framebuffers.init();
}

void Graphics::terminate()
{
    g_fonts.terminate();
    g_framebuffers.terminate();
    g_textures.terminate();

    if(m_painter) {
        delete m_painter;
        m_painter = nullptr;
    }

    g_painter = nullptr;

    m_ok = false;
}

bool Graphics::parseOption(const std::string& option)
{
    if(option == "-no-draw-arrays")
        m_useDrawArrays = false;
    else if(option == "-no-fbos")
        m_useFBO = false;
    else if(option == "-no-mipmaps")
        m_useMipmaps = false;
    else if(option == "-no-hardware-mipmaps")
        m_useHardwareMipmaps = false;
    else if(option == "-no-smooth")
        m_useBilinearFiltering = false;
    else if(option == "-hardware-buffers")
        m_useHardwareBuffers = true;
    else if(option == "-no-non-power-of-two-textures")
        m_useNonPowerOfTwoTextures = false;
    else if(option == "-no-clamp-to-edge")
        m_useClampToEdge = false;
    else if(option == "-no-backbuffer-cache")
        m_cacheBackbuffer = false;
    else if(option == "-opengl1")
        m_prefferedPainterEngine = Painter_OpenGL1;
    else if(option == "-opengl2")
        m_prefferedPainterEngine = Painter_OpenGL2;
    else
        return false;
    return true;
}

bool Graphics::isPainterEngineAvailable(Graphics::PainterEngine painterEngine)
{
    if(m_painter)
        return true;

    return false;
}

bool Graphics::selectPainterEngine(PainterEngine painterEngine)
{
    Painter *painter = nullptr;
    Painter *fallbackPainter = nullptr;
    PainterEngine fallbackPainterEngine = Painter_Any;

    if(m_painter) {
        if(!painter && (painterEngine == Painter_OpenGL2 || painterEngine == Painter_Any)) {
            m_selectedPainterEngine = Painter_OpenGL2;
            painter = m_painter;
        }
        fallbackPainter = m_painter;
        fallbackPainterEngine = Painter_OpenGL2;
    }

    if(!painter) {
        painter = fallbackPainter;
        m_selectedPainterEngine = fallbackPainterEngine;
    }

    // switch painters GL state
    if(painter) {
        if(painter != g_painter) {
            if(g_painter)
                g_painter->unbind();
            painter->bind();
            g_painter = painter;
        }

        if(painterEngine == Painter_Any)
            return true;
    } else
        g_logger.fatal("Neither OpenGL 1.0 nor OpenGL 2.0 painter engine is supported by your platform, "
                       "try updating your graphics drivers or your hardware and then run again.");

    return m_selectedPainterEngine == painterEngine;
}

void Graphics::resize(const Size& size)
{
    m_viewportSize = size;
    if(m_painter)
        m_painter->setResolution(size);
}

bool Graphics::canUseDrawArrays()
{
    return m_useDrawArrays;
}

bool Graphics::canUseShaders()
{
    return true;
}

bool Graphics::canUseFBO()
{
    return m_useFBO;
}

bool Graphics::canUseBilinearFiltering()
{
    // bilinear filtering is supported by any OpenGL implementation
    return m_useBilinearFiltering;
}

bool Graphics::canUseHardwareBuffers()
{
    return m_useHardwareBuffers;
}

bool Graphics::canUseNonPowerOfTwoTextures()
{
    return m_useNonPowerOfTwoTextures;
}

bool Graphics::canUseMipmaps()
{
    // mipmaps is supported by any OpenGL implementation
    return m_useMipmaps;
}

bool Graphics::canUseHardwareMipmaps()
{
    return m_useHardwareMipmaps;
}

bool Graphics::canUseClampToEdge()
{
    return m_useClampToEdge;
}

bool Graphics::canUseBlendFuncSeparate()
{
    return true;
}

bool Graphics::canUseBlendEquation()
{
    return true;
}

bool Graphics::canCacheBackbuffer()
{
    if(!m_alphaBits)
        return false;
    return m_cacheBackbuffer;
}

bool Graphics::hasScissorBug()
{
    return false;
}