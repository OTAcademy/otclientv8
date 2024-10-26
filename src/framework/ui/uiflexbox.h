#ifndef UIFLEXBOX_H
#define UIFLEXBOX_H

#include "uilayout.h"

enum class FlexDirection {
    ROW,
    COLUMN
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
    void setSpacing(int spacing) { m_spacing = spacing; update(); }

    FlexDirection getFlexDirection() { return m_flexDirection; }
    int getSpacing() { return m_spacing; }

    virtual bool isUIFlexBox() { return true; }

protected:
    bool internalUpdate();

private:
    FlexDirection m_flexDirection;
    int m_spacing;
};

#endif
