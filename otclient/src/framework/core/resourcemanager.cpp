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

#include "resourcemanager.h"
#include "filestream.h"

#include <framework/core/application.h>
#include <framework/luaengine/luainterface.h>
#include <framework/platform/platform.h>
#include <framework/util/crypt.h>
#include <framework/http/http.h>
#include <queue>

#include <boost/process.hpp>

#include <physfs.h>
#include <zip.h>

ResourceManager g_resources;

void ResourceManager::init(const char *argv0)
{
    m_binaryPath = boost::filesystem::system_complete(argv0);
    PHYSFS_init(argv0);
    PHYSFS_permitSymbolicLinks(1);
}

void ResourceManager::terminate()
{
    PHYSFS_deinit();
}

bool ResourceManager::launchCorrect(const std::string& app) { // curently works only on windows
    const char* localDir = PHYSFS_getPrefDir(app.c_str(), app.c_str());
    if (!localDir)
        return false;

    boost::filesystem::path path(localDir);
    time_t lastWrite = 0;
    boost::filesystem::path binary = "";
    for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {})) {
        if (boost::filesystem::is_directory(entry.path()))
            continue;
        if (entry.path().extension() == ".exe") {
            auto writeTime = boost::filesystem::last_write_time(entry.path());
            if (writeTime > lastWrite) {
                lastWrite = writeTime;
                binary = entry.path();
            }
        }
    }
    for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {})) { // remove old
        if (boost::filesystem::is_directory(entry.path()))
            continue;
        if (entry.path().extension() == ".exe") {
            if (binary == entry.path())
                continue;
            boost::filesystem::remove(entry.path());
        }
    }
    if(lastWrite != 0) {
        boost::process::spawn(binary);
        return true;
    }
    return false;
}

bool ResourceManager::setup(const std::string& app, const std::string& existentFile)
{
    const char* localDir = PHYSFS_getPrefDir(app.c_str(), app.c_str());
    if (!localDir) {
        g_logger.fatal(stdext::format("Unable to get local dir, error: %s", PHYSFS_getLastError()));
        return false;
    }

    if (!PHYSFS_mount(localDir, NULL, 0)) {
        g_logger.fatal(stdext::format("Unable to mount local directory '%s': %s", localDir, PHYSFS_getLastError()));
        return false;
    }

    if (!PHYSFS_setWriteDir(localDir)) {
        g_logger.fatal(stdext::format("Unable to set write dir '%s': %s", localDir, PHYSFS_getLastError()));
        return false;
    }

    // search for modules directory
    std::vector<std::string> possiblePaths = { std::string(localDir), g_platform.getCurrentDir()};
    const char* baseDir = PHYSFS_getBaseDir();
    if (baseDir)
        possiblePaths.push_back(baseDir);

    for(const std::string& dir : possiblePaths) {
        if(!PHYSFS_mount(dir.c_str(), NULL, 0))
            continue;

        if(PHYSFS_exists(existentFile.c_str())) {
            g_logger.debug(stdext::format("Found work dir at '%s'", dir));
            return true;
        }
        if(dir != localDir)
            PHYSFS_unmount(dir.c_str());
    }

    for(const std::string& dir : possiblePaths) {
        std::string path = dir + "/data.zip";
        if (PHYSFS_mount(path.c_str(), NULL, 0)) {
            if (PHYSFS_exists(existentFile.c_str())) {
                m_dataDir = dir;
                m_loadedFromArchive = true;
                g_logger.debug(stdext::format("Found work dir at '%s'", "data.zip"));
                return true;
            }
            PHYSFS_unmount(path.c_str());
        }
    }

    if (loadDataFromSelf(existentFile)) {
        m_loadedFromMemory = true;
        m_loadedFromArchive = true;
        return true;
    }

    g_logger.fatal("Unable to find working directory (or data.zip)");
    return false;
}

