Updater = { }

Updater.maxRetries = 5

--[[
HOW IT WORKS:
1. init
2. show
3. generateChecksum and get checksums from url
4. compareChecksums
5. download files with different chekcums
6. call c++ update function
]]--

local getStatusId = 0
local downloadId = 0
local downloadRetries = 0
local filesUrl = ""

local updaterWindow = nil
local initialPanel = nil
local updatePanel = nil
local progressBar = nil
local updateProgressBar = nil
local downloadStatusLabel = nil
local downloadProgressBar = nil

local generateChecksumsEvent = nil
local updateableFiles = nil
local binaryChecksum = nil
local binaryFile = ""
local fileChecksums = {}
local checksumIter = 0
local downloadIter = 0
local aborted = false
local statusData = nil
local thingsUpdate = {}
local toUpdate = {}


local function onGet(operationId, url, err, data)
  if aborted then
    return
  end
  if(operationId == getStatusId) then
    gotStatus(data, err)
  end
end

local function onGetProgress(operationId, url, progress)

end

local function onDownload(operationId, url, err, path, checksum)
  if aborted then
    return
  end
  if downloadId == operationId then
    downloadedFile(path, err, checksum)
  end
end

local function onDownloadProgress(operationId, url, progress, speed)  
  if aborted then
    return
  end
  if downloadId == operationId then
    print("Download speed: " .. speed .. " kbps")
    downloadProgressBar:setPercent(progress)
  end
end

-- public functions
function Updater.init()
  updaterWindow = g_ui.displayUI('updater')
  updaterWindow:hide()
  
  initialPanel = updaterWindow:getChildById('initialPanel')
  updatePanel = updaterWindow:getChildById('updatePanel')
  progressBar = initialPanel:getChildById('progressBar')
  updateProgressBar = updatePanel:getChildById('updateProgressBar')
  downloadStatusLabel = updatePanel:getChildById('downloadStatusLabel')
  downloadProgressBar = updatePanel:getChildById('downloadProgressBar')
  
  updatePanel:hide()
  
  scheduleEvent(Updater.show, 50)
  
  connect(g_http, {
    onGet = onGet,
    onGetProgess = onGetProgress,
    onDownload = onDownload,
    onDownloadProgress = onDownloadProgress
  })
end

function Updater.terminate()
  disconnect(g_http, {
    onGet = onGet,
    onGetProgess = onGetProgress,
    onDownload = onDownload,
    onDownloadProgress = onDownloadProgress
  })
  updaterWindow:destroy()
  updaterWindow = nil
  
  if generateChecksumsEvent ~= nil then
	  removeEvent(generateChecksumsEvent)
	  generateChecksumsEvent = nil
  end  
end

local function clear()
  if generateChecksumsEvent ~= nil then
	  removeEvent(generateChecksumsEvent)
	  generateChecksumsEvent = nil
  end  
  updateableFiles = nil
  binaryChecksum = nil
  binaryFile = ""
  fileChecksums = {}
  checksumIter = 0
  downloadIter = 0
  aborted = false
  statusData = nil
  toUpdate = {}  
  progressBar:setPercent(0)
  updateProgressBar:setPercent(0)
  downloadProgressBar:setPercent(0)
end

function Updater.show()
  if not g_resources.isLoadedFromArchive() or Services.updater == nil or Services.updater:len() < 4 then
    Updater.hide()
    return EnterGame.firstShow()
  end
  updaterWindow:show()
  
  clear()
  
  updateableFiles = g_resources.listUpdateableFiles()
  if #updateableFiles < 1 then
    return updateError("Can't get list of files")
  end
  binaryChecksum = g_resources.selfChecksum():lower()
  if binaryChecksum:len() ~= 32 then
    return updateError("Invalid binary checksum: " .. binaryChecksum)  
  end
  
  getStatusId = g_http.get(Services.updater)  
  if generateChecksumsEvent == nil then
	  generateChecksumsEvent = scheduleEvent(generateChecksum, 5)
  end
end

function Updater.updateThings(things)
  thingsUpdate = things
  Updater:show()
end

function Updater.hide()
  updaterWindow:hide()
end

function Updater.abort()
  aborted = true
  Updater:hide()
  EnterGame.firstShow()
end

