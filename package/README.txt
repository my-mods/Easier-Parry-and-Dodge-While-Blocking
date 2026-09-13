# Easier Parry and Dodge While Blocking

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

# Settings

Install [Mod Setting Menu 1.0.5 or later](https://www.nexusmods.com/thebloodofdawnwalker/mods/271) and UE4SS through Vortex. On first use, load a save once to initialize the settings file, then return to Main Menu > Mod Settings > All Mods. Select this mod, change settings and press Apply. **Load a save after Apply.** Restore discards unapplied changes; Reset selects this mod’s defaults.

The stable menu ID is `oOCamilleOo_EasierParryAndDodgeWhileBlocking`. The mod generates `settings.ini` beside `mod_settings.ini` in its UE4SS folder. That file stores the active preferences and is not shipped in the archive. Supported legacy preferences are imported on first use.

Missing, duplicate or invalid settings stop configuration loading and are reported in `Dawnwalker/Binaries/Win64/ue4ss/UE4SS.log`. Preserve the file before correcting it. If a menu save fails, preserve its temporary/backup files and follow the menu’s recovery instructions. Settings are read only when a save loads. Waiting at the main menu performs no settings work; travel and possession events use the current snapshot. Settings are never polled. `debugLogging` controls additional diagnostic logging; it defaults to Off.

| Group | Setting | Choices or range |
| --- | --- | --- |
| General | Parry timing adjustment | Off, On |
| Parry | Parry window | 20 percentage choices from 10% to 5,000% |
| Dodge | Dodge while blocking | Off, On (default) |
| Dodge | Perfect dodge window | 20 percentage choices from 10% to 5,000%; default 100% |
| Diagnostics | Logging | Off, On |

**Parry window presets:** 10%, 25%, 50%, 75%, 100%, 125%, 150%, 175%, 200%, 250%, 300%, 400%, 500%, 750%, 1,000%, 1,500%, 2,000%, 3,000%, 4,000%, and 5,000%. The spacing keeps smaller adjustments close together and reaches large windows quickly.

**100%** is the normal difficulty-adjusted window; **200%** is twice as long and remains the default. **10%** is one tenth as long; **5,000%** is 50 times as long. This changes the window for a successful parry, not animation speed.

Reset restores the 200% default. For manual INI edits, `parryWindowPercent` accepts values from 10 to 5000. Gameplay accepts values between the menu choices; opening the menu page requires one of the listed values.

**Dodge while blocking:** On keeps the mod's dodge and guard recovery behavior. Off blocks the player's dodge ability while the block input is held. Release block to dodge. This control remains available when Parry timing adjustment is Off. Press Apply, then load a save. It uses the game's native ability activation check; it does not poll inputs or settings.

**Perfect dodge window:** Uses the same 20 percentages listed above. **100%** keeps the game's normal timing and is the default; **200%** doubles the time before an incoming hit in which a dodge can qualify as perfect or ultra. The game's direction and attacker-distance requirements still apply. This control remains available regardless of Parry timing adjustment and Dodge while blocking. It changes the native perfect-dodge timing check; invulnerability duration, animation speed, stamina costs and dodge activation restrictions keep their existing behavior. Press Apply, then load a save.

For manual edits, close the game and change `dodgeWindowPercent` in the generated `settings.ini` (10 to 5000). Values between menu presets work in gameplay; the menu requires a listed value. Existing settings receive the new 100% default on the next save load, with the previous file retained as `settings.ini.before-dodge-window`.

Console commands are not used to change settings.

Conditional rows and groups show relevant controls as you edit. Hidden options keep their saved values; hiding an option does not reset it. The interface uses toggles and labeled percentage choices; the numeric representation in settings.ini is an implementation detail.

**Logging** is the final menu setting and the only diagnostic control. Leave it Off for normal play; On writes troubleshooting details to `Dawnwalker/Binaries/Win64/ue4ss/UE4SS.log`.

When standard Lua Blueprint hooks are unavailable, GuardTrace records the first dispatcher error once per mod launch and stops those registration attempts. Native guard, dodge-request, attack-queue and combo tracing remain enabled across save loads. Additional native diagnostics observe the game's existing calls:

- `dodge.state_check`: the actual `CanEnterState(8)` result, with the current combat state. State 8 is dodge; this observes an eligibility check, not a new input request.
- `dodge.commit`: the actual ability-commit result. A false result records a rejected commit without attempting it again.
- `dodge.animation`: the direction tag returned by animation selection. It confirms selection was reached, not that a visible animation frame was rendered.
- `dodge.cancel.pre/post` and `dodge.end_call.pre/post`: explicit native cancellation/end calls, with input and suppression tags before and after. Block and light/heavy attack abilities receive the same commit/end/cancel observations when they call those functions.

These events restore decision details without requiring Blueprint hooks. They do not reproduce every Blueprint activation/end notification or tag callback: cancellation performed entirely in native engine code may bypass the observed Blueprint-callable end/cancel functions. A request with no later decision event is inconclusive; it may have been rejected earlier or omitted by capture limits. No successful activation or complete cancellation history is inferred from a request alone.

The trace is read-only, filters exact ability classes and local player contexts, and uses bounded event/snapshot/output budgets. It does not retry inputs, call eligibility/commit functions to obtain diagnostic results, retain borrowed return structs, scan for objects during events, or add an idle polling worker. Actually unloaded Blueprint functions retain finite retries after relevant construction events. Restart the game after changing the loader profile to check its capabilities again.

Trace output is limited to 2,048 records per save-load capture. Load a save for a new capture. Logging Off disables diagnostic capture and pending trace work without changing parry timing or dodge behavior.
