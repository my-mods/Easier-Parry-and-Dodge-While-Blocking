-- MIT. Coalesced, event-only handoff to this mod's native riposte helper.
local M={}
function M.new(api,report)
    local desired,pending,lastMode,lastLogging,started,failed=nil,false,nil,nil,false,false
    local function fail(reason)
        if failed then return end
        failed=true
        report('Riposte direction unavailable: '..tostring(reason)..'. Parry timing and dodge settings remain available.')
    end
    return function(values)
        if failed then return end
        if not pending and values.riposteDirection==lastMode and values.debugLogging==lastLogging then return end
        desired={values.riposteDirection,values.debugLogging}
        if pending then return end
        if type(api._EPRSet)~='function' or type(api._EPRStart)~='function' or type(api.ExecuteInGameThread)~='function' then
            fail('native helper or game-thread scheduler is missing');return
        end
        pending=true
        local ok,err=pcall(api.ExecuteInGameThread,function()
            pending=false
            local nextValues=desired
            local applied,reason=pcall(function()
                api._EPRSet(nextValues[1],nextValues[2])
                if not started then assert(api._EPRStart(),'native startup failed; see UE4SS.log');started=true end
                lastMode,lastLogging=nextValues[1],nextValues[2]
            end)
            if not applied then fail(reason) end
        end)
        if not ok then pending=false;fail(err) end
    end
end
return M
