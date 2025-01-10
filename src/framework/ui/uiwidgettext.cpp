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

#include "uiwidget.h"
#include "uitranslator.h"
#include <framework/graphics/fontmanager.h>
#include <framework/graphics/painter.h>
#include <framework/core/application.h>
#include <regex>

void UIWidget::initText()
{
    m_font = g_fonts.getDefaultFont();
    m_textAlign = Fw::AlignCenter;
    m_textOverflowLength = 0;
    m_textOverflowCharacter = "...";
}

void UIWidget::updateText()
{
    if ((hasEventListener(EVENT_TEXT_CLICK) || hasEventListener(EVENT_TEXT_HOVER)) && m_textEvents.empty())
        processCodeTags();

    if (m_textWrap && m_rect.isValid()) {
        m_drawTextColors = m_textColors;

        if (m_textOverflowLength > 0 && m_text.length() > m_textOverflowLength)
            m_drawText = m_font->wrapText(m_text.substr(0, m_textOverflowLength - m_textOverflowCharacter.length()) + m_textOverflowCharacter, getWidth() - m_textOffset.x, &m_drawTextColors);
        else
            m_drawText = m_font->wrapText(m_text, getWidth() - m_textOffset.x, &m_drawTextColors);
    } else {
        if (m_textOverflowLength > 0 && m_text.length() > m_textOverflowLength)
            m_drawText = m_text.substr(0, m_textOverflowLength - m_textOverflowCharacter.length()) + m_textOverflowCharacter;
        else
            m_drawText = m_text;

        m_drawTextColors = m_textColors;
    }

    // update rect size
    if(!m_rect.isValid() || m_textHorizontalAutoResize || m_textVerticalAutoResize) {
        Size textBoxSize = getTextSize();
        textBoxSize += Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom) + m_textOffset.toSize();
        Size size = getSize();
        if(size.width() <= 0 || (m_textHorizontalAutoResize && !m_textWrap))
            size.setWidth(textBoxSize.width());
        if(size.height() <= 0 || m_textVerticalAutoResize)
            size.setHeight(textBoxSize.height());
        setSize(size);
    }

    m_textMustRecache = true;
}

void UIWidget::parseTextStyle(const OTMLNodePtr& styleNode)
{
    for(const OTMLNodePtr& node : styleNode->children()) {
        if(node->tag() == "text")
            setText(node->value());
        else if(node->tag() == "text-align")
            setTextAlign(Fw::translateAlignment(node->value()));
        else if(node->tag() == "text-offset")
            setTextOffset(node->value<Point>());
        else if(node->tag() == "text-wrap")
            setTextWrap(node->value<bool>());
        else if(node->tag() == "text-auto-resize")
            setTextAutoResize(node->value<bool>());
        else if(node->tag() == "text-horizontal-auto-resize")
            setTextHorizontalAutoResize(node->value<bool>());
        else if(node->tag() == "text-vertical-auto-resize")
            setTextVerticalAutoResize(node->value<bool>());
        else if(node->tag() == "text-only-upper-case")
            setTextOnlyUpperCase(node->value<bool>());
        else if(node->tag() == "font")
            setFont(node->value());
        else if (node->tag() == "shadow")
            setShadow(node->value<bool>());
        else if (node->tag() == "text-overflow") {
            auto split = stdext::split(node->value(true), " ");
            if (split.size() == 2) {
                setTextOverflowLength(stdext::safe_cast<uint16>(g_ui.getOTUIVarSafe(split[0])));
                setTextOverflowCharacter(g_ui.getOTUIVarSafe(split[1]));
            }
            else
                throw OTMLException(node, "text-overflow param must have its length followed by its character (eg. \"13 ...\")");
        }
        else if (node->tag() == "text-overflow-length")
            setTextOverflowLength(node->value<uint16>());
        else if (node->tag() == "text-overflow-character")
            setTextOverflowCharacter(node->value<>());
    }
}

void UIWidget::drawText(const Rect& screenCoords)
{
    if(m_drawText.length() == 0 || m_color.aF() == 0.0f)
        return;

    if(screenCoords != m_textCachedScreenCoords || m_textMustRecache) {
        Rect coords = Rect(screenCoords.topLeft() + m_textOffset, screenCoords.bottomRight());
        m_textMustRecache = false;
        m_textCachedScreenCoords = coords;

        if (hasEventListener(EVENT_TEXT_CLICK) || hasEventListener(EVENT_TEXT_HOVER))
            cacheRectToWord();
    }

    if (!m_drawTextColors.empty()) {
        m_font->drawColoredText(m_drawText, m_textCachedScreenCoords, m_textAlign, m_drawTextColors, m_shadow);
    } else {
        m_font->drawText(m_drawText, m_textCachedScreenCoords, m_textAlign, m_color, m_shadow);
    }

    if (m_textUnderline.getVertexCount() > 0)
        g_drawQueue->addFillCoords(m_textUnderline, m_color);
}

