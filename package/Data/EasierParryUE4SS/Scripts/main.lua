-- Prepare preferences at startup; committed menu values update the active session.
local directory = assert(debug.getinfo(1, 'S').source:sub(2):match('^(.*[/\\])'))
-- Owned diagnostic capability state survives save-load scopes, never game objects.
EasierParryGuardTraceState = {}
local function report(message) print('[Save Settings] '..message..'\n') end
local live=dofile(directory..'LiveSettings.lua').new(directory,report)
local prepared,values=pcall(function() return dofile(directory..'SettingsModel.lua').load() end)
if prepared and type(values)=='table' then live.seed(values) else report('Settings preparation failed: '..tostring(values)) end
local session = dofile(directory..'UE4SSCommonSession.lua').new(_G, directory, report,{settings=live,loadSettings=function() return dofile(directory..'SettingsModel.lua').load() end})
local ok, err = pcall(function()
    dofile(directory..'UE4SSDawnwalkerSaveLoad.lua').start(_G, session, directory..'Gameplay.lua', report)
end)
if not ok then report('Save-load hooks unavailable: '..tostring(err)) end
