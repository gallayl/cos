# Berry command scripts

## Status

**Implemented locally / hardware validation in progress** (as of 2026-11-08). The
resolver, `path` command, `/flash/.cos_path` persistence, command-script
execution path, and `ping.be` pilot are implemented in the workspace; full
target/hardware acceptance and the native `ping` migration decision remain open.

### Current progress

- [x] Resolver, `path` shell command, `/flash/.cos_path` persistence, and Berry
      `main(args)` command-script execution path implemented (`components/shell/src/shell_script.*`,
      `shell_input.c` hook, `shell_script_init()` from shell init). Interactive
      dispatch: alias expansion → native `esp_console_run` → script resolver on
      `ESP_ERR_NOT_FOUND`; explicit tokens containing `/` resolve only that path
      (no fallback) and report `<token>: script not found` when missing.
- [x] Host unit tests added/passing (pre-hardware, `test/build-r9`):
      `test_shell_script` 14/0, `test_shell` 63/0, `test_shell_berry` 26/0.
- [~] Hardware validation partial (2026-11-08, user report): native `ping` takes
      precedence (R9-HW5). `path add /sdcard/asdasd` correctly rejected (R9-HW3
      partial). `path clear`; `be_ping` resolved from `/sdcard/scripts` CWD; from
      `/flash`, relative explicit paths `../sdcard/scripts/be_ping` and
      `../sdcard/scripts/be_ping.be` ran (R9-HW4/6 partial). Breakout starts/closes
      with `berry run` full/relative path (R9-HW9 partial; script-while-UI-app-active
      still needed). R9-HW8 blocked: `be_ping.be 127.0.0.1 1000` hit `map has no
      attribute 'ok'` due to `native_net_ping` popping the result map before
      `be_return(vm)` in `components/berry_lang/src/berry_network.cpp` — **fixed in
      workspace, reflash+retest required**. Full checklist/evidence:
      `docs/reports/2026-11-08-r9-hardware-validation-guide.md`.
- [ ] Native commands such as `ping`/`nslookup` remain the active implementation
      until the pilot passes target+host validation and migration is a deliberate
      follow-up (native `ping` still registered/wins bare `ping`).

### Follow-ups

- Reflash firmware with `native_net_ping` fix, then retest R9-HW8 (`ping.be`/pilot
  via explicit path or `berry exec`, network) and complete remaining R9-HW items.
- Complete hardware validation per `docs/reports/2026-11-08-r9-hardware-validation-guide.md`
  and record evidence; then decide/record native `ping` migration (remove/make
  native registration unreachable only when script lookup is active and pilot
  validated) and update `README.md` if migrated.
- Audit other `berry_network` map-return paths for the same `be_pop`/`be_return`
  stack issue (only `native_net_ping` fixed in this pass).
- Consider QEMU/integration coverage for lookup order, native precedence, explicit
  paths, quoting, status propagation, Berry errors, and VM/app lifecycle isolation
  (host unit tests already cover core resolver behavior).

### Follow-ups

- Complete hardware validation per `docs/reports/2026-11-08-r9-hardware-validation-guide.md`
  and record evidence; then decide/record native `ping` migration (remove/make
  native registration unreachable only when script lookup is active and pilot
  validated) and update `README.md` if migrated.
- Consider QEMU/integration coverage for lookup order, native precedence, explicit
  paths, quoting, status propagation, Berry errors, and VM/app lifecycle isolation
  (host unit tests already cover core resolver behavior).

## Summary

When a command is not a registered native shell command, COS may resolve it to
a Berry command script. Resolution checks the current working directory first,
then each configured search directory in order. Native commands always take
precedence. No built-in aliases are required.

The initial pilot is `ping`. It should preserve the current `ping <host>
[timeout_ms]` interface while moving command-specific argument handling and
output into Berry. Network operations remain provided by native Berry APIs.

## Goals

- Allow Berry scripts to be invoked like shell commands, with positional
  arguments.
- Keep registered native commands and their existing behavior working.
- Search the current virtual working directory and an ordered, configurable
  set of virtual directories.
