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

#ifndef FRAMEWORK_GRAPHICS_DECLARATIONS_H
#define FRAMEWORK_GRAPHICS_DECLARATIONS_H

#include <framework/global.h>
#include "glutil.h"

class Texture;
class TextureManager;
class Image;
class AnimatedTexture;
class BitmapFont;
class CachedText;
class FrameBuffer;
class FrameBufferManager;
class Shader;
class ShaderProgram;
class PainterShaderProgram;

using ImagePtr = std::shared_ptr<Image>;
using TexturePtr = std::shared_ptr<Texture>;
using AnimatedTexturePtr = std::shared_ptr<AnimatedTexture>;
using BitmapFontPtr = std::shared_ptr<BitmapFont>;
using CachedTextPtr = std::shared_ptr<CachedText>;
using FrameBufferPtr = std::shared_ptr<FrameBuffer>;
using ShaderPtr = std::shared_ptr<Shader>;
using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;
using PainterShaderProgramPtr = std::shared_ptr<PainterShaderProgram>;

using ShaderList = std::vector<ShaderPtr>;

#endif
