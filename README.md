# Easier Parry and Dodge While Blocking

![Easier Parry and Dodge While Blocking](Nexus/thumbnail.jpg)

Makes parrying more forgiving in *The Blood of Dawnwalker*. Includes native guard recovery after dodging. Attack inputs retain the base game’s guard behavior.

- **200% parry timing window** by default (2 times normal), adjustable from **10% to 5,000%** of the game's difficulty-adjusted baseline. Choose from 20 percentages with closer spacing at low values and wider gaps at high values.
- Keeps held guard available after a dodge, using the game's native ability lifecycle. Attack inputs retain the base game’s guard behavior.
- Temporarily lowers guard for a dodge and resumes when the game's combat rules allow it. Actual guard release and ability cancellation still end guarding.
- **Dodge while blocking** can be turned Off in Mod Settings independently of parry timing. It defaults to On; Off blocks the player's dodge ability while block is held.
- **Perfect dodge window** is independently adjustable from **10% to 5,000%**, with the same 20 percentage choices as parry. It defaults to **100%** (normal timing); **200%** doubles the native timing window.
- Forward dodges use native state-change and timed completion to restore held guard. Failed or cancelled attempts release their input suppression.
- Native guard handling uses gameplay events. Optional Lua diagnostics observe transitions without changing guard or bindings.
- Parry and perfect-dodge timing load a fresh settings snapshot after save loading. The game handles dodge timing through its native combat settings, with no recurring timing checker or extra Lua callback per dodge.

Requires a Dawnwalker-compatible **UE4SS 3.x** installation. The archive includes both the dodge asset and the parry timing script.

## Installation

- Vortex: Install Easier-Parry-and-Dodge-While-Blocking.zip through Vortex, enable it and deploy.
- Manual: Copy the archive's Dawnwalker folder into The Blood of Dawnwalker game directory, preserving the folder structure.

## Settings

Use [Mod Setting Menu 1.0.5 or later](https://www.nexusmods.com/thebloodofdawnwalker/mods/271) from the main menu. Press Apply, then load a save. See [SETTINGS.md](SETTINGS.md) for all controls and supported settings. Console settings commands are retired.

With Logging On, native combat tracing records dodge state-check and ability-commit results, selected dodge direction, explicit ability end/cancel calls, and combat-state/input-tag snapshots. These diagnostics work independently of Blueprint tracing. An unavailable Blueprint dispatcher is reported once per mod launch; save loads do not restart those failed attempts.

Created by **oOCamilleOo**. Original mod code is under the [MIT license](LICENSE); underlying game assets remain the property of their respective rights holders. Nexus listing materials are maintained separately in [Nexus](Nexus/README.txt).


Bundled library

This mod includes the MIT-licensed ue4ss-common Lua helpers (https://github.com/my-mods/ue4ss-common). No separate library installation is required. Its license is included in LICENSES/EasierParryUE4SS-ue4ss-common.txt.
