-- this is the first file executed when the application starts
-- we have to load the first modules form here

-- setup logger
g_logger.setLogFile(g_app.getCompactName() .. ".log")
g_logger.info(os.date("== application started at %b %d %Y %X"))

-- print first terminal message
g_logger.info(g_app.getName() .. ' ' .. g_app.getVersion() .. ' rev ' .. g_app.getBuildRevision() .. ' (' .. g_app.getBuildCommit() .. ') made by ' .. g_app.getAuthor() .. ' built on ' .. g_app.getBuildDate() .. ' for arch ' .. g_app.getBuildArch())

-- add data directory to the search path
if not g_resources.directoryExists("/data") then
  g_logger.fatal("Data dir doesn't exist.")
end

-- add modules directory to the search path
if not g_resources.directoryExists("/modules") then
  g_logger.fatal("Modules dir doesn't exist.")
end

Services = {
  website = "http://otclient.ovh", -- currently not used
  updater = "http://otclient.ovh/api/updater.php",
  news = "http://otclient.ovh/api/news.php",
--  newLogin = "http://otclient.ovh/api/newlogin.php", -- password less login, will be added in future (may)
  stats = "http://otclient.ovh/api/stats.php",
  crash = "http://otclient.ovh/api/crash.php",
  feedback = "http://otclient.ovh/api/feedback.php"
}

Servers = {
  MasterBaiak = "http://baiak-master.com/login.php",
  OTClientV8 = "http://otclient.ovh/api/login.php",
  Wartera = "https://wartera.online/login.php",
  KasteriaTest = "https://www.xavato.eu/login.php",
  Kasteria = "http://login.otupdate.ovh/login.php",
  Xavato = "https://www.xavato.eu/login2.php",
  TestBloodWar = "http://168.228.157.20/login.php"
}

-- settings
g_configs.loadSettings("/config.otml")

-- load mods
g_modules.discoverModules()

-- libraries modules 0-99
g_modules.autoLoadModules(99)
g_modules.ensureModuleLoaded("corelib")
g_modules.ensureModuleLoaded("gamelib")

-- client modules 100-499
g_modules.autoLoadModules(499)
g_modules.ensureModuleLoaded("client")

-- game modules 500-999
g_modules.autoLoadModules(999)
g_modules.ensureModuleLoaded("game_interface")

-- mods 1000-9999
g_modules.autoLoadModules(9999)
