# HW validation preflight checklist (template)

Reusable, docs-only checklist for preparing an on-device/hardware validation pass. Copy this template into a session/report artifact (for example under `docs/reports/artifacts/hw-<date>-<topic>/`) and fill it in before running commands. Do **not** claim device state or checks you did not verify.

- **Related:** [`../2026-11-18-pending-hardware-validation-inventory.md`](../2026-11-18-pending-hardware-validation-inventory.md), [`hw-validation-session.md`](./hw-validation-session.md), [`../../README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), [`.agents/skills/on-device-validation/SKILL.md`](../../../.agents/skills/on-device-validation/SKILL.md), [`.agents/skills/global-gates/SKILL.md`](../../../.agents/skills/global-gates/SKILL.md)
- **Process framing:** recommended practice guidance (docs/templates), **not** a CI-enforced gate in this slice. Does not weaken `AGENTS.md`/`docs/README.md` gates; merge/review/approval and hardware validation remain human actions unless explicitly waived with rationale recorded (see session template).

## Usage and ownership (recommended)

- **When to use:** before on-device/HW validation passes, especially when batching pending-HW backlog items or before release/milestone closure.
- **Who records:** the person performing validation records preflight facts, per-item evidence, or explicit waivers in the relevant session/report/item/PR context.
- **Waiver/exception authority:** maintainer/reviewer, with rationale recorded in the relevant item/report/PR (and in session waiver fields where applicable).

## 1) Session intent and scope

| Field | Value |
|---|---|
| Date/time (local) | `<YYYY-MM-DD HH:MM TZ>` |
| Operator | `<name/handle>` |
| Session/report ID | `<e.g. hw-YYYY-MM-DD-topic>` |
| Target item(s) / plan IDs | `<links/IDs; per-item traceability tracked in session template>` |
| Purpose | `<read-only baseline / targeted validation / batched pass / release closure prep>` |
| Notes | `<what this pass is/is not intended to prove>` |

## 2) Device, firmware, and environment baseline

| Field | Value |
|---|---|
| Serial port | `<COMx / /dev/tty...>` |
| Baud rate | `<default 115200 or actual>` |
| Flashed firmware identity | `<commit/build/tag as known>` |
| ELF / image identity (SHA256) | `<as applicable/reported>` |
| Boot profile | `<default/profile name; next boot if relevant>` |
| Storage / SD state | `<mounted paths, card present?, free space if known>` |
| Reboot acceptable? | `<yes/no; conditions/limits>` |
| Required environment/peripherals/network | `<Wi-Fi credentials/network, BT peripherals, lighting/touch/display conditions, power, etc.>` |
| Device access confirmed | `<yes/no; who has physical access if different>` |

## 3) Tooling prerequisites

| Check | Status | Notes/evidence |
|---|---|---|
| Serial port available (no conflicting monitor) | `<ok/missing/n-a>` | `<details>` |
| `python scripts/hw_console_session.py` usable | `<ok/missing/n-a>` | `<python version/path>` |
| `pyserial` available (for automation) | `<ok/missing/n-a>` | `<e.g. python -m pip show pyserial / install note>` |
| `idf.py monitor` fallback available (if needed) | `<ok/missing/n-a>` | `<ESP-IDF env details>` |
| Artifact/log output path prepared | `<ok/missing/n-a>` | `<path>` |

If automation prerequisites are missing, record the concrete blocker and remediation (install/configure command, env, exact rerun command). Do not claim automated capture was executed.

## 4) Rules and safety constraints

- [ ] **No `/flash` writes** unless explicitly approved and required by a separate, in-scope plan; this template/default workflow assumes command plans avoid `/flash` writes and avoid `make flash-data`/erase/flash operations.
- [ ] No erase/flash/factory-reset/profile-changing/restart commands unless explicitly requested, safe, and recorded in the command plan.
- [ ] Prefer read-only/status probes for baseline snapshots; treat state-changing or rebooting steps as explicit and evidenced.
- [ ] Do not guess port, firmware identity, device state, or outcomes.

## 5) Known blockers and affected-build checks

| Check/item | Status | Evidence/rationale | Action |
|---|---|---|---|
| Known diagnostics/profile crash caveats checked (if applicable) | `<ok/blocked/n-a>` | `<link to inventory/report/issue>` | `<avoid affected commands/profile or proceed>` |
| Script/runtime/storage drift checked (if applicable) | `<ok/blocked/n-a>` | `<e.g. autoexec, SD script compatibility>` | `<use adapted paths/plan>` |
| Required peripherals/network available for planned checks | `<ok/blocked/n-a>` | `<details>` | `<defer/manual/waive later if needed>` |
| Other environment-specific blockers | `<ok/blocked/n-a>` | `<details>` | `<action>` |

Reference: [`../2026-11-18-pending-hardware-validation-inventory.md`](../2026-11-18-pending-hardware-validation-inventory.md) and relevant `docs/reports/*` / `docs/plan/*` items.

## 6) Quick read-only probe and command-plan references

- [ ] Quick read-only probe/identity snapshot planned (as applicable): `boot_profile`, `services`, `wifi status`, `mem`, `berry profile`, `svc_mem`, `uptime`, `info`, `path list` (adapt to avoid known-unsafe commands on affected builds).
- [ ] Command plan(s) identified and reviewed (one command per line, reproducible, bounded timeouts/idle windows).
- [ ] Per-item mapping prepared so each planned check can be traced to a plan/item ID in the session template.

**Command plan references:**

| Plan file | Purpose | Notes |
|---|---|---|
| `<docs/reports/artifacts/.../plans/...txt or link>` | `<baseline / targeted>` | `<adaptations, no /flash writes>` |

## 7) Preflight outcome

- **Ready to run?** `<yes/no/partial>`
- **Proceeding scope:** `<what will run now vs deferred>`
- **Deferred/manual prerequisites:** `<physical actions, credentials, reflashing, human access, etc.>`
- **Next artifact:** create/update [`hw-validation-session.md`](./hw-validation-session.md)-based session record with evidence paths/logs and per-item outcomes.

## Closure/context notes

- **Per-item traceability required:** batched sessions may collect evidence efficiently, but HW/manual closure claims must record per-item evidence or an explicit waiver (rationale, approver, context, date).
- **No implied closure:** completing this preflight does not validate behavior and does not replace required automated checks, review/approval, or merge.
- **Baseline context:** this template/workflow targets observed HW preflight/setup/traceability friction (see `docs/plan/needs-clarification/dx-delivery-acceleration-bottlenecks.md`, `docs/reports/2026-11-18-pending-hardware-validation-inventory.md`, and related reports). Quantitative time/count/age improvement targets are deferred to follow-up measurement after adoption.
