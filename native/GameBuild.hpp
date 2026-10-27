// Patch 1.1 entry and layout evidence. These are operation contracts, not a game-version allowlist.
#pragma once
#include "CodeCompatibility.hpp"
namespace EasierParry::Build {
inline constexpr uintptr_t ResolveReaction=0x6025600, ReactToHit=0x6042f18, RiposteQuery=0x5ffaaf4, QueueAttack=0x6042770, OnDodge=0x603f294;
inline uintptr_t ResolveReactionReturn=0x6027c02;
inline constexpr uintptr_t PlayerCombatVtable=0x805c1c0, ReactSlot=PlayerCombatVtable+0x8e8, WorldTime=0x5ea8960;
inline constexpr size_t CombatState=0xb40, CombatStub=0x1088, GuardDirection=0x978;
inline uint64_t riposteGraphName{},riposteClassName{},ripostePackageName{},riposteBaseName{};
inline std::array<uint64_t,4> riposteTags{};
inline const NativeCompatibility::Check query_args{0x5ffaaf4,"query result pointer and FFrame code advance", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RBX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CMP,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RDX,ZYDIS_REGISTER_NONE,32,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RAX,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_SETNZ,1,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_ADD,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RDX,ZYDIS_REGISTER_NONE,32,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RAX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check query_result{0x5ffaaf4,"query writes the eight-byte tag result", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RBX,ZYDIS_REGISTER_NONE,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RAX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check queue_owner{0x5ffc220,"queue wrapper preserves owner argument", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check queue_call{0x5ffc220,"queue wrapper forwards owner and tag container", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RAX,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CALL,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,100935536,64}}}}
}};
inline const NativeCompatibility::Check queue_result{0x5ffc220,"queue wrapper consumes byte return", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RDI,ZYDIS_REGISTER_NONE,0,8},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8}}}}
}};
inline const NativeCompatibility::Check dodge_owner{0x5ffb598,"dodge wrapper preserves owner argument", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check dodge_call{0x5ffb598,"dodge wrapper forwards tag reference", {
    {ZYDIS_MNEMONIC_LEA,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RSP,ZYDIS_REGISTER_NONE,56,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CMOVNZ,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RBX,ZYDIS_REGISTER_NONE,56,64}}}},
    {ZYDIS_MNEMONIC_CMP,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RBX,ZYDIS_REGISTER_NONE,32,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_SETNZ,1,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_DIL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_ADD,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RBX,ZYDIS_REGISTER_NONE,32,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CALL,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,100922004,64}}}}
}};
inline const NativeCompatibility::Check hit_arguments{0x6042f18,"player reaction forwards attack and response to base reaction", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R9,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R15,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R14,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RBX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RAX,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CALL,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,100823956,64}}}}
}};
inline const NativeCompatibility::Check base_owner{0x6027394,"base reaction owner identity", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check base_attack{0x6027394,"base reaction attack argument", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R15,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check base_response{0x6027394,"base reaction response output", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R12,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R9,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check resolved_call{0x6027394,"resolved reaction owner, candidate byte and weapon arguments", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8,0,0,0,64},{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_R15,ZYDIS_REGISTER_NONE,48,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_DL,0,0,0,8},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_CALL,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,100816384,64}}}}
}};
inline const NativeCompatibility::Check outcome_branch{0x6027394,"resolved result two selects the parry branch", {
    {ZYDIS_MNEMONIC_MOVZX,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_ECX,0,0,0,32},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RSP,ZYDIS_REGISTER_NONE,33,8},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_CL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_EDX,0,0,0,32},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_ECX,0,0,0,32}}}},
    {ZYDIS_MNEMONIC_TEST,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_AL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_JZ,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,INT64_MIN,64}}}},
    {ZYDIS_MNEMONIC_SUB,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_EDX,0,0,0,32},{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,1,32}}}},
    {ZYDIS_MNEMONIC_JZ,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,INT64_MIN,64}}}},
    {ZYDIS_MNEMONIC_SUB,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_EDX,0,0,0,32},{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,1,32}}}},
    {ZYDIS_MNEMONIC_JZ,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,INT64_MIN,64}}}}
}};
inline const NativeCompatibility::Check parry_output{0x6027394,"parry branch selects owner parry response before enemy notification", {
    {ZYDIS_MNEMONIC_LEA,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RDI,ZYDIS_REGISTER_NONE,616,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R12,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check resolver_arguments{0x6025600,"resolver receives weapon, byte candidate and owner", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RSI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_BL,0,0,0,8},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_DL,0,0,0,8}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDI,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}}
}};
inline const NativeCompatibility::Check clock_context{0x5ea8960,"game-clock context ABI", {
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_R8D,0,0,0,32},{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,1,32}}}},
    {ZYDIS_MNEMONIC_MOV,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RDX,0,0,0,64},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_RCX,0,0,0,64}}}},
    {ZYDIS_MNEMONIC_CALL,1,{{{ZYDIS_OPERAND_TYPE_IMMEDIATE,0,0,0,20832096,64}}}}
}};
inline const NativeCompatibility::Check clock_load{0x5ea8960,"game-clock gameplay-time field", {
    {ZYDIS_MNEMONIC_MOVSD,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_XMM0,0,0,0,128},{ZYDIS_OPERAND_TYPE_MEMORY,0,ZYDIS_REGISTER_RAX,ZYDIS_REGISTER_NONE,1744,64}}}}
}};
inline const NativeCompatibility::Check clock_result{0x5ea8960,"game-clock float return ABI", {
    {ZYDIS_MNEMONIC_CVTPD2PS,2,{{{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_XMM0,0,0,0,128},{ZYDIS_OPERAND_TYPE_REGISTER,ZYDIS_REGISTER_XMM0,0,0,0,128}}}}
}};
inline void validate(uintptr_t image){
using namespace NativeCompatibility;
activeBit(image);
direction(image,0x95adb70,"EBlockingDirection::Top",1);
direction(image,0x95adb80,"EBlockingDirection::Bottom",2);
direction(image,0x95adb90,"EBlockingDirection::Left",4);
direction(image,0x95adba0,"EBlockingDirection::Right",8);
namedPointer(image,0x80af290,"GetDirectionRequiredForCombo",RiposteQuery);
namedPointer(image,0x80af410,"QueueAttack",0x5ffc220);
namedPointer(image,0x80af3a0,"OnDodge",0x5ffb598);
pointer(image,ReactSlot,ReactToHit,"Riposte player ReactToHit virtual slot differs");
property(image,0x90d3aa0,"CurrentState",CombatState);
property(image,0x90d37e0,"BlockingDirection",GuardDirection);
property(image,0x90d3fa0,"AIStub",CombatStub);
property(image,0x8f5a1b0,"OwnerAIStub",0x28);
require(image,query_args);
require(image,query_result);
require(image,queue_owner);
require(image,queue_call);
require(image,queue_result);
require(image,dodge_owner);
require(image,dodge_call);
require(image,hit_arguments);
require(image,base_owner);
require(image,base_attack);
require(image,base_response);
ResolveReactionReturn=require(image,resolved_call);
uintptr_t parryBranch{},parryOutput{};require(image,outcome_branch,nullptr,&parryBranch);
require(image,parry_output,&parryOutput);
if(parryOutput<parryBranch||parryOutput-parryBranch>15)throw std::runtime_error("Riposte result two no longer selects the parry response");
require(image,resolver_arguments);
require(image,clock_context);
require(image,clock_load);
require(image,clock_result);
}
}