function generateChecksum()
  local entries = #updateableFiles
  local fromEntry = math.floor((checksumIter) * (entries / 100))
  local toEntry = math.floor((checksumIter + 1) * (entries / 100))
  if checksumIter == 99 then
    toEntry = #updateableFiles
  end
  for i=fromEntry+1,toEntry do
    local fileName = updateableFiles[i]
    fileChecksums[fileName] = g_resources.fileChecksum(fileName):lower()
  end
    
  checksumIter = checksumIter + 1
  if checksumIter == 100 then
    generateChecksumsEvent = nil
    gotChecksums()
  else
    progressBar:setPercent(math.ceil(checksumIter * 0.95))
    generateChecksumsEvent = scheduleEvent(generateChecksum, 5)
  end
end

function gotStatus(data, err)
  if err:len() > 0 then
    return updateError(err)
  end
  local status, result = pcall(function() return json.decode(data) end)
  if not status then
    return updateError("Error while parsing json from server:\n" .. result)  
  end
  statusData = result
  if statusData["url"] == nil or statusData["files"] == nil or statusData["binary"] == nil then
    return updateError("Invalid json data from server:\n" .. data)    
  end  
  if statusData["things"] ~= nil then
    for file, checksum in pairs(statusData["things"]) do
      for thingtype, thingdata in pairs(thingsUpdate) do
        if string.match(file:lower(), thingdata[1]:lower()) then
          statusData["files"][file] = checksum
          break
        end
      end    
    end
  end  
  if checksumIter == 100 then
    compareChecksums()
  end
end

function gotChecksums()
  if statusData ~= nil then
    compareChecksums()
  end
end

function compareChecksums()
  for file, checksum in pairs(statusData["files"]) do
    file = file
    checksum = checksum:lower()
    if file == statusData["binary"] then
      if binaryChecksum ~= checksum then
        binaryFile = file
        table.insert(toUpdate, binaryFile)
      end      
    else
      local localChecksum = fileChecksums[file]
      if localChecksum ~= checksum then
        table.insert(toUpdate, file)
      end
    end
  end
  if #toUpdate == 0 then
    return upToDate()
  end  
  -- outdated
  filesUrl = statusData["url"]
  initialPanel:hide()
  updatePanel:show()
  updatePanel:getChildById('updateStatusLabel'):setText(tr("Updating %i files", #toUpdate))
  updaterWindow:setHeight(190)
  downloadNextFile(false)
end

function upToDate()
  EnterGame.firstShow()
  Updater.hide()
end

function updateError(err)
  updaterWindow:hide()
  local msgbox = displayErrorBox("Updater error", err)
  msgbox.onOk = function() EnterGame.firstShow() end
end

function urlencode(url)
  url = url:gsub("\n", "\r\n")
  url = url:gsub("([^%w ])", function(c) string.format("%%%02X", string.byte(c)) end)
  url = url:gsub(" ", "+")
  return url
end

function downloadNextFile(retry) 
  if aborted then
    return
  end
  
  updaterWindow:show() -- fix window hide bug
  EnterGame.hide()
  
  if downloadIter == #toUpdate then    
    return downloadingFinished()
  end
  
  if retry then
    retry = " (" .. downloadRetries .. " retry)"
  else
    retry = ""
  end    
   
  local file = toUpdate[downloadIter + 1]
  downloadStatusLabel:setText(tr("Downloading %i of %i%s:\n%s", downloadIter + 1, #toUpdate, retry, file))
  downloadProgressBar:setPercent(0)
  downloadId = g_http.download(filesUrl .. urlencode(file), file)
end

function downloadedFile(path, err, checksum)
  if err:len() > 0 then
    if downloadRetries > Updater.maxRetries then
      return updateError("Can't download file: " .. path .. ".\nError: " .. err)
    else
      downloadRetries = downloadRetries + 1
      return downloadNextFile(true)
    end
  end
  if statusData["files"][path] == nil then
      return updateError("Invalid file path: " .. path)    
  elseif statusData["files"][path] ~= checksum then
      return updateError("Invalid file checksum.\nFile: " .. path .. "\nShould be:\n" .. statusData["files"][path] .. "\nIs:\n" .. checksum)  
  end
  downloadIter = downloadIter + 1
  updateProgressBar:setPercent(math.ceil((100 * downloadIter) / #toUpdate))
  downloadProgressBar:setPercent(100)
  downloadNextFile(false)
end

function downloadingFinished()
  UIMessageBox.display(tr("Success"), tr("Download complate.\nUpdating client..."), {}, nil, nil)  
  scheduleEvent(function()
      local files = {}
      for file, checksum in pairs(statusData["files"]) do
        table.insert(files, file)
      end
      g_resources.updateClient(files, binaryFile) 
      g_app.exit()
    end, 500)
end
