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
        ["logLevel"]="logLevel"
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
        return dofile(directory..'ModDmmApi.lua').subscribe(id,function(values,...)
            dofile(directory..'ModDiagnostics.lua').setLevel(values.logLevel)
            local ok,err=pcall(callback,values,...)
            if not ok and report then report('Settings callback failed: '..tostring(err)) end
        end)
    end)
    return live
end
return M
