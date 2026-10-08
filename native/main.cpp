// Host calls use the pinned UE4SS C++ interface and its registered Lua state.
#include <Mod/CppUserModBase.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <DynamicOutput/Output.hpp>
#include "Bridge.hpp"
using namespace RC;
using Lua=LuaMadeSimple::Lua;
static_assert(sizeof(CppUserModBase)==192,"Unsupported UE4SS C++ host layout");
static int nativeLogLevel=2;
class EasierParryMod final:public CppUserModBase {
public:
    EasierParryMod(){ ModName=STR("Easier Parry and Dodge While Blocking");ModVersion=STR("1.5.0");ModAuthors=STR("my-mods"); }
    void on_lua_start(StringViewType name,Lua& lua,Lua&,Lua&,Lua*) override {
        if(name!=STR("EasierParryUE4SS"))return;
        lua.register_function("_EPRSetLogV2",[](const Lua& l){
            // get_integer removes the argument from the host Lua stack.
            // Each next setting is therefore at index 1.
            auto number=[&](int lo,int hi){return static_cast<int>(std::clamp<int64_t>(l.get_integer(1),lo,hi));};
            EasierParry::Settings s;
            s.riposteDirection=number(0,2);s.logLevel=number(0,4);s.debugLogging=s.logLevel==4;nativeLogLevel=s.logLevel;
            EasierParry::configure(s);return 0;
        });
        lua.register_function("_EPRReset",[](const Lua&){EasierParry::reset();return 0;});
        lua.register_function("_EPRStart",[](const Lua& l){
            std::wstring error;bool ok=EasierParry::start(error);
            if(!ok&&nativeLogLevel>=1)Output::send(STR("[EasierParry] Disabled: ")+error+STR("\n"));
            l.set_bool(ok);return 1;
        });
    }
    void on_lua_stop(StringViewType name,Lua&,Lua&,Lua&,Lua*) override {
        if(name==STR("EasierParryUE4SS")) EasierParry::deactivate();
    }
    ~EasierParryMod()override {EasierParry::stop();}
};
extern "C" __declspec(dllexport) CppUserModBase* start_mod(){return new EasierParryMod;}
extern "C" __declspec(dllexport) void uninstall_mod(CppUserModBase* mod){delete mod;}
