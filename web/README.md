# Web port: runtime bring-up

For the current Vietnamese AI handoff, paused work state, active build ownership
and remaining blockers, read [HANDOFF.md](HANDOFF.md). Earlier bring-up notes are
preserved in [README_AI_HANDOFF.md](../README_AI_HANDOFF.md).

The optional local game build executes 96 real lifted functions and the game's
`nninitStartup` bootstrap in Wasm. It also maps the main ELF, applies relocations
to symbolic import slots, and reads the local archive through C stdio. It does
not by itself run constructors/nnMain. The separate runtime build now completes
306 constructors, enters nnMain, initializes NVN resources and uploads real
game buffers through WebGPU in Chrome.
There is still no rendered menu/match or browser gameplay/FPS evidence. The
default kernel build remains dump-free. See [STATUS.md](STATUS.md) for current
evidence and the exact stop point.

## Run on Windows

From the repository root, install the project-local, pinned SDK once:

```powershell
git clone --depth 1 https://github.com/emscripten-core/emsdk.git deps/emsdk
python deps/emsdk/emsdk.py install 6.0.12
python deps/emsdk/emsdk.py activate 6.0.12
```

Skip clone/install if that SDK already exists. No permanent PATH changes are
needed. `SSBU_EMSDK` can point to a different activated SDK directory.

Build and test:

```powershell
python web/build.py --test
python web/test_server.py
python web/serve.py --port 8787
```

Open **http://127.0.0.1:8787** in a desktop browser and click **Chạy kiểm tra**.
The server serves only `build/web-poc/`, binds to loopback, and provides the
COOP/COEP headers required for pthreads. It does not serve the repository or a
dump directory. Stop with Ctrl+C.

Outputs (ignored by Git):

- `build/web-poc/core32.{mjs,wasm}`: 32-bit pointers, pthreads, Wasm longjmp.
- `build/web-poc/core64.{mjs,wasm}`: the same fixture, 64-bit pointers.
- `build/web-poc/fiber32.{mjs,wasm}`: same-thread stackful fibers using Asyncify.
- `build/web-poc/manifest.json`: compiler, upstream revision and build commands.
- `build/web-poc/node-report.json`: SDK Node results, including failed checks.
- `build/web-poc/browser-report.json`: last browser run (overwritten on rerun).

## Local game integration (validated 13.0.4 dump required)

After the native build has produced `build/native/work/gen_eh_o2`:

```powershell
deps/dump-tools/Scripts/python.exe web/prepare_game.py
deps/dump-tools/Scripts/python.exe web/build.py --with-game --test
deps/dump-tools/Scripts/python.exe web/serve.py --port 8787
```

On the page, run the checks, then select these local files:

- `build/native/binaries/13.0.4/main.elf` for image mapping/relocations/bootstrap.
- `build/dump-13.0.4/romfs/data.arc` for sampled 64-bit archive range reads.

File objects are mounted read-only using WORKERFS in a dedicated worker. The
browser does not upload them, copy the archive into MEMFS/OPFS, or read the full
17.6 GB archive. Six 64 KiB samples include positions crossing 2/4 GiB and near
EOF; these are **not** full-file integrity checks. The archive worker uses a
16 MiB initial Wasm heap. The ELF worker grows its heap as it maps the program.

`prepare_game.py` selects bounded leaf functions from the existing native lift,
extracts each corresponding original instruction sequence from the ELF, and
executes four deterministic states/function in Unicorn AArch64. Wasm compares
all 32 integer/SP registers, all 32 vector registers, NZCV and the whole 64 KiB
test data window. This checks 384 states per pointer width; it does not cover
arbitrary entry states, all game functions, exceptional FP values or scheduling.
Generated game C, vector oracles and compiled game Wasm remain ignored in build/.

