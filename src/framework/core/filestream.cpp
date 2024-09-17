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

#include "filestream.h"
#include "binarytree.h"
#include <framework/core/application.h>

#define PHYSFS_DEPRECATED
#include <physfs.h>
#include <zlib.h>

FileStream::FileStream(const std::string& name, PHYSFS_File *fileHandle, bool writeable) :
    m_name(name),
    m_fileHandle(fileHandle),
    m_pos(0),
    m_writeable(writeable),
    m_caching(false)
{
}

FileStream::FileStream(const std::string& name, std::string&& buffer) :
    m_name(name),
    m_fileHandle(nullptr),
    m_pos(0),
    m_writeable(false),
    m_caching(true)
{
    if (!initFromGzip(buffer)) {
        m_strData = std::move(buffer);
    }
}

bool FileStream::initFromGzip(const std::string& buffer)
{
    if (buffer.size() < 10 || (uint8_t)buffer[0] != 0x1f ||
        (uint8_t)buffer[1] != 0x8b || (uint8_t)buffer[2] != 0x08) {
        return false;
    }

    uint32_t fileSize = *(uint32_t*)(&buffer[buffer.size() - 4]);
    if (fileSize > 512 * 1024 * 1024 || fileSize * 2 < buffer.size()) {
        return false;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    stream.next_in = (Bytef*)buffer.c_str();
    stream.avail_in = buffer.size();
    if (inflateInit2(&stream, 15 + 32) != Z_OK)
        return false;

    m_data.grow(fileSize, true);
    stream.next_out = &m_data[0];
    stream.avail_out = m_data.size();

    inflate(&stream, Z_SYNC_FLUSH);

    inflateEnd(&stream);
    return true;
}


FileStream::~FileStream()
{
    VALIDATE(!g_app.isTerminated());
    if(!g_app.isTerminated())
        close();
}

void FileStream::close()
{
    if(m_fileHandle && PHYSFS_isInit()) {
        if(!PHYSFS_close(m_fileHandle))
            throwError("close failed", true);
        m_fileHandle = nullptr;
    }

    m_data.clear();
    m_strData.clear();
    m_pos = 0;
}

void FileStream::flush()
{
    if(!m_writeable)
        throwError("filestream is not writeable");

    if(m_fileHandle) {
        if(m_caching) {
            if(!PHYSFS_seek(m_fileHandle, 0))
                throwError("flush seek failed", true);
            uint len = m_data.size();
            if(PHYSFS_writeBytes(m_fileHandle, m_data.data(), len) != len)
                throwError("flush write failed", true);
        }

        if(PHYSFS_flush(m_fileHandle) == 0)
            throwError("flush failed", true);
    }
}

int FileStream::read(void* buffer, uint32 size, uint32 nmemb)
{
    if (!m_caching) {
        int res = PHYSFS_readBytes(m_fileHandle, buffer, size * nmemb);
        if (res == -1)
            throwError("read failed", true);
        return res;
    } if (!m_strData.empty()) {
        int writePos = 0;
        uint8* outBuffer = (uint8*)buffer;
        for (uint i = 0; i < nmemb; ++i) {
            if (m_pos + size > m_strData.size())
                return i;

            for (uint j = 0; j < size; ++j)
                outBuffer[writePos++] = m_strData[m_pos++];
        }
        return nmemb;
    } else {
        int writePos = 0;
        uint8 *outBuffer = (uint8*)buffer;
        for(uint i=0;i<nmemb;++i) {
            if(m_pos+size > m_data.size())
                return i;

            for(uint j=0;j<size;++j)
                outBuffer[writePos++] = m_data[m_pos++];
        }
        return nmemb;
    }
}

void FileStream::write(const void *buffer, uint32 count)
{
    if(!m_caching) {
        if(PHYSFS_writeBytes(m_fileHandle, buffer, count) != count)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + count);
        memcpy(&m_data[m_pos], buffer, count);
        m_pos += count;
    }
}

void FileStream::seek(uint32 pos)
{
    if (!m_caching) {
        if (!PHYSFS_seek(m_fileHandle, pos))
            throwError("seek failed", true);
    } else if(!m_strData.empty()) {
        if (pos > m_strData.size())
            throwError("seek failed");
        m_pos = pos;
    } else {
        if(pos > m_data.size())
            throwError("seek failed");
        m_pos = pos;
    }
}