- Run command scripts without invoking Berry UI-app lifecycle behavior.
- Return meaningful command success or failure to the shell.
- Make the resolver useful for later command migrations without requiring
  default aliases.

## Non-goals

- Replacing or changing the existing `berry run`, `berry eval`, or `berry close`
  interfaces.
- Replacing native service implementations with Berry code. Hardware,
  networking, filesystem, and other platform operations remain native APIs.
- Implementing a general-purpose DOS/Windows executable loader, environment
  variable expansion, batch files, or arbitrary script extensions.
- Making scripts discoverable through `help` or Tab completion in the initial
  version.
- Allowing scripts to shadow registered native commands.

## Command resolution

Resolution applies to interactive shell input after existing alias expansion
and native command dispatch. It does not implicitly apply to Berry's existing
`action(command)` API, which calls registered console commands directly.

1. If the command name matches a registered native command, execute that
   command and do not search for a script.
2. If the command token contains a virtual path separator, treat it as an
   explicit script path. Resolve relative paths from the current working
   directory and absolute paths using the existing COS virtual-path rules.
   Do not search other directories for an explicit path.
3. Otherwise, check the current working directory for a matching Berry command
   script.
4. If no match exists, check each configured search directory in order.
5. If no script is found, preserve the current `Unknown command` behavior.

Only `.be` command scripts are supported. A bare command name such as `ping`
maps to `ping.be`. A name already ending in `.be` is used as provided. For
example, `tools/ping.be` is an explicit relative path; `ping.be` is a bare
script name searched in the current directory and configured search
directories.

Search is case-sensitive, matching COS virtual filesystem behavior. The first
matching script wins. Native command precedence is intentional: a script
cannot override commands such as `rm`, `restart`, or `berry`.

### Search path configuration

Provide a shell `path` command to inspect and update the ordered list of
virtual directories. The proposed interface is:

```text
path                 Show the current search path
path add <directory> Append a directory if it is not already present
path remove <directory>
                     Remove a directory
path clear           Remove all configured directories
```

Paths are normalized to absolute virtual paths when added, relative to the
current working directory. Search order is insertion order. The current
working directory is always searched first and is not part of the configured
list. Duplicate normalized paths are ignored.

Persist the configured list on `/flash` so it survives reboot, using a
dedicated COS configuration file rather than overloading the existing alias
file. An absent configuration means an empty search list. Invalid entries or
an unavailable directory must be reported when configured; a directory that
becomes unavailable later must not prevent searching later entries. The
command must report persistence failures instead of claiming the update
succeeded.

## Script invocation and status

Each command script defines a `main(args)` function. COS invokes it once with
`args` as an ordered Berry list of strings, excluding the command name. The
function's integer return value is the shell command status: zero means
success; any nonzero value means failure. A missing `main` function, a
non-integer result, a Berry exception, a file read failure, or a Berry runtime
initialization failure is reported as an error and results in a nonzero shell
status.

The shell must preserve the existing command-line quoting and argument
tokenization behavior. It must not reconstruct arguments by splitting the
original command line on spaces. If ESP Console's unknown-command path does
not expose the already-tokenized arguments, implementation must use or add a
shared parser so native and script commands interpret quoting consistently.

Command scripts run in the shared Berry VM but use a command execution path
separate from app execution. Running one must not close an active Berry UI app,
hide the text console, create a UI task, or change app state. The VM must be
serialized using its existing execution guard. Invocation arguments must be
scoped to the call and must not leak into later script runs.

Normal script output uses the existing Berry `log()` behavior unless a later
console-output API is introduced. Command scripts should print concise,
user-facing usage and result messages and return a status suitable for shell
automation.

## Ping pilot

The pilot script is `ping.be`, resolved through the same command mechanism as
other scripts. It must preserve the current interface and observable behavior:

- Usage: `ping <host> [timeout_ms]`.
- Default timeout: 1000 ms.
- Explicit timeout must be a valid integer within the bounds defined by the
  network component.