The ELF path checks headers/ranges, maps the three LOAD segments at 64-bit guest
addresses, verifies all BSS bytes are zero, and enforces text/rodata read-only
permissions. Relative/ABS64/GLOB_DAT/JUMP_SLOT relocations are applied, using
unmapped symbolic addresses for unresolved imports (not fake SDK functions).
Counts and selected resulting slots are compared to an independently prepared
Python reference. The real `nninitStartup` slice then runs with a 1 MiB guest
stack and three handlers: no CLI argc/argv and FS allocator registration. Other
host calls, SVCs, indirect functions and nnMain explicitly stop.

Additional outputs: `game32/64.{mjs,wasm}`, `assets.{mjs,wasm}`,
`elf-node-report.json`, `elf-browser-report.json`, `asset-browser-report.json`.
The Node ELF test uses MEMFS, covering eight malformed-input/recovery scenarios;
browser reports separately verify WORKERFS with the actual files.

The UI also lets you download its JSON report. Node tests exercise CPU/memory/
thread/fiber functionality; WebGPU is tested separately in the browser.

## What is implemented

`guest_memory.c` keeps 64-bit guest addresses separate from Wasm pointers. It
supports virtual reservations, lazily committed zero-filled pages, aliases,
per-mapping write permissions, cross-page copies, conservative dirty flags,
and aligned 8/16/32/64-bit CAS. A 16 GiB synthetic reservation is backed by only
the pages committed by the fixture; the module uses a small fixed linear memory.

`runtime/runtime.h` has an opt-in `RECOMP_WEB_MEMORY` branch. It redirects the
actual `rd*`, `wr*`, `rdv`, `wrv`, and `stx*` helpers into this adapter. The default
identity-mapping branch remains in place. `runtime_memory.c` binds the address
space per host thread. The probe uses the real `Ctx`, arithmetic and SIMD helpers
with self-authored functions; it does not invoke the dump-dependent lifter.

The fixture covers:

- reserve vs backing, unmapped access, overflow, capacity, protection, zeroing;
- scalar/vector unaligned access across page boundaries, alias lifetime;
- actual runtime helper loads/stores and exclusive-store success/conflict;
- arithmetic edge cases and a synthetic indirect call with a high guest stack;
- two live pthreads contending on one 64-bit counter, TLS/address-space isolation;
- explicit continuation state resumed on a second live worker, then local longjmp;
- nested same-thread stackful fiber suspend/resume with stack locals and longjmp;
- WebGPU compute with 64 verified readback values, validation and canvas clear.

The continuation fixture proves only a technique for explicit task state. It is
**not** a transformation of the game's stackful fibers. Emscripten Asyncify fibers
cannot resume on another worker; real task-system semantics remain a major gate.

## Constructor/nnMain and shader builds

After preparing the local dump and native lift:

```powershell
deps/dump-tools/Scripts/python.exe web/prepare_sdk.py
deps/dump-tools/Scripts/python.exe web/prepare_runtime.py --main
deps/dump-tools/Scripts/python.exe web/build_runtime.py --debug-graph
deps/dump-tools/Scripts/python.exe web/build_shader.py
deps/dump-tools/Scripts/python.exe web/build_maxwell.py
deps/dump-tools/Scripts/python.exe web/build_gpu_probe.py
```

The shader build requires Rust with `wasm32-unknown-unknown`, and the local
glslang executable. It downloads a checksum-pinned Naga source archive if absent,
patches a private copy under deps/, and places the compiler/test fixture in the
web build. No game data is required for its compute/readback test.

`build_maxwell.py` additionally builds the real Maxwell translator and glslang
16.6.0 for Wasm. It locates CMake/Ninja on PATH or in Visual Studio and checks
the glslang source commit. `game-shaders.mjs` connects it to the WGSL compiler;
the renderer must supply actual draw topology, parameter values and resources.

For local MXD1 captures produced by a native diagnostic run with
`SSBU_SHADERDUMP_ALL=<absolute build/maxwell-capture path>`:

