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

#ifdef WIN32

#include "platform.h"
#include <winsock2.h>
#include <windows.h>
#include <framework/global.h>
#include <framework/stdext/stdext.h>
#include <framework/core/eventdispatcher.h>
#include <boost/algorithm/string.hpp>
#include <tchar.h>
#include <Psapi.h>
#include <iphlpapi.h>
#include <tlhelp32.h>

void Platform::processArgs(std::vector<std::string>& args)
{
    int nargs;
    wchar_t** wchar_argv = CommandLineToArgvW(GetCommandLineW(), &nargs);
    if (!wchar_argv)
        return;

    args.clear();
    if (wchar_argv) {
        for (int i = 0; i < nargs; ++i)
            args.push_back(stdext::utf16_to_utf8(wchar_argv[i]));
    }
}

bool Platform::spawnProcess(std::string process, const std::vector<std::string>& args)
{
    std::string commandLine;
    for (uint i = 0; i < args.size(); ++i)
        commandLine += stdext::format(" \"%s\"", args[i]);

    boost::replace_all(process, "/", "\\");
    if (!boost::ends_with(process, ".exe"))
        process += ".exe";

    std::wstring wfile = stdext::utf8_to_utf16(process);
    std::wstring wcommandLine = stdext::utf8_to_utf16(commandLine);

    if ((size_t)ShellExecuteW(NULL, L"open", wfile.c_str(), wcommandLine.c_str(), NULL, SW_SHOWNORMAL) > 32)
        return true;
    return false;
}

int Platform::getProcessId()
{
    return GetCurrentProcessId();
}

bool Platform::isProcessRunning(const std::string& name)
{
    if (FindWindowA(name.c_str(), NULL) != NULL)
        return true;
    return false;
}

bool Platform::killProcess(const std::string& name)
{
    HWND window = FindWindowA(name.c_str(), NULL);
    if (window == NULL)
        return false;
    DWORD pid = GetProcessId(window);
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (handle == NULL)
        return false;
    bool ok = TerminateProcess(handle, 1) != 0;
    CloseHandle(handle);
    return ok;
}

std::string Platform::getTempPath()
{
    std::string ret;
    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    ret = stdext::utf16_to_utf8(path);
    boost::replace_all(ret, "\\", "/");
    return ret;
}

std::string Platform::getCurrentDir()
{
    std::string ret;
    wchar_t path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, path);
    ret = stdext::utf16_to_utf8(path);
    boost::replace_all(ret, "\\", "/");
    ret += "/";
    return ret;
}

bool Platform::fileExists(std::string file)
{
    boost::replace_all(file, "/", "\\");
    std::wstring wfile = stdext::utf8_to_utf16(file);
    DWORD dwAttrib = GetFileAttributesW(wfile.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool Platform::copyFile(std::string from, std::string to)
{
    boost::replace_all(from, "/", "\\");
    boost::replace_all(to, "/", "\\");
    if (CopyFileW(stdext::utf8_to_utf16(from).c_str(), stdext::utf8_to_utf16(to).c_str(), FALSE) == 0)
        return false;
    return true;
}

bool Platform::removeFile(std::string file)
{
    boost::replace_all(file, "/", "\\");
    if (DeleteFileW(stdext::utf8_to_utf16(file).c_str()) == 0)
        return false;
    return true;
}

ticks_t Platform::getFileModificationTime(std::string file)
{
    boost::replace_all(file, "/", "\\");
    std::wstring wfile = stdext::utf8_to_utf16(file);
    WIN32_FILE_ATTRIBUTE_DATA fileAttrData;
    memset(&fileAttrData, 0, sizeof(fileAttrData));
    GetFileAttributesExW(wfile.c_str(), GetFileExInfoStandard, &fileAttrData);
    ULARGE_INTEGER uli;
    uli.LowPart = fileAttrData.ftLastWriteTime.dwLowDateTime;
    uli.HighPart = fileAttrData.ftLastWriteTime.dwHighDateTime;
    return uli.QuadPart;
}

bool Platform::openUrl(std::string url, bool now)
{
    if (now) {
        return (size_t)ShellExecuteW(NULL, L"open", stdext::utf8_to_utf16(url).c_str(), NULL, NULL, SW_SHOWNORMAL) >= 32;
    } else {
        g_dispatcher.scheduleEvent([url] {
            ShellExecuteW(NULL, L"open", stdext::utf8_to_utf16(url).c_str(), NULL, NULL, SW_SHOWNORMAL);
        }, 50);
    }
    return true;
}

bool Platform::openDir(std::string path, bool now)
{
    if (now) {
        return (size_t)ShellExecuteW(NULL, L"open", L"explorer.exe", stdext::utf8_to_utf16(path).c_str(), NULL, SW_SHOWNORMAL) >= 32;
    } else {
        g_dispatcher.scheduleEvent([path] {
            ShellExecuteW(NULL, L"open", L"explorer.exe", stdext::utf8_to_utf16(path).c_str(), NULL, SW_SHOWNORMAL);
        }, 50);
    }
    return true;
}

std::string Platform::getCPUName()
{
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    DWORD bufSize = sizeof(buf);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "";
    RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)buf, (LPDWORD)&bufSize);
    return buf;
}

