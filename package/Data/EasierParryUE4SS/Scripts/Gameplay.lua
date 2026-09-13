-- Easier Parry - UE4SS
-- The Blood of Dawnwalker, PC build 25129649 / executable CL-257186.
--
-- Updates the live FGameplayAttributeData on Coen's CharDevAttributeSet. The value is captured
-- from the running game, multiplied from that baseline, and applied when a save has loaded.

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
    config.enabled=values.enabled==1;config.factor=values.parryWindowPercent/100;config.debugLogging=values.debugLogging==1
    config.dodgeWhileBlocking=values.dodgeWhileBlocking==1
    config.dodgeWindowFactor=values.dodgeWindowPercent/100
    return true
end

local function IsLive(object)
    if object == nil then return false end
    local ok, valid = pcall(function() return object:IsValid() end)
    return ok and valid == true
end

local function SafeName(object)
    if object == nil then return "<nil>" end
    local ok, name = pcall(function() return object:GetFullName() end)
    if ok and name ~= nil then return tostring(name) end
    return tostring(object)
end

local function IsDefaultObject(object)
    return string.find(SafeName(object), "Default__", 1, true) ~= nil
end

-- Guard input and dodge suspension are handled by the native ability assets.
-- The optional dodge setting adds a native activation requirement at save load.

local function NearlyEqual(left, right)
    return type(left) == "number" and type(right) == "number"
        and math.abs(left - right) <= 0.0001
end

local function ReadAttribute(attributeSet)
    if not IsLive(attributeSet) then return nil, nil, nil, "attribute owner is not live" end
    local ok, raw = pcall(function() return attributeSet[ATTRIBUTE_FIELD] end)
    if not ok or raw == nil then return nil, nil, nil, "attribute property was not readable" end

    if type(raw) == "number" then return raw, raw, "number", nil end

    -- FGameplayAttributeData is already a struct view, not a parameter wrapper.
    -- Keep it local to this read so no struct view survives a player replacement.
    local okBase, base = pcall(function() return raw.BaseValue end)
    local okCurrent, current = pcall(function() return raw.CurrentValue end)
    if okBase and okCurrent and type(base) == "number" and type(current) == "number" then
        return base, current, "struct", nil
    end

    return nil, nil, nil, "FGameplayAttributeData values were not readable"
end

local function NumberText(value)
    return string.format("%.9g", value)
end

local function WriteWithReflection(attributeSet, base, current, kind)
    return pcall(function()
        local property = attributeSet:Reflection():GetProperty(ATTRIBUTE_FIELD)
        if property == nil or not property:IsValid() then error("property reflection failed") end

        local valueText = NumberText(current)
        if kind == "struct" then
            valueText = string.format("(BaseValue=%s,CurrentValue=%s)", NumberText(base), NumberText(current))
        end

        property:ImportText(
            valueText,
            property:ContainerPtrToValuePtr(attributeSet),
            0,
            attributeSet
        )
    end)
end

local function WriteDirect(attributeSet, base, current, kind)
    return pcall(function()
        if kind == "number" then
            attributeSet[ATTRIBUTE_FIELD] = current
            return
        end

        local raw = attributeSet[ATTRIBUTE_FIELD]
        raw.BaseValue = base
        raw.CurrentValue = current
    end)
end

local function WriteAttribute(attributeSet, base, current, kind)
    if not IsLive(attributeSet) then return false, "attribute owner is not live" end
    local ok = WriteWithReflection(attributeSet, base, current, kind)
    if not ok then ok = WriteDirect(attributeSet, base, current, kind) end
    if not ok then return false, "both reflection and direct writes failed" end

    local actualBase, actualCurrent = ReadAttribute(attributeSet)
    if not NearlyEqual(actualBase, base) or not NearlyEqual(actualCurrent, current) then
        return false, string.format(
            "write did not stick (wanted %.4f/%.4f, read %s/%s)",
            base,
            current,
            tostring(actualBase),
            tostring(actualCurrent)
        )
    end
    return true, nil
end

local function FindPlayerAttributeSet()
    -- Discover once per live engine/CDO lifetime; readiness events can recover
    -- missing objects without a timer or a permanently exhausted attempt budget.
    if not bootstrapAttempted or not IsLive(engine) or not IsLive(gameplayStatics) then
        bootstrapAttempted = true
        local ok = pcall(function()
            if not IsLive(engine) then engine = FindFirstOf("Engine") end
            gameplayStatics = StaticFindObject("/Script/Engine.Default__GameplayStatics")
        end)
        if not ok or not IsLive(engine) or not IsLive(gameplayStatics) then
            Log("Player resolver unavailable; waiting for a player lifecycle event")
        end
    end
    if not IsLive(engine) or not IsLive(gameplayStatics) then
        return nil, nil, "waiting for the game engine"
    end
    -- The viewport follows the current world. A cached pawn/controller can
    -- remain valid AND locally controlled in the world left by a save load.
    -- Index zero resolves one local player, without a global UObject scan.
    local viewport = engine.GameViewport
    if not IsLive(viewport) then return nil, nil, "waiting for the game viewport" end
    local player = gameplayStatics:GetPlayerPawn(viewport, 0)
    if not IsLive(player) or not player:IsPlayerControlled() or not player:IsLocallyControlled() then
        return nil, nil, "waiting for the local player's possessed pawn"
    end
    local attributeSet = player[ATTRIBUTE_SET_FIELD]
    if not IsLive(attributeSet) then
        return player, nil, "waiting for the player's CharDevAttributeSet"
    end
    return player, attributeSet, nil
