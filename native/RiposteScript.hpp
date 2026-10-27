// Identify only the native query expression we override, relative to its end.
// Other weak-spot logic and total graph length are not riposte dependencies.
#pragma once
#include <array>
#include <cstdint>
namespace EasierParry::RiposteScript {
inline constexpr uintptr_t queryEnd=0x88;
inline constexpr uintptr_t prefixSize=0x32;
struct Bytes {uintptr_t offset;const char* value;size_t length;};
inline constexpr std::array<Bytes,6> bytes{{
{0x57,"\x0f",1},{0x60,"\x00",1},{0x69,"\x19\x00",2},
{0x73,"\x0a\x00\x00\x00",4},{0x7f,"\x1c",1},{0x88,"\x16",1}
}};
struct Object {uintptr_t offset;const char* name;};
inline constexpr std::array<Object,1> objects{{{0x80,"GetDirectionRequiredForCombo"}}};
inline std::array<uint64_t,objects.size()> names{};
struct Property {uintptr_t offset,reference;};
inline constexpr std::array<Property,3> properties{{{0x58,0x58},{0x61,0x58},{0x77,0x58}}};
}
