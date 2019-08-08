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


#include "graphicalapplication.h"
#include <framework/core/adaptiverenderer.h>
#include <framework/core/clock.h>
#include <framework/core/eventdispatcher.h>
#include <framework/platform/platformwindow.h>
#include <framework/ui/uimanager.h>
#include <framework/graphics/graphics.h>
#include <framework/graphics/particlemanager.h>
#include <framework/graphics/texturemanager.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/graphics/atlas.h>
#include <framework/input/mouse.h>
#include <framework/util/extras.h>
#include <framework/util/stats.h>

#ifdef FW_SOUND
#include <framework/sound/soundmanager.h>
#endif

GraphicalApplication g_app;

void GraphicalApplication::init(std::vector<std::string>& args)
{
    Application::init(args);

    // setup platform window
    g_window.init();
    g_window.hide();
    g_window.setOnResize(std::bind(&GraphicalApplication::resize, this, std::placeholders::_1));
    g_window.setOnInputEvent(std::bind(&GraphicalApplication::inputEvent, this, std::placeholders::_1));
    g_window.setOnClose(std::bind(&GraphicalApplication::close, this));

    g_mouse.init();

    // initialize ui
    g_ui.init();

    // initialize graphics
    g_graphics.init();

    // fire first resize event
    resize(g_window.getSize());

#ifdef FW_SOUND
    // initialize sound
    g_sounds.init();
#endif
}

void GraphicalApplication::deinit()
{
    // hide the window because there is no render anymore
    g_window.hide();

    Application::deinit();
}

void GraphicalApplication::terminate()
{
    // destroy particles
    g_particles.terminate();

    // destroy any remaining widget
    g_ui.terminate();

    Application::terminate();
    m_terminated = false;

#ifdef FW_SOUND
    // terminate sound
    g_sounds.terminate();
#endif

    g_mouse.terminate();

    // terminate graphics
    g_graphics.terminate();
    g_window.terminate();

    g_atlas.terminate();


    m_terminated = true;
}

void GraphicalApplication::run()
{
    m_running = true;

    // first clock update
    g_clock.update();

    // run the first poll
    poll();
    g_clock.update();

    // show window
    g_window.show();

    // run the second poll
    poll();
    g_clock.update();

    g_atlas.init();

    g_lua.callGlobalField("g_app", "onRun");

    auto foregoundBuffer = g_framebuffers.createFrameBuffer();
    foregoundBuffer->resize(g_painter->getResolution());

    auto lastRender = stdext::micros();
    auto lastForegroundRender = stdext::millis();

    while(!m_stopping) {
        m_iteration += 1;

        // poll all events before rendering
        g_clock.update();

        poll();

        g_clock.update();

        if (!g_window.isVisible()) {
            stdext::millisleep(1);
            g_adaptiveRenderer.refresh();
            continue;
        }

        int frameDelay = getMaxFps() <= 0 ? 0 : (1000000 / getMaxFps()) - 2000;

        if (lastRender + frameDelay > stdext::micros()) {
            stdext::millisleep(1);
            continue;
        }
        lastRender = stdext::micros();

        g_adaptiveRenderer.newFrame();
        bool updateForeground = false;
        
        if(lastForegroundRender + g_adaptiveRenderer.foregroundUpdateInterval() <= stdext::millis()) {
            lastForegroundRender = stdext::millis();
            m_mustRepaint = false;
            updateForeground = true;
        }


        if(updateForeground) {
            AutoStat s(STATS_MAIN, "RenderForeground");
            foregoundBuffer->resize(g_painter->getResolution());
            foregoundBuffer->bind();
            g_painter->setAlphaWriting(true);
            g_painter->clear(Color::alpha);
            g_ui.render(Fw::ForegroundPane);
            foregoundBuffer->release();
        }

        g_painter->clear(Color::black);
        g_painter->setAlphaWriting(false);

        {
            AutoStat s(STATS_MAIN, "RenderBackground");
            g_ui.render(Fw::BackgroundPane);
        }

        g_painter->setColor(Color::white);
        g_painter->setOpacity(1.0);
        foregoundBuffer->draw(Rect(0, 0, g_painter->getResolution()));
        

        {
            AutoStat s(STATS_MAIN, "SwapBuffers");
            g_window.swapBuffers();
        }
    }

    m_stopping = false;
    m_running = false;
}

void GraphicalApplication::poll() {
#ifdef FW_SOUND
    g_sounds.poll();
#endif

    // poll window input events
    g_window.poll();
    g_particles.poll();
    g_textures.poll();

    Application::poll();
}

void GraphicalApplication::close()
{
    m_onInputEvent = true;
    Application::close();
    m_onInputEvent = false;
}

void GraphicalApplication::resize(const Size& size)
{
    m_onInputEvent = true;
    g_graphics.resize(size);
    g_ui.resize(size);
    m_onInputEvent = false;

    m_mustRepaint = true;
}

void GraphicalApplication::inputEvent(const InputEvent& event)
{
    m_onInputEvent = true;
    g_ui.inputEvent(event);
    m_onInputEvent = false;
}
