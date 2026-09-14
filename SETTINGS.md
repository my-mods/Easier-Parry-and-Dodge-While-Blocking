# Settings

Install [Mod Setting Menu 1.0.6 or later](https://www.nexusmods.com/thebloodofdawnwalker/mods/271) and UE4SS through Vortex. Settings are prepared at startup. Open Main Menu > Mod Settings > All Mods before or after loading a save. Select this mod, change settings and press Apply. **Apply updates the active game.** Restore discards unapplied changes; Reset selects this mod’s defaults.

The stable menu ID is `oOCamilleOo_EasierParryAndDodgeWhileBlocking`. The mod generates `settings.ini` beside `mod_settings.ini` in its UE4SS folder. That file stores the active preferences and is not shipped in the archive. Supported legacy preferences are imported on first use.

Missing, duplicate or invalid settings stop configuration loading and are reported in `Dawnwalker/Binaries/Win64/ue4ss/UE4SS.log`. Preserve the file before correcting it. If a menu save fails, preserve its temporary/backup files and follow the menu’s recovery instructions. Settings files are read at startup and save-load boundaries; menu callbacks use committed values without file I/O. After startup preparation, waiting at the main menu performs no settings work; travel and possession events use the current snapshot. Settings are never polled. `debugLogging` controls additional diagnostic logging; it defaults to Off.

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

**Dodge while blocking:** On enables the mod's dodge and guard recovery behavior. Off restores vanilla dodge and guard behavior, including the game's original activation checks and dodge completion paths. This control remains available when Parry timing adjustment is Off. Press Apply to save and update the active game; any active dodge finishes before the switch takes effect. It does not poll inputs or settings.

**Perfect dodge window:** Uses the same 20 percentages listed above. **100%** keeps the game's normal timing and is the default; **200%** doubles the time before an incoming hit in which a dodge can qualify as perfect or ultra. The game's direction and attacker-distance requirements still apply. This control remains available regardless of Parry timing adjustment and Dodge while blocking. It changes the native perfect-dodge timing check; invulnerability duration, animation speed, stamina costs and dodge activation restrictions keep their existing behavior. Press Apply to save and update the active game.

For manual edits, close the game and change `dodgeWindowPercent` in the generated `settings.ini` (10 to 5000). Values between menu presets work in gameplay; the menu requires a listed value. Existing settings receive the new 100% default at startup, with the previous file retained as `settings.ini.before-dodge-window`.

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

## Live Apply

Mod Setting Menu 1.0.6 or later is required. Its callback bridge also requires `HookProcessConsoleExec = 1` in `UE4SS-settings.ini`. Manage that loader setting through your Vortex loader configuration; this archive contains no replacement global UE4SS INI.

Settings are prepared when the game starts and are available from the main menu before the first save. Press **Apply** to save and update the active game. Changes made while loading are retained for the next valid player. Restore and Discard leave saved settings unchanged; Reset takes effect after Apply.

Parry timing, perfect-dodge timing and dodge while blocking update independently. Timing changes use retained original values; returning to 100% restores normal timing. Parry timing Off restores only the parry override. Queued changes share one bounded worker.