end

local function SameObject(left, right)
    -- UE4SS may return a new Lua wrapper for the same native object.
    return IsLive(left) and IsLive(right) and left:GetAddress() == right:GetAddress()
end

local function RestoreBaseline()
    if active == nil or not IsLive(active.attributeSet) then
        active = nil
        return
    end

    -- Do not overwrite a value recalculated by the game while we were detached.
    local base, current = ReadAttribute(active.attributeSet)
    if base == nil then active = nil; return end
    local restoredBase = NearlyEqual(base, active.targetBase) and active.baselineBase or base
    local restoredCurrent = NearlyEqual(current, active.targetCurrent) and active.baselineCurrent or current
    if NearlyEqual(base, restoredBase) and NearlyEqual(current, restoredCurrent) then
        active = nil
        return
    end
    local ok, reason = WriteAttribute(
        active.attributeSet,
        restoredBase,
        restoredCurrent,
        active.kind
    )
    if ok then
        Log("Released timing override %.4f / %.4f", restoredBase, restoredCurrent)
    else
        error("Could not restore the parry baseline: "..tostring(reason))
    end
    active = nil
end

local function Attach(player, attributeSet)
    local base, current, kind, reason = ReadAttribute(attributeSet)
    if base == nil then return false, reason end

    active = {
        attributeSet = attributeSet,
        player = player,
        baselineBase = base,
        baselineCurrent = current,
        targetBase = base * config.factor,
        targetCurrent = current * config.factor,
        kind = kind,
    }

    local ok, writeReason = WriteAttribute(
        attributeSet,
        active.targetBase,
        active.targetCurrent,
        kind
    )
    if not ok then
        -- A write can fail after changing only one field. Retain its baseline
        -- and target so the next tick repairs it instead of multiplying again.
        return false, writeReason
    end

    Log(
        "Applied x%.3f: ParryWindowMultiplier %.4f/%.4f -> %.4f/%.4f",
        config.factor,
        base,
        current,
        base * config.factor,
        current * config.factor
    )
    return true, nil
end

local function RecordFailure(reason)
    if reason ~= lastFailure then
        Debug("%s", tostring(reason))
        lastFailure = reason
    end
end

local function Poll()
    if not config.enabled then return "disabled" end

    local player, attributeSet, reason = FindPlayerAttributeSet()
    if attributeSet == nil then
        -- Keep the numeric baseline during temporary unpossession, but never
        -- repair a detached set. Repossession of that same set must not compound.
        RecordFailure(reason)
        return "waiting"
    end
    if active == nil or not SameObject(active.attributeSet, attributeSet) then
        -- Release only our unchanged value before handing over. This also avoids
        -- compounding if a later possession returns to the previous live set.
        RestoreBaseline()
        local attached, attachReason = Attach(player, attributeSet)
        if not attached then RecordFailure(attachReason); return "error" end
        lastFailure = nil
        return "attach"
    end

    active.player = player

    local attributeSet = active.attributeSet
    local base, current, _, reason = ReadAttribute(attributeSet)
    if base == nil then
        RecordFailure(reason)
        return "error"
    end
    if not NearlyEqual(base, active.targetBase) or not NearlyEqual(current, active.targetCurrent) then
        local repaired, repairReason = WriteAttribute(
            attributeSet,
            active.targetBase,
            active.targetCurrent,
            active.kind
        )
        if repaired then
            Debug("Reapplied the runtime value after the game recalculated it")
            lastFailure = nil
            return "repair"
        else
            RecordFailure(repairReason)
            return "error"
        end
    end
end

-- Debug-only wall-time sampling around the timing worker, including native calls.
-- Windows CRT os.clock has millisecond granularity: zero is not proof of zero cost.
-- No profiler timers, object lookups, or retained samples; only fixed-size counters.
local perf, queuedAt = nil, nil
local PERF_REPORT_LIMIT = 120
local function PerfClock() return diagnostics.now() end
local function NewPerf(now)
    return {count=0, total=0, max=0, queueMax=0, slow=0, errors=0,
        attach=0, repair=0, waiting=0, reports=0, lastReport=now,
        worstPhase="none", lastError="none"}
