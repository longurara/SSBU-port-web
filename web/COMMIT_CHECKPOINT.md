# Source checkpoint — 22 October 2026

The existing upstream commit `938e66b` is preserved. Work accumulated in the workspace
was split by subsystem into 14 commits using actual commit dates. These are retrospective
source checkpoints, not a reconstructed daily history or proof that each intermediate
commit independently builds. The final documentation commit contains this file.

## Commits

- `44132dd` — chore: configure local tooling and browser artifact ignores (3 files)
- `af5c59c` — feat(extract): inspect packages and prepare local game dumps (4 files)
- `82cefe9` — feat(web): add guest memory and portable runtime accessors (10 files)
- `0f99913` — feat(web): add pinned Wasm builds and ELF runtime preparation (26 files)
- `5eb83ac` — feat(web): add portable fibers and cross-worker scheduling (18 files)
- `4417f72` — feat(web): emulate OS, filesystem and synchronization services (16 files)
- `adb30b7` — feat(web): add host runtime and bounded memory diagnostics (12 files)
- `bb51d54` — feat(web): implement native NVN recording and resource bridges (24 files)
- `88f80f5` — feat(web): compile Maxwell shaders through SPIR-V and WGSL (20 files)
- `983c762` — feat(webgpu): add resource, sampling and presentation backends (22 files)
- `cc522de` — feat(webgpu): submit real draws and prove optional shader branches (25 files)
- `9ba5487` — feat(web): add local browser boot, input and diagnostic reporting (9 files)
- `6dc1947` — test(native): dismiss first-run notices in battle navigation (1 files)
- Final documentation commit — README, current/historical handoff and archived plans.

## Checks performed for this checkpoint

- Python syntax: 39 files.
- JavaScript syntax: 70 modules.
- Synthetic extraction package tests: 3 passed.
- Uniform branch proof: 26 checks passed.
- Browser input mapping: 12 contracts passed.
- Staged whitespace checks passed before each commit.

These checks do not establish browser gameplay. Full historical GPU/runtime evidence
is described in STATUS.md and HANDOFF.md; those tests were not all rerun during Git packaging.

## What Git contains

Handwritten source, tests, configuration, shader compiler sources, manifests with
observed addresses, and documentation. The two original parent-folder planning documents
are archived under `docs/web-port/`; their originals remain untouched.

Ignored `build/`, `deps/`, dumps, keys, generated lifted game code, binary outputs, local
browser reports and screenshots are not part of these commits. Existing handoff references
to those local artifacts require the original workspace or a separate local backup.

## Repository and execution state

Target repository: https://github.com/longurara/SSBU-port-web.git, branch `main`.
The original Denkishi remote is retained as `upstream`. Upload is authorized by the user;
the completed push must be verified against the remote branch SHA.

Game-development goal remains paused at the user request. Git packaging/upload does not
resume game execution. The coordinator was stopped; an already-running compiler was left
to finish. HANDOFF.md records the process snapshot and manual continuation instructions.