bool ResourceManager::loadDataFromSelf(const std::string& existentFile) {
    std::ifstream file(m_binaryPath.string(), std::ios::binary);
    if (!file.is_open())
        return false;
    std::string buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (buffer.size() < 1024 * 1024) // less then 1 MB
        return false;

    std::string toFind = { 0x50, 0x4b, 0x03, 0x04 }; // zip header
    size_t pos = toFind.rfind(toFind);
    if (pos == std::string::npos)
        return false;

    size_t m_memoryDataBufferSize = buffer.size() - pos;
    if (m_memoryDataBufferSize < 128 || m_memoryDataBufferSize > 512 * 1024 * 1024) // max 512MB
        return false;

    m_memoryDataBuffer = new char[m_memoryDataBufferSize];
    memcpy(m_memoryDataBuffer, &buffer[pos], m_memoryDataBufferSize);
    if (PHYSFS_mountMemory(m_memoryDataBuffer, m_memoryDataBufferSize, [](void* pointer) { delete[] pointer; }, "data.zip", NULL, 0)) {
        if (PHYSFS_exists(existentFile.c_str())) {
            g_logger.debug("Found work dir in memory");
            return true;
        }
        PHYSFS_unmount("data.zip");
    }
    
    delete[] m_memoryDataBuffer;
    return false;
}

bool ResourceManager::fileExists(const std::string& fileName)
{
    return (PHYSFS_exists(resolvePath(fileName).c_str()) && !PHYSFS_isDirectory(resolvePath(fileName).c_str()));
}

bool ResourceManager::directoryExists(const std::string& directoryName)
{
    return (PHYSFS_isDirectory(resolvePath(directoryName).c_str()));
}

void ResourceManager::readFileStream(const std::string& fileName, std::iostream& out)
{
    std::string buffer = readFileContents(fileName);
    if(buffer.length() == 0) {
        out.clear(std::ios::eofbit);
        return;
    }
    out.clear(std::ios::goodbit);
    out.write(&buffer[0], buffer.length());
    out.seekg(0, std::ios::beg);
}

std::string ResourceManager::readFileContents(const std::string& fileName)
{
    std::string fullPath = resolvePath(fileName);

    PHYSFS_File* file = PHYSFS_openRead(fullPath.c_str());
    if(!file)
        stdext::throw_exception(stdext::format("unable to open file '%s': %s", fullPath, PHYSFS_getLastError()));

    int fileSize = PHYSFS_fileLength(file);
    std::string buffer(fileSize, 0);
    PHYSFS_read(file, (void*)&buffer[0], 1, fileSize);
    PHYSFS_close(file);

    if (!decryptBuffer(buffer)) {
        g_logger.fatal(stdext::format("unable to decrypt file: %s", fullPath));
    }

    return buffer;
}

bool ResourceManager::writeFileBuffer(const std::string& fileName, const uchar* data, uint size)
{
    PHYSFS_file* file = PHYSFS_openWrite(fileName.c_str());
    if(!file) {
        g_logger.error(stdext::format("unable to open file for writing '%s': %s", fileName, PHYSFS_getLastError()));
        return false;
    }

    PHYSFS_write(file, (void*)data, size, 1);
    PHYSFS_close(file);
    return true;
}

bool ResourceManager::writeFileStream(const std::string& fileName, std::iostream& in)
{
    std::streampos oldPos = in.tellg();
    in.seekg(0, std::ios::end);
    std::streampos size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    in.read(&buffer[0], size);
    bool ret = writeFileBuffer(fileName, (const uchar*)&buffer[0], size);
    in.seekg(oldPos, std::ios::beg);
    return ret;
}

bool ResourceManager::writeFileContents(const std::string& fileName, const std::string& data)
{
    return writeFileBuffer(fileName, (const uchar*)data.c_str(), data.size());
}

