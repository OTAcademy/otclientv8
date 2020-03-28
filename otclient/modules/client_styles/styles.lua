function init()
  local files
  local loaded_files = {}
  local layout = g_resources:getLayout()

  if layout:len() > 0 then
    files = g_resources.listDirectoryFiles('/layouts/' .. layout .. '/styles')
    loaded_files = {}
    for _,file in pairs(files) do
      if g_resources.isFileType(file, 'otui') then
        g_ui.importStyle('/layouts/' .. layout .. '/styles/' .. file)
        loaded_files[file] = true
      end
    end
  end

  files = g_resources.listDirectoryFiles('/data/styles')
  for _,file in pairs(files) do
    if g_resources.isFileType(file, 'otui') and not loaded_files[file] then
      g_ui.importStyle('/data/styles/' .. file)
    end
  end

  if layout:len() > 0 then
    files = g_resources.listDirectoryFiles('/layouts/' .. layout .. '/fonts')
    loaded_files = {}
    for _,file in pairs(files) do
      if g_resources.isFileType(file, 'otfont') then
        g_ui.importFont('/layouts/' .. layout .. '/fonts/' .. file)
        loaded_files[file] = true
      end
    end
  end

  files = g_resources.listDirectoryFiles('/data/fonts')
  for _,file in pairs(files) do
    if g_resources.isFileType(file, 'otfont') and not loaded_files[file] then
      g_fonts.importFont('/data/fonts/' .. file)
    end
  end

  if layout:len() > 0 then
    files = g_resources.listDirectoryFiles('/layouts/' .. layout .. '/particles')
    loaded_files = {}
    for _,file in pairs(files) do
      if g_resources.isFileType(file, 'otps') then
        g_ui.importParticle('/layouts/' .. layout .. '/particles/' .. file)
        loaded_files[file] = true
      end
    end
  end
  
  files = g_resources.listDirectoryFiles('/data/particles')
  for _,file in pairs(files) do
    if g_resources.isFileType(file, 'otps') and not loaded_files[file] then
      g_particles.importParticle('/data/particles/' .. file)
    end
  end

  g_mouse.loadCursors('/data/cursors/cursors')
  if layout:len() > 0 and g_resources.directoryExists('/layouts/' .. layout .. '/cursors/cursors') then
    g_mouse.loadCursors('/layouts/' .. layout .. '/cursors/cursors')    
  end
end

function terminate()
end

