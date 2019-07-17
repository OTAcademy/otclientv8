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

#include "framebuffer.h"
#include "graphics.h"
#include "texture.h"

#include <framework/platform/platformwindow.h>
#include <framework/core/application.h>

uint FrameBuffer::boundFbo = 0;

FrameBuffer::FrameBuffer(bool withDepth)
{
    m_depth = withDepth && g_graphics.canUseDepth();
    internalCreate();
}

void FrameBuffer::internalCreate()
{
    m_prevBoundFbo = 0;
    m_fbo = 0;
    if (g_graphics.canUseFBO()) {
        glGenFramebuffers(1, &m_fbo);
        if(!m_fbo)
            g_logger.fatal("Unable to create framebuffer object");
        if (m_depth) {
            glGenRenderbuffers(1, &m_depthRbo);
            if(!m_depthRbo)
                g_logger.fatal("Unable to create renderbuffer object");
        }
    }
}

FrameBuffer::~FrameBuffer()
{
#ifndef NDEBUG
    assert(!g_app.isTerminated());
#endif
    if (g_graphics.ok() && m_fbo != 0) {
        if(m_fbo != 0)
            glDeleteFramebuffers(1, &m_fbo);
        if(m_depthRbo != 0)
            glDeleteRenderbuffers(1, &m_depthRbo);
    }
}

void FrameBuffer::resize(const Size& size)
{
    assert(size.isValid());

    if(m_texture && m_texture->getSize() == size)
        return;

    m_texture = TexturePtr(new Texture(size));
    m_texture->setSmooth(m_smooth);
    m_texture->setUpsideDown(true);

    if(m_depth) {
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size.width(), size.height());
    }

    if(m_fbo) {
        internalBind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture->getId(), 0);
        if (m_depth) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE)
            g_logger.fatal(stdext::format("Unable to setup framebuffer object - %i - %ix%i %s", status, size.width(), size.height(), m_depth ? "(depth)" : ""));
        internalRelease();
    } else {
        if(m_backuping) {
            m_screenBackup = TexturePtr(new Texture(size));
            m_screenBackup->setUpsideDown(true);
        }
    }
}

void FrameBuffer::bind(const FrameBufferPtr& depthFramebuffer)
{
    g_painter->saveAndResetState();
    internalBind();
    g_painter->setResolution(m_texture->getSize());
    if (!m_depth && depthFramebuffer && depthFramebuffer->hasDepth()) {
        m_depthRbo = depthFramebuffer->getDepthRenderBuffer();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);
    }
}

void FrameBuffer::release()
{
    if (!m_depth && m_depthRbo) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        m_depthRbo = 0;
    }
    internalRelease();
    g_painter->restoreSavedState();
}

void FrameBuffer::draw()
{
    Rect rect(0,0, getSize());
    g_painter->drawTexturedRect(rect, m_texture, rect);
}

void FrameBuffer::draw(const Rect& dest, const Rect& src)
{
    g_painter->drawTexturedRect(dest, m_texture, src);
}

void FrameBuffer::draw(const Rect& dest)
{
    g_painter->drawTexturedRect(dest, m_texture, Rect(0,0, getSize()));
}

void FrameBuffer::internalBind()
{
    if(m_fbo) {
        assert(boundFbo != m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        m_prevBoundFbo = boundFbo;
        boundFbo = m_fbo;
    } else if(m_backuping) {
        // backup screen color buffer into a texture
        m_screenBackup->copyFromScreen(Rect(0, 0, getSize()));
    }
}

void FrameBuffer::internalRelease()
{
    if(m_fbo) {
        assert(boundFbo == m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_prevBoundFbo);
        boundFbo = m_prevBoundFbo;
    } else {
        Rect screenRect(0, 0, getSize());

        // copy the drawn color buffer into the framebuffer texture
        m_texture->copyFromScreen(screenRect);

        // restore screen original content
        if(m_backuping) {
            glDisable(GL_BLEND);
            g_painter->setColor(Color::white);
            g_painter->drawTexturedRect(screenRect, m_screenBackup, screenRect);
            glEnable(GL_BLEND);
        }
    }
}

Size FrameBuffer::getSize()
{
    if(m_fbo == 0) {
        // the buffer size is limited by the window size
        return Size(std::min<int>(m_texture->getWidth(), g_window.getWidth()),
                    std::min<int>(m_texture->getHeight(), g_window.getHeight()));
    }
    return m_texture->getSize();
}