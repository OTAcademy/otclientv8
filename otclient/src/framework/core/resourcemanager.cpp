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
#include "resource.h"

#include <framework/core/application.h>
#include <framework/luaengine/luainterface.h>
#include <framework/platform/platform.h>
#include <framework/util/crypt.h>
#include <framework/http/http.h>
#include <queue>
#include <regex>

#include <boost/process.hpp>
#include <locale>

#include <physfs.h>
#include <zip.h>
#include <zlib.h>

ResourceManager g_resources;

void ResourceManager::init(const char *argv0, bool failsafe)
{
    m_binaryPath = std::filesystem::absolute(argv0);
    m_failsafe = failsafe;
    PHYSFS_init(argv0);
    PHYSFS_permitSymbolicLinks(1);
}

void ResourceManager::terminate()
{
    PHYSFS_deinit();
}

int ResourceManager::launchCorrect(const std::string& product, const std::string& app) { // curently works only on windows
    auto init_path = m_binaryPath.parent_path();
    init_path /= "init.lua";
    if (std::filesystem::exists(init_path)) // debug version
        return 0;

    const char* localDir = PHYSFS_getPrefDir(product.c_str(), app.c_str());
    if (!localDir)
        return 0;
    
    auto fileName2 = m_binaryPath.stem().string();
    fileName2 = stdext::split(fileName2, "-")[0];
    stdext::tolower(fileName2);

    std::filesystem::path path(std::filesystem::u8path(localDir));
    auto lastWrite = std::filesystem::last_write_time(m_binaryPath);
    std::filesystem::path binary = m_binaryPath;
    for (auto& entry : boost::make_iterator_range(std::filesystem::directory_iterator(path), {})) {
        if (std::filesystem::is_directory(entry.path()))
            continue;

        auto fileName1 = entry.path().stem().string();
        fileName1 = stdext::split(fileName1, "-")[0];
        stdext::tolower(fileName1);
        if (fileName1 != fileName2)
            continue;

        if (entry.path().extension() == m_binaryPath.extension()) {
            auto writeTime = std::filesystem::last_write_time(entry.path());
            if (writeTime > lastWrite) {
                lastWrite = writeTime;
                binary = entry.path();
            }
        }
    }
    for (auto& entry : boost::make_iterator_range(std::filesystem::directory_iterator(path), {})) { // remove old
        if (std::filesystem::is_directory(entry.path()))
            continue;

        auto fileName1 = entry.path().stem().string();
        fileName1 = stdext::split(fileName1, "-")[0];
        stdext::tolower(fileName1);
        if (fileName1 != fileName2)
            continue;

        if (entry.path().extension() == m_binaryPath.extension()) {
            if (binary == entry.path())
                continue;
            std::error_code ec;
            std::filesystem::remove(entry.path(), ec);
        }
    }

    if (binary == m_binaryPath)
        return 0;

    boost::process::child c(binary.string());
    std::error_code ec;
    if (c.wait_for(std::chrono::seconds(5), ec)) {
        return c.exit_code() != 0 ? -1 : 1;
    }

    c.detach();
    return 1;
}

bool ResourceManager::launchFailsafe() {
    static bool launched = false;
    if (launched || m_failsafe)
        return false;

    // check if has failsafe
    std::ifstream file(m_binaryPath.string(), std::ios::binary);
    if (!file.is_open())
        return false;

    std::string buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (buffer.size() < 1024 * 1024) // less then 1 MB
        return false;

    std::string toFind = { 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x01 }; // zip header
    toFind[toFind.size() - 1] = 0; // otherwhise toFind would find toFind in buffer
    size_t pos = buffer.find(toFind);
    if (pos == std::string::npos)
        return false;

    launched = true;
    boost::process::spawn(m_binaryPath.string(), "--failsafe");
    stdext::millisleep(100);

    return true;
}

