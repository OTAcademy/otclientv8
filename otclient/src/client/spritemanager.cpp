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

#include "spritemanager.h"
#include "game.h"
#include <framework/core/resourcemanager.h>
#include <framework/core/filestream.h>
#include <framework/graphics/image.h>
#include <framework/graphics/atlas.h>

SpriteManager g_sprites;

SpriteManager::SpriteManager()
{
    m_spritesCount = 0;
    m_signature = 0;
}

void SpriteManager::terminate()
{
    unload();
}

bool SpriteManager::loadSpr(std::string file)
{
    m_spritesCount = 0;
    m_signature = 0;
    m_spriteSize = 32;
    m_loaded = false;
    g_atlas.reset(0);
    g_atlas.reset(1);

    try {
        file = g_resources.guessFilePath(file, "spr");

        m_spritesFile = g_resources.openFile(file);
        // cache file buffer to avoid lags from hard drive
        m_spritesFile->cache();

        m_signature = m_spritesFile->getU32();
        if (m_signature == *((uint32_t*)"V8HD")) {
            g_logger.info("HD Sprites");
            m_spriteSize = 64;
            g_game.enableFeature(Otc::GameSpritesAlphaChannel);
            g_game.enableFeature(Otc::GameSpritesU32);
            m_signature = m_spritesFile->getU32();
        }

        m_spritesCount = g_game.getFeature(Otc::GameSpritesU32) ? m_spritesFile->getU32() : m_spritesFile->getU16();
        m_spritesOffset = m_spritesFile->tell();
        m_loaded = true;
        g_lua.callGlobalField("g_sprites", "onLoadSpr", file);
        return true;
    } catch(stdext::exception& e) {
        g_logger.error(stdext::format("Failed to load sprites from '%s': %s", file, e.what()));
        return false;
    }
}

#ifdef WITH_ENCRYPTION

void SpriteManager::saveSpr(std::string fileName)
{
    if (!m_loaded)
        stdext::throw_exception("failed to save, spr is not loaded");

    try {
        FileStreamPtr fin = g_resources.createFile(fileName);
        if (!fin)
            stdext::throw_exception(stdext::format("failed to open file '%s' for write", fileName));

        fin->cache();

        fin->addU32(m_signature);
        if (g_game.getFeature(Otc::GameSpritesU32))
            fin->addU32(m_spritesCount);
        else
            fin->addU16(m_spritesCount);

        uint32 offset = fin->tell();
        uint32 spriteAddress = offset + 4 * m_spritesCount;
        for (int i = 1; i <= m_spritesCount; i++)
            fin->addU32(0);

        for (int i = 1; i <= m_spritesCount; i++) {
            m_spritesFile->seek((i - 1) * 4 + m_spritesOffset);
            uint32 fromAdress = m_spritesFile->getU32();
            if (fromAdress != 0) {
                fin->seek(offset + (i - 1) * 4);
                fin->addU32(spriteAddress);
                fin->seek(spriteAddress);

                m_spritesFile->seek(fromAdress);
                fin->addU8(m_spritesFile->getU8());
                fin->addU8(m_spritesFile->getU8());
                fin->addU8(m_spritesFile->getU8());

                uint16 dataSize = m_spritesFile->getU16();
                fin->addU16(dataSize);
                std::vector<char> spriteData(m_spriteSize * m_spriteSize);
                m_spritesFile->read(spriteData.data(), dataSize);
                fin->write(spriteData.data(), dataSize);

                spriteAddress = fin->tell();
            }
            //TODO: Check for overwritten sprites.
        }

        fin->flush();
        fin->close();
    }
    catch (std::exception& e) {
        g_logger.error(stdext::format("Failed to save '%s': %s", fileName, e.what()));
    }
}

