# Easier Parry and Dodge While Blocking

![Easier Parry and Dodge While Blocking](Nexus/thumbnail.jpg)

Makes parrying more forgiving in *The Blood of Dawnwalker*. Includes native guard recovery after dodging. Attack inputs retain the base game’s guard behavior.

- **200% parry timing window** by default (2 times normal), adjustable from **10% to 5,000%** of the game's difficulty-adjusted baseline. Choose from 20 percentages with closer spacing at low values and wider gaps at high values.
- Keeps held guard available after a dodge, using the game's native ability lifecycle. Attack inputs retain the base game’s guard behavior.
- Temporarily lowers guard for a dodge and resumes when the game's combat rules allow it. Actual guard release and ability cancellation still end guarding.
- **Dodge while blocking** defaults to On. Off restores vanilla dodge and guard behavior, independently of parry timing. Switching waits for any active dodge to finish.
- **Perfect dodge window** is independently adjustable from **10% to 5,000%**, with the same 20 percentage choices as parry. It defaults to **100%** (normal timing); **200%** doubles the native timing window.
- Forward dodges use native state-change and timed completion to restore held guard. Failed or cancelled attempts release their input suppression.
- Native guard handling uses gameplay events. Optional Lua diagnostics observe transitions without changing guard or bindings.
- Parry and perfect-dodge timing accept saved changes during play and load a fresh snapshot after save loading. The game handles dodge timing through its native combat settings, with no recurring timing checker or extra Lua callback per dodge.

Requires a Dawnwalker-compatible **UE4SS 3.x** installation. The archive includes both the dodge asset and the parry timing script.

## Installation

- Vortex: Install Easier-Parry-and-Dodge-While-Blocking.zip through Vortex, enable it and deploy.
- Manual: Copy the archive's Dawnwalker folder into The Blood of Dawnwalker game directory, preserving the folder structure.

## Settings

Use [Mod Setting Menu 1.0.6 or later](https://www.nexusmods.com/thebloodofdawnwalker/mods/271) from the main menu. Press Apply to save and update the active game. See [SETTINGS.md](SETTINGS.md) for all controls and supported settings. Console settings commands are retired.

With Logging On, native combat tracing records dodge state-check and ability-commit results, selected dodge direction, explicit ability end/cancel calls, and combat-state/input-tag snapshots. These diagnostics work independently of Blueprint tracing. An unavailable Blueprint dispatcher is reported once per mod launch; save loads do not restart those failed attempts.

Created by **oOCamilleOo**. Original mod code is under the [MIT license](LICENSE); underlying game assets remain the property of their respective rights holders. Nexus listing materials are maintained separately in [Nexus](Nexus/README.txt).


Bundled library

This mod includes the MIT-licensed ue4ss-common Lua helpers (https://github.com/my-mods/ue4ss-common). No separate library installation is required. Its license is included in LICENSES/EasierParryUE4SS-ue4ss-common.txt.

## Live settings

Mod Setting Menu 1.0.6 or later is required. Its callback bridge also requires `HookProcessConsoleExec = 1` in `UE4SS-settings.ini`. Manage that loader setting through your Vortex loader configuration; this archive contains no replacement global UE4SS INI.

Settings are prepared when the game starts and are available from the main menu before the first save. Press **Apply** to save and update the active game. Changes made while loading are retained for the next valid player. Restore and Discard leave saved settings unchanged; Reset takes effect after Apply.

Parry timing, perfect-dodge timing and dodge while blocking update independently. Timing changes use retained original values; returning to 100% restores normal timing. Parry timing Off restores only the parry override. Queued changes share one bounded worker.

Logging is the final, sole diagnostic control. It changes immediately; verbose logging is Off by default. Logs are written to `Dawnwalker/Binaries/Win64/ue4ss/UE4SS.log`. Settings are never polled.
