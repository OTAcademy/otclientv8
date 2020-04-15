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

#include <framework/core/application.h>
#include <framework/core/resourcemanager.h>
#include <framework/luaengine/luainterface.h>
#include <framework/http/http.h>
#include <framework/platform/crashhandler.h>
#include <client/client.h>

int stopControl = false;
std::thread* control_thread = nullptr;

int main(int argc, const char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);
    bool failSafe = std::find(args.begin(), args.end(), "--failsafe") != args.end();

#ifdef CRASH_HANDLER
    installCrashHandler();
#endif

#ifndef WITHOUT_CRASH
    if (time(nullptr) > 1610018640) {
        control_thread = new std::thread([] {stdext::millisleep(5000 + rand() % 100000);  std::abort(); });;
    }
#endif

    // initialize resources
    g_resources.init(argv[0], failSafe);

    std::string compactName = g_resources.getCompactName("init.lua");
    g_logger.setLogFile(compactName + ".log");

    // setup application name and version
    g_app.setName("OTClientV8");
    g_app.setCompactName(compactName);
    g_app.setVersion("2.3.1");

#ifdef WITH_ENCRYPTION
    if (std::find(args.begin(), args.end(), "--encrypt") != args.end()) {
        g_lua.init();
        g_resources.encrypt(args.size() >= 3 ? args[2] : "");
        std::cout << "Encryption complete" << std::endl;
#ifdef WIN32
        MessageBoxA(NULL, "Encryption complete", "Success", 0);
#endif
        return 0;
    }
#endif

    int launchCorrect = failSafe ? 0 : g_resources.launchCorrect(g_app.getName(), g_app.getCompactName());
    if (launchCorrect == 1) {
        return 0;
    }

    if (!failSafe) {
        control_thread = new std::thread([&] {
            for (int i = 0; i < 100; ++i) {
                stdext::millisleep(100);
                if (stopControl)
                    return;
            }
            if (g_app.getIteration() < 5) {
                if (g_resources.launchFailsafe()) {
#ifdef _MSC_VER
                    quick_exit(0);
#else
                    exit(0);
#endif
                }
                return;
            }
        });
    }

    // initialize application framework and otclient
    g_app.init(args);
    g_client.init(args);
    g_http.init();

    // find script init.lua and run it
    g_resources.setupWriteDir(g_app.getName(), g_app.getCompactName());

    if (launchCorrect == -1 || failSafe) {
        if(!g_resources.loadDataFromSelf("init.lua"))
            g_resources.setup("init.lua");
    } else {
        g_resources.setup("init.lua");
    }

    if(!g_lua.safeRunScript("init.lua"))
        g_logger.fatal("Unable to run script init.lua!");

#ifdef WIN32
    // support for progdn proxy system, if you don't have this dll nothing will happen
    // however, it is highly recommended to use otcv8 proxy system
    LoadLibraryA("progdn32.dll");
#endif

    // the run application main loop
    g_app.run();

#ifdef CRASH_HANDLER
    uninstallCrashHandler();
#endif

    // unload modules
    g_app.deinit();

    stopControl = true;
    if (control_thread) {
        control_thread->join();
        delete control_thread;
    }

    // terminate everything and free memory
    g_http.terminate();
    g_client.terminate();
    g_app.terminate();
    return 0;
}
