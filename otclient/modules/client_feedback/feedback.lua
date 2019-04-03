local feedbackWindow
local textEdit
local okButton
local cancelButton
local postId = 0
local tries = 0
local replyEvent = nil

local function onPost(operationId, url, err, data)
  if operationId ~= postId then
    return
  end
  if err:len() > 0 then
    print("Sending feedback error: " .. err)
    tries = tries + 1
    if tries < 3 then 
      replyEvent = scheduleEvent(send, 1000)
    end
  else
    print("Feedback has been sent!")
  end
end

function init()
  feedbackWindow = g_ui.displayUI('feedback')
  feedbackWindow:hide()

  textEdit = feedbackWindow:getChildById('text')
  okButton = feedbackWindow:getChildById('okButton')
  cancelButton = feedbackWindow:getChildById('cancelButton')

  okButton.onClick = send
  cancelButton.onClick = hide
  feedbackWindow.onEscape = hide  
  
  connect(g_http, {onPost = onPost})
end

function terminate()
  feedbackWindow:destroy()
  disconnect(g_http, {onPost = onPost})
  if replyEvent ~= nil then
    removeEvent(replyEvent)
  end
end

function show()
  feedbackWindow:show()
  feedbackWindow:raise()
  feedbackWindow:focus()
  
  textEdit:setMaxLength(8192)
  textEdit:setText('')
  textEdit:setEditable(true)
  textEdit:setCursorVisible(true)
  feedbackWindow:focusChild(textEdit, KeyboardFocusReason)
  
  tries = 0
end

function hide()
  feedbackWindow:hide()
  textEdit:setEditable(false)
  textEdit:setCursorVisible(false)
end

function send()
  local text = textEdit:getText()
  if text:len() > 1 then
    local localPlayer = g_game.getLocalPlayer()
    local playerData = nil
    if localPlayer ~= nil then
      playerData = {
        name = localPlayer:getName(),
        position = localPlayer:getPosition()
      }
    end
    local data = json.encode({
      text = text,
      version = g_app.getVersion(),
      host = g_settings.get('host'),
      player = playerData
    })
    postId = g_http.post(Services.feedback, data)
  end 
  hide()
end