-- MIT. Preference loading and pure numeric-to-gameplay conversion.
local MOD_NAME = "EasierParryUE4SS"
local INI_NAME = "EasierParryUE4SS.ini"
local DEFAULTS_NAME = "EasierParryUE4SS.defaults.ini"
local ATTRIBUTE_SET_FIELD = "CharDevAttributeSet"
local ATTRIBUTE_FIELD = "ParryWindowMultiplier"
local SCRIPT_SOURCE = debug.getinfo(1, "S").source

local config = {
    enabled = true,
    factor = 2.0,
    pollMilliseconds = 1000, -- Accepted for old INIs; unused (no periodic checks).
    debugLogging = false,
    dodgeWhileBlocking = true,
    dodgeWindowFactor = 1.0,
    dodgeInterruptsGuard = false, -- Legacy INI compatibility; native guard assets own this behavior.
}

local active = nil
local engine, gameplayStatics = nil, nil
local bootstrapAttempted = false
local SetGuardTracing
local lastFailure = nil

local scriptDirectory = assert(SCRIPT_SOURCE:gsub('^@',''):match('^(.*[/\\])'))
local Diagnostics = dofile(scriptDirectory .. 'UE4SSCommonDiagnostics.lua')
local diagnostics = Diagnostics.new({prefix='['..MOD_NAME..'] ',output=function(text) print(text..'\n') end})
local function Log(message, ...) diagnostics.log(message, ...) end
local function Debug(message, ...) diagnostics.debug(message, ...) end

local function Trim(value)
    return (tostring(value or ""):gsub("^%s+", ""):gsub("%s+$", ""))
end

local function ParseBoolean(value)
    local lower = string.lower(Trim(value))
    if lower == "true" or lower == "yes" or lower == "on" or lower == "1" then return true end
    if lower == "false" or lower == "no" or lower == "off" or lower == "0" then return false end
    return nil
end

local function ScriptIniPath(name)
    local source = (SCRIPT_SOURCE or ""):gsub("^@", "")
    local directory = string.match(source, "^(.*)[/\\][^/\\]*$")
    if directory ~= nil and directory ~= "" then return directory .. "/" .. name end
    return "ue4ss/Mods/" .. MOD_NAME .. "/Scripts/" .. name
end

local function IniPath()
    local directory = os.getenv("LOCALAPPDATA")
    if not directory or not (directory:match("^%a:[/\\]") or directory:match("^\\\\")) then return nil end
    return directory:gsub("[/\\]+$", "") .. "/Dawnwalker/Saved/Config/" .. INI_NAME
end

local function ReadIni(path)
    local file, err, code = io.open(path, "rb")
    if not file then return nil, err, code end
    local contents, readError = file:read("*a")
    file:close()
    return contents, readError
end

local function ApplyIni(contents, path)
    -- Editors may save a UTF-8 BOM before the first section header.
    contents = contents:gsub("^\239\187\191", "")
    local section, seen = "", {}
    for line in contents:gmatch("[^\r\n]+") do
        local clean = Trim((line:gsub("[;#].*$", "")))
        local sectionName = string.match(clean, "^%[([^%]]+)%]$")
        if sectionName ~= nil then
            section = string.lower(Trim(sectionName))
        elseif section == "general" and clean ~= "" then
            local key, value = string.match(clean, "^([%w_]+)%s*=%s*(.-)%s*$")
            if key ~= nil then
                key = string.lower(key)
                if seen[key] then return false end
                seen[key] = true
                if key == "enabled" or key == "debuglogging" or key == "dodgeinterruptsguard" then
                    local parsed = ParseBoolean(value)
                    if parsed == nil then return false end
                    if parsed ~= nil then
                        local field = ({enabled="enabled", debuglogging="debugLogging", dodgeinterruptsguard="dodgeInterruptsGuard"})[key]
                        config[field] = parsed
                    end
                elseif key == "factor" or key == "pollmilliseconds" then
                    local parsed = tonumber(value)
                    if parsed == nil or parsed ~= parsed or math.abs(parsed) == math.huge then
                        Log("WARNING: invalid %s in %s; import stopped", key, path)
                        return false
                    elseif key == "factor" then
                        config.factor = math.max(0.1, math.min(50.0, parsed))
                    else
                        config.pollMilliseconds = math.floor(math.max(100, math.min(5000, parsed)))
                    end
                end
            end
        end
    end
    Log("Loaded configuration from %s (factor=%.3f, enabled=%s)", path, config.factor, tostring(config.enabled))
    return true
end

local function LoadConfig()
    local directory = ScriptIniPath(''):gsub('[^/\\]*$', '')
    local Store = dofile(directory .. 'ParrySettings.lua')
    local schema = dofile(directory .. 'SettingsSchema.lua')
    local values, err = Store.load(directory, schema, function()
        local defaults, de = ReadIni(ScriptIniPath(DEFAULTS_NAME))
        if not defaults then return nil, de end
        if not ApplyIni(defaults, DEFAULTS_NAME) then return nil, "Invalid legacy defaults" end
        local path = IniPath()
        if not path then return nil, 'LOCALAPPDATA unavailable for legacy migration' end
        local personal, pe, pc = ReadIni(path)
        if not personal and pc == 2 then path = ScriptIniPath(INI_NAME); personal, pe, pc = ReadIni(path) end
        if not personal and pc ~= 2 then return nil, pe end
        if personal and not ApplyIni(personal, path) then return nil, "Invalid legacy INI; original left unchanged" end
        return {enabled=config.enabled and 1 or 0, parryWindowPercent=config.factor*100, debugLogging=config.debugLogging and 1 or 0},
            nil, personal and {{path=path, text=personal}} or nil
    end)
    if not values then Log('Settings rejected: %s', tostring(err)); return false end
    return values
end

local M={load=LoadConfig}
function M.convert(values)
    return {enabled=values.enabled==1,factor=values.parryWindowPercent/100,
        debugLogging=values.debugLogging==1,dodgeWhileBlocking=values.dodgeWhileBlocking==1,
        dodgeWindowFactor=values.dodgeWindowPercent/100}
end
return M