void FileStream::skip(uint len)
{
    seek(tell() + len);
}

uint FileStream::size()
{
    if (!m_caching)
        return PHYSFS_fileLength(m_fileHandle);
    else if (!m_strData.empty())
        return m_strData.size();
    else
        return m_data.size();
}

uint FileStream::tell()
{
    if(!m_caching)
        return PHYSFS_tell(m_fileHandle);
    else
        return m_pos;
}

bool FileStream::eof()
{
    if(!m_caching)
        return PHYSFS_eof(m_fileHandle);
    else if (!m_strData.empty())
        return m_pos >= m_strData.size();
    else
        return m_pos >= m_data.size();
}

uint8 FileStream::getU8()
{
    uint8 v = 0;
    if(!m_caching) {
        if(PHYSFS_readBytes(m_fileHandle, &v, 1) != 1)
            throwError("read failed", true);
    } else if (!m_strData.empty()) {
        if (m_pos + 1 > m_strData.size())
            throwError("read failed");

        v = m_strData[m_pos];
        m_pos += 1;
    } else {
        if(m_pos+1 > m_data.size())
            throwError("read failed");

        v = m_data[m_pos];
        m_pos += 1;
    }
    return v;
}

uint16 FileStream::getU16()
{
    uint16 v = 0;
    if(!m_caching) {
        if(PHYSFS_readULE16(m_fileHandle, &v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 2 > m_strData.size())
            throwError("read failed");

        v = stdext::readULE16((const uint8_t*)&m_strData[m_pos]);
        m_pos += 2;
    } else {
        if(m_pos+2 > m_data.size())
            throwError("read failed");

        v = stdext::readULE16(&m_data[m_pos]);
        m_pos += 2;
    }
    return v;
}

uint32 FileStream::getU32()
{
    uint32 v = 0;
    if(!m_caching) {
        if(PHYSFS_readULE32(m_fileHandle, &v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 4 > m_strData.size())
            throwError("read failed");

        v = stdext::readULE32((const uint8_t*)&m_strData[m_pos]);
        m_pos += 4;
    } else {
        if (m_pos + 4 > m_data.size())
            throwError("read failed");

        v = stdext::readULE32(&m_data[m_pos]);
        m_pos += 4;
    }
    return v;
}

uint64 FileStream::getU64()
{
    uint64 v = 0;
    if(!m_caching) {
        if(PHYSFS_readULE64(m_fileHandle, (PHYSFS_uint64*)&v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 8 > m_strData.size())
            throwError("read failed");
        v = stdext::readULE64((const uint8_t*)&m_strData[m_pos]);
        m_pos += 8;
    } else {
        if (m_pos + 8 > m_data.size())
            throwError("read failed");
        v = stdext::readULE64(&m_data[m_pos]);
        m_pos += 8;
    }
    return v;
}

int8 FileStream::get8()
{
    int8 v = 0;
    if(!m_caching) {
        if(PHYSFS_readBytes(m_fileHandle, &v, 1) != 1)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 1 > m_strData.size())
            throwError("read failed");

        v = m_strData[m_pos];
        m_pos += 1;
    } else {
        if(m_pos+1 > m_data.size())
            throwError("read failed");

        v = m_data[m_pos];
        m_pos += 1;
    }
    return v;
}

int16 FileStream::get16()
{
    int16 v = 0;
    if(!m_caching) {
        if(PHYSFS_readSLE16(m_fileHandle, &v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 2 > m_strData.size())
            throwError("read failed");

        v = stdext::readSLE16((const uint8_t*)&m_strData[m_pos]);
        m_pos += 2;
    } else {
        if (m_pos + 2 > m_data.size())
            throwError("read failed");

        v = stdext::readSLE16(&m_data[m_pos]);
        m_pos += 2;
    }
    return v;
}

int32 FileStream::get32()
{
    int32 v = 0;
    if(!m_caching) {
        if(PHYSFS_readSLE32(m_fileHandle, &v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 4 > m_strData.size())
            throwError("read failed");

        v = stdext::readSLE32((const uint8_t*)&m_strData[m_pos]);
        m_pos += 4;
    } else {
        if (m_pos + 4 > m_data.size())
            throwError("read failed");

        v = stdext::readSLE32(&m_data[m_pos]);
        m_pos += 4;
    }
    return v;
}

int64 FileStream::get64()
{
    int64 v = 0;
    if(!m_caching) {
        if(PHYSFS_readSLE64(m_fileHandle, (PHYSFS_sint64*)&v) == 0)
            throwError("read failed", true);
    } else if(!m_strData.empty()) {
        if (m_pos + 8 > m_strData.size())
            throwError("read failed");
        v = stdext::readSLE64((const uint8_t*)&m_strData[m_pos]);
        m_pos += 8;
    } else {
        if (m_pos + 8 > m_data.size())
            throwError("read failed");
        v = stdext::readSLE64(&m_data[m_pos]);
        m_pos += 8;
    }
    return v;
}

std::string FileStream::getString()
{
    std::string str;
    uint16 len = getU16();
    if (len > 0) {
        std::vector<uint8_t> buffer(len, 0);
        if (m_fileHandle) {
            if (PHYSFS_read(m_fileHandle, &buffer[0], 1, len) == 0)
                throwError("read failed", true);
            else
                str = std::string(buffer.begin(), buffer.end());
        } else if(!m_strData.empty()) {
            if (m_pos + len > m_strData.size()) {
                throwError("read failed");
                return 0;
            }

            str = std::string((char*)&m_strData[m_pos], len);
            m_pos += len;
        } else {
            if (m_pos + len > m_data.size()) {
                throwError("read failed");
                return 0;
            }

            str = std::string((char*)&m_data[m_pos], len);
            m_pos += len;
        }
    } else if (len != 0)
        throwError("read failed because string is too big");
    return str;
}

BinaryTreePtr FileStream::getBinaryTree()
{
    uint8 byte = getU8();
    if(byte != BINARYTREE_NODE_START)
        stdext::throw_exception(stdext::format("failed to read node start (getBinaryTree): %d", byte));

    return std::make_shared<BinaryTree>(asFileStream());
}

void FileStream::startNode(uint8 n)
{
    addU8(BINARYTREE_NODE_START);
    addU8(n);
}

void FileStream::endNode()
{
    addU8(BINARYTREE_NODE_END);
}

void FileStream::addU8(uint8 v)
{
    if(!m_caching) {
        if(PHYSFS_writeBytes(m_fileHandle, &v, 1) != 1)
            throwError("write failed", true);
    } else {
        m_data.add(v);
        m_pos++;
    }
}

void FileStream::addU16(uint16 v)
{
    if(!m_caching) {
        if(PHYSFS_writeULE16(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 2);
        stdext::writeULE16(&m_data[m_pos], v);
        m_pos += 2;
    }
}

void FileStream::addU32(uint32 v)
{
    if(!m_caching) {
        if(PHYSFS_writeULE32(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 4);
        stdext::writeULE32(&m_data[m_pos], v);
        m_pos += 4;
    }
}

void FileStream::addU64(uint64 v)
{
    if(!m_caching) {
        if(PHYSFS_writeULE64(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 8);
        stdext::writeULE64(&m_data[m_pos], v);
        m_pos += 8;
    }
}

void FileStream::add8(int8 v)
{
    if(!m_caching) {
        if(PHYSFS_writeBytes(m_fileHandle, &v, 1) != 1)
            throwError("write failed", true);
    } else {
        m_data.add(v);
        m_pos++;
    }
}

void FileStream::add16(int16 v)
{
    if(!m_caching) {
        if(PHYSFS_writeSLE16(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 2);
        stdext::writeSLE16(&m_data[m_pos], v);
        m_pos += 2;
    }
}

void FileStream::add32(int32 v)
{
    if(!m_caching) {
        if(PHYSFS_writeSLE32(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 4);
        stdext::writeSLE32(&m_data[m_pos], v);
        m_pos += 4;
    }
}

void FileStream::add64(int64 v)
{
    if(!m_caching) {
        if(PHYSFS_writeSLE64(m_fileHandle, v) == 0)
            throwError("write failed", true);
    } else {
        m_data.grow(m_pos + 8);
        stdext::writeSLE64(&m_data[m_pos], v);
        m_pos += 8;
    }
}

void FileStream::addString(const std::string& v)
{
    addU16(v.length());
    write(v.c_str(), v.length());
}

void FileStream::throwError(const std::string& message, bool physfsError)
{
    std::string completeMessage = stdext::format("in file '%s': %s", m_name, message);
    if(physfsError)
        completeMessage += std::string(": ") + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
    stdext::throw_exception(completeMessage);
}

