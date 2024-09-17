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

#include "shaderprogram.h"
#include "graphics.h"

#include <framework/core/application.h>

uint ShaderProgram::m_currentProgram = 0;

ShaderProgram::ShaderProgram(const std::string& name) : m_name(name)
{
    m_linked = false;
    m_programId = glCreateProgram();
    m_uniformLocations.fill(-1);
    if(!m_programId)
        g_logger.fatal("Unable to create GL shader program");
}

ShaderProgram::~ShaderProgram()
{
    VALIDATE(!g_app.isTerminated());
    if(g_graphics.ok())
        glDeleteProgram(m_programId);
}

PainterShaderProgramPtr ShaderProgram::create(const std::string& name, const std::string& vertexShader, const std::string& fragmentShader, bool colorMatrix)
{
    auto program = std::make_shared<PainterShaderProgram>(name);
    if (!program) {
        g_logger.error(stdext::format("Cant creatre shader: %s", program->getName()));
        return nullptr;
    }
    if (!program->addShaderFromSourceCode(Shader::Vertex, vertexShader))
        return nullptr;
    if(!program->addShaderFromSourceCode(Shader::Fragment, fragmentShader))
        return nullptr;

    if (colorMatrix) {
        program->enableColorMatrix();
    }
    if (!program->link()) {
        g_graphics.checkForError(stdext::format("%s (%s)", __FUNCTION__, program->getName()), vertexShader + "\n" + fragmentShader, __LINE__);
        return nullptr;
    }
    g_graphics.checkForError(stdext::format("%s (%s)", __FUNCTION__, program->getName()), vertexShader + "\n" + fragmentShader, __LINE__);
    return program;
}


bool ShaderProgram::addShader(const ShaderPtr& shader) {
    glAttachShader(m_programId, shader->getShaderId());
    m_linked = false;
    m_shaders.push_back(shader);
    return true;
}

bool ShaderProgram::addShaderFromSourceCode(Shader::ShaderType shaderType, const std::string& sourceCode) {
    auto shader = std::make_shared<Shader>(shaderType);
    if(!shader->compileSourceCode(sourceCode)) {
        g_logger.error(stdext::format("failed to compile shader: %s\n%s", getName(), shader->log()));
        return false;
    }
    return addShader(shader);
}

bool ShaderProgram::addShaderFromSourceFile(Shader::ShaderType shaderType, const std::string& sourceFile) {
    auto shader = std::make_shared<Shader>(shaderType);
    if(!shader->compileSourceFile(sourceFile)) {
        g_logger.error(stdext::format("failed to compile shader %s (%s): %s", getName(), sourceFile, shader->log()));
        return false;
    }
    return addShader(shader);
}

void ShaderProgram::removeShader(const ShaderPtr& shader)
{
    auto it = std::find(m_shaders.begin(), m_shaders.end(), shader);
    if(it == m_shaders.end())
        return;

    glDetachShader(m_programId, shader->getShaderId());
    m_shaders.erase(it);
    m_linked = false;
}

void ShaderProgram::removeAllShaders()
{
    while(!m_shaders.empty())
        removeShader(m_shaders.front());
}

bool ShaderProgram::link()
{
    if(m_linked)
        return true;

    glLinkProgram(m_programId);

    int value = GL_FALSE;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &value);
    m_linked = (value != GL_FALSE);
    if (m_linked) {
        return true;
    }

    GLint maxLength = 0;
    glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(m_programId, maxLength, &maxLength, &infoLog[0]);
    g_logger.error(stdext::format("Program %i, %s linking error (%i): %s - %s - %s %s\nExtensions: %s", m_programId, getName(), infoLog.size(),
        std::string(infoLog.begin(), infoLog.end()).c_str(), log().c_str(),
        g_graphics.getRenderer(), g_graphics.getVersion(), g_graphics.getExtensions()));
    return false;
}

bool ShaderProgram::bind()
{
    if(m_currentProgram != m_programId) {
        if (!m_linked) {
            link();
        }
        glUseProgram(m_programId);
        m_currentProgram = m_programId;
    }
    return true;
}

void ShaderProgram::release()
{
    if(m_currentProgram != 0) {
        m_currentProgram = 0;
        glUseProgram(0);
    }
}

std::string ShaderProgram::log()
{
    std::string infoLog;
    int infoLogLength = 0;
    glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 1) {
        std::vector<char> buf(infoLogLength);
        glGetShaderInfoLog(m_programId, infoLogLength-1, NULL, &buf[0]);
        infoLog = &buf[0];
    }
    return infoLog;
}

int ShaderProgram::getAttributeLocation(const char* name)
{
    return glGetAttribLocation(m_programId, name);
}

void ShaderProgram::bindAttributeLocation(int location, const char* name)
{
    return glBindAttribLocation(m_programId, location, name);
}

void ShaderProgram::bindUniformLocation(int location, const char* name)
{
    VALIDATE(m_linked);
    VALIDATE(location >= 0 && location < MAX_UNIFORM_LOCATIONS);
    m_uniformLocations[location] = glGetUniformLocation(m_programId, name);
}