- Successful ping reports the resolved address and round-trip time.
- Invalid arguments, invalid timeout values, and network failures produce a
  useful error and return nonzero.
- The script uses the existing native `net.ping(host, timeout_ms)` API. It
  does not parse console output from `action()` or reimplement networking.
- The native `ping` command remains registered until the Berry pilot passes
  tests on both target and host, and replacement is a deliberate follow-up.

If `ping` is migrated, its native command registration must be removed or
otherwise made unreachable only after script lookup is active. Leaving the
native command registered would correctly cause it to win resolution and
would mask the pilot.

## Errors and precedence

- Not-found during candidate lookup continues to the next candidate.
- Errors other than not-found (for example I/O or memory errors) must be
  reported; they must not be presented as `Unknown command`.
- An explicitly specified script path that does not exist reports a
  script/path error and does not fall back to the search path.
- A script found in an earlier search directory wins even if a later
  directory contains the same name.
- Native commands and aliases retain their current semantics and precedence.
- Script command execution is limited to the interactive shell in the initial
  version; callers using `action()` must use registered commands.

## Acceptance criteria

- A `.be` script in the current directory can be run by its bare name.
- A script in a configured search directory can be run by its bare name.
- Search follows current-directory-first and configured-directory order.
- A registered native command wins over a same-named script.
- An explicit relative or absolute script path runs only that script.
- Quoted arguments, spaces within arguments, empty arguments where supported,
  and argument ordering are preserved.
- The command script's integer status is returned to the shell; script errors
  and invalid results are surfaced as failures.
- Running a command script does not close or otherwise disturb an active Berry
  UI app.
- Missing scripts retain the existing unknown-command message.
- Search configuration can be listed, changed, cleared, and is retained over
  reboot; malformed configuration and persistence errors are reported.
- Tests cover lookup order, native precedence, path errors, argument delivery,
  status propagation, Berry errors, and VM/app lifecycle behavior.
- Ping tests cover the existing default timeout, valid and invalid timeout
  values, output, and network failure behavior.
- README command and Berry documentation describes script lookup and `path`.

## Follow-on command refactors

Only migrate a command when its underlying behavior is available through a
structured Berry native API. Do not make scripts call `action()` and parse
human-readable console output; that would couple scripts to formatting and
would not reliably propagate structured results.

| Priority | Command(s) | Rationale and prerequisites |
| --- | --- | --- |
| 1 | `nslookup` | Natural companion to Ping and likely to validate the same resolver workflow. First expose a native `net.resolve(host, timeout_ms)` API with structured address/error results; preserve its current timeout and output contract. |
| 2 | `uptime`, `info`, `mem` / `memory` | Read-only system commands are low-risk script candidates. Expose typed system/heap information in Berry first; avoid parsing log or console output. `mem` and `memory` currently expose the same operation and should migrate together. |
| 3 | `light` | A small read-only sensor command. Expose the sensor reading and error state as a native Berry API before moving presentation and argument handling into a script. |
| 4 | `time` | A reasonable read-only candidate once time and synchronization state are available through a structured Berry API. Keep clock synchronization itself native. |
| 5 | `ls` / `dir` | Useful Berry filesystem example after `fs.list` has a complete, stable contract for paths, entry types, and errors. Keep filesystem traversal and access controls native. |

Keep the following native for now:

- **Core shell and file mutation:** `cd`, `pwd`, `mkdir`, `rm`, `cp`, `mv`,
  `rmdir`, `touch`, and SD mount management. These underpin shell navigation,
  change state, or have destructive effects.
- **System control:** `restart`, `format`, and command/runtime management.
- **Interactive or asynchronous controls:** `calibrate`, `wifi`, `bt`, `i2c`,
  `server`, `ws`, `screen`, and `display_mode`. These involve hardware,
  interaction, concurrency, or lifecycle concerns and should remain native
  unless a specific Berry API and UX need justify migration.

This list is a migration order, not a commitment to remove the native command
handlers. A command may remain native even after Berry gains access to the
same underlying service.
