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

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "declarations.h"
#include <framework/core/inputevent.h>
#include <framework/otml/declarations.h>

class UIWidget;

//@bindsingleton g_ui
class UIManager
{
public:
    void init();
    void terminate();

    void render(Fw::DrawPane drawPane);
    void resize(const Size& size);
    void inputEvent(const InputEvent& event);

    void updatePressedWidget(const Fw::MouseButton button, const UIWidgetPtr& newPressedWidget, const Point& clickedPos = Point(), bool fireClicks = true);
    bool updateDraggingWidget(const UIWidgetPtr& draggingWidget, const Point& clickedPos = Point());
    void updateHoveredWidget(bool now = false);
    void updateHoveredText(bool now = false);

    void clearStyles();
    bool importStyle(std::string file);
    bool importStyleFromString(std::string data);
    void importStyleFromOTML(const OTMLNodePtr& styleNode);
    void mergeStyle(std::string file, const UIWidgetPtr& widget);
    OTMLNodePtr getStyle(const std::string& styleName);
    std::string getStyleClass(const std::string& styleName);

    UIWidgetPtr loadUIFromString(const std::string& data, const UIWidgetPtr& parent);
    UIWidgetPtr loadUI(std::string file, const UIWidgetPtr& parent);
    UIWidgetPtr displayUI(const std::string& file) { return loadUI(file, m_rootWidget); }
    UIWidgetPtr createWidget(const std::string& styleName, const UIWidgetPtr& parent);
    UIWidgetPtr createWidgetFromOTML(const OTMLNodePtr& widgetNode, const UIWidgetPtr& parent);

    void setMouseReceiver(const UIWidgetPtr& widget) { m_mouseReceiver = widget; }
    void setKeyboardReceiver(const UIWidgetPtr& widget) { m_keyboardReceiver = widget; }
    void setDebugBoxesDrawing(bool enabled) { m_drawDebugBoxes = enabled; }
    void resetMouseReceiver() { m_mouseReceiver = m_rootWidget; }
    void resetKeyboardReceiver() { m_keyboardReceiver = m_rootWidget; }
    UIWidgetPtr getMouseReceiver() { return m_mouseReceiver; }
    UIWidgetPtr getKeyboardReceiver() { return m_keyboardReceiver; }
    UIWidgetPtr getDraggingWidget() { return m_draggingWidget; }
    UIWidgetPtr getHoveredWidget() { return m_hoveredWidget; }
    UIWidgetPtr getPressedWidget() { return m_pressedWidget[Fw::MouseLeftButton]; }
    UIWidgetPtr getRootWidget() { return m_rootWidget; }
    bool isMouseGrabbed() { return m_mouseReceiver != m_rootWidget; }
    bool isKeyboardGrabbed() { return m_keyboardReceiver != m_rootWidget; }

    bool isDrawingDebugBoxes() { return m_drawDebugBoxes; }

    const OTUIVars& getOTUIVars() {
        return m_vars;
    }
    bool hasOTUIVar(const std::string& key) {
        if (stdext::starts_with(key, "$var-")) {
            return m_vars.find(key.substr(5)) != m_vars.end();
        }
        return m_vars.find(key) != m_vars.end();
    }
    std::string getOTUIVar(const std::string& key) {
        if (stdext::starts_with(key, "$var-")) {
            return m_vars[key.substr(5)];
        }
        return m_vars[key];
    }
    std::string getOTUIVarSafe(const std::string& key) {
        if (!hasOTUIVar(key)) {
            return key;
        }
        if (stdext::starts_with(key, "$var-")) {
            return m_vars[key.substr(5)];
        }
        return m_vars[key];
    }
    void addOTUIVar(const std::string& key, const std::string& value) {
        m_vars.insert_or_assign(key, value);
    }

protected:
    void onWidgetAppear(const UIWidgetPtr& widget);
    void onWidgetDisappear(const UIWidgetPtr& widget);
    void onWidgetDestroy(const UIWidgetPtr& widget);

    friend class UIWidget;

private:
    UIWidgetPtr m_rootWidget;
    UIWidgetPtr m_mouseReceiver;
    UIWidgetPtr m_keyboardReceiver;
    UIWidgetPtr m_draggingWidget;
    UIWidgetPtr m_hoveredWidget;
    std::string m_hoveredText;
    UIWidgetPtr m_pressedWidget[Fw::MouseButtonLast + 1];
    stdext::boolean<false> m_hoverUpdateScheduled;
    stdext::boolean<false> m_hoverTextUpdateScheduled;
    stdext::boolean<false> m_drawDebugBoxes;
    std::unordered_map<std::string, OTMLNodePtr> m_styles;
    OTUIVars m_vars;
    UIWidgetList m_destroyedWidgets;
    ScheduledEventPtr m_checkEvent;
    stdext::timer m_moveTimer;
};

extern UIManager g_ui;

#endif
