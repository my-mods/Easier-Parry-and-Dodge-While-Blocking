# Building Easier Parry and Dodge While Blocking

Use Windows x64, Visual Studio 2022 C++ build tools (MSVC 19.44), Windows SDK 10.0.26100.0, CMake 3.25 or newer, Git and Ninja. Start an **x64 Native Tools Command Prompt**. MASM is included with the C++ build tools.

Clone this repository and prepare the exact UE4SS SDK revision:

```bat
git clone https://github.com/my-mods/Easier-Parry-and-Dodge-While-Blocking.git
git clone https://github.com/UE4SS-RE/RE-UE4SS.git ue4ss-sdk
git -C ue4ss-sdk checkout 97b7e501c19d8b2b7c662feee73aaa0dc1f0a4d1
git -C ue4ss-sdk config submodule.deps/first/Unreal.url https://github.com/UE4SS-RE/UEPseudo.git
git -C ue4ss-sdk submodule update --init deps/first/Unreal
```

The Unreal headers must resolve to `eb40a05f49509bdeb1ac39287032b60af585cca8`. CMake checks both revisions. It fetches the pinned MinHook, Zydis and SDK header dependencies declared in `native/CMakeLists.txt`.

```bat
cd Easier-Parry-and-Dodge-While-Blocking
cmake -S native -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DUE4SS_SDK=../ue4ss-sdk
cmake --build build
```

The result is `build/main.dll`. The mod uses `/MD`, C++23 and reproducible compilation/link options. `Framecore2b.def` supplies the exact imported host symbols; a complete UE4SS rebuild is unnecessary.

The output is `build/main.dll`. Package it as `Dawnwalker/Binaries/Win64/ue4ss/Mods/EasierParryUE4SS/dlls/main.dll`, beside the existing Lua mod. Do not include personal `settings.ini` files. The native helper only implements riposte direction; the existing Lua scripts and cooked assets still implement timing and dodge behavior.

## Native riposte behavior

`_EPRSetLogV2` consumes riposte direction (0–2) and logging level (0–4), each from stack index 1. `_EPRStart` installs once on the game thread; `_EPRReset` clears pending openings when a save session closes. The timing enable switch does not control riposte.

The helper owns the player combat class’s ReactToHit virtual slot and forwards its existing entry, including a camera entry detour. Shutdown restores only a slot still owned by this helper. It observes the native reaction resolver’s final result at the verified parry decision, before the game notifies the enemy. The original result and the patch’s weapon checks remain unchanged.

GameBuild.hpp records the native entry points and field layouts used by patch 1.1. CodeCompatibility.hpp decodes the required argument, result and branch relationships with Zydis; named registrations, reflected property descriptors, the active-component bit, direction enum and virtual slot identify the operations. It does not compare complete function bodies or executable hashes. FName construction uses the host API. Only riposte stops when a required contract cannot be verified.

The weak-spot check identifies the native query expression in the named task, relative to the current bytecode position. Unrelated graph instructions, total graph length and query placement may differ. Eight bounded records retain the direction actually parried, with deletion, session, attack, dodge and one-second gameplay-time expiry checks. Gameplay time uses the native float-return clock ABI. No tick hook, polling or recurring worker is added.

Logging defaults to Warning. Only Debug enables diagnostic counters, timing and event-driven summaries. The timing and dodge Lua modules remain independent of this helper.

Zydis and Zycore are linked statically; their notices are included in LICENSES alongside the other dependencies.
