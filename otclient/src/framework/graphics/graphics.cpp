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
    m_maxTextureSize = 2048;
}

void Graphics::init()
{
    g_logger.info(stdext::format("GPU %s", glGetString(GL_RENDERER)));
    g_logger.info(stdext::format("OpenGL %s", glGetString(GL_VERSION)));
    
    // init GL extensions
#ifndef OPENGL_ES
    float glVersion = 1.0;
    try {
        glVersion = std::atof((const char*)glGetString(GL_VERSION));
    }
    catch (std::exception) {}

    if (glVersion < 2.0) {
        g_logger.fatal(stdext::format("Your device doesn't support OpenGL >= 2.0, try to use DX version or install graphics drivers. GPU: %s OpenGL: %s (%f)",
            glGetString(GL_RENDERER) ? (const char*)glGetString(GL_RENDERER) : "-", glGetString(GL_VERSION) ? (const char*)glGetString(GL_VERSION) : "-", glVersion));
    }

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

    // blending is always enabled
    glEnable(GL_BLEND);
    
    m_alphaBits = 0;
    glGetIntegerv(GL_ALPHA_BITS, &m_alphaBits);

    int maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (m_maxTextureSize == -1 || m_maxTextureSize < maxTextureSize)
        m_maxTextureSize = maxTextureSize;

    if (Size(m_maxTextureSize, m_maxTextureSize) < g_window.getDisplaySize())
        m_cacheBackbuffer = false;

    checkDepthSupport();
    if (!m_useDepth)
        g_logger.info("Depth buffer is not supported");

    m_ok = true;

    g_painter = new Painter();
    g_painter->bind();

    g_textures.init();
    g_framebuffers.init();
}

void Graphics::terminate()
{
    g_fonts.terminate();
    g_framebuffers.terminate();
    g_textures.terminate();

    if (g_painter) {
        g_painter->unbind();
        delete g_painter;
        g_painter = nullptr;
    }

    m_ok = false;
}

bool Graphics::parseOption(const std::string& option)
{
    if (option == "-no-draw-arrays")
        m_useDrawArrays = false;
    else if (option == "-no-fbos")
        m_useFBO = false;
    else if (option == "-no-mipmaps")
        m_useMipmaps = false;
    else if (option == "-no-hardware-mipmaps")
        m_useHardwareMipmaps = false;
    else if (option == "-no-smooth")
        m_useBilinearFiltering = false;
    else if (option == "-hardware-buffers")
        m_useHardwareBuffers = true;
    else if (option == "-no-non-power-of-two-textures")
        m_useNonPowerOfTwoTextures = false;
    else if (option == "-no-clamp-to-edge")
        m_useClampToEdge = false;
    else if (option == "-no-backbuffer-cache")
        m_cacheBackbuffer = false;
    else
        return false;
    return true;
}

void Graphics::resize(const Size& size)
{
    m_viewportSize = size;
    if(g_painter)
        g_painter->setResolution(size);
}

void Graphics::checkDepthSupport() 
{
    m_useDepth = false;
    glGetError(); // clear error
    uint32 rbo = 0;
    glGenRenderbuffers(1, &rbo);
    if (!rbo)
        return;
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
    glDeleteRenderbuffers(1, &rbo);
    m_useDepth = glGetError() == GL_NO_ERROR;
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

bool Graphics::canUseDepth()
{
    return m_useDepth;
}

bool Graphics::hasScissorBug()
{
    return false;
}