```powershell
deps/dump-tools/Scripts/python.exe web/prepare_shader_fixtures.py --count 16
deps/emsdk/node/24.19.0_64bit/node.exe web/probe_maxwell.mjs 64 --spread
deps/emsdk/node/24.19.0_64bit/node.exe web/test_spirv_compat.mjs
```

The optional captured fixture then appears under **Chạy kiểm tra** as actual
shader compilation checks. Derived captures and compiler outputs stay under
ignored build/. Passing a shader smoke set does not mean all game shaders,
pipeline creation or drawing works; the broader results are in STATUS.md.

`build_gpu_probe.py` adds a dump-free C resource integration fixture to the
same button. It goes through the actual NVN adapters and pthread/owner-Worker
bridge to WebGPU allocation, flush/invalidate, readback and queue completion.
It does not render the game. `test_gpu_capabilities.mjs` separately checks the
98-field capability mapping and unsupported feature flags.

For optional upload-source diagnostics, call `testGpuResources` with
`{withSdk:true, graphicsFixture:true, inspectGuestPools:true}`. The read-only
`gpu-guest-pool-diagnostics.mjs` inspector validates the pinned Wasm32 resource
registry against actual GPU buffer handles and sizes while an upload waits for
its reply. Captured guest addresses use the pool's CPU backing plus the buffer
offset; a GPU-only pool has no guest CPU address. Validation failures are recorded
without preventing the upload. This option is disabled in the normal fixture.
Run `node web/test_gpu_guest_pools.mjs` for the metadata and range checks.

On the page, select main.elf and the extracted romfs directory, then use
**Thử boot game** for the current diagnostic boot. It stops on unported host/GPU
operations. **Khởi tạo runtime** runs constructors only. **Chạy kiểm tra** also
checks the Wasm shader compiler when built. These controls do not start a
playable game yet.

The optional diagnostic URL `/?inspectPrograms=1` compiles the programs
registered during boot and checks compute/graphics pipeline creation. Its
graphics pairing, vertex layout and triangle/float options are probe choices;
it does not dispatch, draw, or validate game pixels. Results are saved in
`runtime-browser-report.json`. The ordinary boot does not perform this probe.

The dump-free cross-thread fiber checks can be built separately:

```powershell
python web/build_fiber_migration.py
python web/build_host_fiber_probe.py
```

These use the project-local static Wasm32 migration adapter and the pinned
Emscripten 6.0.12 Asyncify implementation. The stock API does not support this
migration. The second fixture also verifies guest FiberType imports, registers,
thread TLS, diagnostic frames, finalization and Worker reuse. These are synthetic
callbacks; see STATUS.md for the latest full-game boundary. An experimental
full graph is built independently with
`python web/build_runtime.py --debug-graph --fibers`; select that variant with
`/?runtime=fibers`.

The compact backend mode reuses the same guest instruction bodies and reduces
Wasm temporaries. Build it with
`python web/build_runtime.py --debug-graph --compact-codegen --jobs 2`, or add
`--fibers` for the experimental Asyncify variant. The corresponding diagnostic
URLs are `/?runtime=compact` and `/?runtime=fibers-compact`. The ordinary page
now uses `fibers-compact`, the variant used for current game bring-up. Build it
with `python web/build_runtime.py --debug-graph --fibers --compact-codegen --jobs 2`.
Use `/?runtime=debug` to select the older non-fiber debug module explicitly.

For subsequent observed-callback rebuilds,
`python web/advance_runtime.py --steps 2 --fibers --compact-codegen --pointer-tables`
also compiles methods in relocated tables referenced by those callbacks or
containing their observed targets. Complete groups are limited to 256 entries;
newly added methods do not recursively expand this reverse lookup.
It requires the local server and the `ssbu-web` Playwright session. Discovery
adds actual native definitions; it does not supply replacement callback stubs.
The broader `prepare_runtime.py --all-pointer-tables` mode is an investigation
option and can expand generated source substantially; it is not the default.

Verification commands:

