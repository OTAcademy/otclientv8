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
        else if (node->tag() == "auto-spacing")
            m_autoSpacing = node->value<bool>();
        else if (node->tag() == "align-items") {
            if (node->value(true) == "start")
                m_alignItems = AlignItems::START;
            else if (node->value(true) == "stretch")
                m_alignItems = AlignItems::STRETCH;
            else if (node->value(true) == "center")
                m_alignItems = AlignItems::CENTER;
        }
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
    availableSpace += std::max<int>(0, parentWidget->getChildCount() - 1) * m_spacing;
    int totalWidgetSize = 0;
    int visibleWidgetCount = 0;

    for (const UIWidgetPtr& widget : widgets) {
        if (!widget->isExplicitlyVisible())
            continue;

        if (widget->isSizePercantage()) {
            widget->updatePercentSize(clippingRect.size());
        }

        totalWidgetSize += (m_flexDirection == FlexDirection::ROW) ? widget->getWidth() : widget->getHeight();
        visibleWidgetCount++;
    }

    int spacing = m_spacing;
    if (m_autoSpacing && visibleWidgetCount > 1) {
        int totalSpacing = availableSpace - totalWidgetSize;
        spacing = std::max(totalSpacing / (visibleWidgetCount - 1), 0);
    }

    int mainAxisPos = 0, crossAxisPos = 0, lineHeight = 0;

    for (const UIWidgetPtr& widget : widgets) {
        if (!widget->isExplicitlyVisible())
            continue;

        Size childSize = widget->getSize();
        Rect destRect;

        if (m_flexDirection == FlexDirection::ROW) {
            if (mainAxisPos + childSize.width() > availableSpace) {
                mainAxisPos = 0;
                crossAxisPos += lineHeight + spacing;
                lineHeight = 0;
            }

            Point pos = topLeft + Point(mainAxisPos, crossAxisPos) - parentWidget->getVirtualOffset();
            mainAxisPos += childSize.width() + spacing;
            lineHeight = std::max(lineHeight, childSize.height());

            if (m_alignItems == AlignItems::STRETCH)
                childSize.setHeight(clippingRect.height());
            else if (m_alignItems == AlignItems::CENTER)
                pos.y = pos.y + (clippingRect.height() - childSize.height()) / 2;

            destRect = Rect(pos, childSize);
        }
        else if (m_flexDirection == FlexDirection::COLUMN) {
            if (mainAxisPos + childSize.height() > availableSpace) {
                mainAxisPos = 0;
                crossAxisPos += lineHeight + spacing;
                lineHeight = 0;
            }

            Point pos = topLeft + Point(crossAxisPos, mainAxisPos) - parentWidget->getVirtualOffset();
            mainAxisPos += childSize.height() + spacing;
            lineHeight = std::max(lineHeight, childSize.width());

            if (m_alignItems == AlignItems::STRETCH)
                childSize.setWidth(clippingRect.width());
            else if (m_alignItems == AlignItems::CENTER)
                pos.x = pos.x + (clippingRect.width() - childSize.width()) / 2;

            destRect = Rect(pos, childSize);
        }

        destRect.expand(-widget->getMarginTop(), -widget->getMarginRight(), -widget->getMarginBottom(), -widget->getMarginLeft());

        if (widget->setRect(destRect))
            changed = true;
    }

    return changed;
}