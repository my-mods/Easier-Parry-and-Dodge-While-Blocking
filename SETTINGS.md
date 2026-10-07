# Settings

Install [Mod Setting Menu 1.0.6 or later](https://www.nexusmods.com/thebloodofdawnwalker/mods/271) and UE4SS through Vortex. Settings are prepared at startup. Open Main Menu > Mod Settings > All Mods before or after loading a save. Select this mod, change settings and press Apply. **Apply updates the active game.** Restore discards unapplied changes; Reset selects this mod’s defaults.

The stable menu ID is `oOCamilleOo_EasierParryAndDodgeWhileBlocking`. The mod generates `settings.ini` beside `mod_settings.ini` in its UE4SS folder. That file stores the active preferences and is not shipped in the archive. Supported legacy preferences are imported on first use.

Missing, duplicate or invalid settings stop configuration loading and are reported in `Dawnwalker/Binaries/Win64/ue4ss/UE4SS.log`. Preserve the file before correcting it. If a menu save fails, preserve its temporary/backup files and follow the menu’s recovery instructions. Settings files are read at startup and save-load boundaries; menu callbacks use committed values without file I/O. After startup preparation, waiting at the main menu performs no settings work; travel and possession events use the current snapshot. Settings are never polled. `debugLogging` controls additional diagnostic logging; it defaults to Off.

| Group | Setting | Choices or range |
| --- | --- | --- |
| General | Parry timing adjustment | Off, On |
| Parry | Parry window | 20 percentage choices from 10% to 5,000% |
| Riposte | Riposte direction | Vanilla (Random), Opposite direction (default), Same direction |
| Dodge | Dodge while blocking | Off, On (default) |
| Dodge | Perfect dodge window | 20 percentage choices from 10% to 5,000%; default 100% |
| Dodge | Dodge invulnerability window | 20 percentage choices from 25% to 5,000%; default 100% |
| Diagnostics | Logging | Off, On |

**Parry window presets:** 10%, 25%, 50%, 75%, 100%, 125%, 150%, 175%, 200%, 250%, 300%, 400%, 500%, 750%, 1,000%, 1,500%, 2,000%, 3,000%, 4,000%, and 5,000%. The spacing keeps smaller adjustments close together and reaches large windows quickly.

**100%** is the normal difficulty-adjusted window; **200%** is twice as long and remains the default. **10%** is one tenth as long; **5,000%** is 50 times as long. This changes the window for a successful parry, not animation speed.

Reset restores the 200% default. For manual INI edits, `parryWindowPercent` accepts values from 10 to 5000. Gameplay accepts values between the menu choices; opening the menu page requires one of the listed values.

**Riposte direction:** Same and Opposite refer to the on-screen side of the attack you successfully parried, even after guard input changes or clears. Opposite swaps Left/Right and Top/Bottom. The game still controls the Critical Riposte skill requirements, opening chance, duration and damage. This setting changes the opening, not your attack input. Combat Camera is not required.

The saved key is `riposteDirection`: 0 = Vanilla (Random), 1 = Opposite direction (default), 2 = Same direction. Existing parry settings receive this new default with the original file retained as `settings.ini.before-riposte-direction`. Choose the desired mode in this mod; the old Combat Camera preference is no longer used. No complete INI replacement is required.

**Dodge while blocking:** On enables the mod's dodge and guard recovery behavior. Off restores vanilla dodge and guard behavior, including the game's original activation checks and dodge completion paths. This control remains available when Parry timing adjustment is Off. Press Apply to save and update the active game; any active dodge finishes before the switch takes effect. It does not poll inputs or settings.

**Perfect dodge window:** Uses the same 20 percentages listed above. **100%** keeps the game's normal timing and is the default; **200%** doubles the time before an incoming hit in which a dodge can qualify as perfect or ultra. The game's direction and attacker-distance requirements still apply. This control remains available regardless of Parry timing adjustment and Dodge while blocking. Invulnerability duration has its own control below. Press Apply to save and update the active game.

For manual edits, close the game and change `dodgeWindowPercent` in the generated `settings.ini` (10 to 5000). Values between menu presets work in gameplay; the menu requires a listed value. Existing settings receive the new 100% default at startup, with the previous file retained as `settings.ini.before-dodge-window`.

**Dodge invulnerability window:** Scales the game's normal and fatigued side/back dodge protection timers independently of the other controls. **100%** keeps the normal duration, **25%** gives one-quarter duration, **200%** doubles it, and **5,000%** gives 50 times the original timer. Reset restores **100%**. Both timers retain their own baseline; a zero baseline remains zero.

Presets: 25%, 50%, 75%, 100%, 125%, 150%, 175%, 200%, 250%, 300%, 400%, 500%, 750%, 1,000%, 1,500%, 2,000%, 2,500%, 3,000%, 4,000%, and 5,000%.

Press Apply to save and use the new duration for subsequent dodges. The game removes protection when the timer expires or the dodge ends or is cancelled, whichever happens first. High percentages do not grant protection after leaving the dodge. Forward dodges use a separate effect and are not changed by this control. Animation speed, stamina costs and dodge activation rules remain the same.

For manual edits, close the game and set `dodgeInvulnerabilityWindowPercent` in the generated `settings.ini` to a number from 25 to 5000. Values between presets work in gameplay; use a listed value when opening the menu. Existing files receive the new 100% default. Values saved by the earlier increase-based control are converted to percentages of normal duration, capped at 5000%. The previous file is retained as `settings.ini.before-dodge-invulnerability-window`.

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

Trace output is limited to 2,048 records per save-load capture. Load a save for a new capture. Logging below Debug disables diagnostic capture and pending trace work without changing parry timing or dodge behavior.

## Live Apply

Mod Setting Menu 1.0.6 or later is required. Its callback bridge also requires `HookProcessConsoleExec = 1` in `UE4SS-settings.ini`. Manage that loader setting through your Vortex loader configuration; this archive contains no replacement global UE4SS INI.

Settings are prepared when the game starts and are available from the main menu before the first save. Press **Apply** to save and update the active game. Changes made while loading are retained for the next valid player. Restore and Discard leave saved settings unchanged; Reset takes effect after Apply.

Riposte direction, parry timing, perfect-dodge timing, dodge invulnerability and dodge while blocking update independently. Returning any timing window to 100% restores the corresponding original timing. Parry timing Off restores only the parry override. Queued changes share one bounded worker. With Logging Debug, changed timing fields report their multiplier and verified before/after seconds; an unavailable field reports its name while other settings continue.

Native riposte diagnostics are aggregated at most once every five seconds during hit reactions and opening queries. They distinguish confirmed parries, applied directions, openings without a saved direction, and records cleared by expiry, attacks, dodges or newer reactions. Logging below Debug skips diagnostic counters, formatting and timing. No polling runs between events.