```powershell
python web/check_compact_codegen.py
python web/check_compact_codegen.py --asyncify
python web/build_diagnostic_probe.py
python web/build_mutex_probe.py
python web/build_semaphore_probe.py
node web/test_runtime_node.mjs compact
node web/test_gpu_samplers.mjs
node web/test_gpu_program_snapshots.mjs
node web/test_gpu_render.mjs
```

The leaf commands compare prepared game functions against their AArch64 oracle,
with and without optimized Asyncify. The diagnostic probe checks fault messages,
trap/frame behavior and memory-helper analysis. The mutex/semaphore probes
exercise actual pthread synchronization; the runtime test runs real constructors.
The GPU Node commands check sampler/program ownership, bounded caches, queue
behavior and cancellation during clear-pipeline creation with a mock GPU.
After `python web/build_gpu_probe.py`, **Chạy kiểm tra** also checks actual
WebGPU sampler texture/depth results and render-target clear readbacks, including
channel masks, scissor, integer/float precision, sRGB and mip/layer isolation.
Build `python web/build_gpu_probe.py --with-sdk` to also exercise guest NVN
render-target/scissor/color-clear recording through operation 22, including
copy/clear submission order and state retained across submissions. Matching or
compatible linear/sRGB view formats, upper-left origin and explicit scissors
are currently required.
These checks do not verify a playable match.

While a runtime attempt is active, the page locks both launch buttons and the
program/directory selectors. **Dừng runtime** terminates that attempt and
releases the controls so it can be run again. The current page is still a
development harness; stopping an attempt does not produce a successful report.

## Constraints before integration

- Mapping/protect/decommit and adapter reads/writes serialize under a shared
  lock. Destroy and borrowed raw spans require external lifetime coordination.
  A read followed by a write is not an atomic read-modify-write; use gm_cas.
- The default page limit is 256; a quiescent initialisation option allows up to
  1,048,576 entries in a growing table. The image loader reserves 512 MiB of guest
  address space and maps only the required pages. There are still 16 reservation
  slots, and this is not a production allocator for the full game. A span is borrowed and
  cannot outlive a mapping. Host copy buffers must not alias backing storage.
- Dirty flags are conservative and not a replacement for the native R1 system.
  R1 snapshots and write-watch integration are not ported.
- Guest exclusive-store helpers preserve the current runtime's value-CAS model;
  full AArch64 exclusive-monitor semantics and ordered-load/store instruction
  emission still need auditing with the lifter.
- Host runtime direct pointer casts, Win32 APIs, module loader and media have not
  been ported. Enabling the helper macro does not make those files web-compatible.
- The default memory helper branch receives a compile-only syntax check in the
  web build. A separate native build completed and booted with the user's dump;
  see `STATUS.md` for the latest smoke-run evidence.
- WebGPU passes compute/readback checks and compilation through the Wasm
  Maxwell -> GLSL -> SPIR-V -> WGSL path for a bounded real shader set. The NVN
  frontend handles device capabilities, buffer upload/readback, descriptor pool
  metadata, ordered pool-binding recordings, queue lifecycle and sync/fences.
  Texture builders use the actual lifted SDK for source storage size/alignment;
  the optional runtime embeds the locally prepared 13.0.4 SDK layout image.
  Initial color/BC texture uploads and readback now use actual WebGPU textures
  and the native portable Tegra deswizzler. Buffer-to-texture command recording,
  submission and physical texture-view registration now work. Draw-time texture
  binding/sampling, depth/stencil uploads, draw submission,
  viewport/LOD shader coverage and rendering remain incomplete.
- Timing output is a synthetic memory-loop measurement, not an FPS estimate.

The user's 13.0.4 dump has been validated and the native baseline built. Next:
verify a native match, measure memory and task behavior, then integrate these
pieces against actual generated code and deterministic match logs. See
`SSBU_WEB_PLAN.md` in the parent workspace for the complete plan.
