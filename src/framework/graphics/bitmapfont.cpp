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

#include "atlas.h"
#include "bitmapfont.h"
#include "texturemanager.h"
#include "graphics.h"
#include "image.h"

#include <framework/core/eventdispatcher.h>
#include <framework/otml/otml.h>
#include <framework/util/extras.h>

void BitmapFont::load(const OTMLNodePtr& fontNode)
{
    OTMLNodePtr textureNode = fontNode->at("texture");
    std::string textureFile = stdext::resolve_path(textureNode->value(), textureNode->source());
    Size glyphSize = fontNode->valueAt<Size>("glyph-size");
    m_glyphHeight = fontNode->valueAt<int>("height");
    m_yOffset = fontNode->valueAt("y-offset", 0);
    m_firstGlyph = fontNode->valueAt("first-glyph", 32);
    m_glyphSpacing = fontNode->valueAt("spacing", Size(0,0));
    int spaceWidth = fontNode->valueAt("space-width", glyphSize.width());

    if(OTMLNodePtr node = fontNode->get("fixed-glyph-width")) {
        for(int glyph = m_firstGlyph; glyph < 256; ++glyph)
            m_glyphsSize[glyph] = Size(node->value<int>(), m_glyphHeight);
    } else {
        calculateGlyphsWidthsAutomatically(Image::load(textureFile), glyphSize);
    }

    // 32 is space
    m_glyphsSize[32].setWidth(spaceWidth);

    // use 127 as spacer [Width: 1], Important for the current NPC highlighting system
    m_glyphsSize[127].setWidth(1);

    // new line actually has a size that will be useful in multiline algorithm
    m_glyphsSize[(uchar)'\n'] = Size(1, m_glyphHeight);

    // read custom widths
    /*
    if(OTMLNodePtr node = fontNode->get("glyph-widths")) {
        for(const OTMLNodePtr& child : node->children())
            m_glyphsSize[stdext::safe_cast<int>(child->tag())].setWidth(child->value<int>());
    }
    */

    // load font texture
    m_texture = g_textures.getTexture(textureFile);
    if (!m_texture)
        return;
    m_texture->update();

    int numHorizontalGlyphs = m_texture->getSize().width() / glyphSize.width();
#ifdef DONT_CACHE_FONTS
    Point offset(0, 0);
#else
    Point offset = g_atlas.cacheFont(m_texture);
    m_texture = g_atlas.get(1);
#endif
    for (int glyph = m_firstGlyph; glyph < 256; ++glyph) {
        m_glyphsTextureCoords[glyph].setRect(((glyph - m_firstGlyph) % numHorizontalGlyphs) * glyphSize.width() + offset.x,
                                                ((glyph - m_firstGlyph) / numHorizontalGlyphs) * glyphSize.height() + offset.y,
                                                m_glyphsSize[glyph].width(),
                                                m_glyphHeight);
    }
}

void BitmapFont::drawText(const std::string& text, const Point& startPos, const Color& color, bool shadow)
{
    Size boxSize = g_painter->getResolution() - startPos.toSize();
    Rect screenCoords(startPos, boxSize);
    drawText(text, screenCoords, Fw::AlignTopLeft, shadow);
}

void BitmapFont::drawText(const std::string& text, const Rect& screenCoords, Fw::AlignmentFlag align, const Color& color, bool shadow)
{
    g_drawQueue->addText(shared_from_this(), text, screenCoords, align, color, shadow);
}

void BitmapFont::drawColoredText(const std::string& text, const Rect& screenCoords, Fw::AlignmentFlag align, const std::vector<std::pair<int, Color>>& colors, bool shadow)
{
    g_drawQueue->addColoredText(shared_from_this(), text, screenCoords, align, colors, shadow);
}

