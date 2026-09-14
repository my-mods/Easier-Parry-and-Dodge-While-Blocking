-- MIT. Event-driven configuration of native dodge timing and guard recovery.
-- No input hooks, global ability scans, or recurring work.
local M = {}
local CLASS = '/Game/_Dawnwalker/Combat/Abilities/Dodge/GA_Dodge.GA_Dodge_C'
local GUARD = 'Player.Input.BlockTagAbilities'
local FIELD = 'ActivationOwnedTags'
local function live(object)
    return object ~= nil and object:IsValid()
end
local function tags(object)
    local values = object[FIELD].GameplayTags
    assert(#values <= 64, 'Unexpected dodge owned tag count')
    local result, found = {}, false
    for i=1,#values do
        local name = values[i].TagName:ToString()
        assert(name:match('^[%w_%.]+$'), 'Unexpected gameplay tag name')
        result[#result+1] = name
        if name == GUARD then found = true end
    end
    return result, found
end
local function write(object, enabled)
    local names = tags(object)
    local target, parents, seen = {}, {}, {}
    for _, name in ipairs(names) do
        if name ~= GUARD then target[#target+1] = name end
    end
    if enabled then target[#target+1] = GUARD end
    local function serialize(values)
        local parts = {}
        for _, name in ipairs(values) do parts[#parts+1] = '(TagName="'..name..'")' end
        return '('..table.concat(parts, ',')..')'
    end
    for _, name in ipairs(target) do
        local parent = name:match('^(.*)%.')
        while parent do
            if not seen[parent] then parents[#parents+1]=parent; seen[parent]=true end
            parent = parent:match('^(.*)%.')
        end
    end
    local property = object:Reflection():GetProperty(FIELD)
    assert(live(property), 'Dodge owned tag property unavailable')
    property:ImportText('(GameplayTags='..serialize(target)..',ParentTags='..serialize(parents)..')',
        property:ContainerPtrToValuePtr(object), 0, object)
    local _, actual = tags(object)
    assert(actual == enabled, 'Dodge owned tag write did not stick')
end

function M.start(resolvePlayer, log, diagnostics, vanillaDodge, windowFactor, scheduleLater)
    scheduleLater=scheduleLater or ExecuteInGameThreadWithDelay
    local playerReady=SaveLoadContext and SaveLoadContext.pawn~=nil
    local componentClass, dodgeClass, combatClass
    local windowPending = windowFactor ~= 1
    local windowTargets = {}
    local ownerAddress, componentAddress, cursor = nil, nil, 1
    local pending, running, dirty, attempts, found = false, false, false, 0, false
    local lastFailure
    local run, schedule
    local function failure(message)
        if message ~= lastFailure then
            lastFailure = message
            log('Dodge settings: %s', message)
        end
    end
    local function patch(object, player, component, specIndex, instanceField, instanceIndex)
        if not live(object) then return false end
        if object:HasAnyFlags(0x3630) then return false end -- defaults/archetypes or loading
        if object:GetClass():GetAddress() ~= dodgeClass:GetAddress() then return false end
        -- GA_Dodge is instanced per actor. Never alter shared class defaults or NPCs.
        assert(object.InstancingPolicy == 1, 'Unsupported dodge ability instancing policy')
        local avatar = object:GetAvatarActorFromActorInfo()
        if not live(avatar) or avatar:GetAddress() ~= player:GetAddress() then return false end
        local address = object:GetAddress()
        local name, classAddress = object:GetFullName(), dodgeClass:GetAddress()
        local function valid()
            if not live(object) or object:HasAnyFlags(0x18000) then return false end
            local class = object:GetClass()
            return live(class) and class:GetAddress() == classAddress and object:GetFullName() == name
        end
        local function inactive()
            if not valid() or not live(component) or component:HasAnyFlags(0x18000) then return false end
            -- Specs are borrowed views; reacquire and verify the indexed instance.
            local items = component.ActivatableAbilities.Items
            if specIndex > #items then return false end
            local spec = items[specIndex]
            local instances = spec[instanceField]
            if instanceIndex > #instances then return false end
            local current = instances[instanceIndex]
            if not live(current) or current:GetAddress() ~= address then return false end
            local count = spec.ActiveCount
            assert(type(count)=='number' and count>=0 and count<=255, 'Dodge active count unavailable')
            return count == 0
        end
        local _, hasGuard = tags(object)
        local target = not vanillaDodge
        if hasGuard == target then return true end
        -- ActivationOwnedTags must remain stable until GAS removes this dodge's
        -- acquired tags. Never change mode halfway through its graph/teardown.
        if not inactive() then return false end
        local key='dodge-block:'..tostring(address)
        Session.change(key, function()
            if not inactive() then return nil, false end
            local _, hasGuard = tags(object)
            return hasGuard
        end, function(value)
            assert(inactive(), 'Dodge ability is active or was replaced')
            write(object, value)
            return true -- Yield after a native property restore.
        end, target)
        return true
    end
    local function patchWindow(player)
        if not live(combatClass) then combatClass=StaticFindObject('/Script/DogwoodCombat.PlayerCombatComponent') end
        if not live(combatClass) then return false end
        local combat = player:GetComponentByClass(combatClass)
        if not live(combat) then return false end
        -- GetConfig returns the native config UObject, not a borrowed struct.
        -- GA_Dodge reads this float for its existing perfect/ultra-dodge check.
        local settings = combat:GetConfig()
        if not live(settings) or settings:HasAnyFlags(0x1B630) then return false end
        local name, classAddress = settings:GetFullName(), settings:GetClass():GetAddress()
        local key = 'dodge-window:'..tostring(settings:GetAddress())..':'..name
        local function get()
            if not live(settings) or settings:HasAnyFlags(0x18000) then return nil, false end
            local class = settings:GetClass()
            if not live(class) or class:GetAddress() ~= classAddress or settings:GetFullName() ~= name then return nil, false end
            local value = settings.PerfectDodgeWindow
            assert(type(value)=='number' and value>=0 and value<math.huge, 'Invalid native PerfectDodgeWindow')
            return value
        end
        local baseline = get()
        if baseline == nil then return false end
        -- Repeated lifecycle events must not multiply an already adjusted asset.
        local remembered = windowTargets[key]
        if not remembered then remembered={base=baseline,last=baseline};windowTargets[key]=remembered
        elseif math.abs(baseline-remembered.last)>1e-5*math.max(1,math.abs(remembered.last)) then remembered.base=baseline end
        local target=remembered.base*windowFactor
        local changed = Session.change(key, get, function(value)
            local _, valid = get()
            assert(valid ~= false, 'Dodge timing config was replaced')
            settings.PerfectDodgeWindow = value
            remembered.last=value
            return true
        end, target)
        if changed then diagnostics.debug('Perfect dodge window: x%.3f, %.4fs -> %.4fs', windowFactor, baseline, target) end
        return true
    end
    local function slice()
        local started=os.clock()
        local player = resolvePlayer()
        playerReady=live(player)
        if not playerReady then return 'waiting' end
        if windowPending then
            if not patchWindow(player) then return 'waiting' end
            windowPending = false
            return 'more' -- Separate timing and ability discovery across frames.
        end
        if not live(componentClass) then componentClass=StaticFindObject('/Script/GameplayAbilities.AbilitySystemComponent') end
        if not live(dodgeClass) then dodgeClass=StaticFindObject(CLASS) end
        if not live(componentClass) or not live(dodgeClass) then return 'waiting' end
        local component = player:GetComponentByClass(componentClass)
        if not live(component) then return 'waiting' end
        local pawnId, componentId = player:GetAddress(), component:GetAddress()
        if pawnId ~= ownerAddress or componentId ~= componentAddress then
            ownerAddress, componentAddress, cursor, found = pawnId, componentId, 1, false
        end
        -- Reacquire borrowed specs in every slice; keep only scalar cursors between frames.
        local items = component.ActivatableAbilities.Items
        assert(#items <= 512, 'Unexpected player ability count')
        for unit=1,8 do
            if cursor > #items then return found and 'done' or 'waiting' end
            local spec = items[cursor]
            cursor = cursor + 1
            local ability = spec.Ability
            if live(ability) and ability:GetClass():GetAddress() == dodgeClass:GetAddress() then
                assert(ability.InstancingPolicy == 1, 'Unsupported dodge ability instancing policy')
                for _, field in ipairs({'NonReplicatedInstances','ReplicatedInstances'}) do
                    local instances = spec[field]
                    assert(#instances <= 1, 'Unexpected per-actor dodge instance count')
                    for i=1,#instances do
                        if patch(instances[i], player, component, cursor-1, field, i) then found=true end
                    end
                end
                return 'more' -- At most one dodge spec is patched in a frame.
            end
            if os.clock()-started >= 0.0005 then return 'more' end
        end
        return 'more'
    end
    schedule = function(delay)
        if pending then return end
        pending = true
        scheduleLater(delay, run)
    end
    local measuredSlice = diagnostics.wrap('dodgeSetup', slice)
    run = function()
        pending = false
        local ok, outcome = pcall(measuredSlice)
        if not ok then
            running=false; failure(tostring(outcome)); return
        end
        if outcome == 'more' then schedule(16); return end
        if outcome == 'waiting' then
            attempts=attempts+1; cursor=1; found=false
            if attempts < 20 then schedule(100); return end
            running=false
            failure('Player dodge settings not ready or dodge still active; will retry on the next settings change or lifecycle event')
            return
        end
        if dirty then dirty=false; cursor=1; found=false; windowPending=windowFactor~=1 or next(windowTargets)~=nil; schedule(16); return end
        running=false; lastFailure=nil
        diagnostics.debug('Native dodge settings applied')
        diagnostics.flush(true)
    end
    local function wake()
        if running then dirty=true; return end
        running=true; dirty=false; attempts=0; cursor=1; found=false
        windowPending=windowFactor~=1 or next(windowTargets)~=nil
        schedule(16)
    end
    -- Construction only schedules work. All reflection happens in registered game-thread callbacks.
    for _,path in ipairs({CLASS,'/Script/DogwoodCombat.PlayerCombatComponent'}) do
        local ok,err=pcall(NotifyOnNewObject,path,wake)
        if not ok then failure('Dodge notifications unavailable: '..tostring(err)) end
    end
    local function update(vanilla,factor)
        local changed=vanillaDodge~=vanilla or windowFactor~=factor
        vanillaDodge,windowFactor=vanilla,factor
        if playerReady and (changed or not running) then wake() end
    end
    return wake,update
end
return M
