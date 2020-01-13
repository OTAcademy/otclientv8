actionPanel = nil

function init()
  local bottomPanel = modules.game_interface.getBottomPanel()
  actionPanel = g_ui.loadUI('actionbar', bottomPanel)
  bottomPanel:moveChildToIndex(actionPanel, 1)
  
  for i=1,60 do
    actionPanel.tabBar:addButton("exura\nvita")
  end
  
  if not g_settings.getBoolean("actionBar", false) then
    hide()
  end
end

function terminate()
  actionPanel:destroy()
end

function show()
  actionPanel:setOn(true)
end

function hide()
  actionPanel:setOn(false)
end

function switchMode(newMode)
  if newMode then
    actionPanel:setImageColor('#ffffff88')  
  else
    actionPanel:setImageColor('white')    
  end
end