void BitmapFont::calculateDrawTextCoords(CoordsBuffer& coordsBuffer, const std::string& text, const Rect& screenCoords, Fw::AlignmentFlag align)
{
    // prevent glitches from invalid rects
    if (!screenCoords.isValid() || !m_texture)
        return;

    int textLenght = text.length();

    // map glyphs positions
    Size textBoxSize;
    const std::vector<Point>& glyphsPositions = calculateGlyphsPositions(text, align, &textBoxSize);

    for (int i = 0; i < textLenght; ++i) {
        int glyph = (uchar)text[i];

        // skip invalid glyphs
        if (glyph < 32)
            continue;

        // calculate initial glyph rect and texture coords
        Rect glyphScreenCoords(glyphsPositions[i], m_glyphsSize[glyph]);
        Rect glyphTextureCoords = m_glyphsTextureCoords[glyph];

        // first translate to align position
        if (align & Fw::AlignBottom) {
            glyphScreenCoords.translate(0, screenCoords.height() - textBoxSize.height());
        } else if (align & Fw::AlignVerticalCenter) {
            glyphScreenCoords.translate(0, (screenCoords.height() - textBoxSize.height()) / 2);
        } else { // AlignTop
            // nothing to do
        }

        if (align & Fw::AlignRight) {
            glyphScreenCoords.translate(screenCoords.width() - textBoxSize.width(), 0);
        } else if (align & Fw::AlignHorizontalCenter) {
            glyphScreenCoords.translate((screenCoords.width() - textBoxSize.width()) / 2, 0);
        } else { // AlignLeft
            // nothing to do
        }

        // only render glyphs that are after 0, 0
        if (glyphScreenCoords.bottom() < 0 || glyphScreenCoords.right() < 0)
            continue;

        // bound glyph topLeft to 0,0 if needed
        if (glyphScreenCoords.top() < 0) {
            glyphTextureCoords.setTop(glyphTextureCoords.top() - glyphScreenCoords.top());
            glyphScreenCoords.setTop(0);
        }
        if (glyphScreenCoords.left() < 0) {
            glyphTextureCoords.setLeft(glyphTextureCoords.left() - glyphScreenCoords.left());
            glyphScreenCoords.setLeft(0);
        }

        // translate rect to screen coords
        glyphScreenCoords.translate(screenCoords.topLeft());

        // only render if glyph rect is visible on screenCoords
        if (!screenCoords.intersects(glyphScreenCoords))
            continue;

        // bound glyph bottomRight to screenCoords bottomRight
        if (glyphScreenCoords.bottom() > screenCoords.bottom()) {
            glyphTextureCoords.setBottom(glyphTextureCoords.bottom() + (screenCoords.bottom() - glyphScreenCoords.bottom()));
            glyphScreenCoords.setBottom(screenCoords.bottom());
        }
        if (glyphScreenCoords.right() > screenCoords.right()) {
            glyphTextureCoords.setRight(glyphTextureCoords.right() + (screenCoords.right() - glyphScreenCoords.right()));
            glyphScreenCoords.setRight(screenCoords.right());
        }

        // render glyph
        coordsBuffer.addRect(glyphScreenCoords, glyphTextureCoords);
    }
}

const std::vector<Point>& BitmapFont::calculateGlyphsPositions(const std::string& text,
                                                         Fw::AlignmentFlag align,
                                                         Size *textBoxSize)
{
    // for performance reasons we use statics vectors that are allocated on demand
    static thread_local std::vector<Point> glyphsPositions(1);
    static thread_local std::vector<int> lineWidths(1);

    int textLength = text.length();
    int maxLineWidth = 0;
    int lines = 0;
    int glyph;
    int i;

    // return if there is no text
    if(textLength == 0) {
        if(textBoxSize)
            textBoxSize->resize(0,m_glyphHeight);
        return glyphsPositions;
    }

    // resize glyphsPositions vector when needed
    if(textLength > (int)glyphsPositions.size())
        glyphsPositions.resize(textLength);

    // calculate lines width
    if((align & Fw::AlignRight || align & Fw::AlignHorizontalCenter) || textBoxSize) {
        lineWidths[0] = 0;
        for(i = 0; i< textLength; ++i) {
            glyph = (uchar)text[i];

            if(glyph == (uchar)'\n') {
                lines++;
                if(lines+1 > (int)lineWidths.size())
                    lineWidths.resize(lines+1);
                lineWidths[lines] = 0;
            } else if(glyph >= 32) {
                lineWidths[lines] += m_glyphsSize[glyph].width() ;
                if((i+1 != textLength && text[i+1] != '\n')) // only add space if letter is not the last or before a \n.
                    lineWidths[lines] += m_glyphSpacing.width();
                maxLineWidth = std::max<int>(maxLineWidth, lineWidths[lines]);
            }
        }
    }

    Point virtualPos(0, m_yOffset);
    lines = 0;
    for(i = 0; i < textLength; ++i) {
        glyph = (uchar)text[i];

        // new line or first glyph
        if(glyph == (uchar)'\n' || i == 0) {
            if(glyph == (uchar)'\n') {
                virtualPos.y += m_glyphHeight + m_glyphSpacing.height();
                lines++;
            }

            // calculate start x pos
            if(align & Fw::AlignRight) {
                virtualPos.x = (maxLineWidth - lineWidths[lines]);
            } else if(align & Fw::AlignHorizontalCenter) {
                virtualPos.x = (maxLineWidth - lineWidths[lines]) / 2;
            } else { // AlignLeft
                virtualPos.x = 0;
            }
        }

        // store current glyph topLeft
        glyphsPositions[i] = virtualPos;

        // render only if the glyph is valid
        if(glyph >= 32 && glyph != (uchar)'\n') {
            virtualPos.x += m_glyphsSize[glyph].width() + m_glyphSpacing.width();
        }
    }

    if(textBoxSize) {
        textBoxSize->setWidth(maxLineWidth);
        textBoxSize->setHeight(virtualPos.y + m_glyphHeight);
    }

    return glyphsPositions;
}