void SpriteManager::saveReplacedSpr(std::string fileName, std::map<uint32_t, ImagePtr>& replacements)
{
    if (!m_loaded)
        stdext::throw_exception("failed to save, spr is not loaded");

    try {
        FileStreamPtr fin = g_resources.createFile(fileName);
        if (!fin)
            stdext::throw_exception(stdext::format("failed to open file '%s' for write", fileName));

        fin->cache();

        const char hdSignature[] = "V8HD";
        fin->addU32(*((uint32_t*)hdSignature));

        fin->addU32(m_signature);
        fin->addU32(m_spritesCount);

        uint32 offset = fin->tell();
        uint32 spriteAddress = offset + 4 * m_spritesCount;
        for (int i = 1; i <= m_spritesCount; i++)
            fin->addU32(0);

        for (int i = 1; i <= m_spritesCount; i++) {
            if (!getSpriteImage(i))
                continue;

            spriteAddress = fin->tell();
            fin->seek(offset + (i - 1) * 4);
            fin->addU32(spriteAddress);
            fin->seek(spriteAddress);

            uint16 dataSize = 64 * 64 * 4;
            fin->addU16(dataSize);
            fin->addU16(0); // transparent px
            fin->addU16(dataSize / 4); // normal px

            ImagePtr sprite = replacements[i];
            if (!sprite) {
                g_logger.info(stdext::format("Missing sprite for %i - upscaling", i));
                sprite = getSpriteImage(i)->upscale();
            }
            if (!sprite) {
                g_logger.info(stdext::format("Missing sprite for %i", i));
                continue;
            }

            if (sprite->getPixelCount() != dataSize / 4)
                stdext::throw_exception(stdext::format("Wrong pixel count for sprite %i - %i", i, sprite->getPixelCount()));

            fin->write(sprite->getPixelData(), dataSize);
        }

        fin->flush();
        fin->close();
    }
    catch (std::exception& e) {
        g_logger.error(stdext::format("Failed to save '%s': %s", fileName, e.what()));
    }
}

void SpriteManager::dumpSprites(std::string dir)
{
    if (dir.empty()) {
        g_logger.error("Empty dir for sprites dump");
        return;
    }
    g_resources.makeDir(dir);
    for (int i = 1; i <= m_spritesCount; i++) {
        auto img = getSpriteImage(i);
        if (!img) continue;
        img->savePNG(dir + "/" + std::to_string(i) + ".png");
    }
}

#endif

void SpriteManager::unload()
{
    m_spritesCount = 0;
    m_signature = 0;
    m_spritesFile = nullptr;
}

ImagePtr SpriteManager::getSpriteImage(int id)
{
    try {
        int spriteDataSize = m_spriteSize * m_spriteSize * 4;

        if (id == 0 || !m_spritesFile)
            return nullptr;

        m_spritesFile->seek(((id - 1) * 4) + m_spritesOffset);

        uint32 spriteAddress = m_spritesFile->getU32();

        // no sprite? return an empty texture
        if (spriteAddress == 0)
            return nullptr;

        m_spritesFile->seek(spriteAddress);

        // color key
        if (m_spriteSize == 32) {
            m_spritesFile->getU8();
            m_spritesFile->getU8();
            m_spritesFile->getU8();
        }

        uint16 pixelDataSize = m_spritesFile->getU16();

        ImagePtr image(new Image(Size(m_spriteSize, m_spriteSize)));

        uint8* pixels = image->getPixelData();
        int writePos = 0;
        int read = 0;
        bool useAlpha = g_game.getFeature(Otc::GameSpritesAlphaChannel);

        // decompress pixels
        while (read < pixelDataSize && writePos < spriteDataSize) {
            uint16 transparentPixels = m_spritesFile->getU16();
            uint16 coloredPixels = m_spritesFile->getU16();

            writePos += transparentPixels * 4;

            if (useAlpha) {
                m_spritesFile->read(&pixels[writePos], std::min<uint16>(coloredPixels * 4, spriteDataSize - writePos));
                writePos += coloredPixels * 4;
                read += 4 + (4 * coloredPixels);
            } else {
                for (int i = 0; i < coloredPixels && writePos < spriteDataSize; i++) {
                    pixels[writePos + 0] = m_spritesFile->getU8();
                    pixels[writePos + 1] = m_spritesFile->getU8();
                    pixels[writePos + 2] = m_spritesFile->getU8();
                    pixels[writePos + 3] = 0xFF;
                    writePos += 4;
                }
                read += 4 + (3 * coloredPixels);
            }
        }

        return image;
    } catch (stdext::exception & e) {
        g_logger.error(stdext::format("Failed to get sprite id %d: %s", id, e.what()));
        return nullptr;
    }
}