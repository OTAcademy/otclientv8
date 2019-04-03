-- private variables

local background
local clientVersionLabel
local newsPanelHolder
local newsPanel
local getNewsId = 0
local updateNewsEvent = nil
local ongoingNewsUpdate = false
local updateTries = 0
local lastNewsUpdate = 0
local images = {}
local newsImageId = 0

local function onGet(operationId, url, err, data)
  if operationId ~= getNewsId then
    return
  end
  if err:len() > 0 then
    gotNewsError(err)
  else
    gotNews(data)
  end
end

-- public functions
function init()
  background = g_ui.displayUI('background')
  background:lower()

  clientVersionLabel = background:getChildById('clientVersionLabel')
  clientVersionLabel:setText(g_app.getName() .. ' ' .. g_app.getVersion() .. '\n' .. g_app.getAuthor())
  
  newsPanelHolder = background:recursiveGetChildById('newsPanelHolder')
  newsPanel = newsPanelHolder:recursiveGetChildById('newsPanel')

  if not g_game.isOnline() then
    addEvent(function() g_effects.fadeIn(clientVersionLabel, 1500) end)
  end

  connect(g_game, { onGameStart = hide })
  connect(g_game, { onGameEnd = show })
  connect(background, { onGeometryChange = updateSize })
  connect(g_http, { onGet = onGet, onDownload = onDownload })
  
  updateNews()
end

function terminate()
  disconnect(g_game, { onGameStart = hide })
  disconnect(g_game, { onGameEnd = show })
  disconnect(background, { onGeometryChange = updateSize })
  disconnect(g_http, { onGet = onGet, onDownload = onDownload })
  
  if updateNewsEvent ~= nil then
	  removeEvent(updateNewsEvent)
	  updateNewsEvent = nil
  end  
  
  clearNews()

  g_effects.cancelFade(background:getChildById('clientVersionLabel'))
  background:destroy()

  Background = nil
end

function hide()
  background:hide()
end

function show()
  background:show()
  if ongoingNewsUpdate == false and os.time() > lastNewsUpdate + 30 then
    updateNews()
  end
end

function hideVersionLabel()
  background:getChildById('clientVersionLabel'):hide()
end

function setVersionText(text)
  clientVersionLabel:setText(text)
end

function getBackground()
  return background
end

function updateSize() 
  if Services.news == nil or Services.news:len() < 4 then
    return
  end
  if(background:getWidth() < 790) then
    newsPanelHolder:hide()
  else
    newsPanelHolder:show()
  end
  newsPanelHolder:setWidth(math.min(math.max(250, background:getWidth() / 4), 300)) 
end

function updateNews()
  if ongoingNewsUpdate == true then
    return
  end  
  if Services.news == nil or Services.news:len() < 4 then
    newsPanelHolder:hide()
    return
  end
  ongoingNewsUpdate = true
  updateTries = 0
  getNewsId = g_http.get(Services.news .. "?lang=" .. modules.client_locales.getCurrentLocale().name)
  lastNewsUpdate = os.time()
end

function clearNews()
  while newsPanel:getChildCount() > 0 do
    local child = newsPanel:getLastChild()
    newsPanel:destroyChildren(child)
  end
end

function gotNews(data) 
  local status, result = pcall(function() return json.decode(data) end)
  if not status then
    return gotNewsError("Error while parsing json from server:\n" .. result .. "\n" .. data) 
  end
  
  clearNews()
  
  for i, news in pairs(result) do
    local title = news["title"]
    local text = news["text"]
    local image = news["image"]
    if title ~= nil then
      newsLabel = g_ui.createWidget('NewsLabel', newsPanel)
      newsLabel:setText(title)
    end
    if text ~= nil then
      newsText = g_ui.createWidget('NewsText', newsPanel)  
      newsText:setText(text)
    end
    if image ~= nil then
      newsImage = g_ui.createWidget('NewsImage', newsPanel)
      newsImage:setId(imageName)
      newsImage:setImageSourceBase64(image)
      newsImage:setImageFixedRatio(true)
      newsImage:setImageAutoResize(true)
      newsImage:setHeight(200)
    end
  end
  
  ongoingNewsUpdate = false
  
end

function gotNewsError(err)
  if updateTries < 3 then
    updateTries = updateTries + 1
    ongoingNewsUpdate = scheduleEvent(function() 
      getNewsId = g_http.get(Services.news .. "?lang=" .. modules.client_locales.getCurrentLocale().name)
      ongoingNewsUpdate = nil
    end, 1000)
    return
  end
  
  clearNews()
  
  errorLabel = g_ui.createWidget('NewsLabel', newsPanel)
  errorLabel:setText(tr("Error"))
  errorInfo = g_ui.createWidget('NewsText', newsPanel)  
  errorInfo:setText(err)
  ongoingNewsUpdate = false
end