end
local function PerfReport(label, now, automatic)
    if not perf then Log("PERF %s: no timing samples collected", label); return end
    if automatic and perf.reports >= PERF_REPORT_LIMIT then return end
    if automatic then perf.reports = perf.reports + 1 end
    perf.lastReport = now
    Log("PERF %s: samples=%d avg=%.3fms max=%.3fms worst=%s slow(>=5ms)=%d queueMax=%.3fms attach=%d repair=%d wait=%d errors=%d lastError=%s; worker only, excludes summary logging; os.clock ms granularity",
        label, perf.count, perf.total / math.max(1, perf.count), perf.max,
        perf.worstPhase, perf.slow, perf.queueMax, perf.attach, perf.repair,
        perf.waiting, perf.errors, perf.lastError)
    if automatic and perf.reports == PERF_REPORT_LIMIT then
        Log("PERF automatic output limit reached; change debugLogging in Mod Settings and load a save for a new capture")
    end
end

local function Tick()
    local start = config.debugLogging and PerfClock() or nil
    local queued = queuedAt
    queuedAt = nil
    local bootstrapping = not bootstrapAttempted
    local ok, outcome = pcall(Poll)
    if not ok then RecordFailure("runtime error: " .. tostring(outcome)) end
    if not config.debugLogging then return end
    local finish = PerfClock()
    if not start or not finish or finish < start then
        if not perf or not perf.clockFailed then
            Log("PERF clock unavailable or moved backwards; sample discarded")
        end
        perf = perf or NewPerf(0)
        perf.clockFailed = true
        return
    end
    perf = perf or NewPerf(start)
    perf.clockFailed = false
    local elapsed = math.floor((finish - start) * 1000 + 0.5)
    local phase = bootstrapping and "bootstrap" or (ok and (outcome or "steady") or "error")
    diagnostics.sample('worker',elapsed)
    local timing = diagnostics.snapshot().timings.worker
    perf.count, perf.total = timing.n, timing.total
    if elapsed >= perf.max then perf.max = elapsed; perf.worstPhase = phase end
    if queued and queued <= start then perf.queueMax = math.max(perf.queueMax, math.floor((start-queued)*1000 + 0.5)) end
    perf.slow = timing.slow
    if outcome == "attach" then perf.attach = perf.attach + 1 end
    if outcome == "repair" then perf.repair = perf.repair + 1 end
    if outcome == "waiting" then perf.waiting = perf.waiting + 1 end
    if not ok or outcome == "error" then
        perf.errors = perf.errors + 1
        perf.lastError = tostring(lastFailure or outcome):sub(1,240):gsub("[\r\n]", " ")
    end
    -- First sample includes bootstrap. Afterwards at most one line per five
    -- seconds during slow work, otherwise one per thirty seconds. Totals persist.
    if perf.count == 1 or finish-perf.lastReport >= 30
        or (elapsed >= 5 and finish-perf.lastReport >= 5) then
        PerfReport("summary", finish, true)
    end
end

if not LoadConfig() then return end
diagnostics = Diagnostics.new({debugLogging=config.debugLogging,prefix='['..MOD_NAME..'] ',
    output=function(text) print(text..'\n') end,slowCallbackMs=5})
if type(ExecuteInGameThreadWithDelay)~='function' or type(CancelDelayedAction)~='function' then
    Log('Readiness requires game-thread one-shot scheduling and cancellation.'); return
end
Session.onClose(RestoreBaseline)
local dodgeReady
if not config.dodgeWhileBlocking or config.dodgeWindowFactor ~= 1 then
    dodgeReady = dofile(scriptDirectory..'DodgeSettings.lua').start(FindPlayerAttributeSet, Log, diagnostics,
        not config.dodgeWhileBlocking, config.dodgeWindowFactor)
end
local pending = false
local function applyReady()
    if dodgeReady then dodgeReady() end
    if pending or not config.enabled then return end
    pending = true
    ExecuteInGameThreadWithDelay(16, function()
        pending = false
        Tick()
    end)
end
RegisterHook('/Script/Engine.PlayerController:ClientRestart', function() end, applyReady)
NotifyOnNewObject('/Script/DogwoodStats.CharDevAttributeSet', applyReady)
RegisterLoadMapPostHook(function()
    bootstrapAttempted = false
    applyReady()
end)
-- One diagnostics switch controls both timing summaries and guard tracing.
local guardTraceControl
SetGuardTracing = function(enabled)
    if not enabled and not guardTraceControl then return end
    local ok, err = pcall(function()
        if guardTraceControl then
            guardTraceControl(enabled)
        elseif enabled then
            guardTraceControl = dofile(ScriptIniPath("GuardTrace.lua"))(Log)
        end
    end)
    if not ok then Log("GuardTrace failed: %s", tostring(err)) end
end
SetGuardTracing(config.debugLogging)

applyReady()
Log("Loaded (enabled=%s, factor=%.3f, debug=%s). Use Mod Settings, Apply, then load a save.", tostring(config.enabled), config.factor, tostring(config.debugLogging))
