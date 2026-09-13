## 1.3.0

- Add Mod Setting Menu controls for parry timing and an independent Dodge while blocking toggle.
- Add a separate Perfect dodge window setting from 10% to 5,000%, with normal timing as the default.
- Apply timing and dodge settings during play without loading a save.
- Restore parry settings after missed loading notifications or delayed player initialization.
- Replace console settings commands with one settings menu and a single Logging switch.

## Unreleased

- Add an independent Perfect dodge window percentage setting using the game's native timing check, with normal timing as the default.

- Add native dodge eligibility/commit results, animation selection, combat-state snapshots and explicit ability end/cancel diagnostics when Blueprint tracing is unavailable.

- Stop repeated Blueprint diagnostic hook failures when the Lua dispatcher is unavailable, while preserving native combat tracing across save loads.

- Add an independent Dodge while blocking toggle to Mod Settings, enabled by default.

- Restore parry settings after loads with missed loading notifications or delayed player initialization.

- Simplify diagnostics to a single Logging switch at the end of Mod Settings.

# Changes

## 1.2.3 — 2026-09-10

- Removes recurring parry checks.

## 1.2.2 — 2026-09-10

- Fix combat controls getting stuck after a forward dodge.
- Restore held guard after dodging.
- Fix cleanup after failed or cancelled dodges.

## 1.2.1 — 2026-09-07

- Restore vanilla attack/combo guard handling by removing the custom guard-input override.

## 1.2.0 — 2026-09-07

- Restore held guard after dodging. Normal attacks cancel guard until guard is released and pressed again.
- Replace optional Lua dodge interruption with native guard handling. Retire dodgeInterruptsGuard and the dodge console toggle; enabled and easierparry on/off now control only parry timing.
- Follow the active local player and attribute owner across save loads and possession changes. Preserve the captured timing baseline during temporary unpossession and partial-write retries.
- Add bounded guard and timing diagnostics under debugLogging, off by default. Diagnostic workers stop when finished.

## 1.1.1 â€” 2026-09-06

- Fix a potential crash when loading a save or respawning.

## 1.1.0 â€” 2026-09-06

- Add optional dodge interruption of guard, enabled by default. Release and press guard again afterward; normal dodge restrictions still apply.
- Separate shipped defaults from a personal INI in %LOCALAPPDATA%/Dawnwalker/Saved/Config, so updates preserve user overrides. Copy custom legacy settings there before upgrading if no personal file exists.
- Load settings once at startup and save console changes to the personal INI while preserving comments and unrelated settings. Direct INI edits require a restart.
- Support UTF-8 INIs with or without a BOM and log configuration errors without overwriting unreadable user files.
- Keep Nexus listing materials outside the Vortex ZIP; retain the README, license, changelog, release notes and Vortex metadata. Shorten the mod description and add the GitHub source link.
- Lua, Windows file-I/O and Vortex package checks pass. Live gameplay validation, including the new dodge option, remains pending.

## 1.0.8 â€” 2026-09-06

- Read INI settings only at startup; retain the selected factor throughout the session.
- Console factor and on/off commands update the session selection and save factor/enabled to the same INI, preserving comments and unrelated settings.
- Keep console selections active even if saving fails, with a warning in the log.
- The former reload command now displays guidance without rereading settings.
- Player lookup and parry attribute application are unchanged.

## 1.0.7 â€” 2026-09-06

- Narrow the update to verified INI-loading defects. Remove v1.0.6 controller tracking and baseline-restoration changes; runtime player handling returns to v1.0.5 behavior.
- Preserve working settings and the applied value when an explicit reload cannot read a valid factor.
- Accept a UTF-8 BOM before [General].
- Select the INI beside main.lua without falling through to unrelated INI copies, and log its path and effective factor.
- The reported death/reload reset to 2.0 remains unconfirmed; ordinary player recreation retains the configured factor in regression tests.

## 1.0.6 â€” 2026-09-06

- Follow the local controllerâ€™s current pawn and parry attribute on each maintenance check, rather than waiting for old objects to become invalid.
- Preserve the configured factor across player, attribute, and controller replacements; avoid compounding when a new pawn shares the previous attribute.
- Avoid restoring a stale baseline over values already reset by the game during loading.
- Log the configuration file path at startup to help identify which INI was loaded.
- Retain the 2.0 default and all existing settings; gameplay validation of death/reload remains pending.

## 1.0.5 â€” 2026-09-06

- Keep Nexus thumbnail, description, attribution, and listing metadata in the repositoryâ€™s Nexus folder, outside the Vortex ZIP.
- Preserve Vortex name/version/description metadata and runtime installation paths.
- Lua and INI payloads are unchanged from v1.0.4.

## 1.0.4 â€” 2026-09-06

- Replace the thumbnail with an official in-game sword-clash screenshot and a simple title overlay.
- Include screenshot source attribution. Lua and INI payloads are unchanged.

## 1.0.3 â€” 2026-09-05

- Include the full Nexus description, existing thumbnail, changelog, and release notes in the Vortex ZIP.
- Preserve Vortex name/version/description metadata and the existing runtime paths.
- Lua and INI payloads are unchanged from v1.0.2. The performance fix remains pending in-game validation.

## 1.0.2 â€” 2026-09-05

- Remove repeated global player searches and object-name resolution from healthy maintenance ticks by caching the player and CharDevAttributeSet.
- Reacquire references only when absent or invalid; preserve value repair without redundant writes.
- Increase the default maintenance interval from 500 ms to 1000 ms.
- Prevent repeated `easierparry on` commands from compounding the multiplier.
- Standardize the fixed output name to `Easier-Parry-UE4SS.zip`, removing the historical v1.0.0 filename label. Display name and internal mod ID are unchanged; replace the same Vortex entry.
- Lua regression and Vortex installer checks passed; in-game stutter validation is pending.

## 1.0.1 â€” 2026-09-05

- Generate Vortex display name, version, and description in the release archive.
- Preserve the existing ZIP filename and runtime payloads. Reinstall/replace through Vortex to read metadata.
