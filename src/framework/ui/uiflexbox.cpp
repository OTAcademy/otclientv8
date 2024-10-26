#include "uiflexbox.h"
#include "uiwidget.h"
#include <framework/core/eventdispatcher.h>

UIFlexBox::UIFlexBox(UIWidgetPtr parentWidget) : UILayout(parentWidget)
{
    m_flexDirection = FlexDirection::ROW;
    m_spacing = 0;
}

void UIFlexBox::applyStyle(const OTMLNodePtr& styleNode)
{
    UILayout::applyStyle(styleNode);

    for (const OTMLNodePtr& node : styleNode->children()) {
        if (node->tag() == "direction") {
            if (node->value(true) == "row")
                setFlexDirection(FlexDirection::ROW);
            else if (node->value(true) == "column")
                setFlexDirection(FlexDirection::COLUMN);
        }
        else if (node->tag() == "spacing")
            setSpacing(node->value<int>());
    }
}

void UIFlexBox::removeWidget(const UIWidgetPtr& widget)
{
    update();
}

void UIFlexBox::addWidget(const UIWidgetPtr& widget)
{
    update();
}

bool UIFlexBox::internalUpdate()
{
    bool changed = false;
    UIWidgetPtr parentWidget = getParentWidget();
    if (!parentWidget)
        return false;

    UIWidgetList widgets = parentWidget->getChildren();
    Rect clippingRect = parentWidget->getPaddingRect();
    Point topLeft = clippingRect.topLeft();

    int availableSpace = m_flexDirection == FlexDirection::ROW ? clippingRect.width() : clippingRect.height();
    int mainAxisPos = 0, crossAxisPos = 0, lineHeight = 0;

    for (const UIWidgetPtr& widget : widgets) {
        if (!widget->isExplicitlyVisible())
            continue;

        Size childSize = widget->getSize();
        Rect destRect;

        if (m_flexDirection == FlexDirection::ROW) {
            if (mainAxisPos + childSize.width() > availableSpace) {
                mainAxisPos = 0;
                crossAxisPos += lineHeight + m_spacing;
                lineHeight = 0;
            }

            Point pos = topLeft + Point(mainAxisPos, crossAxisPos) - parentWidget->getVirtualOffset();
            mainAxisPos += childSize.width() + m_spacing;
            lineHeight = std::max(lineHeight, childSize.height());
            destRect = Rect(pos, childSize);
        }
        else if (m_flexDirection == FlexDirection::COLUMN) {
            if (mainAxisPos + childSize.height() > availableSpace) {
                mainAxisPos = 0;
                crossAxisPos += lineHeight + m_spacing;
                lineHeight = 0;
            }

            Point pos = topLeft + Point(crossAxisPos, mainAxisPos) - parentWidget->getVirtualOffset();
            mainAxisPos += childSize.height() + m_spacing;
            lineHeight = std::max(lineHeight, childSize.width());
            destRect = Rect(pos, childSize);
        }

        destRect.expand(-widget->getMarginTop(), -widget->getMarginRight(), -widget->getMarginBottom(), -widget->getMarginLeft());

        if (widget->setRect(destRect))
            changed = true;
    }

    return changed;
}
