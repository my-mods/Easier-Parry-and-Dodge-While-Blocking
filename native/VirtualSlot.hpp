// MIT. Own one virtual dispatch slot without changing the function entry.
#pragma once
#include <Windows.h>
#include <intrin.h>
namespace EasierParry {
inline bool exchangeSlot(void** slot,void* expected,void* replacement) {
    DWORD protection{};
    if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection))return false;
    const bool changed=_InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
    DWORD ignored{};
    const bool restored=VirtualProtect(slot,sizeof(void*),protection,&ignored)!=0;
    return changed&&restored;
}
}