FileStreamPtr ResourceManager::openFile(const std::string& fileName)
{
    std::string fullPath = resolvePath(fileName);
    return FileStreamPtr(new FileStream(fullPath, readFileContents(fullPath)));
}

FileStreamPtr ResourceManager::appendFile(const std::string& fileName)
{
    PHYSFS_File* file = PHYSFS_openAppend(fileName.c_str());
    if(!file)
        stdext::throw_exception(stdext::format("failed to append file '%s': %s", fileName, PHYSFS_getLastError()));
    return FileStreamPtr(new FileStream(fileName, file, true));
}

FileStreamPtr ResourceManager::createFile(const std::string& fileName)
{
    PHYSFS_File* file = PHYSFS_openWrite(fileName.c_str());
    if(!file)
        stdext::throw_exception(stdext::format("failed to create file '%s': %s", fileName, PHYSFS_getLastError()));
    return FileStreamPtr(new FileStream(fileName, file, true));
}

bool ResourceManager::deleteFile(const std::string& fileName)
{
    return PHYSFS_delete(resolvePath(fileName).c_str()) != 0;
}

bool ResourceManager::makeDir(const std::string directory)
{
    return PHYSFS_mkdir(directory.c_str());
}

std::list<std::string> ResourceManager::listDirectoryFiles(const std::string& directoryPath, bool fullPath /* = false */, bool raw /*= false*/)
{
    std::list<std::string> files;
    auto path = raw ? directoryPath : resolvePath(directoryPath);
    auto rc = PHYSFS_enumerateFiles(path.c_str());

    for (int i = 0; rc[i] != NULL; i++) {
        if(fullPath)
            files.push_back(path + "/" + rc[i]);
        else
            files.push_back(rc[i]);
    }

    PHYSFS_freeList(rc);
    return files;
}

std::string ResourceManager::resolvePath(std::string path)
{
    if(!stdext::starts_with(path, "/")) {
        std::string scriptPath = "/" + g_lua.getCurrentSourcePath();
        if(!scriptPath.empty())
            path = scriptPath + "/" + path;
        else
            g_logger.traceWarning(stdext::format("the following file path is not fully resolved: %s", path));
    }
    stdext::replace_all(path, "//", "/");
    if(!PHYSFS_exists(path.c_str())) {
        static std::string extra_check[] = { "/data", "/modules", "/mods" };
        for (auto extra : extra_check) {
            if (PHYSFS_exists((extra + path).c_str())) {
                path = extra + path;
                break;
            }
        }
    }
    return path;
}

std::string ResourceManager::guessFilePath(const std::string& filename, const std::string& type)
{
    if(isFileType(filename, type))
        return filename;
    return filename + "." + type;
}

bool ResourceManager::isFileType(const std::string& filename, const std::string& type)
{
    if(stdext::ends_with(filename, std::string(".") + type))
        return true;
    return false;
}

std::list<std::string> ResourceManager::listUpdateableFiles() {
    std::list<std::string> ret;
    std::queue<std::string> queue;
    queue.push("/init.lua");
    queue.push("/data");
    queue.push("/modules");
    queue.push("/mods");
    while (!queue.empty()) {
        auto file = queue.front();
        queue.pop();
        if (PHYSFS_isDirectory(file.c_str())) {
            auto list = listDirectoryFiles(file.c_str(), true, true);
            for (auto& it : list)
                queue.push(it);
            continue;
        }
        ret.push_back(file);
    }

    return ret;
}

std::string ResourceManager::fileChecksum(const std::string& path) {
    PHYSFS_File* file = PHYSFS_openRead(path.c_str());
    if(!file)
        return "";

    int fileSize = PHYSFS_fileLength(file);
    std::string buffer(fileSize, 0);
    PHYSFS_read(file, (void*)&buffer[0], 1, fileSize);
    PHYSFS_close(file);

    return g_crypt.md5Encode(buffer, false);
}

