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

The Unreal headers must resolve to `eb40a05f49509bdeb1ac39287032b60af585cca8`. CMake checks both revisions. It fetches the pinned MinHook and SDK header dependencies declared in `native/CMakeLists.txt`.

```bat
cd Easier-Parry-and-Dodge-While-Blocking
cmake -S native -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DUE4SS_SDK=../ue4ss-sdk
cmake --build build
```

The result is `build/main.dll`. The mod uses `/MD`, C++23 and reproducible compilation/link options. `Framecore2b.def` supplies the exact imported host symbols; a complete UE4SS rebuild is unnecessary.

The output is `build/main.dll`. Package it as `Dawnwalker/Binaries/Win64/ue4ss/Mods/EasierParryUE4SS/dlls/main.dll`, beside the existing Lua mod. Do not include personal `settings.ini` files. The native helper only implements riposte direction; the existing Lua scripts and cooked assets still implement timing and dodge behavior.

## Native riposte behavior

The registered `_EPRSet` callback consumes two integers at stack index 1: `riposteDirection` (0–2) followed by `debugLogging` (0–1). `_EPRStart` installs once on the game thread; `_EPRReset` clears pending openings when a save session closes. Lua settings notifications coalesce into one game-thread callback. The timing enable switch never controls this helper.

The helper owns the player combat class's ReactToHit virtual slot at `0x7d64078 + 0x8e8`. It forwards the unchanged function entry at `0x5e1c800`, allowing Combat Camera's entry detour to run in either load order. Pointer replacement uses a compare/exchange while the page is temporarily writable; shutdown restores only a slot still owned by this helper. No instance vtable is cloned. The hit-body contract starts after the five-byte entry instruction because that instruction belongs to the independently chained entry detour. Camera validates the complete entry/body it patches; parry validates the remaining body and the virtual slot it replaces. Changed required code or layouts disable only this native feature, with a precise error.

Forward the hit with a thread-local, stack-scoped context. The response parameter is an output that base ReactToHit finishes resolving; its value on entry cannot identify a successful parry. Capture when the native reaction resolver at `0x5dff468` returns 2 to the checked call site at `0x5e01a38`. The checked base ReactToHit switch then assigns the player's parry action and notifies the enemy. Capturing at this boundary also covers an opening created synchronously by that notification. Other callers, combat components, threads and invalidated sessions cannot claim the context; nested hits restore their caller's context. The native return value is preserved.

Attack data `+0x18` maps cardinal attack directions 1/2/3/4 to guard bits 8/4/1/2 (screen Right/Left/Top/Bottom). This remains valid when current guard input at `+0x960` has cleared or changed. Attack data `+0x28` identifies the enemy combat component; component `+0x1070` identifies its AI stub. Unknown/non-directional attacks keep native selection.

Up to eight one-use records retain indexed enemy/stub identities, direction, sequence and capture time. A newer reaction replaces that enemy's record. Accepted QueueAttack calls, dodge events, changed native settings, save-session close, owner replacement, deletion, invalid combat context and expiry clear records. Identity validation accepts an initially zero serial without allocating one, with deletion watches armed throughout. Sparse atomic watch publication limits deletion scans to occupied records.

Expiry is evaluated lazily at hit/query events using the native gameplay clock (`0x5047f14`, validated along with its world-context helper and component GetWorld slot/body). A record expires after one gameplay second; nonfinite or backwards time clears it. Pausing does not advance gameplay time. There is no tick hook, timer, object enumeration, filesystem watch or recurring Lua work. With Logging Off, idle, Vanilla and deactivated paths skip object lookup, clock calls and bytecode validation.

Four entry hooks cover GetDirectionRequiredForCombo (`0x5dd5890`), QueueAttack (`0x5e1c058`), OnDodge (`0x5e18ba8`) and the reaction resolver (`0x5dff468`). Thirteen scoped code contracts cover the required functions, including the resolver and base ReactToHit's outcome switch. The query calls the original first, preserving parameter consumption and unrelated callers. Only the checked 977-byte LTT_AddWeakSpot graph, class, package, parent, OwnerAIStub property and query call site are accepted. RiposteScript.hpp validates direction comparisons, branch destinations, resolved function/effect names and property bindings while allowing relocated pointers and FName IDs. A matched record is consumed before publishing its direction tag.

The task maps generic Top/Bottom to Bottom/Top weak-spot effects; the indicator reverses that vertical mapping again. Generic direction therefore matches the visible opening side. Same keeps the captured side; Opposite exchanges Top/Bottom and Left/Right. Skill eligibility, opening chance, effect lifetime, removal and damage remain native.

Logging defaults Off. When enabled, event-driven five-second summaries report mode, resolved decisions, parries, unfinished response outputs, opening queries, applied overrides, unmatched or empty queries, guard drift, expiry, attack/dodge cancellations, replacements and total query helper microseconds. Logging also inspects opening queries with no pending record, which previously went uncounted. With Logging Off, those empty queries retain their immediate return. Formatting, diagnostic counters and timers are guarded. Query time includes context and script checks, excluding the original query. No benchmark alone establishes an in-game FPS result.
