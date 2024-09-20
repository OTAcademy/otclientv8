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

#ifndef PAINTERSHADERPROGRAM_H
#define PAINTERSHADERPROGRAM_H

#include "shaderprogram.h"
#include "coordsbuffer.h"
#include <framework/core/timer.h>

class PainterShaderProgram : public ShaderProgram {
protected:
    enum {
        VERTEX_ATTR = 0,
        TEXCOORD_ATTR = 1,
        DEPTH_ATTR = 2,
        COLOR_ATTR = 3,
        DEPTH_TEXCOORD_ATTR = 4,

        PROJECTION_MATRIX_UNIFORM = 0,
        TEXTURE_MATRIX_UNIFORM = 1,
        TRANSFORM_MATRIX_UNIFORM = 2,

        COLOR_UNIFORM = 3,
        TIME_UNIFORM = 5,
        DEPTH_UNIFORM = 6,

        TEX0_UNIFORM = 7,
        TEX1_UNIFORM = 8,
        TEX2_UNIFORM = 9,
        TEX3_UNIFORM = 10,
        ATLAS_TEX0_UNIFORM = 11,
        ATLAS_TEX1_UNIFORM = 12,

        RESOLUTION_UNIFORM = 13,
        OFFSET_UNIFORM = 14,
        CENTER_UNIFORM = 15,
        MAP_WALKOFFSET_UNIFORM = 16
    };

    friend class Painter;

    virtual void setupUniforms();

public:
    PainterShaderProgram(const std::string& name);

    bool link();

    void setTransformMatrix(const Matrix3& transformMatrix);
    void setProjectionMatrix(const Matrix3& projectionMatrix);
    void setTextureMatrix(const Matrix3& textureMatrix);
    void setColor(const Color& color);
    void setMatrixColor(const Matrix4& colors);
    void setDepth(float depth);
    void setResolution(const Size& resolution);
    void setOffset(const Point& offset);
    void setCenter(const Point& center);
    void updateTime();
    void updateWalkOffset(const PointF& offset);

    void addMultiTexture(const std::string& file);
    void bindMultiTextures();
    void clearMultiTextures();

    void enableColorMatrix()
    {
        m_useColorMatrix = true;
    }

private:
    float m_startTime;

    Color m_color;
    float m_depth;
    Matrix3 m_transformMatrix;
    Matrix3 m_projectionMatrix;
    Matrix3 m_textureMatrix;
    Size m_resolution;
    Point m_offset;
    PointF m_walkOffset;
    Point m_center;
    float m_time;
    std::vector<TexturePtr> m_multiTextures;
    bool m_useColorMatrix = false;
};

#endif