std::string ResourceManager::selfChecksum() {
    std::ifstream file(m_binaryPath.string(), std::ios::binary);
    if (!file.is_open())
        return "";

    std::string buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    return g_crypt.md5Encode(buffer, false);
}

void ResourceManager::updateClient(const std::vector<std::string>& files, const std::string& binaryName) {
    if (!m_loadedFromArchive)
        return g_logger.fatal("Client can be updated only while running from archive (data.zip)");

    auto downloads = g_http.downloads();
    
    if (!m_loadedFromMemory) {
        if(!PHYSFS_mount(m_dataDir.c_str(), NULL, 0))
            return g_logger.fatal(stdext::format("Can't mount dir with data: %s", m_dataDir));

        PHYSFS_File* file = PHYSFS_openRead("data.zip");
        if(!file)
            return g_logger.fatal(stdext::format("Can't open data.zip"));

        m_memoryDataBufferSize = PHYSFS_fileLength(file);
        m_memoryDataBuffer = new char[m_memoryDataBufferSize];
        PHYSFS_read(file, m_memoryDataBuffer, 1, m_memoryDataBufferSize);
        PHYSFS_close(file);        
    }

    if(!m_memoryDataBuffer || m_memoryDataBufferSize < 1024)
        return g_logger.fatal(stdext::format("Invalid buffer of memory data.zip"));

    zip_source_t *src;
    zip_t *za;
    zip_error_t error;
    zip_error_init(&error);

    if ((src = zip_source_buffer_create(m_memoryDataBuffer, m_memoryDataBufferSize, 1, &error)) == NULL)
        return g_logger.fatal(stdext::format("can't create source: %s", zip_error_strerror(&error)));

    if ((za = zip_open_from_source(src, 0, &error)) == NULL)
        return g_logger.fatal(stdext::format("can't open zip from source: %s", zip_error_strerror(&error)));

    zip_error_fini(&error);
    zip_source_keep(src);

    for (auto file : files) {
        auto it = downloads.find(file);
        if (it == downloads.end())
            continue;
        if (file == binaryName)
            continue;
        zip_source_t *s; 
        if((s=zip_source_buffer(za, it->second->response.data(), it->second->response.size(), 0)) == NULL)
            return g_logger.fatal(stdext::format("can't create source buffer: %s", zip_strerror(za)));
        if (file[0] == '/')
            file = file.substr(1);
        if(zip_file_add(za, file.c_str(), s, ZIP_FL_OVERWRITE) < 0)
            return g_logger.fatal(stdext::format("can't add file %s to zip archive: %s", file, zip_strerror(za)));
    }

    if (zip_close(za) < 0)
        return g_logger.fatal(stdext::format("can't close zip archive: %s", zip_strerror(za)));

    zip_stat_t zst;
    if (zip_source_stat(src, &zst) < 0)
        return g_logger.fatal(stdext::format("can't stat source: %s", zip_error_strerror(zip_source_error(src))));
    
    size_t zipSize = zst.size;    

    if (zip_source_open(src) < 0)
        return g_logger.fatal(stdext::format("can't open source: %s", zip_error_strerror(zip_source_error(src))));

    if (!m_loadedFromMemory && !PHYSFS_unmount((m_dataDir + "/data.zip").c_str()))
        return g_logger.fatal(stdext::format("can't unmount data.zip: %s", PHYSFS_getLastError()));

    PHYSFS_file* file = PHYSFS_openWrite("data.zip");
    if(!file)
        return g_logger.fatal(stdext::format("can't open data.zip for writing: %s", PHYSFS_getLastError()));

    const size_t chunk_size = 1024 * 1024;
    std::vector<char> chunk(chunk_size);
    while (zipSize > 0) {
        size_t currentChunk = std::min<size_t>(zipSize, chunk_size);
        if ((zip_uint64_t)zip_source_read(src, chunk.data(), currentChunk) < currentChunk)
            return g_logger.fatal(stdext::format("can't read data from source: %s", zip_error_strerror(zip_source_error(src))));
        PHYSFS_write(file, chunk.data(), currentChunk, 1);
        zipSize -= currentChunk;
    }

    PHYSFS_close(file);
    zip_source_close(src);
    zip_source_free(src);

    if (!binaryName.empty()) {
        auto it = downloads.find(binaryName);
        if (it == downloads.end())
            return g_logger.fatal("Can't find new binary data in downloads");

        boost::filesystem::path path(binaryName);
        auto newBinary = path.stem().string() + std::to_string(time(nullptr)) + path.extension().string();
        file = PHYSFS_openWrite(path.string().c_str());
        if (!file)
            return g_logger.fatal(stdext::format("can't open %s for writing: %s", newBinary, PHYSFS_getLastError()));
        PHYSFS_write(file, it->second->response.data(), it->second->response.size(), 1);
        PHYSFS_close(file);

        std::vector<boost::filesystem::path> outDir = { boost::filesystem::path(PHYSFS_getRealDir(newBinary.c_str())) };
        boost::process::spawn(boost::process::search_path(newBinary, outDir));
        return;
    }    

    boost::process::spawn(m_binaryPath);
}

