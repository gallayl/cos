# Project instructions for OpenCode

## Definition of done (verify before declaring a change complete)

Be proactive: after making changes, run the relevant checks yourself without waiting to be asked, fix failures, and re-run until they pass. Do not declare work complete based on code edits alone. The only exception is when validation requires physical hardware/on-device interaction—if so, run everything you can locally/automatically, then clearly state what remains and ask for hardware validation.

Report exactly which commands were run and their outcomes. Do not rely on CI to catch issues. If a check cannot be run or fails due to missing tools/environment, explain the concrete blocker and help the user resolve it (what to install, where/how to configure it, and how to rerun the check).

### Default checks (most changes)

- `make format-check` — formatting must match CI (`.github/workflows/format-check.yml`).
- `make test` — host-based unit tests must pass (`.github/workflows/test.yml`).

If formatting fails, run `make format`, then re-run `make format-check`.

### Additional checks by scope

| Scope | Checks | Notes |
| --- | --- | --- |
| Frontend / `storage/ts` / `storage/flash_data` changes | From `frontend/`: `yarn format:check`, `yarn lint`, `yarn test`, `yarn build` | Matches `.github/workflows/frontend.yml` (Node 24.x, Yarn). Run all four proactively after frontend-related changes. `yarn build` runs TypeScript checking (`tsc`) and Vite build. If deps are missing or stale, run `yarn install --frozen-lockfile` in `frontend/` (requires Node.js 24.x and Yarn installed, with network/access to registry as needed). |
| Firmware / build / lint-covered C-C++ changes | `make build`, and `make lint` when feasible | CI (`.github/workflows/build-and-lint.yml`) builds with ESP-IDF v6.1 and runs clang-tidy against a clang compile DB (`build/clang/compile_commands.json`). These require an ESP-IDF v6.1 environment sourced (`IDF_PATH`, tools; see `README.md`). Do not claim build/lint passed if the environment is unavailable. |
| Documentation / Doxygen comments | `make docs-check` | Matches `docs-check` CI workflow. |
| Integration tests / QEMU flows | `make integration-test-build` and/or `make integration-test` (or `make integration-test-run` after build) | See `README.md` and `.github/workflows/integration-test.yml`; requires ESP-IDF v6.1, QEMU tool, and pinned deps in `integration_test/requirements.txt`. Run when changes affect integration-tested behavior. |

### Reporting

- Summarize results per command (pass/fail) and mention any fixes applied.
- Run checks proactively by default; do not wait for the user to ask to rebuild, run tests, lint, or format-check unless the step requires hardware validation.
- Do not rely on CI. If you could not run or complete a required check (missing ESP-IDF/QEMU/tooling, Node.js/Yarn, wrong environment, etc.), explain the specific cause and give actionable steps to fix it: what to install, required versions, where to install/set it (e.g. `IDF_PATH`, `IDF_TOOLS_PATH`, `PATH`, sourcing `export.sh`/IDF environment; for frontend: install Node.js 24.x and Yarn, run `yarn install --frozen-lockfile` in `frontend/`), and the exact command to rerun.
- Prefer the Makefile targets above for firmware/host checks and the frontend `yarn` scripts in `frontend/` for frontend checks so local verification stays aligned with CI.
