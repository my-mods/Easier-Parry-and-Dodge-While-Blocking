-- MIT. Persistent menu subscription; no work is performed when this module loads.
local M={}
function M.new(directory, report)
    local Adapter=dofile(directory..'UE4SSDawnwalkerSettings.lua')
    local schema=dofile(directory..'SettingsSchema.lua')
    local live=Adapter.new({modId="oOCamilleOo_EasierParryAndDodgeWhileBlocking",schema=schema,report=report,
        ids={
        ["enabled"]="enabled",
        ["parryWindowPercent"]="parryWindowPercent",
        ["dodgeWhileBlocking"]="dodgeWhileBlocking",
        ["dodgeWindowPercent"]="dodgeWindowPercent",
        ["dodgeInvulnerabilityWindowPercent"]="dodgeInvulnerabilityWindowPercent",
        ["riposteDirection"]="riposteDirection",
        ["debugLogging"]="debugLogging"
        }})
    local applyRiposte=dofile(directory..'RiposteSettings.lua').new(_G,report)
    local seed,attach=live.seed,live.attach
    live.seed=function(values)
        local result=seed(values);applyRiposte(result);return result
    end
    live.attach=function(callback)
        return attach(function(values,changes)
            applyRiposte(values);callback(values,changes)
        end)
    end
    live.start(function(id,callback)
        return dofile(directory..'dmm_api.lua').subscribe(id,callback)
    end)
    return live
end
return M