Size BitmapFont::calculateTextRectSize(const std::string& text)
{
    Size size;
    calculateGlyphsPositions(text, Fw::AlignTopLeft, &size);
    return size;
}

std::string BitmapFont::wrapText(const std::string& text, int maxWidth, std::vector<std::pair<int, Color>>* colors)
{
    std::string outText;
    outText.reserve(text.size() * 2); // string append optimization

    int lastSeparator = 0, lastColorSeparator = 0, lineLength = 0, wordLength = 0;
    for (size_t i = 0, c = 0; i < text.size(); ++i) {
        uchar glyph = (uchar)text[i];
        if (text[i] == '\n' || text[i] == ' ') {
            lineLength += wordLength;
            if (lineLength > maxWidth) { // too long line with this word
                if (text[lastSeparator] == ' ') {
                    c -= 1;
                    updateColors(colors, lastColorSeparator, -1);
                    lastSeparator += 1;
                    lastColorSeparator += 1;
                }
                outText += '\n';
                lineLength = wordLength;
            }
            for (size_t j = lastSeparator; j < i; ++j) { // copy word
                outText += text[j];
            }
            if (text[i] == '\n') { // if new line was added reset line length
                outText += '\n';
                wordLength = 0;
                lineLength = 0;
                lastSeparator = i + 1;
                lastColorSeparator = c;
            } else { // space
                wordLength = m_glyphsSize[glyph].width() + m_glyphSpacing.width(); // space
                lastSeparator = i;
                lastColorSeparator = c;
                c += 1;
            }
            continue;
        }

        if (glyph < 32) // invalid character
            continue;

        wordLength += m_glyphsSize[glyph].width() + m_glyphSpacing.width();
        if (wordLength > maxWidth) { // too long word, split it
            if (lineLength != 0) { // add new line if current one is not empty
                outText += '\n';
            }
            if (text[lastSeparator] == ' ') { // ignore space if it's first character in new line
                c -= 1;
                lastSeparator += 1;
                lastColorSeparator += 1;
            }
            for (size_t j = lastSeparator; j < i; ++j) { // copy word
                outText += text[j];
            }
            updateColors(colors, lastColorSeparator, 1);
            outText += '-'; // word continuation
            outText += '\n'; // new line

            wordLength = m_glyphsSize[glyph].width() + m_glyphSpacing.width();
            lineLength = 0;
            lastSeparator = i;
            lastColorSeparator = c;
        }
        c += 1;
    }

    lineLength += wordLength;
    if (lineLength > maxWidth) { // too long line with this word
        if (text[lastSeparator] == ' ') { // ignore space if it's first character in new line
            lastSeparator += 1;
            lastColorSeparator += 1;
        }

        updateColors(colors, lastColorSeparator, 1);
        outText += '\n';
        lineLength = wordLength;
    }
    for (size_t j = lastSeparator; j < text.size(); ++j) { // copy word
        outText += text[j];
    }
    return outText;
}

void BitmapFont::calculateGlyphsWidthsAutomatically(const ImagePtr& image, const Size& glyphSize)
{
    if (!image)
        return;

    int numHorizontalGlyphs = image->getSize().width() / glyphSize.width();
    auto texturePixels = image->getPixels();

    // small AI to auto calculate pixels widths
    for (int glyph = m_firstGlyph; glyph < 256; ++glyph) {
        Rect glyphCoords(((glyph - m_firstGlyph) % numHorizontalGlyphs) * glyphSize.width(),
                         ((glyph - m_firstGlyph) / numHorizontalGlyphs) * glyphSize.height(),
                         glyphSize.width(),
                         m_glyphHeight);
        int width = glyphSize.width();
        for (int x = glyphCoords.left(); x <= glyphCoords.right(); ++x) {
            int filledPixels = 0;
            // check if all vertical pixels are alpha
            for (int y = glyphCoords.top(); y <= glyphCoords.bottom(); ++y) {
                if (texturePixels[(y * image->getSize().width() * 4) + (x * 4) + 3] != 0)
                    filledPixels++;
            }
            if (filledPixels > 0)
                width = x - glyphCoords.left() + 1;
        }
        // store glyph size
        m_glyphsSize[glyph].resize(width, m_glyphHeight);
    }
}

void BitmapFont::updateColors(std::vector<std::pair<int, Color>>* colors, int pos, int newTextLen)
{
    if (!colors) return;
    for (auto& it : *colors) {
        if (it.first > pos) {
            it.first += newTextLen;
        }
    }
}
