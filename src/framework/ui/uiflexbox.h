#ifndef UIFLEXBOX_H
#define UIFLEXBOX_H

#include "uilayout.h"

enum class FlexDirection {
    ROW,
    COLUMN
};

enum class AlignItems {
    START,
    STRETCH,
    CENTER
};

// @bindclass
class UIFlexBox : public UILayout
{
public:
    UIFlexBox(UIWidgetPtr parentWidget);

    void applyStyle(const OTMLNodePtr& styleNode);
    void removeWidget(const UIWidgetPtr& widget);
    void addWidget(const UIWidgetPtr& widget);

    void setFlexDirection(FlexDirection direction) { m_flexDirection = direction; update(); }
    void setAlignItems(AlignItems align) { m_alignItems = align; update(); }
    void setSpacing(int spacing) { m_spacing = spacing; update(); }
    void setAutoSpacing(bool spacing) { m_autoSpacing = spacing; update(); }

    FlexDirection getFlexDirection() { return m_flexDirection; }
    AlignItems getAlignItems() { return m_alignItems; }
    int getSpacing() { return m_spacing; }
    bool getAutoSpacing() { return m_autoSpacing; }

    virtual bool isUIFlexBox() { return true; }

protected:
    bool internalUpdate();

private:
    FlexDirection m_flexDirection;
    AlignItems m_alignItems;
    int m_spacing;
    stdext::boolean<false> m_autoSpacing;
};

#endif
