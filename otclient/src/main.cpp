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
#include <client/client.h>

int main(int argc, const char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    // setup application name and version
    g_app.setName("OTClientV8");
    g_app.setCompactName("otclientv8");
    g_app.setVersion("0.1 alpha");

    // initialize resources
    g_resources.init(args[0].c_str());

    if (std::find(args.begin(), args.end(), "--encrypt") != args.end()) {
        g_resources.encrypt();
        return 0;
    }
    if (g_resources.launchCorrect(g_app.getCompactName())) {
        return 0;
    }

    // initialize application framework and otclient
    g_app.init(args);
    g_client.init(args);
    g_http.init();
    //g_stats.init();

    // find script init.lua and run it
    g_resources.setup(g_app.getCompactName(), "init.lua");

    if(!g_lua.safeRunScript("init.lua"))
        g_logger.fatal("Unable to run script init.lua!");

    // the run application main loop
    g_app.run();

    // unload modules
    g_app.deinit();

    // terminate everything and free memory
    g_http.terminate();
    g_client.terminate();
    g_app.terminate();
    return 0;
}