void ResourceManager::encrypt() {
    const std::string dirsToCheck[] = { "data", "modules", "mods" };
    const std::string luaExtension = ".lua";

    std::queue<boost::filesystem::path> toEncrypt;
    for (auto& dir : dirsToCheck) {
        for(auto&& entry : boost::filesystem::recursive_directory_iterator(boost::filesystem::path(dir))) {
            if (!boost::filesystem::is_regular_file(entry.path()))
                continue;
            toEncrypt.push(entry.path());
        }
    }

    while (!toEncrypt.empty()) {
        auto it = toEncrypt.front();
        toEncrypt.pop();
        boost::filesystem::ifstream in_file(it, std::ios::binary);
        if (!in_file.is_open())
            continue;
        std::string buffer(std::istreambuf_iterator<char>(in_file), {});
        in_file.close();
        if (!encryptBuffer(buffer)) // already encrypted
            continue;
        boost::filesystem::ofstream out_file(it, std::ios::binary);
        if (!out_file.is_open())
            continue;
        out_file.write(buffer.data(), buffer.size());
        out_file.close();
    }
}

bool ResourceManager::decryptBuffer(std::string& buffer) {
    if (buffer.size() < 5 || buffer.substr(0, 4).compare("ENC2") != 0)
        return true;

    uint64_t key = *(uint64_t*)&buffer[4];
    uint32_t size = *(uint32_t*)&buffer[12];
    uint32_t adler = *(uint32_t*)&buffer[16];

    g_crypt.bdecrypt((uint8_t*)&buffer[20], size, key);

    buffer = buffer.substr(20, size);
    uint32_t addlerCheck = stdext::adler32((const uint8_t*)&buffer[0], size);
    if (adler != addlerCheck)
        return false;

    return true;
}

bool ResourceManager::encryptBuffer(std::string& buffer) {
    if (buffer.size() >= 4 && buffer.substr(0, 4).compare("ENC2") == 0)
        return false; // already encrypted

    uint64_t key = rand() << 32 + rand() << 16 + rand();

    std::string new_buffer(20, '0');
    new_buffer[0] = 'E';
    new_buffer[1] = 'N';
    new_buffer[2] = 'C';
    new_buffer[3] = '2';
    *(uint64_t*)&new_buffer[4] = key;
    *(uint32_t*)&new_buffer[12] = (uint32_t)buffer.size();
    *(uint32_t*)&new_buffer[16] = (uint32_t)stdext::adler32((const uint8_t*)&buffer[0], buffer.size());

    new_buffer += buffer;

    g_crypt.bencrypt((uint8_t*)&new_buffer[0] + 20, buffer.size(), key);
    buffer = new_buffer;
    return true;
}