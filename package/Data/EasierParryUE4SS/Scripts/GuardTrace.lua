-- Read-only, event-driven held-guard diagnostics. MIT; see LICENSE.txt.
local directory = assert(debug.getinfo(1,'S').source:gsub('^@',''):match('^(.*[/\\])'))
local Hooks = dofile(directory .. 'UE4SSCommonHooks.lua')
return function(log,scheduleLater)
    local scheduleCallback=scheduleLater or ExecuteInGameThreadWithDelay
    local hooks = Hooks.new({RegisterHook=RegisterHook,UnregisterHook=UnregisterHook})
    -- main.lua owns this table for the mod lifetime; sessions inherit it.
    -- Only strings/booleans belong here. A save load cannot repair host dispatch.
    local runtime = EasierParryGuardTraceState or {}
    local active = true
    local stopped, registered = false, {}
    local queue, head, sequence, dropped = {}, 1, 0, 0
    local flushing, installing = false, false
    local window, samples, total = -1, 0, 0
    local eventWindow, events = -1, 0
    local failedReads, notified = {}, {}
    local combatClass, abilityLibrary, blockTag, gateTag, attackTag, ltKey
    local constantsAttempted = false
    local MAX_RECORDS, MAX_QUEUE, MAX_SNAPSHOTS = 2048, 64, 8
    local scopes, jobs = {}, {}
    local wake

    -- Delayed-action loops ignore callback return values in this UE4SS build.
    -- Use one-shot callbacks and schedule another only while work remains.
    local function runUntilDone(delay, step)
        local function run()
            if step() ~= true then scheduleCallback(delay, run) end
        end
        scheduleCallback(delay, run)
    end

    local function live(object)
        return object ~= nil and object:IsValid()
    end
    -- Unwrap hook parameters (Remote/LocalUnrealParam); never probe UObject.get.
    local function param(value)
        if value == nil then return nil end
        return value:get()
    end
    local function flush()
        if flushing then return end
        flushing = true
        runUntilDone(200, function()
            if not active then queue, head, dropped, flushing = {}, 1, 0, false; return true end
            local lines = {}
            for _ = 1, dropped > 0 and 7 or 8 do
                if head > #queue then break end
                lines[#lines + 1] = queue[head]
                head = head + 1
            end
            if dropped > 0 then
                lines[#lines + 1] = 'capture dropped=' .. dropped .. ' (burst/output limit)'
                dropped = 0
            end
            if #lines > 0 then log("GuardTrace %s", table.concat(lines, "\nGuardTrace ")) end
            if head > #queue then
                queue, head, flushing = {}, 1, false
                return true
            end
            return false
        end)
    end
    local function record(message)
        if not active then return end
        if total >= MAX_RECORDS then stopped = true; return end
        if #queue - head + 1 >= MAX_QUEUE then dropped = dropped + 1; return end
        sequence, total = sequence + 1, total + 1
        if dropped > 0 then message = message .. " dropped=" .. dropped; dropped = 0 end
        queue[#queue + 1] = string.format("#%d t=%.3f %s", sequence, os.clock(), message)
        if total == MAX_RECORDS then
            queue[#queue] = queue[#queue] .. " TRACE_LIMIT_REACHED load a save for a new capture"
            stopped = true
        end
        flush()
    end
    local function read(label, fn)
        if failedReads[label] then return "?" end
        local ok, value = pcall(fn)
        if ok then return tostring(value) end
        failedReads[label] = true
        record("READ_UNAVAILABLE " .. label .. " " .. tostring(value))
        return "?"
    end
    local function sampleAllowed()
        local now = math.floor(os.clock() * 10)
        if now ~= window then window, samples = now, 0 end
        if samples >= MAX_SNAPSHOTS then return false end
        samples = samples + 1
        return true
    end
    local function snapshot(label, combat, player, ability, detail)
        if not live(player) or not player:IsPlayerControlled() or not player:IsLocallyControlled() then return end
        local text = label .. " pawn=" .. tostring(player:GetAddress()) .. (detail or "")
        if ability then text = text .. " ability=" .. tostring(ability:GetAddress()) end
        if not sampleAllowed() then record(text .. " snapshot=rate_limited"); return end
        if live(combat) then
            text = text .. " wants=" .. read("wants", function() return combat:WantsToBlock() end)
                .. " blocking=" .. read("blocking", function() return combat:IsBlocking() end)
                .. " alive=" .. read("alive", function() return combat:IsAlive() end)
        else text = text .. " combat=unavailable" end
        local controller = player:GetController()
        if live(controller) then
            text = text .. " LT=" .. read("LT", function() return controller:GetInputAnalogKeyState(ltKey) end)
        end
        if live(abilityLibrary) then
            local asc = abilityLibrary:GetAbilitySystemComponent(player)
            if live(asc) then
                text = text .. " blockTag=" .. read("blockTag", function() return asc:HasMatchingGameplayTag(blockTag) end)
                    .. " gateTag=" .. read("gateTag", function() return asc:HasMatchingGameplayTag(gateTag) end)
                    .. " attackTag=" .. read("attackTag", function() return asc:HasMatchingGameplayTag(attackTag) end)
            else text = text .. " ASC=unavailable" end
        end
        if live(combat) then
            text = text .. " state=" .. read("state", function() return combat:GetCurrentState() end)
        end
        record(text)
    end
    local function guarded(label, fn)
        return function(...)
            if not active or stopped then return end
            local now = math.floor(os.clock() * 10)
            if now ~= eventWindow then eventWindow, events = now, 0 end
            if events >= 32 then dropped = dropped + 1; return end
            events = events + 1
            local ok, err = pcall(fn, ...)
            if not ok and not notified[label] then
                notified[label] = true
                record("CALLBACK_ERROR " .. label .. " " .. tostring(err))
            end
            -- No callback overrides return values or changes any game state.
        end
    end
    local function combatEvent(label, withBool)
        return guarded(label, function(context, value)
            local combat = param(context)
            if not live(combat) then return end
            local detail = withBool and (" requested=" .. tostring(param(value))) or ""
            snapshot(label, combat, combat:GetCharacter(), nil, detail)
        end)
    end
    local function abilityEvent(label, argument)
        return guarded(label, function(context, value)
            local ability = param(context)
            if not live(ability) then return end
            local player = ability:GetAvatarActorFromActorInfo()
            if not live(player) then return end
            local combat = live(combatClass) and player:GetComponentByClass(combatClass) or nil
            local detail = argument and (" " .. argument .. "=" .. tostring(param(value))) or ""
            snapshot(label, combat, player, ability, detail)
        end)
    end
    local function addScope(name, class, paths)
        scopes[name] = {class=class, paths=paths, next=1, attempts=0, queued=false}
    end
    local blockClass = '/Game/_Dawnwalker/Player/Abilities/Input/GA_Input_CombatBlock.GA_Input_CombatBlock_C'
    local dodgeClass = '/Game/_Dawnwalker/Combat/Abilities/Dodge/GA_Dodge.GA_Dodge_C'
    addScope('block', blockClass, {
        {'K2_ActivateAbility', 'block.activate'},
        {'K2_OnEndAbility', 'block.end', 'cancelled'},
        {'Removed_8EB8A6B74BA524392159438191716079', 'block.end_requested'},
        {'TagCountChanged_748E6D7442CECCD88071F2B6BAED63BF', 'block.suppression', 'count'},
        {'Added_9BE6EBD14C58975D3C81CC8B5B48D989', 'block.attack'},
        {'Added_D145CA7143BA7EB922BA8F97C939F64F', 'block.STOCK_next_attack_end'},
        {'Removed_996F0E954925C2383CBAD78F9EBEFB3A', 'block.attack_release'},
    })
    addScope('dodge', dodgeClass, {{'K2_ActivateAbility', 'dodge.activate'}, {'K2_OnEndAbility', 'dodge.end', 'cancelled'}})
    for _, name in ipairs({'Light', 'Heavy'}) do
        local class = '/Game/_Dawnwalker/Player/Abilities/Input/GA_Input_Combat' .. name .. 'Attack.GA_Input_Combat' .. name .. 'Attack_C'
        addScope(name, class, {{'K2_ActivateAbility', name .. '.activate'}})
    end
    -- The native calls made by these graphs still execute without Lua's
    -- Blueprint dispatcher. Filter by exact class path; no global discovery,
    -- retained ability instances, guessed fields or extra gameplay calls.
    local abilityNames = {}
    for name, scope in pairs(scopes) do abilityNames[scope.class] = name end
    local function booleanResult(value)
        local result = param(value)
        assert(type(result) == 'boolean', 'Native hook result is not a boolean')
        return tostring(result)
    end
    local function nativeAbilityEvent(action, result)
        return guarded('ability.' .. action, function(context, value)
            local ability = param(context)
            if not live(ability) then return end
            local class = ability:GetClass()
            if not live(class) then return end
            local name = abilityNames[class:GetFullName():match('^[^ ]+ (.+)$')]
            if not name then return end
            local player = ability:GetAvatarActorFromActorInfo()
            if not live(player) then return end
            local combat = live(combatClass) and player:GetComponentByClass(combatClass) or nil
            local detail = result and (' result=' .. booleanResult(value)) or ''
            snapshot(name .. '.' .. action, combat, player, ability, detail)
        end)
    end
    -- On this host a native post hook receives (context, original result,
    -- inputs...). Never re-run CanEnterState or CommitAbility to infer a result.
    local stateResult = guarded('dodge.state_check', function(context, result, requested)
        if param(requested) ~= 8 then return end
        local combat = param(context)
        if not live(combat) then return end
        snapshot('dodge.state_check', combat, combat:GetCharacter(), nil,
            ' target=8 result=' .. booleanResult(result))
    end)
    local animationResult = guarded('dodge.animation', function(context, result)
        local combat = param(context)
        if not live(combat) then return end
        local player = combat:GetCharacter()
        if not live(player) or not player:IsPlayerControlled() or not player:IsLocallyControlled() then return end
        -- FGameplayTag is borrowed from the call. Copy only its name now.
        local direction = read('dodge.direction', function() return param(result).TagName:ToString() end)
        snapshot('dodge.animation', combat, player, nil, ' direction=' .. direction)
    end)
    local function noop() end
    local natives = {
        {'/Script/DogwoodCombat.CombatComponentBase:SetDesiredBlockState', 'guard.set', true},
        {'/Script/DogwoodCombat.CombatComponentBase:TryActivateDodgeAbility', 'dodge.request'},
        {'/Script/DogwoodCombat.PlayerCombatComponent:TryQueueComboAttack', 'attack.combo'},
        {'/Script/DogwoodCombat.PlayerCombatComponent:QueueAttack', 'attack.queue'},
        {'/Script/DogwoodCombat.CombatComponentBase:CanEnterState', pre=noop, post=stateResult},
        {'/Script/DogwoodCombat.CombatComponentBase:PlayDodgeAnimation', pre=noop, post=animationResult},
        {'/Script/GameplayAbilities.GameplayAbility:K2_CommitAbility', pre=noop, post=nativeAbilityEvent('commit', true)},
        {'/Script/GameplayAbilities.GameplayAbility:K2_CancelAbility', pre=nativeAbilityEvent('cancel.pre'), post=nativeAbilityEvent('cancel.post')},
        {'/Script/GameplayAbilities.GameplayAbility:K2_EndAbility', pre=nativeAbilityEvent('end_call.pre'), post=nativeAbilityEvent('end_call.post')},
    }
    local function missingBlueprintDispatcher(message)
        if not message:find("Was unable to register a hook with Lua function 'RegisterHook'", 1, true) then return false end
        local pointer = message:match('UFunction::Func:%s*(0[xX]%x+)')
        local dispatcher = message:match('ProcessInternal:%s*(0[xX]%x+)')
        local native = message:match('FUNC_Native:%s*(%d+)')
        return native == '0' and pointer and not pointer:match('^0[xX]0+$')
            and dispatcher and dispatcher:match('^0[xX]0+$') ~= nil
    end
    local function reportBlueprintUnavailable()
        if runtime.blueprintUnavailable and not runtime.reported then
            -- Emit this once directly: a save load can cancel queued output.
            log('GuardTrace BLUEPRINT_UNAVAILABLE standard Lua dispatcher is unavailable; '
                .. 'Blueprint tracing disabled until mod restart. Native tracing remains enabled. '
                .. 'Native state-check, commit, animation and explicit end/cancel diagnostics replace the missing decision detail; '
                .. 'Blueprint-only lifecycle and tag callbacks remain unavailable. '
                .. 'First path=%s\n%s', runtime.path, runtime.reason)
            runtime.reported = true
        end
    end
    local function register(path, pre, post)
        if registered[path] and live(registered[path][3]) then return true end
        local lookupOK, fn = pcall(StaticFindObject, path)
        if not lookupOK or not live(fn) then return false, 'UFunction not loaded' end
        if registered[path] then
            local removed, err = hooks.remove(path)
            if not removed then return false, 'Hook cleanup failed: '..tostring(err) end
            registered[path] = nil
        end
        local first, second = hooks.register(path, path, pre, post)
        if first then
            registered[path] = {first, second, fn}
            record('HOOK_READY ' .. path)
            return true
        end
        local message = tostring(second)
        if path:sub(1, 6) == '/Game/' and missingBlueprintDispatcher(message) then
            runtime.blueprintUnavailable, runtime.path, runtime.reason = true, path, message
        end
        return false, message
    end
    local nativeIndex = 1
    local function schedule(name, reset)
        local scope = scopes[name]
        if not active or stopped or runtime.blueprintUnavailable or scope.queued then return end
        if reset then scope.attempts, scope.next, scope.done = 0, 1, false end
        if scope.done then return end
        if scope.attempts >= 2 then return end
        scope.queued = true
        jobs[#jobs + 1] = name
        wake()
    end
    wake = function()
        if not active or stopped or installing then return end
        installing = true
        runUntilDone(100, function()
            if not active or stopped then installing = false; return true end
            if not constantsAttempted then
                constantsAttempted = true
                local ok, err = pcall(function()
                    combatClass = StaticFindObject('/Script/DogwoodCombat.PlayerCombatComponent')
                    abilityLibrary = StaticFindObject('/Script/GameplayAbilities.Default__AbilitySystemBlueprintLibrary')
                    blockTag = {TagName=FName('Player.Input.Block')}
                    gateTag = {TagName=FName('Player.Input.BlockTagAbilities')}
                    attackTag = {TagName=FName('Player.Input.LightAttack')}
                    ltKey = {KeyName=FName('Gamepad_LeftTriggerAxis')}
                end)
                if not ok then record('SETUP_ERROR ' .. tostring(err)) end
                return false
            end
            if nativeIndex <= #natives then
                local spec = natives[nativeIndex]; nativeIndex = nativeIndex + 1
                local pre = spec.pre or combatEvent(spec[2] .. '.pre', spec[3])
                local post = spec.post or combatEvent(spec[2] .. '.post', spec[3])
                local ok, err = register(spec[1], pre, post)
                if not ok then record('HOOK_UNAVAILABLE ' .. spec[1] .. ' ' .. tostring(err)) end
                return false
            end
            if runtime.blueprintUnavailable then
                jobs = {}
                for _, scope in pairs(scopes) do scope.queued = false end
                reportBlueprintUnavailable()
                installing = false
                return true
            end
            local name = jobs[1]
            if name then
                local scope = scopes[name]
                local spec = scope.paths[scope.next]
                local ok, err = register(scope.class .. ':' .. spec[1], abilityEvent(spec[2], spec[3]))
                if ok then
                    scope.next = scope.next + 1
                    if scope.next > #scope.paths then scope.done = true end
                elseif not runtime.blueprintUnavailable then
                    scope.attempts = scope.attempts + 1
                    if scope.attempts >= 2 then record('HOOK_PENDING_CLASS ' .. scope.class .. ':' .. spec[1] .. ' ' .. tostring(err)) end
                end
                if scope.done or scope.attempts >= 2 then scope.queued = false; table.remove(jobs, 1) end
                return false
            end
            installing = false
            record('SETUP_IDLE waiting for ability construction; no readiness polling')
            return true
        end)
    end
    if type(ExecuteInGameThreadWithDelay) ~= 'function' or type(RegisterHook) ~= 'function'
        or type(NotifyOnNewObject) ~= 'function' then
        log('GuardTrace unavailable: this UE4SS build lacks required event APIs.'); return
    end
    if not runtime.blueprintUnavailable then
        for name, scope in pairs(scopes) do
            local scopeName = name
            local ok, err = pcall(NotifyOnNewObject, scope.class, function()
                -- Construction notifications only enqueue setup. No UObject reads here.
                schedule(scopeName, true)
            end)
            if not ok then record('NOTIFY_UNAVAILABLE ' .. scope.class .. ' ' .. tostring(err)) end
            schedule(name, false)
        end
    end
    -- Native setup is needed even when an earlier session disabled Blueprint tracing.
    wake()
    log('GuardTrace enabled: read-only trace GT4; native state-check/commit results, animation selection, end/cancel calls, combat state, raw LT and input tags. Limit 2048 records per save-load capture; controlled by Logging (debugLogging).')
    -- Keep installed hooks dormant while off; their guards return before clocks
    -- or UObject access. Pending one-shots drain without output or rescheduling.
    return function(enabled)
        if active == enabled then return end
        active = enabled
        queue, head, dropped = {}, 1, 0
        if not active then return end
        stopped, total, sequence = false, 0, 0
        window, samples, eventWindow, events = -1, 0, -1, 0
        for name in pairs(scopes) do schedule(name, true) end
        wake()
    end

end