bool ResourceManager::setupWriteDir(const std::string& product, const std::string& app) {
    const char* localDir = PHYSFS_getPrefDir(product.c_str(), app.c_str());
    if (!localDir) {
        g_logger.fatal(stdext::format("Unable to get local dir, error: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
        return false;
    }

    if (!PHYSFS_mount(localDir, NULL, 0)) {
        g_logger.fatal(stdext::format("Unable to mount local directory '%s': %s", localDir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
        return false;
    }

    if (!PHYSFS_setWriteDir(localDir)) {
        g_logger.fatal(stdext::format("Unable to set write dir '%s': %s", localDir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
        return false;
    }
    m_writeDir = std::filesystem::path(std::filesystem::u8path(localDir));
    return true;
}

bool ResourceManager::setup(const std::string& existentFile)
{
    // search for modules directory
    std::string localDir(PHYSFS_getWriteDir());
    std::vector<std::string> possiblePaths = { localDir, g_platform.getCurrentDir()};
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
        if (!PHYSFS_mount(path.c_str(), NULL, 0)) {
            continue;
        }
        if (!PHYSFS_exists(existentFile.c_str())) {
            PHYSFS_unmount(path.c_str());
            continue;
        }
        if (!PHYSFS_unmount(path.c_str())) {
            continue;
        }

        if (dir != localDir) {
            if (!PHYSFS_mount(dir.c_str(), NULL, 0)) {
                continue;
            }
        }

        PHYSFS_File* file = PHYSFS_openRead("data.zip");
        if (!file) {
            g_logger.fatal(stdext::format("Can't open data.zip"));
            return false;
        }

        m_memoryDataBufferSize = PHYSFS_fileLength(file);
        if (m_memoryDataBufferSize > 0) {
            m_memoryDataBuffer = new char[m_memoryDataBufferSize];
            PHYSFS_readBytes(file, m_memoryDataBuffer, m_memoryDataBufferSize);
        }
        PHYSFS_close(file);     
        if (dir != localDir)
            PHYSFS_unmount(dir.c_str());

        if (PHYSFS_mountMemory(m_memoryDataBuffer, m_memoryDataBufferSize, [](void* pointer) { delete[](char*)pointer; }, "memory_data.zip", NULL, 0)) {
            if (PHYSFS_exists(existentFile.c_str())) {
                m_dataDir = dir;
                m_loadedFromArchive = true;
                g_logger.debug(stdext::format("Found work dir at '%s'", path.c_str()));
                return true;
            }
            PHYSFS_unmount("memory_data.zip");
        } 
        delete[] m_memoryDataBuffer;
    }

    if (loadDataFromSelf(existentFile)) {
        return true;
    }

    g_logger.fatal("Unable to find working directory (or data.zip)");
    return false;
}

std::string ResourceManager::getCompactName(const std::string& existentFile) {
    // search for modules directory
    std::vector<std::string> possiblePaths = { g_platform.getCurrentDir() };
    const char* baseDir = PHYSFS_getBaseDir();
    if (baseDir)
        possiblePaths.push_back(baseDir);

    std::string fileData;
    if (loadDataFromSelf(existentFile)) {
        fileData = readFileContents(existentFile);
        PHYSFS_unmount("memory_data.zip");
        m_loadedFromMemory = false;
        m_loadedFromArchive = false;
    }

    if (fileData.empty()) {
        try {
            for (const std::string& dir : possiblePaths) {
                if (!PHYSFS_mount(dir.c_str(), NULL, 0))
                    continue;

                if (PHYSFS_exists(existentFile.c_str())) {
                    fileData = readFileContents(existentFile);
                    PHYSFS_unmount(dir.c_str());
                    break;
                }
                PHYSFS_unmount(dir.c_str());
            }
        } catch (...) {}
    }

    if (fileData.empty()) {
        try {
            for (const std::string& dir : possiblePaths) {
                std::string path = dir + "/data.zip";
                if (!PHYSFS_mount(path.c_str(), NULL, 0))
                    continue;

                if (PHYSFS_exists(existentFile.c_str())) {
                    fileData = readFileContents(existentFile);
                    PHYSFS_unmount(path.c_str());
                    break;
                }
                PHYSFS_unmount(path.c_str());
            }
        } catch (...) {}
    }

    std::smatch regex_match;
    if (std::regex_search(fileData, regex_match, std::regex("APP_NAME[^\"]+\"([^\"]+)"))) {
        if (regex_match.size() == 2 && regex_match[1].str().length() > 0 && regex_match[1].str().length() < 30) {
            return regex_match[1].str();
        }
    }
    return "otclientv8";
}

bool ResourceManager::loadDataFromSelf(const std::string& existentFile) {
    std::ifstream file(m_binaryPath.string(), std::ios::binary);
    if (!file.is_open())
        return false;

    std::string buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (buffer.size() < 1024 * 1024) // less then 1 MB
        return false;

    std::string toFind = { 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x01 }; // zip header
    toFind[toFind.size() - 1] = 0; // otherwhise toFind would find toFind in buffer
    size_t pos = buffer.find(toFind);
    if (pos == std::string::npos)
        return false;

    m_memoryDataBufferSize = buffer.size() - pos;
    if (m_memoryDataBufferSize < 128 || m_memoryDataBufferSize > 512 * 1024 * 1024) // max 512MB
        return false;

    m_memoryDataBuffer = new char[m_memoryDataBufferSize];
    memcpy(m_memoryDataBuffer, &buffer[pos], m_memoryDataBufferSize);
    if (PHYSFS_mountMemory(m_memoryDataBuffer, m_memoryDataBufferSize, [](void* pointer) { delete[] (char*)pointer; }, "memory_data.zip", NULL, 0)) {
        if (PHYSFS_exists(existentFile.c_str())) {
            g_logger.debug("Found work dir in memory");
            m_loadedFromMemory = true;
            m_loadedFromArchive = true;
            return true;
        }
        PHYSFS_unmount("memory_data.zip");
        return false;
    }
    
    delete[] m_memoryDataBuffer;
    return false;
}

bool ResourceManager::fileExists(const std::string& fileName)
{
    if (fileName.find("/downloads") != std::string::npos) {
        return g_http.getFile(fileName.substr(10)) != nullptr;
    }

    return (PHYSFS_exists(resolvePath(fileName).c_str()) && !PHYSFS_isDirectory(resolvePath(fileName).c_str()));
}

bool ResourceManager::directoryExists(const std::string& directoryName)
{
    if (directoryName == "/downloads")
        return true;
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

std::string ResourceManager::readFileContents(const std::string& fileName, bool safe)
{
    std::string fullPath = resolvePath(fileName);
    
    if (fullPath.find("/downloads") != std::string::npos) {
        auto dfile = g_http.getFile(fullPath.substr(10));
        if (dfile)
            return std::string(dfile->response.begin(), dfile->response.end());
    }

    PHYSFS_File* file = PHYSFS_openRead(fullPath.c_str());
    if(!file)
        stdext::throw_exception(stdext::format("unable to open file '%s': %s", fullPath, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

    int fileSize = PHYSFS_fileLength(file);
    std::string buffer(fileSize, 0);
    PHYSFS_readBytes(file, (void*)&buffer[0], fileSize);
    PHYSFS_close(file);

    if (safe) {
        return buffer;
    }

    static std::string unencryptedExtensions[] = { ".otml", ".otmm", ".dmp", ".log", ".dll", ".exe", ".zip" };

    if (!decryptBuffer(buffer)) {
        bool ignore = (customEncryption == 0);
        for (auto& it : unencryptedExtensions) {
            if (fileName.find(it) == fileName.size() - it.size()) {
                ignore = true;
            }
        }
        if(!ignore)
            g_logger.fatal(stdext::format("unable to decrypt file: %s", fullPath));
    }

    return buffer;
}

bool ResourceManager::writeFileBuffer(const std::string& fileName, const uchar* data, uint size)
{
    PHYSFS_file* file = PHYSFS_openWrite(fileName.c_str());
    if(!file) {
        g_logger.error(stdext::format("unable to open file for writing '%s': %s", fileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
        return false;
    }

    PHYSFS_writeBytes(file, (void*)data, size);
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
        stdext::throw_exception(stdext::format("failed to append file '%s': %s", fileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
    return FileStreamPtr(new FileStream(fileName, file, true));
}

FileStreamPtr ResourceManager::createFile(const std::string& fileName)
{
    PHYSFS_File* file = PHYSFS_openWrite(fileName.c_str());
    if(!file)
        stdext::throw_exception(stdext::format("failed to create file '%s': %s", fileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
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

    if (!rc)
        return files;

    for (int i = 0; rc[i] != NULL; i++) {
        if(fullPath)
            files.push_back(path + "/" + rc[i]);
        else
            files.push_back(rc[i]);
    }

    PHYSFS_freeList(rc);
    files.sort();
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
    static std::map<std::string, std::string> cache;

    auto it = cache.find(path);
    if (it != cache.end())
        return it->second;

    PHYSFS_File* file = PHYSFS_openRead(path.c_str());
    if(!file)
        return "";

    int fileSize = PHYSFS_fileLength(file);
    std::string buffer(fileSize, 0);
    PHYSFS_readBytes(file, (void*)&buffer[0], fileSize);
    PHYSFS_close(file);

    auto checksum = g_crypt.md5Encode(buffer, false);
    cache[path] = checksum;

    return checksum;
}

std::string ResourceManager::selfChecksum() {
    std::ifstream file(m_binaryPath.string(), std::ios::binary);
    if (!file.is_open())
        return "";

    std::string buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    return g_crypt.md5Encode(buffer, false);
}

std::string ResourceManager::readCrashLog(bool txt) 
{
    try {
        return readFileContents(txt ? "/crashreport.log" : "/exception.dmp");
    } catch (stdext::exception&) {
    }
    return "";
}

void ResourceManager::deleteCrashLog() 
{
    deleteFile("/exception.dmp");
    deleteFile("/crashreport.log");
}

void ResourceManager::updateClient(const std::vector<std::string>& files, std::string binaryName) {
    if (!m_loadedFromArchive)
        return g_logger.fatal("Client can be updated only while running from archive (data.zip)");
    if (!binaryName.empty() && binaryName[0] == '/')
        binaryName = binaryName.substr(1);

    auto downloads = g_http.downloads();

    if(!m_memoryDataBuffer || m_memoryDataBufferSize < 1024)
        return g_logger.fatal(stdext::format("Invalid buffer of memory data.zip"));

    g_logger.info(stdext::format("Updating client, buffer size %i", m_memoryDataBufferSize));

    zip_source_t *src;
    zip_t *za;
    zip_error_t error;
    zip_error_init(&error);

    if ((src = zip_source_buffer_create(m_memoryDataBuffer, m_memoryDataBufferSize, 0, &error)) == NULL)
        return g_logger.fatal(stdext::format("can't create source: %s", zip_error_strerror(&error)));

    if ((za = zip_open_from_source(src, 0, &error)) == NULL)
        return g_logger.fatal(stdext::format("can't open zip from source: %s", zip_error_strerror(&error)));

    zip_error_fini(&error);
    zip_source_keep(src);
    bool newFiles = false;
    for (auto file : files) {
        if (file.empty())
            continue;
        if (file.size() > 1 && file[0] == '/')
            file = file.substr(1);
        auto it = downloads.find(file);
        if (it == downloads.end())
            continue;
        if (file == binaryName)
            continue;
        zip_source_t *s; 
        if((s=zip_source_buffer(za, it->second->response.data(), it->second->response.size(), 0)) == NULL)
            return g_logger.fatal(stdext::format("can't create source buffer: %s", zip_strerror(za)));
        if(zip_file_add(za, file.c_str(), s, ZIP_FL_OVERWRITE) < 0)
            return g_logger.fatal(stdext::format("can't add file %s to zip archive: %s", file, zip_strerror(za)));
        newFiles = true;
    }

    if (zip_close(za) < 0)
        return g_logger.fatal(stdext::format("can't close zip archive: %s", zip_strerror(za)));

    zip_stat_t zst;
    if (zip_source_stat(src, &zst) < 0)
        return g_logger.fatal(stdext::format("can't stat source: %s", zip_error_strerror(zip_source_error(src))));
    
    size_t zipSize = zst.size;    

    if (zip_source_open(src) < 0)
        return g_logger.fatal(stdext::format("can't open source: %s", zip_error_strerror(zip_source_error(src))));

    if (newFiles) {
        PHYSFS_file* file = PHYSFS_openWrite("data.zip");
        if (!file)
            return g_logger.fatal(stdext::format("can't open data.zip for writing: %s", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));

        const size_t chunk_size = 1024 * 1024;
        std::vector<char> chunk(chunk_size);
        while (zipSize > 0) {
            size_t currentChunk = std::min<size_t>(zipSize, chunk_size);
            if ((zip_uint64_t)zip_source_read(src, chunk.data(), currentChunk) < currentChunk)
                return g_logger.fatal(stdext::format("can't read data from source: %s", zip_error_strerror(zip_source_error(src))));
            PHYSFS_writeBytes(file, chunk.data(), currentChunk);
            zipSize -= currentChunk;
        }

        PHYSFS_close(file);
    }
    zip_source_close(src);
    zip_source_free(src);

    if (!binaryName.empty()) {
        auto it = downloads.find(binaryName);
        if (it == downloads.end())
            return g_logger.fatal("Can't find new binary data in downloads");

        std::filesystem::path path(binaryName);
        auto newBinary = path.stem().string() + "-" + std::to_string(time(nullptr)) + path.extension().string();
        PHYSFS_file* file = PHYSFS_openWrite(newBinary.c_str());
        if (!file)
            return g_logger.fatal(stdext::format("can't open %s for writing: %s", newBinary, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())));
        PHYSFS_writeBytes(file, it->second->response.data(), it->second->response.size());
        PHYSFS_close(file);

        std::filesystem::path newBinaryPath(std::filesystem::u8path(PHYSFS_getWriteDir()));
        installDlls(newBinaryPath);
        newBinaryPath /= newBinary;
        stdext::millisleep(250);
        boost::process::spawn(newBinaryPath.string());
        stdext::millisleep(250);
        return;
    }    

    stdext::millisleep(250);
    boost::process::spawn(m_binaryPath.string());
    stdext::millisleep(250);
}

#ifdef WITH_ENCRYPTION
void ResourceManager::encrypt(const std::string& seed) {
    const std::string dirsToCheck[] = { "data", "modules", "mods" };
    const std::string luaExtension = ".lua";

    g_logger.setLogFile("encryption.log");
    g_logger.info("----------------------");

    std::queue<std::filesystem::path> toEncrypt;
    // you can add custom files here
    toEncrypt.push(std::filesystem::path("init.lua"));

    for (auto& dir : dirsToCheck) {
        if (!std::filesystem::exists(dir))
            continue;
        for(auto&& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path(dir))) {
            if (!std::filesystem::is_regular_file(entry.path()))
                continue;
            std::string str(entry.path().string());
            // skip encryption for bot configs
            if (str.find("game_bot") != std::string::npos && str.find("default_config") != std::string::npos) {
                continue;
            }
            toEncrypt.push(entry.path());
        }
    }

    uint32_t uintseed = seed.empty() ? 0 : stdext::adler32((const uint8_t*)seed.c_str(), seed.size());

    while (!toEncrypt.empty()) {
        auto it = toEncrypt.front();
        toEncrypt.pop();
        std::ifstream in_file(it, std::ios::binary);
        if (!in_file.is_open())
            continue;
        std::string buffer(std::istreambuf_iterator<char>(in_file), {});
        in_file.close();
        if (buffer.size() >= 4 && buffer.substr(0, 4).compare("ENC3") == 0)
            continue; // already encrypted

        if (it.extension().string() == luaExtension && it.filename().string() != "init.lua") {
            std::string bytecode = g_lua.generateByteCode(buffer, it.string());
            if (bytecode.length() > 10) {
                buffer = bytecode;
                g_logger.info(stdext::format("%s - lua bytecode encrypted", it.string()));
            } else {
                g_logger.info(stdext::format("%s - lua but not bytecode encrypted", it.string()));
            }
        }

        if (!encryptBuffer(buffer, uintseed)) { // already encrypted
            g_logger.info(stdext::format("%s - already encrypted", it.string()));
            continue;
        }

        std::ofstream out_file(it, std::ios::binary);
        if (!out_file.is_open())
            continue;
        out_file.write(buffer.data(), buffer.size());
        out_file.close();
        g_logger.info(stdext::format("%s - encrypted", it.string()));
    }
}
#endif 

bool ResourceManager::decryptBuffer(std::string& buffer) {
    if (buffer.size() < 5)
        return true;
    if (buffer.substr(0, 4).compare("ENC3") != 0) {
        return false;
    }

    uint64_t key = *(uint64_t*)&buffer[4];
    uint32_t compressed_size = *(uint32_t*)&buffer[12];
    uint32_t size = *(uint32_t*)&buffer[16];
    uint32_t adler = *(uint32_t*)&buffer[20];

    if (compressed_size < buffer.size() - 24)
        return false;

    g_crypt.bdecrypt((uint8_t*)&buffer[24], compressed_size, key);
    std::string new_buffer;
    new_buffer.resize(size);
    unsigned long new_buffer_size = new_buffer.size();
    if (uncompress((uint8_t*)new_buffer.data(), &new_buffer_size, (uint8_t*)&buffer[24], compressed_size) != Z_OK)
        return false;

    uint32_t addlerCheck = stdext::adler32((const uint8_t*)&new_buffer[0], size);
    if (adler != addlerCheck) {
        uint32_t cseed = adler ^ addlerCheck;
        if (customEncryption == 0) {
            customEncryption = cseed;
        }
        if ((addlerCheck ^ customEncryption) != adler) {
            return false;
        }
    }

    buffer = new_buffer;
    return true;
}

#ifdef WITH_ENCRYPTION
bool ResourceManager::encryptBuffer(std::string& buffer, uint32_t seed) {
    if (buffer.size() >= 4 && buffer.substr(0, 4).compare("ENC3") == 0)
        return false; // already encrypted

    // not random beacause it would require to update to new files each time
    int64_t key = stdext::adler32((const uint8_t*)&buffer[0], buffer.size());
    key <<= 32;
    key += stdext::adler32((const uint8_t*)&buffer[0], buffer.size() / 2);

    std::string new_buffer(24 + buffer.size() * 2, '0');
    new_buffer[0] = 'E';
    new_buffer[1] = 'N';
    new_buffer[2] = 'C';
    new_buffer[3] = '3';

    unsigned long dstLen = new_buffer.size() - 24;
    if (compress((uint8_t*)&new_buffer[24], &dstLen, (const uint8_t*)buffer.data(), buffer.size()) != Z_OK) {
        g_logger.error("Error while compressing");
        return false;
    }
    new_buffer.resize(24 + dstLen);

    *(int64_t*)&new_buffer[4] = key;
    *(uint32_t*)&new_buffer[12] = (uint32_t)dstLen;
    *(uint32_t*)&new_buffer[16] = (uint32_t)buffer.size();
    *(uint32_t*)&new_buffer[20] = ((uint32_t)stdext::adler32((const uint8_t*)&buffer[0], buffer.size())) ^ seed;

    g_crypt.bencrypt((uint8_t*)&new_buffer[0] + 24, new_buffer.size() - 24, key);
    buffer = new_buffer;
    return true;
}
#endif

void ResourceManager::installDlls(std::filesystem::path dest)     
{
#ifdef WIN32
    static std::list<std::string> dlls = {
        {"libEGL.dll"},
        {"libGLESv2.dll"},
        {"d3dcompiler_46.dll"}
    };

    int added_dlls = 0;
    for (auto& dll : dlls) {
        auto dll_path = m_binaryPath.parent_path();
        dll_path /= dll;
        if (!std::filesystem::exists(dll_path)) {
            continue;
        }
        auto out_path = dest;
        out_path /= dll;
        if (std::filesystem::exists(out_path)) {
            continue;
        }
        std::filesystem::copy_file(dll_path, out_path);
    }
#endif
}