void UIWidget::onTextChange(const std::string& text, const std::string& oldText)
{
    callLuaField("onTextChange", text, oldText);
}

void UIWidget::onFontChange(const std::string& font)
{
    callLuaField("onFontChange", font);
}

void UIWidget::setText(std::string text, bool dontFireLuaCall)
{
    if(m_textOnlyUpperCase)
        stdext::toupper(text);

    m_textColors.clear();
    m_drawTextColors.clear();
    m_textEvents.clear();

    if(m_text == text)
        return;

    std::string oldText = m_text;
    m_text = text;
    updateText();

    if(!dontFireLuaCall) {
        onTextChange(text, oldText);
    }
}

void UIWidget::setColoredText(const std::vector<std::string>& texts, bool dontFireLuaCall)
{
    m_textColors.clear();
    m_drawTextColors.clear();
    m_textEvents.clear();

    std::string text = "";
    for(size_t i = 0, p = 0; i < texts.size() - 1; i += 2) {
        Color c(Color::white);
        stdext::cast<Color>(texts[i + 1], c);
        text += texts[i];

        std::regex regex(R"(\[text-event\](.*?)\[/text-event\])");
        std::smatch match;
        std::regex_search(texts[i], match, regex);

        for (auto& c : texts[i]) {
            if ((uint8)c >= 32)
                p += 1;
        }

        if (match.size() > 0) {
            p -= 25;
        }

        m_textColors.push_back(std::make_pair(p, c));
    }

    if (m_textOnlyUpperCase)
        stdext::toupper(text);

    std::string oldText = m_text;
    m_text = text;
    updateText();

    if (!dontFireLuaCall) {
        onTextChange(text, oldText);
    }
}

void UIWidget::setFont(const std::string& fontName)
{
    m_font = g_fonts.getFont(fontName);
    updateText();
    onFontChange(fontName);
}

void UIWidget::processCodeTags() {
    std::string tempText = m_text;
    m_text.clear();
    m_textEvents.clear();

    std::regex regex(R"(\[text-event\](.*?)\[/text-event\])");
    std::smatch match;

    while (std::regex_search(tempText, match, regex)) {
        m_text += tempText.substr(0, match.position());

        std::string word = match[1];
        size_t startPos = m_text.length();
        size_t endPos = startPos + word.length();

        m_textEvents.push_back({ word, startPos, endPos });
        m_text += word;
        tempText = tempText.substr(match.position() + match.length());
    }
    
    m_text += tempText;
}

void UIWidget::buildTextUnderline(Rect& wordRect, CoordsBuffer& textUnderlineCoords) {
    static const int dotSize = 2;
    static const int dotSpacing = 2;

    int currentX = wordRect.x();
    int y = wordRect.y() + wordRect.height() - m_font->getUnderlineOffset();
    while (currentX < wordRect.x() + wordRect.width()) {
        textUnderlineCoords.addRect(Rect(currentX, y, dotSize, dotSize));
        currentX += dotSize + dotSpacing;
    }

    int underLineWidth = currentX - wordRect.x() - dotSpacing;
    if (underLineWidth < wordRect.width()) {
        textUnderlineCoords.addRect(Rect(currentX, y, std::min<int>(dotSize, wordRect.width() - underLineWidth), dotSize));
    }
}

void UIWidget::updateRectToWord(const std::vector<Rect>& glyphCoords)
{
    m_rectToWord.clear();
    m_textUnderline.clear();

    Rect wordRect;
    bool inNewLine = false;

    for (const auto& textEvent : m_textEvents) {
        for (size_t i = textEvent.startPos; i < textEvent.endPos; ++i) {
            if (m_drawText[i] == '\n') {
                m_rectToWord.push_back({ wordRect, textEvent.word });
                buildTextUnderline(wordRect, m_textUnderline);

                inNewLine = true;
                continue;
            }

            if (i == textEvent.startPos || inNewLine) {
                wordRect = glyphCoords[i];
                inNewLine = false;
            }
            else
                wordRect.expand(0, glyphCoords[i].width(), 0, 0);
        }

        m_rectToWord.push_back({ wordRect, textEvent.word });
        buildTextUnderline(wordRect, m_textUnderline);
    }
}
