// MIT. Event-driven directional openings. All UObject work stays on the game thread.
#include <Mod/CppUserModBase.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObject.hpp>
#include <DynamicOutput/Output.hpp>
#include <Unreal/Core/Windows/AllowWindowsPlatformTypes.hpp>
#include <Windows.h>
#include <MinHook.h>
#include <array>
#include <atomic>
#include <bit>
#include <cstring>
#include <intrin.h>
#include "Bridge.hpp"
#include "GameBuild.hpp"
#include "RiposteScript.hpp"
#include "VirtualSlot.hpp"
namespace EasierParry {
namespace {
using namespace RC::Unreal;
template<class T> T& field(void* p,size_t offset){return *reinterpret_cast<T*>(static_cast<char*>(p)+offset);}
uintptr_t moduleBase{};
template<class Fn> Fn at(uintptr_t rva){return reinterpret_cast<Fn>(moduleBase+rva);}
Settings settings;
DWORD gameThread{};
std::atomic_bool active{};
bool installed{},attempted{},riposteSupported{},listening{},slotOwned{};
std::wstring startError;
std::array<void*,4> hooked{};size_t hookCount{};
struct Metrics {
    uint64_t riposteParries{},riposteQueries{},riposteApplied{},riposteMisses{},riposteMicros{},riposteGuardDrift{};
    uint64_t decisions{},lateResponses{},expired{},attackClears{},dodgeClears{},replaced{},emptyQueries{};
} metrics;
uint64_t sessionGeneration{};
struct HitContext {void* combat;void* attack;void* response;uint64_t generation;bool decided{};};
thread_local HitContext* currentHit{};
struct Identity {uintptr_t address{};int index{-1},serial{};bool operator==(const Identity&)const=default;};
Identity identity(void* object) {
    if(!object)return {};
    auto index=field<int>(object,0xc);auto item=FUObjectArray::IndexToObject(index);
    if(!item||item->GetUObject()!=object||!FUObjectArray::IsValid(item,false))return {};
    return {reinterpret_cast<uintptr_t>(object),index,item->GetSerialNumber()};
}
void* resolveAttacker(const Identity& id) {
    if(!id.address)return nullptr;
    auto item=FUObjectArray::IndexToObject(id.index);
    // First serial assignment is valid; deletion watches still reject reuse.
    return item&&reinterpret_cast<uintptr_t>(item->GetUObject())==id.address&&FUObjectArray::IsValid(item,false)&&
        (!id.serial||item->GetSerialNumber()==id.serial)?item->GetUObject():nullptr;
}
Identity ownerId;
struct Session final:FUObjectDeleteListener {
    std::array<std::atomic_int,17> watched;
    std::atomic_uint32_t riposteWatches{},invalidated{};
    Session(){for(auto& x:watched)x.store(-1);}
    void NotifyUObjectDeleted(const UObjectBase*,int32_t index)override {
        uint32_t mask=watched[0].load(std::memory_order_relaxed)==index?1:0;
        for(auto pending=riposteWatches.load(std::memory_order_acquire);pending;pending&=pending-1){
            const auto slot=1+std::countr_zero(pending)*2;
            if(watched[slot].load(std::memory_order_relaxed)==index)mask|=1u<<slot;
            if(watched[slot+1].load(std::memory_order_relaxed)==index)mask|=1u<<(slot+1);
        }
        if(mask)invalidated.fetch_or(mask,std::memory_order_release);
    }
    void OnUObjectArrayShutdown()override {
        active=false;invalidated.fetch_or(0x1ffff);
        if(listening){FUObjectArray::RemoveUObjectDeleteListener(this);listening=false;}
    }
} session;
struct Riposte {Identity combat,stub;double captured{};uint8_t block{};uint64_t sequence{};};
std::array<Riposte,8> ripostes{};
uint64_t riposteSequence{};uint32_t riposteMask{};
void clearRiposte(size_t i) {
    riposteMask&=~(1u<<i);session.riposteWatches.store(riposteMask,std::memory_order_release);
    ripostes[i]={};session.watched[1+i*2]=-1;session.watched[2+i*2]=-1;
}
void clearRipostes(){while(riposteMask)clearRiposte(std::countr_zero(riposteMask));}
void clearSession(){clearRipostes();ownerId={};session.watched[0]=-1;++sessionGeneration;}
bool live(){return active.load(std::memory_order_relaxed)&&GetCurrentThreadId()==gameThread;}
bool managed(void* combat) {
    const auto deleted=session.invalidated.exchange(0,std::memory_order_acq_rel);
    if(deleted&1)clearSession();
    else for(size_t i=0;i<ripostes.size();++i)if(deleted&(3u<<(1+i*2)))clearRiposte(i);
    if(!combat||field<uintptr_t>(combat,0)!=moduleBase+Build::PlayerCombatVtable)return false;
    const auto id=identity(combat);
    if(!id.address)return false;
    if(ownerId.address!=id.address||ownerId.index!=id.index||(ownerId.serial&&ownerId.serial!=id.serial))clearSession();
    ownerId=id;session.watched[0]=id.index;
    if(!(field<uint8_t>(combat,0x8e)&0x10)||field<uint8_t>(combat,0xb28)==10||field<uint8_t>(combat,0xb28)==11){clearRipostes();return false;}
    return true;
}
double expireRipostes(void* combat) {
    // Read native gameplay time only at hit/query events; pause/dilation remain
    // native and no tick hook, background timer or settings watcher is needed.
    const double now=at<double(*)(void*)>(Build::WorldTime)(combat);
    for(auto pending=riposteMask;pending;pending&=pending-1){
        const auto i=std::countr_zero(pending);const double age=now-ripostes[i].captured;
        if(!std::isfinite(now)||age<0||age>=1.0){if(settings.debugLogging)++metrics.expired;clearRiposte(i);}
    }
    return now;
}
using Script=void(*)(void*,void*,void*);
Script originalRiposteQuery{};
bool(*originalQueueAttack)(void*,void*){};
void(*originalOnDodge)(void*,void*){};
void(*originalReactToHit)(void*,void*,void*,void*){};
uint8_t(*originalResolveReaction)(void*,uint8_t,void*){};
uint64_t nextReport{};
uint64_t clockMicros(){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return t.QuadPart/f.QuadPart*1000000+t.QuadPart%f.QuadPart*1000000/f.QuadPart;}
void reportMetrics() {
    if(!settings.debugLogging)return;
    const auto now=GetTickCount64();if(now<nextReport)return;nextReport=now+5000;
    RC::Output::send(L"[EasierParry] Riposte mode="+std::to_wstring(settings.riposteDirection)+
        L" parries="+std::to_wstring(metrics.riposteParries)+L" queries="+std::to_wstring(metrics.riposteQueries)+
        L" applied="+std::to_wstring(metrics.riposteApplied)+L" misses="+std::to_wstring(metrics.riposteMisses)+
        L" queryUs="+std::to_wstring(metrics.riposteMicros)+L" guardDrift="+std::to_wstring(metrics.riposteGuardDrift)+
        L" decisions="+std::to_wstring(metrics.decisions)+L" lateResponses="+std::to_wstring(metrics.lateResponses)+
        L" expired="+std::to_wstring(metrics.expired)+L" attackClears="+std::to_wstring(metrics.attackClears)+
        L" dodgeClears="+std::to_wstring(metrics.dodgeClears)+L" replaced="+std::to_wstring(metrics.replaced)+
        L" emptyQueries="+std::to_wstring(metrics.emptyQueries)+L"\n");
}
void riposteUnavailable(const std::wstring& reason) {
    riposteSupported=false;clearRipostes();
    if(settings.logLevel>=2)RC::Output::send(L"[EasierParry][WARN] Riposte direction unavailable: "+reason+L". Using vanilla openings; parry timing and dodge features remain available.\n");
}
void replaceReaction(void* attack) {
    auto target=field<void*>(attack,0x28);
    // Another reaction from this enemy replaces its old pending parry, even
    // when this hit was an ordinary block, omniblock or an unguarded hit.
    for(auto pending=riposteMask;pending;pending&=pending-1){
        const auto i=std::countr_zero(pending);
        if(ripostes[i].combat.address==reinterpret_cast<uintptr_t>(target)){
            if(settings.debugLogging)++metrics.replaced;
            clearRiposte(i);
        }
    }
}
void rememberRiposte(void* combat,void* attack) {
    auto target=field<void*>(attack,0x28);
    // Native parries can use the previous guard after the current guard clears
    // or moves. The incoming hit keeps the direction that was actually parried.
    const auto block=parriedBlock(field<uint8_t>(attack,0x18));
    if(!target||target==combat||riposteSide(block,settings.riposteDirection)<0)return;
    const auto targetId=identity(target);if(!targetId.address)return;
    const auto stubId=identity(field<void*>(target,0x1070));if(!stubId.address)return;
    const auto now=expireRipostes(combat);
    if(!std::isfinite(now))return;
    // A nested reaction may have recorded this enemy since our entry. Keep
    // exactly one record, belonging to the latest confirmed native decision.
    replaceReaction(attack);
    size_t slot=0;
    for(size_t i=0;i<ripostes.size();++i){
        if(!(riposteMask&(1u<<i))){slot=i;break;}
        if(ripostes[i].sequence<ripostes[slot].sequence)slot=i;
    }
    ripostes[slot]={targetId,stubId,now,block,++riposteSequence};
    session.watched[1+slot*2].store(targetId.index,std::memory_order_relaxed);
    session.watched[2+slot*2].store(stubId.index,std::memory_order_relaxed);
    riposteMask|=1u<<slot;
    session.riposteWatches.store(riposteMask,std::memory_order_release);
    if(settings.debugLogging){
        ++metrics.riposteParries;
        if(field<uint8_t>(combat,0x960)!=block)++metrics.riposteGuardDrift;
    }
}
uint8_t resolveReaction(void* combat,uint8_t candidate,void* attackerWeapon) {
    const auto result=originalResolveReaction(combat,candidate,attackerWeapon);
    auto hit=currentHit;
    if(!hit||hit->combat!=combat||hit->decided||reinterpret_cast<uintptr_t>(_ReturnAddress())!=moduleBase+Build::ResolveReactionReturn||
       !live()||!riposteSupported||!settings.riposteDirection||hit->generation!=sessionGeneration)return result;
    hit->decided=true;
    if(!managed(combat)||hit->generation!=sessionGeneration)return result;
    if(settings.debugLogging)++metrics.decisions;
    // The checked base ReactToHit switch assigns the parry action for result 2,
    // then notifies the enemy. Capture before that notification can create its
    // opening, but after the game has finished resolving the hit outcome.
    if(result==2){
        if(settings.debugLogging&&(!hit->response||field<void*>(hit->response,0)!=field<void*>(combat,0x268)))++metrics.lateResponses;
        rememberRiposte(combat,hit->attack);
    }
    return result;
}
bool riposteGraph(void* frame) {
    auto node=field<void*>(frame,0x10),task=field<void*>(frame,0x18);
    if(!node||!task||field<uint64_t>(node,0x18)!=Build::riposteGraphName)return false;
    auto cls=field<void*>(node,0x20);
    if(!cls||field<uint64_t>(cls,0x18)!=Build::riposteClassName||field<void*>(task,0x10)!=cls)return false;
    auto package=field<void*>(cls,0x20),parent=field<void*>(cls,0x40);
    if(!package||field<uint64_t>(package,0x18)!=Build::ripostePackageName)return false;
    auto script=field<uintptr_t>(node,0x60);
    // The original exec has consumed EX_EndFunctionParms before we inspect.
    if(!parent||field<uint64_t>(parent,0x18)!=Build::riposteBaseName||!script||field<int>(node,0x68)!=RiposteScript::size||
       field<uintptr_t>(frame,0x20)!=script+RiposteScript::queryEnd+1){
        riposteUnavailable(L"weak-spot task layout or query call site differs");return false;
    }
    for(const auto& part:RiposteScript::bytes)if(std::memcmp(reinterpret_cast<void*>(script+part.offset),part.value,part.length)!=0){
        riposteUnavailable(L"weak-spot direction branch differs at bytecode "+std::to_wstring(part.offset));return false;
    }
    for(size_t i=0;i<RiposteScript::objects.size();++i){
        auto object=field<void*>(reinterpret_cast<void*>(script),RiposteScript::objects[i].offset);
        if(!object||field<uint64_t>(object,0x18)!=RiposteScript::names[i]){
            riposteUnavailable(L"weak-spot direction function or effect reference differs");return false;
        }
    }
    for(const auto& property:RiposteScript::properties){
        const auto value=field<uintptr_t>(reinterpret_cast<void*>(script),property.offset);
        if(!value||value!=field<uintptr_t>(reinterpret_cast<void*>(script),property.reference)){
            riposteUnavailable(L"weak-spot direction variable binding differs");return false;
        }
    }
    return true;
}
void applyRiposte(void* combat,void* frame,void* result) {
    if(!managed(combat))return;
    if(!riposteGraph(frame))return;
    if(settings.debugLogging)++metrics.riposteQueries;
    expireRipostes(combat);
    if(!riposteMask){if(settings.debugLogging)++metrics.emptyQueries;return;}
    auto task=field<void*>(frame,0x18),stub=field<void*>(task,0x28);
    bool applied=false;
    for(auto pending=riposteMask;pending;pending&=pending-1){
        const auto i=std::countr_zero(pending);
        const auto entry=ripostes[i];
        if(entry.stub.address!=reinterpret_cast<uintptr_t>(stub))continue;
        auto enemy=resolveAttacker(entry.combat);
        const bool valid=enemy&&resolveAttacker(entry.stub)==stub&&field<void*>(enemy,0x1070)==stub&&field<uint8_t>(enemy,0xb28)!=10;
        clearRiposte(i); // Consume before publishing; repeated queries stay native.
        const auto side=riposteSide(entry.block,settings.riposteDirection);
        if(valid&&side>=0){*static_cast<uint64_t*>(result)=Build::riposteTags[side];applied=true;}
        break;
    }
    if(settings.debugLogging){
        if(applied)++metrics.riposteApplied;else ++metrics.riposteMisses;
    }
}
void riposteQuery(void* combat,void* frame,void* result) {
    originalRiposteQuery(combat,frame,result);
    if(!live()||!riposteSupported||!settings.riposteDirection||(!riposteMask&&!settings.debugLogging)||!frame||!result)return;
    const auto before=settings.debugLogging?clockMicros():0;
    if(riposteMask)applyRiposte(combat,frame,result);
    else if(riposteGraph(frame)){++metrics.riposteQueries;++metrics.emptyQueries;}
    // Include context and bytecode validation, not just the final record lookup.
    if(settings.debugLogging){metrics.riposteMicros+=clockMicros()-before;reportMetrics();}
}
bool queueAttack(void* combat,void* tags) {
    if(!live()||!riposteMask||ownerId.address!=reinterpret_cast<uintptr_t>(combat))return originalQueueAttack(combat,tags);
    const auto sequence=riposteSequence;
    const auto result=originalQueueAttack(combat,tags);
    if(result&&live()&&riposteMask&&ownerId.address==reinterpret_cast<uintptr_t>(combat)&&sequence==riposteSequence){
        if(settings.debugLogging)metrics.attackClears+=std::popcount(riposteMask);
        clearRipostes();
    }
    return result;
}
void onDodge(void* combat,void* direction) {
    if(live()&&riposteMask&&ownerId.address==reinterpret_cast<uintptr_t>(combat)){
        if(settings.debugLogging)metrics.dodgeClears+=std::popcount(riposteMask);
        clearRipostes();
    }
    originalOnDodge(combat,direction);
}

void reactToHit(void* combat,void* animation,void* attack,void* response) {
    if(!live()||!riposteSupported||!settings.riposteDirection||!attack||!managed(combat)){
        originalReactToHit(combat,animation,attack,response);return;
    }
    replaceReaction(attack);
    HitContext hit{combat,attack,response,sessionGeneration};
    // Stack-scoped and thread-local: nested hits restore their caller's context,
    // and another thread cannot read these borrowed attack/response pointers.
    struct Scope {HitContext* previous;~Scope(){currentHit=previous;}} scope{currentHit};
    currentHit=&hit;
    // Forward the existing entry, including Combat Camera's entry detour. The
    // response is an output; inspecting it here would read an unfinished result.
    originalReactToHit(combat,animation,attack,response);
    if(settings.debugLogging)reportMetrics();
}
template<class Fn> void hook(uintptr_t rva,Fn detour,Fn& original) {
    auto address=at<void*>(rva);
    if(MH_CreateHook(address,reinterpret_cast<void*>(detour),reinterpret_cast<void**>(&original))!=MH_OK)
        throw std::runtime_error("Cannot create riposte hook at "+NativeCompatibility::location(rva));
    hooked[hookCount++]=address;
    if(MH_QueueEnableHook(address)!=MH_OK)throw std::runtime_error("Cannot queue riposte hook at "+NativeCompatibility::location(rva));
}
}
void reset(){if(GetCurrentThreadId()==gameThread)clearSession();}
void configure(const Settings& value) {
    if(installed&&GetCurrentThreadId()!=gameThread)return;
    if(settings.debugLogging)reportMetrics();
    clearSession();settings=value;settings.riposteDirection=std::clamp(settings.riposteDirection,0,2);
    metrics={};nextReport=0;active=installed;
}
void deactivate(){active=false;}
bool start(std::wstring& error) {
    if(attempted){error=startError;return installed;}attempted=true;
    gameThread=GetCurrentThreadId();moduleBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    try {
        NativeCompatibility::validateContract(moduleBase,Build::code,Build::pointers);
                    const auto prop=Build::TaskOwnerProperty;
            if(!NativeCompatibility::accessible(moduleBase,prop,64)||
               field<uint32_t>(at<void*>(prop),0x32)!=0x28||
               !NativeCompatibility::accessible(moduleBase,0x8d43a10,12)||
               field<uintptr_t>(at<void*>(prop),0)!=moduleBase+0x8d43a10||
               std::memcmp(at<void*>(0x8d43a10),"OwnerAIStub",12)!=0)
               throw std::runtime_error("Weak-spot task OwnerAIStub property layout differs");
            if(!NativeCompatibility::accessible(moduleBase,0x8e15780,64)||
               field<uint32_t>(at<void*>(0x8e15780),0x32)!=0x1070||
               !NativeCompatibility::accessible(moduleBase,0x8d3ebbc,7)||
               field<uintptr_t>(at<void*>(0x8e15780),0)!=moduleBase+0x8d3ebbc||
               std::memcmp(at<void*>(0x8d3ebbc),"AIStub",7)!=0)
                throw std::runtime_error("Enemy combat AIStub property layout differs");
            const uintptr_t blockNames[]{0x92f5b70,0x92f5b88,0x92f5b30,0x92f5b50};
            const char* blockText[]{"EBlockingDirection::Top","EBlockingDirection::Bottom","EBlockingDirection::Left","EBlockingDirection::Right"};
            if(!NativeCompatibility::accessible(moduleBase,0x92f8600,64))throw std::runtime_error("Blocking direction enum is unavailable");
            for(size_t i=0;i<4;++i){
                const auto length=std::strlen(blockText[i])+1;
                auto entry=at<void*>(0x92f8600+i*16);
                if(!NativeCompatibility::accessible(moduleBase,blockNames[i],length)||
                   field<uintptr_t>(entry,0)!=moduleBase+blockNames[i]||field<uint64_t>(entry,8)!=(1ull<<i)||
                   std::memcmp(at<void*>(blockNames[i]),blockText[i],length)!=0)
                    throw std::runtime_error("Blocking direction enum value differs");
            }
            auto name=[](uint64_t& value,const char* text){
                at<void(*)(uint64_t*,const char*,int)>(Build::MakeName)(&value,text,1);
                if(!value)throw std::runtime_error("Required weak-spot name is unavailable");
            };
            name(Build::riposteGraphName,"ExecuteUbergraph_LTT_AddWeakSpot");
            name(Build::riposteClassName,"LTT_AddWeakSpot_C");
            name(Build::ripostePackageName,"/Game/_Dawnwalker/AI/LogicTree/Tasks/LTT_AddWeakSpot");
            name(Build::riposteBaseName,"RebelAILogicNode_Task_BlueprintBase");
            const char* tags[]{"RebelAI.Direction.Top","RebelAI.Direction.Bottom","RebelAI.Direction.Left","RebelAI.Direction.Right"};
            for(size_t i=0;i<4;++i)name(Build::riposteTags[i],tags[i]);
            for(size_t i=0;i<RiposteScript::objects.size();++i)name(RiposteScript::names[i],RiposteScript::objects[i].name);

        auto status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)throw std::runtime_error("MinHook initialization failed");
        hook(Build::RiposteQuery,&riposteQuery,originalRiposteQuery);
        hook(Build::QueueAttack,&queueAttack,originalQueueAttack);
        hook(Build::OnDodge,&onDodge,originalOnDodge);
        hook(Build::ResolveReaction,&resolveReaction,originalResolveReaction);
        originalReactToHit=at<decltype(originalReactToHit)>(Build::ReactToHit);
        FUObjectArray::AddUObjectDeleteListener(&session);listening=true;
        if(MH_ApplyQueued()!=MH_OK)throw std::runtime_error("Riposte hook activation failed");
        // Mark ownership before exchange, so rollback also covers a failure to
        // restore page protection after a successful pointer exchange.
        slotOwned=true;
        if(!exchangeSlot(at<void**>(Build::ReactSlot),reinterpret_cast<void*>(originalReactToHit),reinterpret_cast<void*>(&reactToHit)))
            throw std::runtime_error("Cannot own ReactToHit virtual slot; another mod or page protection changed");
        riposteSupported=true;installed=true;active=true;return true;
    }catch(const std::exception& failure){
        const std::string reason=failure.what();startError.assign(reason.begin(),reason.end());stop();error=startError;return false;
    }
}
void stop() {
    active=false;installed=false;
    if(slotOwned){exchangeSlot(at<void**>(Build::ReactSlot),reinterpret_cast<void*>(&reactToHit),reinterpret_cast<void*>(originalReactToHit));slotOwned=false;}
    for(size_t i=0;i<hookCount;++i)MH_QueueDisableHook(hooked[i]);
    if(hookCount)MH_ApplyQueued();
    for(size_t i=0;i<hookCount;++i)MH_RemoveHook(hooked[i]);hookCount=0;
    if(listening){FUObjectArray::RemoveUObjectDeleteListener(&session);listening=false;}
}
}