double Platform::getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}

double Platform::getMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
}

std::string Platform::getOSName()
{
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    RTL_OSVERSIONINFOEXW osInfo = { 0 };
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);

    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll == NULL)
        return std::string();

    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtDll, "RtlGetVersion");
    if (RtlGetVersion == NULL)
        return std::string();

    if (RtlGetVersion((PRTL_OSVERSIONINFOW)&osInfo) != 0)
        return std::string();

    DWORD productType = 0;
    typedef BOOL(WINAPI* GetProductInfoPtr)(DWORD, DWORD, DWORD, DWORD, PDWORD);
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == NULL)
        return std::string();

    GetProductInfoPtr GetProductInfo = (GetProductInfoPtr)GetProcAddress(hKernel32, "GetProductInfo");

    if (GetProductInfo)
        GetProductInfo(osInfo.dwMajorVersion, osInfo.dwMinorVersion, 0, 0, &productType);

    std::string osName = "Windows ";
    if (osInfo.dwMajorVersion == 10) {
        if (osInfo.dwBuildNumber >= 22000)
            osName += "11";
        else
            osName += "10";
    }
    else if (osInfo.dwMajorVersion == 6) {
        if (osInfo.dwMinorVersion == 3)
            osName += "8.1";
        else if (osInfo.dwMinorVersion == 2)
            osName += "8";
        else if (osInfo.dwMinorVersion == 1)
            osName += "7";
        else if (osInfo.dwMinorVersion == 0) {
            if (osInfo.wProductType == VER_NT_WORKSTATION)
                osName += "Vista";
            else {
                osName += "Server 2008";
                switch (productType) {
                case PRODUCT_STANDARD_SERVER: osName += " Standard"; break;
                case PRODUCT_ENTERPRISE_SERVER: osName += " Enterprise"; break;
                case PRODUCT_DATACENTER_SERVER: osName += " Datacenter"; break;
                case PRODUCT_WEB_SERVER: osName += " Web Edition"; break;
                default: osName += " Edition";
                }
            }
        }
    }
    else if (osInfo.dwMajorVersion == 5) {
        if (osInfo.dwMinorVersion == 2)
            osName += "Server 2003/XP x64";
        else if (osInfo.dwMinorVersion == 1)
            osName += "XP";
        else
            osName += "2000";
    }
    else {
        osName += "Unknown";
    }

    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: {
        osName += ", 64-bit";
        break;
    }
    case PROCESSOR_ARCHITECTURE_INTEL: {
        osName += ", 32-bit";
        break;
    }
    case PROCESSOR_ARCHITECTURE_ARM64: {
        osName += ", ARM64";
        break;
    }
    case PROCESSOR_ARCHITECTURE_ARM: {
        osName += ", ARM";
        break;
    }
    default: {
        osName += ", Unknown Architecture";
        break;
    }
    }

    return osName;
}

std::string Platform::traceback(const std::string& where, int level, int maxDepth)
{
    std::stringstream ss;
    ss << "\nat:";
    ss << "\n\t[C++]: " << where;
    return ss.str();
}

std::vector<std::string> Platform::getMacAddresses()
{
    std::vector<std::string> ret;
    IP_ADAPTER_INFO AdapterInfo[32];
    DWORD dwBufLen = sizeof(AdapterInfo);

    DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
    if (dwStatus != ERROR_SUCCESS) {
        return ret;
    }

    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to  current adapter info
    do {
        char buffer[20];
        sprintf_s(buffer, sizeof(buffer), "%02x%02x%02x%02x%02x%02x%02x%02x", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2],
                  pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5], pAdapterInfo->Address[6], pAdapterInfo->Address[7]);
        ret.push_back(std::string(buffer));
        pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list
    } while (pAdapterInfo);                    // Terminate if last adapter
    std::sort(ret.begin(), ret.end());
    return ret;
}


std::string Platform::getUserName()
{
    char buffer[30];
    DWORD length = sizeof(buffer) - 1;
    GetUserNameA(buffer, &length);
    buffer[29] = 0; // just in case
    return std::string(buffer);
}

std::vector<std::string> Platform::getDlls()
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    std::vector<std::string> ret;
    HANDLE hProcess = GetCurrentProcess();
    if (!hProcess) 
        return ret;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char szModName[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hMods[i], szModName,
                                    sizeof(szModName) / sizeof(TCHAR))) {
                ret.push_back(szModName);
            }
        }
    }

    return ret;
}

std::vector<std::string> Platform::getProcesses()
{
    std::vector<std::string> ret;

    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return ret;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return ret;
    }

    do {
        ret.push_back(pe32.szExeFile);
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);

    return ret;
}

std::vector<std::string> windows;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    char title[50];
    GetWindowText(hwnd, title, sizeof(title));
    title[sizeof(title) - 1] = 0;
    std::string window_title(title);
    if (window_title.size() >= 2) {
        windows.push_back(window_title);
    }
    return TRUE;
}

std::vector<std::string> Platform::getWindows()
{
    windows.clear();
    EnumWindows(EnumWindowsProc, NULL);
    return windows;
}

#endif
