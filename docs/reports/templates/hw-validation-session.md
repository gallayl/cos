# HW validation session & evidence (template)

Reusable, docs-only template for batched on-device/hardware validation sessions with per-item traceability. Copy into a session/report artifact (for example under `docs/reports/artifacts/hw-<date>-<topic>/summary.md` or a dated report in `docs/reports/`) and fill with executed evidence. Do **not** claim checks or hardware results beyond captured/recorded evidence.

- **Related:** [`hw-validation-preflight.md`](./hw-validation-preflight.md), [`../2026-11-18-pending-hardware-validation-inventory.md`](../2026-11-18-pending-hardware-validation-inventory.md), [`../../README.md`](../../README.md), root [`AGENTS.md`](../../../AGENTS.md), [`.agents/skills/on-device-validation/SKILL.md`](../../../.agents/skills/on-device-validation/SKILL.md), [`.agents/skills/global-gates/SKILL.md`](../../../.agents/skills/global-gates/SKILL.md)
- **Process framing:** recommended practice guidance (docs/templates), **not** a CI-enforced gate in this slice. Does not weaken `AGENTS.md`/`docs/README.md` gates; merge/review/approval and hardware validation remain human actions unless explicitly waived with rationale recorded.

## Usage, closure policy, and waiver authority

- **When to use (recommended):** for on-device/HW validation passes, batched when pending-HW backlog items are actionable or before release/milestone closure.
- **Per-item evidence required for HW/manual closure:** every claimed closure must map to a specific item/plan ID with evidence paths/logs and an outcome, or an explicit waiver.
- **Batched sessions do not replace per-item traceability:** one session/log may cover multiple items, but each item must still have its own outcome mapping and evidence/waiver.
- **Explicit human waiver path:** allowed when evidence cannot be obtained in-scope; record **rationale, approver, context, and date** in the waiver fields and in the relevant item/report/PR.
- **Waiver/exception authority:** maintainer/reviewer, with rationale recorded.
- **No implied closure:** this record does not replace required automated checks, review/approval, or merge.

## 1) Session metadata

| Field | Value |
|---|---|
| Session/report ID | `<hw-YYYY-MM-DD-topic>` |
| Date/time (local) | `<YYYY-MM-DD HH:MM TZ>` |
| Operator | `<name/handle>` |
| Branch/commit context | `<branch, commit SHA(s), PR if applicable>` |
| Preflight record | `<path/link to completed preflight checklist>` |
| Command plan(s) | `<paths/links>` |
| Log/artifact path(s) | `<raw transcript(s), summaries, screenshots, notes>` |
| Port / baud | `<COMx / /dev/tty...>, <baud>` |
| Reset/reboot used? | `<yes/no; when/why>` |
| `/flash` writes performed? | `<no expected; if yes, explain explicit approval/scope>` |

## 2) Firmware and environment context

| Field | Value |
|---|---|
| Flashed firmware identity | `<commit/build/tag as known>` |
| ELF / image identity (SHA256) | `<as applicable/reported>` |
| Boot profile | `<default/profile; next boot if relevant>` |
| Storage / SD state | `<mounted paths, card present?, relevant scripts/data>` |
| Network/peripherals/environment | `<Wi-Fi, BT, lighting, touch, power, etc.>` |
| Known blockers / affected-build notes | `<crash caveats, drift, missing prerequisites, N/A conditions>` |

## 3) Per-item outcome mapping (required for traceability)

Record one row per validated item/scope unit (or per distinct check group within an item when needed). Outcomes: `pass`, `fail`, `inconclusive`, or `waived`.

| Item / plan ID | Checks / commands executed | Evidence paths / logs (and excerpts/markers) | Outcome | Notes |
|---|---|---|---|---|
| `<docs/plan/... or ID>` | `<commands, manual steps, probe refs>` | `<artifact/log paths, timestamps/lines>` | `<pass/fail/inconclusive/waived>` | `<what was/was not proven>` |

**Outcome guidance:** `pass` = executed and evidenced with expected result; `fail` = executed and evidenced but did not meet expectation; `inconclusive` = executed but evidence insufficient/ambiguous/interrupted; `waived` = not executed/closed by explicit human waiver (complete waiver fields below).

## 4) Anomalies and blockers

| Time/context | Observation | Impact | Follow-up |
|---|---|---|---|
| `<timestamp/step>` | `<reset, timeout, NO_MEM, drift, missing output, etc.>` | `<affected items/checks>` | `<retest, fix, defer, waiver>` |

## 5) Waivers (required when any outcome is `waived`)

| Item / plan ID | Rationale | Approver | Context | Date | Evidence/reference |
|---|---|---|---|---|---|
| `<item>` | `<why evidence not obtained / why acceptable to waive now>` | `<maintainer/reviewer name/handle>` | `<item/report/PR context, scope limits>` | `<YYYY-MM-DD>` | `<link to discussion/report/PR comment if applicable>` |

Do not record a waiver without rationale, approver, context, and date. A waiver does not claim the check passed.

## 6) Manual/hardware-only steps and remaining actions

- **Completed manually (evidenced):** `<touch/visual/physical steps performed, who observed, evidence>`
- **Not yet completed (human/device required):** `<explicit remaining steps>`
- **Pending human actions:** `<review/approval, merge, additional hardware validation, reflashing, environment access>`
- **Doc/queue follow-ups:** `<items/reports/tracker to update with evidence, waiver references, or blockers>`

## 7) Summary

- **Overall session status:** `<completed / partial / blocked>`
- **Items with evidence-backed outcomes:** `<list>`
- **Items waived (explicit):** `<list or none>`
- **Items still pending:** `<list or none>`
- **Key evidence:** `<primary logs/artifacts>`

## Baseline/adoption note

This template supports per-item traceability and explicit waiver recording for HW/manual closure claims (batched collection allowed, no replacement for per-item traceability). Baseline evidence: `docs/reports/2026-11-18-pending-hardware-validation-inventory.md` and related HW reports/artifacts. Quantitative time/count/age improvement targets are deferred to follow-up measurement after adoption.
