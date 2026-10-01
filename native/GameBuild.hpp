#pragma once
#include "CodeCompatibility.hpp"
namespace EasierParry::Build {
inline constexpr uintptr_t ResolveReaction=0x5dff468;
inline constexpr uintptr_t ResolveReactionReturn=0x5e01a38;
inline constexpr uintptr_t ReactToHit=0x5e1c800;
inline constexpr uintptr_t RiposteQuery=0x5dd5890;
inline constexpr uintptr_t QueueAttack=0x5e1c058;
inline constexpr uintptr_t OnDodge=0x5e18ba8;
inline constexpr uintptr_t TaskOwnerProperty=0x8ca04d0;
inline uint64_t riposteGraphName{},riposteClassName{},ripostePackageName{},riposteBaseName{};
inline std::array<uint64_t,4> riposteTags{};
inline constexpr uintptr_t PlayerCombatVtable=0x7d64078;
inline constexpr uintptr_t MakeName=0x13512bc;
inline constexpr uintptr_t WorldTime=0x5047f14, ReactSlot=PlayerCombatVtable+0x8e8;
// Reference: Steam 25232147. Hash only functions/layouts this feature uses.
inline constexpr std::array<NativeCompatibility::Code,13> code{{
{0x5dd5890,87,"2ff82a31983d7af5033b74a28a96bbc377b9d89241a96b5b327178e3e8c1c301"},
{0x5e1c058,190,"2e63adda340c572121eb27f7ca4fc2b3c2000372d66764fd89de54d3b144384b"},
{0x5e18ba8,299,"e1cd5a3138f36c89e73f36091bb8f15a3856d9df82485947f0f9ee15981c0722"},
{0x5dd74de,17,"88df69ac95184dc2c614b9f83be91f4b79f581414156f0bbfa4c8bd68dc6f67c"},
{0x28879ac,43,"0a26234bc9f2f603f2dcd517fd2b50db5fca9d04c275a848dac97d63590d21ba"},
{0x5e12ca4,85,"cb75a89693f977b53aadab41626fd8f843d0f8b681556376ca2ac0d344381794"},
{0x13512bc,317,"9b5feb9eca626d5851bb2351844a7da12720c9dc7528052b37bf28761cdb2aae"},
{0x5e1c805,1065,"a929ed6fd4259012e29b88f9a9e8c734cd034a2e1f83302fccdb3fe5b11d5b1d"},
{0x5047f14,41,"efb6cd23625e342ed2e75a1ab24e9587d137d758ddb0330ae14b08b0d43a3ca0"},
{0x113b1b0,95,"28ec1aac6ae9a0e3b75f7c895ac556494238aaea2bb8bdc55c96063ad74b8a7e"},
{0x118cb1c,119,"1531a1b3ebfeabdb2b7ebe5d7378f1debdbee9b4a00249f5e08e2e75a667f5ae"},
{0x5dff468,96,"97e3461110b2eafebd1805c414942e05b8a27867f717d0d1be6e7595a3daed0d"},
{0x5e011dc,3762,"cc2991c11ff874ca618c9c35c38efda1c25311ef11f8517c720c2026c8dd32d5"},
}};
inline constexpr std::array<NativeCompatibility::Pointer,2> pointers{{{ReactSlot,ReactToHit},{PlayerCombatVtable+0x198,0x118cb1c}}};
}
