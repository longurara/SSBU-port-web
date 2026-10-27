> **27/10/2026 16:50Z — FIRST FULL-NATIVE GAME TEST TERMINATED; THREAD FIX LINK LIVE80924.**
> Actual published37917f build booted once. Observed opening930s,title1052s,
> notification1166s,reward1270s,Rapidash Spirit card1374s. Real Z/A inputs recorded
> and always released. After card transition terminated around1530s with
> thread started twice, ThreadType80613100e0, guestFunction710392acb0,
> lr7103993ee4,sp7106df3f40,threadFailed1.6340frames/136483draws,
> 560realcompute dispatches/1680writtenbuffers/2073packedvertexdraws.
> Evidence build/codex-all-functions-runtime-terminal.{json,log}; screenshots
> output/playwright/codex-all-functions-runtime-{opening,title,notification,reward,spirit}.png.
> No game currently active. Initial black screenshots were a background-tab capture
> issue: real GPU samples had nonblack pixels; bringToFront before screenshot fixed
> visibility. Preserve production rendering; no graphics patch was needed for that.
> Reproduced same thread-reuse trap in dump-free real-pthread OS fixture before fix.
> Root cause: DestroyThread was a no-op, CreateThread appended duplicate addresses,
> lookup returned old started record. Code075ad1d implements real join-once waits,
> registry retirement/reuse,TLS decommit after join,unique lifetime IDs,bounded TLS
> slots,serialized starts,and retained rejection of duplicate starts/live-holder destruction.
> OS fixture PASS2247checks/601actualguestthreadcallbacks:300same-address and300new
> address lifetimes,repeatedwaits,clean TLS,capacity255+main,never-started destruction,
> two concurrent native waiters joining one guest pthread. Existing events/input checks pass.
> Expanded fixture exceeded its old64KiB stack; STACK_OVERFLOW_CHECK=2 proved this.
> Fixture now uses2MiB stacks matching actual runtime; actual runtime stack unchanged.
> Full-native compile-only --fibers --compact-codegen --jobs8 PASS,only host_os rebuilt.
> All1204objects audited159619functions,maxlocals40; final linked guard still required.
> New full link parent80924 started16:50:28Z,source075ad1d,same full graph andjobs8,
> plus --trace-asyncify (BINARYEN_PASS_DEBUG=1). Pass names/timings are not percent.
> State build/codex-thread-lifecycle-build-state.json; stdout/stderr
> build/codex-thread-lifecycle-link{,.err}.log. Do NOT resume obsolete75144/81680.
> NEXT: verify process and link completion,then NEW verifier
> build/codex-verify-thread-lifecycle-publication.py rejects old37917f and validates
> full native coverage,local+HTTP receipt/hash,locals guard,11JS. Only then ONE fresh
> boot,install guest reader afresh (global address may move),and test past thread reuse
> into actual menu/CPU match/completion. Game completion remains unproven.

> **27/10/2026 15:49Z — FULL-NATIVE BUILD PUBLISHED; FIRST BOOT LIVE.**
> Link parent75144 and wasm-opt81680 finished normally; stdout Linked runtime-fibers-compact.mjs.
> Guarded receipt 15:46:12Z: SHA256 37917f83987428a400e4524ddce7251339ad8324ad52ddcb4b401765c3c3eb89,
> 580540926 bytes,161120 defined functions,maximumLocals10812. Full-native manifest
> and independent coverage,local+HTTP hash/receipt,and11JS verified15:46:39Z by
> build/codex-verify-all-functions-publication.py; report all-functions-publication-verified.json.
> No build is currently active; do not resume/restart obsolete PIDs or suspended metadata.
> User explicitly requested actual game test. ONE controlled boot started in existing
> Playwright ssbu-web session; runtime about15:48Z. At elapsed118s:306ctors,
> 11796961imports,heap820969472,threadFailed0,frames0,done:false. Preserve running game.
> Movie trace installed; unique validated GuestMemory address20817288 (different from
> old ddc3). New evidence build/codex-all-functions-runtime-*.log and
> output/playwright/codex-all-functions-runtime-live.png. Poll using page-only
> build/codex-compute-runtime-poll.cjs redirected to all-functions filenames.
> NEXT: observe actual opening/title,inspect before deliberate input,pass old3353750
> failure then actual menu/CPU match/completion. None of those is yet proven on new build.
> Build success and306ctors do NOT establish playable-game completion.

> **26/10/2026 16:17Z — ALL-NATIVE COMPILE+OBJECT AUDIT PASS; FULL LINK LIVE75144.**
> Preflight parent70560 finished normally; stdout Prepared objects, not linked.
> Audited ALL1204objects:159617definedfunctions, maximum pre-link numericlocals40,
> no limit violation. build/codex-all-functions-objects-audit.{json,log}. This is NOT
> final linked locals/browser proof; retain publisher's final50000guard.
> Full link started16:17:33Z parent75144, code8c64675, --fibers --compact-codegen
> --jobs8 WITHOUT --compile-only. Reuses fully compiled cached objects. State
> codex-all-functions-build-state.json phase linking-full-native; logs
> build/codex-all-functions-link{,.err}.log. Do NOT restart on silent Binaryen passes.
> Latest user explicitly wants all missing native code at once, avoid single-callback
> rebuilds. Entire112890original f functions/70119ELFpointers covered, total113930
> functions with unwind,306ctors; source1.09GB. Preserve full coverage even if repair
> needed; do not revert to observed-only graph. Config allNativeFunctions:true persists.
> NEXT: verify actual parent/children/CPU and linker completion. Use NEW
> build/codex-verify-all-functions-publication.py, which rejects oldddc3, validates
> full graph/function-count lower bound, independent coverage, local+HTTP hash/receipt,
> numeric-local guard and11JS. Only after successful guarded publication boot once.
> No game currently active (ddc3 terminal6944s missing3353750); actual compute1625
> dispatches had passed. Menu/CPU match/completion still unproven. Goal active.

> **26/10/2026 16:09Z — USER REQUESTED ALL NATIVE COVERAGE; COMPILE PREFLIGHT LIVE70560.**
> Latest human instruction: prioritize finding/adding EVERYTHING because rebuilds
> are slow. This explicitly supersedes the earlier observed-callback-only strategy.
> Added --all-functions to prepare_runtime.py; runtime_roots.json allNativeFunctions:true
> persists full coverage for subsequent prepares. New observed target3353750 retained.
> Original index audit:112890native f definitions in1159files,70119unique code-relative
> pointer targets from288143ELFslots; previously30908pointer targets/43561native funcs
> missing. ALL targets have original definitions, no stubs. Audit all-callback-audit.json.
> Prepared --main --all-functions --all-pointer-tables into staging, independently
> compared EVERY original f definition and EVERY ELF pointer target against fn_table.
> EXACT112890tableentries, zero missing/extra,70119pointers covered,3353750included.
> Total113930functions including unwind,306ctors,source1090117063bytes. Reports
> build/codex-all-functions-{prepare.log,coverage.json}; coverage helper independently
> rescans original sources+ELF, not just manifest. Graph copied into build/web-runtime.
> Code8c64675 pushed.13pruning tests PASS. Protected headers unchanged. Full coverage
> is preparation proof, NOT compilation/browser/playable-game proof.
> Compile-only preflight started16:09:53Z parent70560, --fibers --compact-codegen
> --jobs8 --compile-only. State build/codex-all-functions-build-state.json;
> logs codex-all-functions-compile{,.err}.log. Verified parent live and objects advancing.
> Wait wrapper exec session14232 waits70560; no compiler restart on quiet output.
> NEXT after real completion/Prepared objects: run build/codex-audit-all-function-objects.py
> to inspect every compiled object's numeric locals (>=113930defined funcs, max<=50000).
> Preserve full coverage if an object needs repair. Then run full SAME flags/jobs8
> WITHOUT --compile-only to reuse cached objects and link; record new linker PID/state.
> Do not boot oldddc3. Only after guarded full link/hash/HTTP/source verification boot
> once. Old compute verifier rejects7cf0, so NEW verifier must reject ddc3 instead!
> Last ddc3 session TERMINAL6944s actual uncompiled7103353750/lr71038fb850,
> guestFunction71038fb320,threadFailed0. Full build/codex-compute-runtime-terminal.json.
> Compute PASSED in actual game:1625dispatches/4404writtenbuffers/5267packed draws.
> ddc3 opening794s/A826release828,title932s/A960release964; notification was held
> a long time before A6434release6438; greenreward6536s/A6562release6566; blueVenusaur
> Spirit card6640s. One A6812released6822(holdtimeout10s,still released), black6876,
> terminal6944. Do not interpret long notification dwell as boot time. No game/compiler
> from oldddc3 active. Native/codec/graphics patches remain; goal menu/CPU match/
> completion NOT achieved. Notify important changes only, no repeat narrow rebuilds.

> **26/10/2026 14:02Z — COMPUTE BUILD COMPLETE, VERIFIED ddc3 BOOTED ONCE.**
> Parent78396/emcc68700/wasm-opt68940 finished, Linked runtime-fibers-compact.mjs.
> Guarded publish14:02:19Z; SHA256ddc3d11b3efd4a357b29180ddd62446cf27d3b923ee663fe8d8e888b4fd6f69d,
> 283733816bytes/100377functions/maximumLocals10812. Sourcef786574, nativegraph70362,
> 306ctors unchanged. No compiler active. Build-state now complete with receipt.
> Local+HTTP WASM/receipt/11JS verified14:02:36Z via compute verifier; actual artifact
> differs7cf0. build/codex-compute-runtime-publication-verified.json.
> Controlled integrated helper booted exactly once; compute-runtime-boot.log started.
> Trace+bounded guest reader installed; NEW GuestMemory20120312 (old20120040 invalid).
> compute-runtime-reader.log validated unique signature. poll-1 at38s/306ctors/
> threadFailed0/heap720240640. Current game is LIVE; preserve it, no restart/reload.
> NEXT: page-safe codex-querylod-page-poll.cjs -> compute-runtime-poll-N.log;
> screenshot NEW codex-compute-runtime-screen.cjs -> compute-runtime-live.png.
> Observe opening/title/reward before input, codex-querylod-press-a.cjs real focused
> Z/A holds10presents then releases; C-memory input verification not claimed by it.
> Then verify actual compute dispatch+writeback in this ddc3 game and reach actual
> menu/CPU match/completion. Old blue-spirit-card screenshot/terminal belongs7cf0.
> Runtime ddc3 is only verified build+boot so far; goal NOT complete. Important
> changes only, no repeat codec/fixture tests unless new evidence requires them.

> **26/10/2026 12:21Z — REAL COMPUTE f786574 PUSHED; FULL RUNTIME BUILD LIVE78396.**
> Implemented actual DispatchCompute recording/execution in existing command/state
> ownership path, bridge op26, WebGpuCompute pipeline/dispatch and actual shader WM
> atomic write-mask(binding38). Misaligned storage is copied in/out; actual written
> ranges land through host_gpu_resources GPU->guest writeback before next command.
> GPU-only pools retain physical results. No fake/skipped dispatch. RequestedStage5
> selects native source6; compile variants/snapshots and queue ordering preserved.
> New prod web/gpu-compute.mjs; build/probe copy lists updated, runtime-worker captures
> lastComputePacket on failure. Dynamic CB0, storage images, overlapping writable
> bindings remain explicit unsupported errors until required; no invented resources.
> Browser numeric test22checks PASS (two dependent dispatches, actual write-mask,
> offsets/copyback/guards/alias/bounds/stale snapshot). SDK C probe3527checks PASS,
> TWO actual recorded dispatches via pthread bridge; second stages first output
> from guest memory. Source input unchanged, guest OUTPUT/NEXT numeric bytes correct,
> guard words intact; lists/program refs/pools all released. Fixture WGSL injected
> only in fixture Worker. Reports build/codex-compute-sdk-probe-check.{json,log}.
> Actual captured Maxwell compute s00016_0.mxd -> full compiler -> GPU pipeline PASS,
> metadata429instructions/12blocks/CB24/SSBO7/written7,6active bindings. 967619WGSL
> bytes. NO dispatch without its real bound inputs, not proof of game's first call.
> build/codex-compute-actual-pipeline.{json,log}. Node program/snapshot/fetch/SPIRV tests
> PASS; short C SDK probe built. Important: withSdk:false skips texture/compute C
> block; only the withSdk:true report3527checks proves C writeback (371does NOT).
> Full build started12:21:16Z, parentPID78396, sourcef786574, args MUST remain
> --fibers --compact-codegen --jobs8. State build/codex-compute-runtime-build-state.json,
> stdout codex-compute-runtime-build.log, stderr codex-compute-runtime-build.err.log.
> Parent verified live; changed host GPU objects compiled and linker warning emitted.
> No native graph change/prepare required; graph70362/306ctors. Do NOT restart.
> Served runtime STILL old7cf0 until successful guarded publication; no game active
> (old stream-pack session terminal1272s native DispatchCompute, blue spirit card).
> NEXT: inspect parent+children/CPU/logs, not file state alone. After completion use
> build/codex-verify-compute-publication.py (rejects7cf0; local+HTTP hash/receipt/
> locals+11JS including gpu-compute), then boot ONCE via existing integrated helper.
> Install trace/reader; keep new screenshot/log names compute-runtime-*; original
> page-safe poll codex-querylod-page-poll.cjs, real input helper always releases Z.
> Goal active: actual compute execution/menu/CPU match/completion remain unverified.
> No repeat codec tests, no boot stale artifact, no compiler restart on silence.

> **26/10/2026 12:04Z — STREAM FIX PASSED; TERMINAL UNIMPLEMENTED COMPUTE DISPATCH.**
> Current session TERMINATED1272s, unimplemented NVN:nvnCommandBufferDispatchCompute,
> address/lr710009fc24,guestFunction7100031db0,sp80054af440,x0=8004db8a68,
> x8=71052a9ff8,threadFailed1. Preserve build/codex-stream-pack-terminal.{json,log}.
> Actual opening704s/A740release744; title850s/A906release910; notification1014s/
> A1048release1050; greenreward1138s/A1200release1204. Last actual screen is BLUE
> spirit card (pink wizard, ADVANCED text), output/playwright/codex-stream-pack-live.png.
> ImageQueryLod and excess9vertexstreams are now passed in actual game. Not menu/match.
> CodeHEAD120660f, docs990428c pushed; worktree clean. No compiler active. No reboot
> until compute implementation/tests; WASM7cf0 must change for C bridge support.
> NEXT IMPLEMENT REAL COMPUTE, no skip/zero outputs: native reference runtime/nvn_gl.c
> compute_dispatch2466+, compute_bind_glsl2615+. Skinning writes storage buffers and
> native compute_flush copies results back to guest memory before later CPU/draw reads.
> Existing web shader compiler supports compute(type6->metadata.stage5), but web NVN
> recorder and JS backend have NO dispatch execution yet. No compute code written.
> Architecture: host_gpu_commands.c Queue.programs[6], retained immutable snapshots,
> execute_binding_chain passes all6tokens to host_gpu_draw_execute; DrawState already
> uniform[6][16],storage[6][32],textures[6][32]. Add DISPATCH enum/name/validation in
> host_gpu_draw.c and DispatchCompute to known command names. Record x1/x2/x3 groups;
> execute must use stage5 state/program and preserve command-list/queue ordering.
> Add bridge operation26 JSON compute state, snapshots/program/15UBO/16SSBO/32textures.
> C gpu_range stages actual bytes on submission. Need new resource writeback API:
> host_gpu_resources.c transfer(pool,offset,size,1) reads GPU->guest; for CPU-backed
> pools this MUST land before later draw_range reuploads CPU memory. GPU-only pools
> keep authoritative GPU bytes. Return backend-written mask through WebGpuReply.value
> then write back bound actual storage ranges (conservative ssbosWritten acceptable,
> translator currently marks all used SSBOs when global stores exist). Do not fake.
> JS: add gpu-compute.mjs, backend op26/staging decode/constructor/close/report capture,
> full actual Maxwell->SPIRV->Naga pipeline, real dispatchWorkgroups and writable
> SSBO alignment/copyback. Binding scheme compute base0, CB0..17, SSBO20..35,
> sampled images group1 slot0..31; CBuf1 retained source constants, CBuf2 real handle
> table as gpu-draw.mjs, CBuf>=3 from UBO slots n-3. Dynamic CB0/images need explicit
> support/error; do not invent resources. Align writable storage via temp only when
> needed and copy back; handle/reject writable aliasing explicitly. Real SSBO bytes
> must remain visible to both later GPU work and guest CPU. Existing generic
> compileSnapshot compiles all stages; requestedStage currently0/1 only. Extend to5
> mappedsource6 if selecting compute from combined record; preserve variant caches.
> Tests: dump-free C GPU probe can exercise record/sublist/snapshot/lifetime+writeback,
> WebGPU compute numeric outputs and subsequent vertex use, alignments/alias/bounds;
> actual retained compute fixture if available. web/build_gpu_probe.py is short build;
> full runtime only after tests, --fibers --compact-codegen --jobs8 (mandatory), keep
> existing Binaryen locals/hash validation. Add new JS to build_runtime.py and probe
> copy lists if used. runtime-worker.mjs lastComputePacket diagnostic before cleanup.
> Never edit protected guest_memory.h/runtime_memory.h/runtime/runtime.h. No compiler
> restart on silence, no skips or dummy compute. Goal active, important notices only.

> **26/10/2026 11:43Z — VERTEX STREAM FIX120660f PUSHED; NEW LIVE BOOT.**
> Stage-fixed0b101cc session passed title/notification/reward and actual ImageQueryLod.
> Opening724s/A752released758; title856s/A902released906; notification992s/A1024;
> reward1122s/A1158released1160; terminal1182s: NVN streams exceed WebGPU vertex
> binding count. Full capture build/codex-querylod-stage-terminal.{json,log};
> failed-packet.json. Actual draw strip4 vertices/1instance/base0,9float32x4 attrs
> on distinct instance streams, same buffer349 with16-byte ranges. Failure draw
> capture includes all9complete16-byte vertex inputs and both actual shaders.
> Fix120660f interleaves actual fetched attributes into <=2buffers by vertex/instance
> step when stream count exceeds device binding limit; preserves signed/unsigned,
> normalized/default channels, stride0, baseVertex/firstInstance, validates bounds,
> staging64MiB/stride/attribute limits. No input/draw skipped; readback has overhead.
> GPU fixture1040checks/25draws PASS, including5packed draws with9streams, indexed
> base+firstInstance, typed data, zero-stride/defaults, unaligned normalized3channels,
> rejected OOB draw/pixels unchanged. Node vertex38contracts/index11contracts PASS.
> Evidence build/codex-stream-pack-check.json; source/published/HTTP3modules verified
> build/codex-stream-pack-publication.json. Native pixel parity still unverified.
> After saving terminal booted exactly once11:43Z; stream-pack-boot.log started:true.
> Trace+reader installed; GuestMemory20120040; poll-1 at16s/306ctors/threadFailed0.
> WASM7cf0 unchanged, no compiler active. PRESERVE current session.
> NEXT: page-safe helper build/codex-querylod-page-poll.cjs -> stream-pack-poll-N.log;
> screenshot build/codex-stream-pack-screen.cjs -> stream-pack-live.png. Earlier
> querylod-stage-* screenshots/logs belong to terminated0b101cc session. Observe
> actual scene before real Z/A input; helper codex-querylod-press-a.cjs releases key.
> Goal active: actual menu/CPU match/completion remain unverified. No repeat build,
> codec tests or reload on observation timeout. Notify important changes only.

> **26/10/2026 11:16Z — STAGE RESOURCE FIX 0b101cc PUSHED, NEW LIVE BOOT.**
> The f133896-only session passed actual opening714s (A736/release738), title826s
> (A846/release850), notification940s (A960/release962), reward1032s (A1050/release1054),
> then terminal1076s: ImageQueryLod requires actual48-byte sampler state.
> Saved build/codex-querylod-fixed-terminal.{json,log}; full capture retained.
> ROOT CAUSE: the same immutable program owns vertex+fragment sources. Vertex
> compile options had lodQuerySamplers:{} and forceOutputs63, yet compileRecord
> compiled BOTH sources, including unused fragment type5 with vertex resources.
> Fix0b101cc passes requestedStage0/1 from draw; compileRecord selects the actual
> bound stage BEFORE translation, preserves independent variant keys, rejects
> invalid requested/unsupported graphics stages. No draw/guest state skipped.
> Tests Node program ownership+combined-stage/cache+snapshots PASS; real GPU compiles
> combined actual captured vertex+failure fragment with correct per-stage options;
> full draw/shader-lowering fixture713checks/20draws PASS, including combined-program
> textured pixel readback. build/codex-querylod-stage-check.json. Native pixel parity
> and actual menu/CPU match/completion still NOT proven.
> Source/published/HTTP3modules matched: build/codex-querylod-stage-publication.json.
> After preserving terminal, booted once11:16Z; stage-boot.log started:true.
> Trace+bounded reader installed, GuestMemory20120040. stage-poll-1:10s/306ctors/
> threadFailed0. WASM7cf0 unchanged; NO compiler active. Preserve this live session.
> NEXT: use page-safe build/codex-querylod-page-poll.cjs -> stage-poll-N.log.
> Screenshot NEW build/codex-querylod-stage-screen.cjs -> querylod-stage-live.png;
> earlier fixed-* evidence belongs to failed f133896-only run. Observe actual scene
> before real input; page-only codex-querylod-press-a.cjs always releases Z.
> Goal active. Notify important changes only; no repeat test/rebuild/reload on silence.

> **26/10/2026 10:54Z — FIX f133896 NOW BOOTED ONCE; PRESERVE LIVE GAME.**
> Previously active session reached actual title (840s), accepted A860s/released864s,
> then terminal958s with the OLD unsupported ImageQueryLod. Captured
> build/codex-querylod-old-terminal.{log,json}; no callback/compiler failure.
> This confirms that session had pre-fix modules; it was not restarted while live.
> After terminal, controlled helper booted verified fixed JS exactly once10:54Z.
> build/codex-querylod-fixed-boot.log started:true; trace+reader installed successfully,
> GuestMemory20120040. Fixed-poll-1:14s/306ctors/threadFailed0, heap300154880.
> Runtime WASM remains7cf0; JS source/published/HTTP hashes recorded in
> build/codex-querylod-publication.json. Code f133896 is pushed; no compiler active.
> NEXT: keep this fixed session; page-safe helper build/codex-querylod-page-poll.cjs;
> once worker evaluates promptly, existing codex-av-poll-compact/clock helpers work.
> Observe actual opening/title before input. Original C-input-verified helper
> build/codex-boot23-press-a.cjs available; fallback codex-querylod-press-a.cjs uses
> real keyboard/focused canvas/10presents but DOES NOT claim C memory verification.
> Screenshot helper codex-querylod-active-screen.cjs uses querylod-active.png;
> copy important scenes to distinct paths. Old querylod-{opening,title}.png are
> PRE-FIX evidence. New fixed screenshots should use fixed-* names.
> Goal NOT complete: fixed shader passed isolated GPU tests, but actual menu,
> CPU match and completion still unverified. Do not reboot this session on timeout.

> **26/10/2026 — FIX ImageQueryLod PUSHED f133896; LIVE SESSION PRESERVED.**
> JS lowering reconstructs isotropic fragment 2D query from actual derivatives,
> actual texture-view base dimensions/level count and actual sampler state.
> Returns BOTH components: filter/clamp-dependent accessed mip and unclamped lambda;
> no zero replacement/skipped draw. Nonzero bias/snap/reduction, anisotropy!=1,
> arrays/cubes/non-fragment queries remain explicit errors. No WASM rebuild needed.
> Tests: Node compatibility+LOD state guards PASS; actual captured Maxwell shader
> through full compiler/swizzles and real GPU module PASS; GPU analytic numeric
> 16cases x144pixels x2components PASS; draw fixture453checks/16draws PASS.
> Fixtures web/lod-query-test.frag, web/test-gpu-lod-query.mjs, build_shader.py.
> Reports build/codex-querylod-{gpu,full-compile,publication}.json; served JS hashes
> match source. Runtime WASM still verified7cf0. Native pixel parity NOT proven.
> Controlled boot helper REFUSED because a runtime is already active (guard worked).
> DO NOT reload/kill it. Page-only evidence10:46:58Z at614s,306ctors/threadFailed0,
> 45901draws/2393presents, black screen; actual current page differs from old terminal.
> New session origin not established; it may have loaded pre-fix modules. Cannot call
> it a verified f133896 boot. Keep it until actual terminal or useful visible scene.
> Poll page only via build/codex-querylod-page-poll.cjs (worker evaluate pending;
> exec session37723, do not interpret timeout as failure). Screenshot helper
> build/codex-querylod-active-screen.cjs -> output/playwright/codex-querylod-active.png.
> NEXT: observe active session, act only on actual scene. If old ImageQueryLod
> terminal repeats, preserve report then boot verified fixed JS exactly once.
> Menu/CPU match/completion still unverified. Goal active; notify important changes only.

> **26/10/2026 — 7cf0 TERMINAL WEBGPU ImageQueryLod SAU CÁC BẢNG THƯỞNG.**
> Phiên đã terminal1156s, error WebGPU graphics draw failed, gpuError
> unsupported instruction ImageQueryLod at Function. Actual report
> build/codex-title-7cf0-terminal-page.json (đừng in toàn bộ GPU/media base64).
> lastShaderFailure giữ actual Maxwell control/data/options. Đã dịch lại bằng compiler thật,
> lưu build/codex-querylod.{frag,spv}; helper/log build/codex-querylod-translate.{cjs,log}.
> GLSL actual: textureQueryLod(mx_t1_sampler2D,vec2(...)); chỉ đọc q[1]*256, không q[0].
> Packet texture stage1 slot1: actual sampler bias0/snap0/anisotropy1, min5/mag1,
> LOD clamps -1000/1000. Không được thay ImageQueryLod bằng0 hoặc bỏ draw.
> Naga29.0.4 SPIR-V reader không hỗ trợ ImageQueryLod; WGSL không có textureQueryLod.
> Hướng đang nghiên cứu: hạ actual 2D unclamped y sang derivative LOD thật, dùng kích thước
> texture mip0 và bias từ sampler thật, chứng minh chỉ y được dùng; x/clamp/anisotropy khác
> phải explicit unsupported tới khi có xử lý đúng. Chưa có code fix/test cho hướng này.
> Nguồn chuẩn: registry.khronos.org/OpenGL/extensions/ARB/ARB_texture_query_lod.txt,
> y là lambda sau bias/trước clamp. Kiểm tra cả shader actual và GPU numeric derivatives.
> Code liên quan web/spirv-compat.mjs lowerSpirv, gpu-draw.mjs shaderOptions quanh136,
> game-shaders.mjs, shader-compiler.mjs (Naga WASM). Chỉ JS fix có thể không cần full runtime rebuild.
> Không compiler đang chạy. HEAD trước note0b826b0/9af4523; served WASM7cf0 đã verified.
> Chưa menu/CPU match. Tiếp xử lý shader từ actual capture, không reboot bản lỗi trước fix.

> **26/10/2026 — 7cf0 ĐÃ QUA TITLE VÀ HIỆN THÔNG BÁO/PHẦN THƯỞNG THẬT, PHIÊN CÒN CHẠY.**
> Không rebuild/reload phiên hiện tại. Opening thật808s, clock video12816000us/audio14708000us.
> A838s giữ10presents/thả, phim đóng840s, title trắng ổn định908s.
> A932s trên title được nhận; đến990s game còn chạy/threadFailed0 và hiện bảng
> huy chương Smash màu hồng. Screenshot codex-title-7cf0-notification.png.
> A1020s đóng bảng, tiếp1104s hiện màn phần thưởng nền vàng/icon xanh,
> screenshot codex-title-7cf0-reward.png. A1132s được nhận/thả,1138s đang chuyển màn đen.
> Các ảnh tại output/playwright/, logs build/codex-title-7cf0-{opening-press-a,title-press-a,
> notification-press-a,reward-press-a,poll-10,screen-8}.log. Không gọi đây là menu/CPU match.
> Text trong các bảng thông báo/phần thưởng chưa hiện; hình/icon/background có thật.
> TIẾP: poll phiên đang chạy, xem màn hình trước khi input, xác nhận qua các bảng thưởng,
> vào menu rồi CPU match+completion. Không spam cập nhật chờ theo yêu cầu người dùng.

> **26/10/2026 — BUILDab6c9df COMPLETE; GAME7cf0 ĐANG BOOT ĐÚNG1LẦN.**
> Chỉ thông báo thay đổi quan trọng theo yêu cầu tiết kiệm quota của người dùng.
> PID76168/78148/79596 đã kết thúc, Linked runtime-fibers-compact.mjs.
> Publish2026-10-26T10:12:50Z, SHA256
> 7cf054167924ed66816977be33522161d52aaabec503f4ec01a9af3028d477d7,
> 283730072bytes /100375functions /maximumLocals10812. Local+HTTP SHA/receipt/5JS
> khớp source lúc10:13:35Z; build/codex-title-ab6c9df-publication-verified.json.
> State đã complete, không restart compiler. Phiêne272 terminal đã lưu trước boot mới.
> Boot7cf0 đúng1lần:306ctors/33workers/threadFailed0 ở2s. Logs
> build/codex-title-7cf0-{boot,trace,poll-1,reader}.log. Trace+reader đã cài;
> unique GuestMemory20120040. Helper screen riêng build/codex-title-7cf0-screen.cjs.
> TIẾP: giữ phiên game, chờ opening/title thật, A rồi kiểm tra callback1af1f40/menu,
> CPU match+completion. Callback mới chỉ chứng minh có trong build; goal chưa đạt.

> **26/10/2026 — e272 TERMINAL7101af1f40; BUILDab6c9df ĐANG CHẠY.**
> Chỉ báo người dùng khi có thay đổi quan trọng, tránh lặp trạng thái chờ/tốn quota.
> e272 loading hiện292s, opening thật686s (video2205000us/audio4089333us).
> A712s giữ10presents/thả; phim còn chạy720s rồi title776s. A804s,
> title trắng ổn định870s; A896s giữ10presents/thả, vẫn chạy916s rồi terminal924s:
> uncompiled indirect target7101af1f40, guestFunction7101b15370/lr71022d279c,
> threadFailed0. Actual report build/codex-title-e272-terminal-page.json.
> Screens output/playwright/codex-title-e272-{opening,title,stable-title,live}.png.
> Original callback group71050a9168:6targets1af1c70/1af1d20/1af1de0/1af1e60/1af1f40/1af2050,
> 4hàm trước đó chưa compile, tất cả native definitions có thật; body c_023e.c:10534.
> Evidence build/codex-title-e272-callback-group.json. Codeab6c9df thêm đúng actual observed1af1f40.
> Prepare --main --pointer-tables xong:70362functions(+4),306ctors, missing0,
> cả6targets verified trong generated table. Log build/codex-title-1af1f40-prepare.log.
> Full build PID76168 bắt đầu2026-10-26T08:46:37.1204794Z,
> --fibers --compact-codegen --jobs8. Log/state build/codex-title-ab6c9df-build{,.err}.log / -state.json.
> Không restart compiler/không boot lại e272 đã terminal. TIẾP: chờ build thành công,
> xác minh local+HTTP SHA/receipt/5JS khác e272, boot mới1lần, menu/CPU match+completion.

> **26/10/2026 — BUILD56590a8 COMPLETE; GAMEe272 ĐANG BOOT ĐÚNG1LẦN.**
> Yêu cầu mới của người dùng: làm tới khi game chạy được, chỉ thông báo khi có việc quan trọng
> để tránh tốn quota; không lặp thông báo đang chờ khi trạng thái không đổi.
> PID78584/42876/78104 đã kết thúc. Linked runtime-fibers-compact.mjs.
> Publish2026-10-26T08:23:11Z, SHA256
> e272c95972ad4662ec4d8c563bb416c468a2b533b02e91ee0417c9a78086c90e,
> 283719841bytes /100369functions /maximumLocals10812. Local+HTTP SHA/receipt/5JS
> khớp source lúc08:26:23Z; build/codex-title-56590a8-publication-verified.json.
> State đã complete, không restart compiler. Bảna525 terminal đã lưu trước boot mới.
> Boot e272 đúng1lần:306ctors/33workers/threadFailed0 ở2s. Logs
> build/codex-title-e272-{boot,trace,poll-1,reader}.log. Trace+reader đã cài;
> unique GuestMemory20119976. Screenshot helper riêng build/codex-title-e272-screen.cjs.
> TIẾP: giữ phiên game, chờ opening/title thật, A rồi kiểm tra callback1b1a230/menu,
> CPU match+completion. Chỉ chứng minh callback group có trong build; goal chưa đạt.

> **26/10/2026 — a525 TERMINAL7101b1a230; BUILD56590a8 ĐANG CHẠY.**
> Phiêna525 đã phát gần hết phim thật: clock video125662000us/audio127545333us,
> duration129799000us; screenshot output/playwright/codex-title-a525-opening-end.png.
> A1652s giữ10presents/thả đúng, vẫn chạy1674s rồi terminal1694s:
> uncompiled indirect target7101b1a230, guestFunction71032e9a80/lr71022d2774,
> threadFailed0. Báo cáo build/codex-title-a525-terminal-page.json. Chưa menu/match.
> Phim đã EOF/reset/close tự nhiên trước A; diagnostic movieError cuối có
> No current movie sample, chưa chứng minh là nguyên nhân dừng (actual terminal là target trên).
> Original group71050a93b8 gồm6targets:1b19f30/1b1a010/1b1a0f0/1b1a150/1b1a230/1b1a340,
> 4hàm trước đó chưa compile, tất cả có native definitions. Evidence
> build/codex-title-a525-callback-group.json. Code56590a8 thêm đúng observed callback1b1a230.
> Prepare --main --pointer-tables đã xong:70358functions(+5),306ctors, missing0;
> cả6targets có trong generated table. Log build/codex-title-1b1a230-prepare.log.
> Full build PID78584 bắt đầu2026-10-26T06:56:03.5740448Z,
> --fibers --compact-codegen --jobs8. Log/state build/codex-title-56590a8-build{,.err}.log / -state.json.
> Không restart compiler/không boot lại bảna525 đã terminal. TIẾP: chờ build thành công,
> xác minh local+HTTP SHA/receipt/5JS khác a525, boot mới1lần, menu/CPU match+completion.

> **26/10/2026 — BUILD608df20 COMPLETE; GAMEa525 ĐANG BOOT ĐÚNG1LẦN.**
> PID67540/55800/66224 đã kết thúc; Linked runtime-fibers-compact.mjs.
> Receipt publish2026-10-25T23:21:26Z, SHA256
> a52567bab5d02ac873d48e261016d9c126a6062d4a2b28b81aa6c75f92f5e7e7,
> 283709042bytes /100362functions /maximumLocals10812. Xác minh local+HTTP SHA,
> receipt và5 JS modules khớp source lúc2026-10-26T06:21:45Z:
> build/codex-title-608df20-publication-verified.json. State đã complete, không restart compiler.
> Sau khi xác nhậnc738 terminal, boot mớia525 đúng1lần:306ctors/33workers/threadFailed0 ở2s.
> Logs build/codex-title-a525-{boot,trace,poll-1,reader}.log. Trace và guest reader đã cài,
> unique GuestMemory20119896. Helper screen riêng build/codex-title-a525-screen.cjs.
> Giữ phiên đang chạy; chờ loading/opening/title thật rồi A. Callback5080 mới chỉ chứng minh
> đã có trong build, chưa chứng minh chạy qua. Menu/CPU match+completion vẫn chưa xác nhận.

> **26/10/2026 — c738 DỪNG SAU TITLE Ở CALLBACK7103375080; BUILD608df20 ĐANG CHẠY.**
> Bảnc738 đã phát opening thật: video13016000us/audio14900000us, screenshot
> output/playwright/codex-title-c738-opening.png. A922s giữ10presents/thả đúng,
> title934s; A950s vẫn chạy, title trắng976/1034s. A1052s trên title ổn định,
> game còn chạy1070s rồi terminal1096s: uncompiled indirect target7103375080,
> lr71038fb850, guestFunction710392acb0, threadFailed0. Chưa menu/match.
> Actual report build/codex-title-c738-terminal-page.json; không in toàn bộ GPU base64.
> Original native body c_0449.c:9873, gọi trực tiếp7103374a30. Relocation group
> 71051847f8 có đúng1target5080. Evidence build/codex-title-c738-callback-group.json.
> Code608df20 thêm đúng observed target5080. Prepare --main --pointer-tables xong:
> functions70353(+2), ctors306, missing definitions0; cả5080/4a30 có trong generated table.
> Full build PID67540 bắt đầu2026-10-25T22:01:55.5287530Z,
> --fibers --compact-codegen --jobs8; log/state build/codex-title-608df20-build{,.err}.log / -state.json.
> Không restart compiler; phiênc738 đã terminal, không boot lại bản cũ.
> TIẾP: chờ build thành công, xác minh local+HTTP SHA/receipt/5JS, boot mới đúng1lần,
> chờ opening/title thật, thử A rồi menu/CPU match+completion. Goal chưa đạt.

> **26/10/2026 — BUILD4338f4e COMPLETE; GAMEc738 ĐANG BOOT.**
> PID64564/45900/68352 đã kết thúc, Linked runtime-fibers-compact.mjs.
> Publish2026-10-25T21:38:44Z, SHA256
> c738d9c8493abf950a698804c35c2acdfbf00b812582d68dcaf9fe2774970850,
> 283696477bytes /100360functions /maximumLocals10812. Local+HTTP WASM,
> receipt và5 JS modules khớp source; evidence build/codex-title-4338-publication-verified.json.
> Đã boot mới đúng1lần, 306ctors/33workers/threadFailed0 ở6s; trace đã cài.
> build/codex-title-c738-{boot,trace,poll-1}.log. Giữ phiên đang chạy, không reboot.
> Helper screenshot build/codex-title-c738-screen.cjs giữ riêng evidence bản cũ.
> Guest reader mới đã cài: unique GuestMemory20119864, build/codex-title-c738-reader.log.
> TIẾP: chờ opening/title thật, A→title rồi A→menu,
> kiểm tra callback65a0 đã qua, CPU match+completion. Chưa chứng minh menu/match.

> **26/10/2026 — BUILD GROUP CALLBACK65a0 ĐANG CHẠY, KHÔNG RESTART.**
> Code4338f4e thêm observed target71023265a0. Graph70351(+70), đủ19hàm từ
> actual relocation group71050b6a78, ctors306/callback missing0 đã xác minh.
> Full build PID64564 bắt đầu2026-10-25T20:17:21.9782289Z, --fibers --compact-codegen --jobs8.
> Log/state build/codex-title-4338f4e-build{,.err}.log / -state.json.
> Build trước e2360f4 đã complete349e; phiên349e đã terminal ở65a0 sau A trên title.
> Giữ evidence349e, không reboot bản cũ/không nhầm pid56760/65988 là build hiện tại.
> TIẾP: chờ build4338f4e thành công, xác minh served WASM/receipt/source,
> boot mới1lần và thử menu/CPU match. Goal chưa đạt.

> **26/10/2026 — BOOT349e VÀO TITLE ĐƯỢC; THIẾU CALLBACK TIẾP71023265a0.**
> Boot349e giữ nguyên, loading logo hiện ở302s. Movie mở/VP9+Vorbis configure ở~465s,
> còn chờ Start đến~800s; poll zero queues trước đó không chứng minh decoder treo.
> Actual clock3 video3707000us/audio5241333us, canvas có openingKirby thật.
> A862s được giữ qua10presents rồi thả, phim→title SSBU Ultimate ở878s;
> screenshot output/playwright/codex-title-349e-{opening,title}.png.
> A tiếp930s: game terminal946s, uncompiled indirect target71023265a0,
> caller71017f4a60/lr7103725c7c, threadFailed1. Actual report
> build/codex-title-349e-terminal-page.json và after-title-a.log. Chưa menu/match.
> Đã xác minh original relocation code-pointer group71050b6a78:19hàm,6chưa compile
> (23264e0/2326540/23265a0/2326b90/2326bd0/2326e40), tất cả có lifted body thật.
> Evidence build/codex-title-349e-callback-group.json. Không băng qua null/table header
> để đoán callback; thêm actual observed target65a0 sẽ kéo toàn bộ group theo cơ chế sẵn có.
> runtime_roots.json đã thêm65a0; prepare --main --pointer-tables đã xong:
> functions70351(+70), ctors306, callback missing0, đủ19targets trong generated table.
> log build/codex-title-65a0-prepare.log. Không restart/reload bản349e đã terminal.
> TIẾP: xác minh generated table có đủ19targets, full build --fibers --compact-codegen
> --jobs8 giữ optimizer/validation, hash/receipt rồi boot mới1lần/menu/CPU match.

> **26/10/2026 — BUILD CALLBACK ĐÃ XONG; GAME349e ĐANG BOOT ĐÚNG1LẦN.**
> Build PID56760/wasm-opt65988 đã kết thúc; Linked runtime-fibers-compact.mjs.
> Publish2026-10-25T19:55:11Z, SHA256
> 349ee09297daa06e6b5fcf38c57af5e44a0829cbc31f6c12f95db33e02869f25,
> 282930438bytes /100273functions /maximumLocals10812. Local+HTTP WASM hash,
> served receipt và5 JS modules (worker/webm/video/audio/backend) khớp source.
> State build/codex-title-e2360f4-build-state.json đã complete. KHÔNG restart compiler.
> Sau khi xác nhận phiên5ef2 terminal, đã boot349e1lần với7font+romfs/main.elf.
> ssbu-web URL runtime=fibers-compact&diagnosticFrames=1;18s/306ctors/33workers,
> threadFailed0, chưa presents/movie. Không kết luận callback đã chạy qua hoặc menu đã vào.
> Movie trace đã cài; GuestMemory unique mới20118744, reader đã cài. Evidence
> build/codex-title-349e-{boot,trace,poll-1,reader}.log. Helper screenshot mới
> build/codex-title-349e-screen.cjs →output/playwright/codex-title-349e-live.png.
> Bounded poll helpers đã sửa: done=true không evaluate worker đã dừng; compact giữ
> terminal error/address/caller/lr. Nếu cần full terminal result dùng page-only helper
> build/codex-av-terminal-page.cjs (lọc các field cần thiết, không in GPU base64).
> TIẾP: giữ boot349e, đợi movie/title, A bỏ intro rồi chụp title và A tiếp,
> kiểm tra callback71023263b0 thật đã qua, menu/input/CPU match+completion. Goal chưa đạt.

> **26/10/2026 — BUILD SỬA CALLBACK TITLE ĐANG CHẠY, KHÔNG RESTART.**
> Code e2360f4 thêm observed lifted callback71023263b0; prepare --main --pointer-tables
> đã xong: functions70281(+2), ctors306, referencedPointerTableTargets18677,
> callback tables378/targets2443, missing definitions0. Generated body/table đã kiểm tra.
> Full build PID56760 bắt đầu2026-10-25T18:33:24.6677128Z, --fibers --compact-codegen --jobs8.
> Log/state build/codex-title-e2360f4-build{,.err}.log / -state.json.
> runtime_c02ed.c và runtime_table.c đã compile; chưa publish bản mới.
> Phiên game5ef2 trước đã terminal sau A trên title; giữ bằng chứng, không boot lại bản cũ.
> TIẾP: xác minh build terminal/receipt/hash/source, boot bản mới1lần, A bỏ intro,
> A tiếp title rồi kiểm tra menu/input/CPU match. Goal còn active, chưa chơi được trận.

> **26/10/2026 — GAME THẬT ĐÃ HIỆN PHIM + TITLE, A CÓ TÁC DỤNG; THIẾU CALLBACK KHI VÀO TIẾP.**
> Boot5ef2 đã tiến qua nạp tài nguyên: tạo VP9 decoder3 và Vorbis decoder4,
> selected video0/audio2. Lúc đầu chưa Start/feed; các mẫu poll8/9 không chứng minh
> decoder bị kẹt. Sau đó input/output thật hoạt động; clock4 audio38857333us,
> video37040000us. Không ép clock/readiness, không fake audio/EOS.
> Ảnh actual canvas thấy opening (Dr.Mario/LittleMac) rồi title SSBU Ultimate:
> output/playwright/codex-av-5ef2-opening.png và codex-av-5ef2-title.png.
> Z/A ở1018s giữ qua10presents, input C buttons1 rồi thả: phim dừng/dtor cả2decoder,
> extractor đóng; title hiện ở1056s. Đây là input có tác dụng, chưa phải trận đấu.
> A tiếp ở1078s: nnMain terminal1090s, uncompiled indirect target71023263b0,
> caller7103726120/lr7103726294, ctors306/threadFailed0. Actual report:
> build/codex-av-5ef2-terminal-page.json. Không reload/boot lại phiên đã terminal.
> Target có lifted definition thật trong gen_eh_o2/c_02ed.c, chưa trong runtime_table.
> Đã thêm đúng observed target vào runtime_roots.json, prepare --main --pointer-tables
> đã xong: graph70281(+2), ctors306, missing definitions0.
> Log build/codex-title-callback-prepare.log. Không dùng stub hoặc bỏ qua hàm.
> TIẾP: kiểm tra graph mới chứa callback + referenced vtables, compile full
> --fibers --compact-codegen --jobs8 (giữ optimizer/validation), receipt/sourcehash,
> boot đúng1lần rồi vào menu và thử trận CPU. Chưa đạt goal; âm thanh nghe được chưa test.

> **26/10/2026 — FULL BUILD AV ĐÃ XONG; ĐANG BOOT GAME THẬT 5ef2.**
> PID25188/wasm-opt63736 đã kết thúc; log Linked runtime-fibers-compact.mjs.
> Receipt publish 2026-10-25T18:07:10Z: SHA256
> 5ef2ab1b3e7ac388d60245096727a1327ee421a923460b46ee3e21a97ebac8ce,
> 282925256 bytes / 100260 functions / maximumLocals10812.
> File hash và HTTP served WASM đều khớp receipt; runtime-worker/movie-webm/
> movie-video/movie-audio/movie-backend served đều khớp source hiện tại.
> Đã Stop runtime5c88 bằng nút thật, done=true; boot5ef2 đúng1lần với7fonts+romfs.
> Session ssbu-web giữ URL runtime=fibers-compact&diagnosticFrames=1, 33workers,
> 306ctors/threadFailed0. 104s: frames0/draws0, chưa movie handle, manager0;
> chưa chứng minh audio clock/readiness/menu/input/match. KHÔNG coi fixture là game chạy.
> Bounded movie trace đã cài; unique GuestMemory mới20118712, reader đã cài.
> Evidence build/codex-av-5ef2-{boot,trace-install,reader,poll-1,poll-2,clock-1}.log.
> TIẾP: giữ phiên hiện tại, poll actual movies/clock, chụp canvas khi present,
> kiểm tra input thật và trận CPU. Không restart compiler hoặc reload game tùy tiện.

> **25/10/2026 — SDK VIDEO+VORBIS ĐÃ QUA FIXTURE; FULL BUILD AV ĐANG CHẠY.**
> Code commit dd968d5 nối MovieAudioDecoder vào backend, Extractor selected Map đa-track
> giữ một cursor/pending sample chung, không rewind khi Select/Unselect. Filter track
> trước xử lý lace, bỏ cached sample khi unselect đúng track. Seek0 quay đầu có audio
> priming; audio-only seek giữ một packet overlap thật. Nonzero AV seek chưa chứng minh.
> Decoder C type5/output1 nhận rate/channels thật, audio/raw +pcm-encoding2, 64 output
> slots; video vẫn48. PCM COPY dùng guest buffer/capacity/timestamp/lifetime thật.
> EOS time lấy từ output cuối + duration codec, không còn timestamp0 mặc định; EOF
> vẫn phải thật. MovieBackend giữ track info để Stop/Reset được sau SourceClose.
> Browser SDK AV fixture qua production OS router/guest allocator/ClientStreamReader:
> 120 video packets→120 NV12 outputs, 203 Vorbis→202 PCM (priming1), mọi hash/time/byte
> khớp direct decode. Giữ đủ48 video/64 audio, nonzero PCM367279bytes, EOF và EOS1 mỗi
> decoder, liveHandles/pendingCompletions/guest objects/allocations sau dtor đều0.
> Evidence build/codex-movie-sdk-av-120.json (13911checks ở lượt đo). Test-only muxer
> giữ nguyên payload/header/track number của cả3 track; chọnvideo0/audio2 như game.
> Full actual108813844byte source OPEN cũng PASS cả3 track; maxInput video283126,
> audio719/716, selected0+2 ra packet đúng thứ tự, EOFfalse. codex-av-full-source.json.
> Node multi-track/queue/Vorbis/WebM tests PASS, strictC11 PASS, OS182 PASS,
> SDK video-only240frame + concurrent Dtor vẫn PASS. Không sửa protected headers.
> Repro: build_movie_bridge_probe.py --sdk-av; test_movie_av.py --movie <data.arc>
> --offset1689801320 --length108813844 (giữa option và số phải có khoảng trắng).
> FULL BUILD AV PID25188 bắt đầu2026-10-25T16:05:41Z, wasm-opt PID63736 đã xác nhận chạy.
> Flags --fibers --compact-codegen --jobs8, giữ optimize-level1/validation. State/log:
> build/codex-av-dd968d5-build-state.json, build/codex-av-dd968d5-build{,.err}.log.
> Kiểm tra process thật trước restart; log im lặng không chứng minh đã dừng.
> Game hiện vẫn5c88/audio clock0; giữ nguyên đến khi bản AV publish thành công.
> TIẾP: chờ build AV này, xác minh receipt/hash, boot đúng một lần với7 fonts+romfs,
> kiểm tra actual audio clock/first-frame readiness/title/menu/input/match. Chưa đạt goal.
>
> **25/10/2026 — VORBIS JS ADAPTER ĐÃ QUA STREAM THẬT, CHƯA NỐI SDK.**
> web/movie-audio.mjs mới: Xiph/Vorbis header validation, AudioDecoder thật,
> AudioData→PCM s16 interleaved; queue64 giữ reservation từ trước decode đến Release.
> First Vorbis packet chỉ priming, không tạo output; sau đó mỗi packet có PCM thật.
> Format/range/lifetime/error được kiểm tra, đóng mọi AudioData sau copy.
> Node test_movie_audio.mjs PASS. Browser production adapter track2:256 packet→255
> PCM hash/time khớp toàn bộ raw AudioDecoder probe. Track3:6000 packet→5999 PCM,
> 5908 block có tín hiệu, maxOccupied64, backpressure371, không còn reservation;
> last PCM t105174666us. Report build/codex-movie-audio-adapter-track3-6000.json.
> Repro: python web/test_movie_audio.py --movie build/dump-13.0.4/romfs/data.arc
> --offset 1689801320 --length 108813844 --packets 6000 --track 3.
> CHƯA đổi MovieBackend selection hoặc Decoder C audio, chưa full rebuild/reload.
> TIẾP: import MovieAudioDecoder vào backend, extractor đa-track (video+audio),
> SDK type5/format1/rate/channels/PCM format/64 slots và events; fixture AV trước build.
> Game5c88 đang giữ nguyên, audio clock0 còn chặn title. Không ép clock/readiness.
>
> **25/10/2026 — FULL BUILD XONG, GAME THẬT ĐÃ GIẢI MÃ HẾT VIDEO; CÒN KẸT AUDIO CLOCK.**
> Bản mới: SHA 5c88f7a92ccc717481213c2de4b282344f7470890828efb231142fcbcda32e57,
> 282924589 bytes / 100259 functions / maxLocals10812, publish 13:49:08Z.
> Build PID61344/wasm-opt22848 đã kết thúc, log Linked runtime-fibers-compact.mjs;
> state build/codex-movie-dc49f05-build-state.json đã cập nhật complete. KHÔNG restart.
> Game mới đang chạy session ssbu-web, URL runtime=fibers-compact&diagnosticFrames=1,
> 306 ctors/33 workers. Khi chuyển bản, page cũ đã about:blank/0 worker; boot đúng một lần.
> Source JS/receipt/Wasm hash kiểm tra khớp. Chưa có menu/input hiệu lực/match.
> Movie thật hash40 0x1dcc80344b, 108813844 bytes, duration129799000us, VP9 1280x720
> +2 Vorbis48k stereo. Entry data.arc offset1689801320 (khớp manifest streams).
> Actual backend: Extractor2/Decoder3, video track0 selected, 3890 packets submitted,
> 3891 output acquired/released gồm EOS thật=1, 1945 COPY, source EOFtrue, không còn
> video input/frame/held/reservation. Audio SelectTrack trả unsupported1006.
> Player lần này CÒN SỐNG, không bị placeholder xóa như bản030a; vẫn đen sau video.
> Root sequence0x80545a3900 state2/callback1865010; chain +232→0x804abcf1c0,
> +128→0x805013c580,+248→0x80051f7310,+8→0x805013c6c0,+232→player0x802eb75ca0.
> Component state216=3/callback3398860. Readiness vtable+120=f36e26c0.
> Root cause đã đọc đúng chain: global7105339028→manager80051e0030,+8→state8004e80000,
> +0→session8049630000. Session+88→video holder804faeb5d0,+0→video8048845680,
> video+168=129733000us; session+104→audio holder804faeb600,+0→audio80487f5480,
> audio+264=0. f36e26c0 ưu tiên audio pointer không-null và trả clock0 này, nên title
> gate không mở dù video đã EOF/EOS. Không sửa pointer/clock/flags/animation để ép qua.
> A và Plus thật đã giữ qua10 presents, shared Npad bits1/1024 đúng, chưa thấy chuyển cảnh.
> Screenshot output/playwright/codex-movie-5c88-live.png đen. Preserve runtime hiện tại.
> BROWSER VORBIS ĐÃ THỬ THẬT: AudioDecoder.isConfigSupported({codec:'vorbis',rate48000,
> channels2,description:actualCodecPrivate4344bytes}) supported=true. Probe riêng trên
> Blob của game, không đổi backend/guest: 256 packet →255 PCM s16 interleaved blocks,
> 163 block có tín hiệu, không lỗi; first packet priming không có output. First audible
> block t1953333us/hash ea5a6b25, last t5222666/hash6f85525a. AudioData f32-planar được
> copyTo({format:'s16',planeIndex:0}) thành PCM thật; không tạo PCM im lặng giả.
> Evidence build/codex-movie-{after-a,title-gate,player,clock,audio-decode-256}.json/.log;
> guest reader signature unique ở20118696; helper build/codex-install-live-guest-reader.cjs.
> Diagnostic worker có self.__ssbuDiagnosticMovies; trace self.__ssbuMovieTrace giữ64
> control results và aggregate counts, không đổi kết quả request. Helpers ignored:
> codex-movie-install-trace.cjs, codex-movie-poll.cjs, codex-movie-read-clock.cjs.
> TIẾP: implement Vorbis AudioDecoder/PCM adapter, đa-track Extractor và SDK audio
> Configure/format/64 output slots/events. Browser/SDK fixture real audio+video trước
> full rebuild --fibers --compact-codegen --jobs8. H264 còn unsupported. Goal ACTIVE.
>
> **Cập nhật 25/10/2026: Decoder SDK và sự kiện đã qua video thật; source đã nối runtime.**
> host_movie_decoder.c mới có ctor/vtable 0..18, 8 input allocation qua guest
> allocator, 48 output lease, NV12 metadata/copy và Stop/Reset/Flush/Start/EOS.
> web_movie_notify dùng owner ID tăng dần và mutex riêng, không giữ mutex qua
> JS request/callback, không lấy Decoder gate. JS chỉ dùng key HostEvent đã đăng
> ký qua web_os_try_signal_event, không đọc guest memory hoặc tạo event mới.
> Dtor gỡ registry rồi chờ in-flight call, đóng backend, hủy notifier/free input.
> Fixture dùng production OS import/indirect, SDK Extractor đọc thẳng vào buffer
> Decoder và actual WebCodecs: 240 frame hash/time khớp direct, giữ đủ 48 output,
> EOF/EOS thật=1, late registration/format/owned buffer/capacity/flush/start/stop PASS.
> Deterministic concurrent Dtor khi STATUS request còn chờ JS PASS; backend/guest
> allocations/metadata/Decoder/Extractor đều sạch sau teardown. Report được giữ
> tại build/codex-movie-sdk-decoder-240.json. OS probe 182 PASS, C11 syntax PASS.
> build_movie_bridge_probe.py --sdk-decoder; test_movie_video.py --movie <file>
> --sdk-decoder --excerpt-eof --frames 240. Excerpt chỉ là fixture, không vào game.
> host_os.c route codec trước fallback, bỏ movie vtable lỗi cũ; runtime-worker.mjs
> tạo MovieBackend/notify/dispatch/close. build_runtime.py thêm source+header hashes,
> bridge gnu11 và 3 movie modules publish; không đổi optimize-level1/validation.
> Backend reset giữ track metadata để Stop vẫn giải phóng queue sau SourceClose;
> Submit sau SourceClose vẫn bị từ chối. Vorbis/H264 vẫn unsupported rõ ràng.
> Full build ĐÃ BẮT ĐẦU lúc 2026-10-25T12:29:37Z, PID 61344; đã compile các adapter
> và chuyển sang link. State build/codex-movie-dc49f05-build-state.json; stdout/stderr
> build/codex-movie-dc49f05-build{,.err}.log. Phải kiểm tra process thật trước restart;
> không suy ra đã dừng chỉ vì log chưa đổi. Flags --fibers --compact-codegen --jobs 8.
> TIẾP: theo dõi build này rồi boot kiểm tra gate/menu/match khi publish hoàn tất.
> Game đang chạy vẫn Wasm 030a cũ; CHƯA có bằng chứng menu/match. Xem web/MOVIE.md.
>
> **Cập nhật 25/10/2026: hàng đợi Decoder JS đã qua 240 khung hình thật.**
> Đã giữ chỗ output trước decode và trong suốt thời gian giữ lease; không dùng
> decodeQueueSize để suy ra số output còn chờ. Đọc header VP8/VP9 để đếm frame
> hiển thị, hidden reference, show-existing và superframe. Tách superframe thành
> từng coded frame trước WebCodecs: browser thử nghiệm làm mất một ảnh nếu gửi
> chung packet có hai output; sau sửa cả hai hash khớp ảnh gốc.
> Backend CONFIGURE time=8 bật 8 input slot, INPUT_AVAILABLE=29 trả bitmask slot
> khi codec đã nhận dữ liệu, STATUS=30 trả lỗi. Output gồm cả lease giới hạn 48.
> EOS drain chạy ngoài request chain để Release vẫn tiếp tục giải phóng chỗ;
> chỉ báo EOS sau EOF thật và decode xong. SDK Stop/Flush dùng RESET, không dùng
> FLUSH của probe. Reset hủy input/lease; callback/drain cũ không tác động decoder mới.
> Test thật: 240 packet VP9 giữ nguyên payload trong container thử hữu hạn,
> đạt 8 input chờ và 48 output giữ; 240 hash/time khớp direct decode, EOS=1,
> handle và reservation còn lại=0. Direct decode đầy queue 16 rồi chạy tiếp.
> Reports: build/codex-movie-direct-240.json, build/codex-movie-queue-240.json.
> Node test_movie_video.mjs, test_movie_backend_queue.mjs, test_movie_backend.mjs,
> test_movie_webm.mjs đều PASS; SDK Extractor real/excerpt 12-frame vẫn PASS.
> Repro: test_movie_video.py --movie <file> --frames 240 [--queued-excerpt].
> CHƯA Decoder C SDK/events, runtime wiring hay full build; game vẫn bản 030a,
> chưa menu hoặc match. Tiếp tục Decoder vtable/guest buffer/event, dùng backend
> time=8 và INPUT_AVAILABLE; xem web/MOVIE.md. Không dùng excerpt trong game.
>
> **MỚI 25/10/2026 — SDK EXTRACTOR ĐÃ ĐỌC REAL VIDEO QUA GUEST CALLBACK.**
> Source host_movie_codec.c mới: ExtractorC1/C2, vtable7800000a00/functionbase
> 7500003000, SourceReader/Path/Memory, Count/Config/Select/Unselect/Seek,
> Peek/Read/Advance/Time/MetaData/BufferedRange vàD0/D1. ChưaDecoderSDK!
> CHƯA routecodec vàohost_os.c,fullbuildsources hoặc runtime-worker. Game030a
> giữnguyên. Khôngfullbuild/reload, protectedheaders khôngsửa.
> ReaderABI từnnmovie_mf.c:vt+16Open(reader,path,outHandle),+24ReadAt(reader,
> handle,offset,guestBuf,size),+32Close,+40GetSize. Gm scratchqua guestallocator,
> callbackCtxfullrestore. Partialreadloop, bounds512MiB/chunk1MiB; release
> handle+scratchTRƯỚCJSmetadataOpen nhưnative. GUEST_DELETE710392e590 chỉcho
> deletingwrapperdtor, khôngnhầmvớifreecallbackmoviebuffers. Perinstancegate/
> refcount/remove-before-wait teardown; concurrentdestroy chưafixturetest.
> host_movie.c/.h mởtypedmetadata/guestalloc/call5 helpers; valuesfixture2820PASS.
> New SDKfixture: build_movie_bridge_probe.py --sdk-extractor;
> test_movie_video.py --movie <localfile> --sdk-extractor [--excerpt-eof].
> Tệp90.670s thật80762819bytes qua467ClientReadercallbacks,134topchecksPASS,
> 12frameVP9hash+timekhớpdirectdecode; EOS0/EOFfalse, handle/pending0.
> Report codex-movie-sdk-extractor-real.json. Fake-readerfixture lấyFILEthật,
> cốýpartialReadAt173123byte vàGetSize trả0x123 đểkiểmtraSDKignoreresult,
> khôngreturnhostpointerchohandle/guestbuffer. Đâyfixturekhôngphảibootgame.
> SDKexcerptfixture136checksPASS/reader2/EOFtrue/EOS1/12frame, giữnguyên
> packetpayload từsource; metadata/sampleinfo/bufferedrange/SDKSeek0 vàdtor/
> liveextractor/valueallocation0 đềuđượccheck. SourcePath/Memory+nonzeroSDKseek
> chưafixturecover; Decodercủafixture vẫnprotocolbridgeC→JS, chưaSDKslots/events.
> Report codex-movie-sdk-extractor-eof.json. C11 -Wall -Wextra syntaxPASScorecodec.
> TIẾP:SDKDecoderctor/vtableConfigure→backendhandle, slotinput/outputlifetime,
> events+notify vớiCregistrygeneration, guestmemorycopy/frameformat, Stop/Reset/
> Flush/EOS/destroy. SauproductionSDKfixturemớirouteOS+runtimeworker vàfullbuild
> --fibers --compact-codegen --jobs8. Vorbis/H264 vẫnchưa, khôngfakeEOS/readiness.
> Chưa menu/inputhiệulực/match; goalactive. Chi tiết web/MOVIE.md.
>
> **MỚI 25/10/2026 — C PTHREAD ↔ JS MOVIE BRIDGE ĐÃ QUA REAL VIDEO.**
> web/movie-backend.mjs: serializedWebM source/track/peek/read/advance/seek,
> realVP9decode/output acquire/copy/release/reset/drain/EOS. Seekdùngkeyframe
> indexthật, outputgiữsởhữusauSourceClose. UnsupportedVorbis báo1006 rõràng.
> MovieBackendEOS chỉchấpnhận khiextractorđãđọctớiEOFthật; earlyEOSbịreject.
> host_movie_bridge.c/.h: pthreadcond completion +boundedstaging +SDKstatus,
> EMASMproxy yêu cầugnu11 nhưGPUbridge. Chưađưa vào fullruntime source list,
> chưa nốiExtractor/Decoder virtualcalls/events tronggame hoặc runtime-worker.
> Buffer/MediaData C adapter từlượttrướcvẫnsourceonly, game030a...chưarebuild.
> Fixture browser: build_movie_bridge_probe.py +test_movie_video.py --bridge.
> Openingcandidate419.855s actual12frame C→JS→C:382checksPASS, time/hashkhớp
> cả12frame với directbrowserdecode; EOFfalse/EOS0 (khôngfakekếtthúc).
> codex-movie-bridge-opening.json, handle0/pendingcompletion0 saucleanup.
> Testonlyexcerptmuxer lấy12packetVP9thật, giữnguyênpayload vàtimestamps,
> tạoWebMhữuhạnmới trongmemory. ActualEOFtrue/EOS1/12frame/139checksPASS,
> codex-movie-bridge-excerpt-eof.json; khôngđưaexcerpt vào game hoặc sửaasset.
> Node test_movie_backend.mjs PASSserial/selection/seek/EOF/handle/errorcompletion
> +shortreplyhead khôngconsumeoutput. test_movie_webm.mjs PASS.
> Clanggnu11 -Wall -Wextra syntaxPASSbridge+fixture. Workerfixture dọnpthreadpool
> TRƯỚCpostreport, rồi tabphụđóng. Heartbeatgame9468s/33workers/306ctors/SHA030a
> donefalse, khôngreload/restart. Protectedheaders khôngsửa.
> TIẾP:SDK C Extractor/Decoderobjects, ClientStreamReader callbacks qua guestCtx,
> trackmetadata link, input/output slots+eventkeys, gluebridge vào runtimeworker;
> productionSDKfixture realvideo trước fullgamebuild --jobs8. Web/MOVIE.md chi tiết.
> Chưa menu/inputhiệulực/match; goalactive.
>
> **MỚI 25/10/2026 — C MOVIE VALUE IMPORTS ĐÃ SỬA, CHƯA FULLBUILD.**
> Source web/host_movie.c/.h đã thay Buffer ABI sai trong host_os.c và thực hiện
> MediaData constructors/copy/assign/destruct/Clear/Set/Find/Contains/CountEntries.
> Buffer đúngrawoffset0/4/16/24/40; owningcopydeep, borrowedcopykhôngsởhữu,
> assignment/swap/free dùng đúng allocator callback+userdata của từngallocation.
> SetAllocator đổi không làm cácallocationcũ bịfreebằngcallbackmới.
> Không đưa host malloc pointers vào guest. Metadata string nằmtrong guestmemory,
> copy độc lập, Clear/destruct/type replacement giải phóng đúnglifetime.
> Registry locks khônggiữqua callbackguest, Ctxđược khôi phục đầyđủ saucallback.
> web_os_import routeadaptertrướcstub; web_os_reset gọi resetquiescent.
> build_runtime.py thêmhost_movie.c vàheaderfingerprint, khôngđổi flagsbuild.
> Các protectedheaders khôngsửa; khôngrebuild/reloadgame030a73c0 đanglive.
> Fixture productionC: python web/build_movie_values_probe.py PASS2819checks,
> haiWasm pthreads/callbackreentry/contextrestore/ownership/type/lifetime/leak0.
> RuntimeC11 -Wall -Wextra syntaxPASS; fixturebuilderđãdùngđúngstd=c11.
> python web/build_os_probe.py PASS182checks gồmrawABI vàMediaData quaOSrouter.
> Reports build/web-movie-values/report.json, build/web-os; khôngphảigamecheck.
> web/MOVIE.md cậpnhật phầnđãlàm/cònthiếu. Tiếp theo vẫnExtractor/Decoder thật,
> guestClientStreamReader/trackselection/seek/decoderbuffer/events vàJSbridge.
> Cầnfixtureproductionimports+realvideo trướcfullgamebuild--jobs8.
> Emscriptenlocal settings cóUSE_VORBIS/USE_OGG nếu cần giải mãVorbisaudio bằngC;
> chưathêmports, chưađánhgiáfallbackunsupportedaudio của game. KhôngfakeEOS.
> Chưa menu/input hiệu lực/match; goal vẫnactive.
>
> **MỚI 25/10/2026 — ĐÃ CÓ GIẢI MÃ VP9 THẬT TRÊN BROWSER, CHƯA NỐI GAME.**
> web/movie-webm.mjs + movie-video.mjs đọc WebM và giải mã WebCodecs thật,
> copy I420/NV12 thành SDK NV12. Không fake sample/frame/EOS. Chưa import vào
> runtime-worker hoặc thay C movie stub; game030a73c0 vẫn ở title gate cũ.
> node web/test_movie_webm.mjs PASS (lacing/timestamps/bounds/NV12 odd sizes).
> Đã đọc metadata+firstvideo của toàn bộ128 WebM archive, không lỗi;
> VP9 +Vorbis, VP9CodecPrivate0. Manifest codex-movie-webm-metadata.json.
> Một file90.670s được demux hết5440frame tớiEOF thực, first3000/last90653000us.
> Browser thật decode12frame1280x720, timestamps khớp, NV12=1382400byte/frame.
> Đã lặp lại với opening candidate419.855s/3tracks/hash40=20e266a6d8,12framePASS;
> chưa xác nhận candidate bằng captured movie file-open của boot này.
> MediaSHA0fcd53c42de14577b81f8b9934a332da811b7e52f2ea3d772192d9a40d312976.
> Repro: python web/test_movie_video.py --movie <localfile> [--offset N --length N].
> Probe mở/đóng tab phụ, inputFile local, không đưa media lên served directory.
> Nó copy chỉ2newJS vào build/web-poc, không reload/rebuild/boot game.
> browser reports build/movie-video-probe/report.json và
> codex-opening-candidate-browser-decode.json. Media giữignored.
> Heartbeat sau probe:7418s,33workers,306ctors,SHA030a...,donefalse.
> Chi tiết protocol/giới hạn/bước nối C: web/MOVIE.md. Tiếp theo làm SDK adapter
> +Buffer ABI/MediaData/guest allocator+callbacks/events+worker bridge, fixture
> production imports với real video trước fullbuild. Chưa audioVorbis/rawH264/MP4.
> Node openAsBlob(data.arc>4GiB) báosize sai trên máy này; QA đọc archive dùng
> FileHandle.read với offset64-bit, browserFile.slice thực không gặp lỗi đó.
> Chưa menu hoặc match; goal vẫn active.
>
> **MỚI 25/10/2026 17:28 VN — TITLE ĐANG CHỜ MOVIE OBJECT ĐÃ BỊ DỌN, KHÔNG PHẢI CHỜ BUILD.**
> Actual runtime030a73c0/ssbu-web giữ nguyên, không reload/rebuild/ghi guest state.
> Đã xác nhận layout thật bằng resource string tạiLayout+48: title+us_en,
> không còn chỉ suy đoán từ tên press_any. Captures codex-font-layout-names.json.
> Outer sequence data0x80217d3f00: current state200=2, current callback8=
> 0x7101865010, frame204=37838; flags240/241=1/1. Nó vẫn được cập nhật.
> f1865010 tại5020..5050 đọc+232 -> wrapper0x804abcef40 ->+128 ->data
> 0x805013c580 ->+248 ->0x8005277310 ->+8 ->data0x805013c6c0 ->+232.
> Giá trị cuối bằng0 nên return tại18650c4; KHÔNG gọi readiness vtable+120.
> Nhánh thành công5054..50c0 mới bật title layout, request animation state1,
> clear title task flag112 và chuyển outer sequence sang state3.
>
> Title task wrapper0x804abcb200 vtable71050abba0 -> update7101bfb5e0,
> data+128=0x805013c080. State216=-1/frame220=0/callback8=0, flag112=1.
> f32ca620 scheduler tạica658..ca66c BỎ QUA task khi flag112==1. Vì vậy
> title animation đứng yên có nguyên nhân ở gate phía trên, không phải deltaTime.
> Source ctorf1865650 tạo các callback in/press-any tương ứng với data này.
>
> Movie wrapper804abcef40 đang enabled(flag112=0), data805013c580 state216=1,
> frame220=38026, callback1bf3340. Movie component805013c6c0 state216=4,
> frame220=11, pending state136=5, callback3398990, player pointer232=0.
> f3398990 là cleanup: dừng/xóa player, ghi0 vào+232 tại3398a20 rồi request5.
> f3398da0 chỉ update component khi+232!=0, nên sau cleanup giá trị này và
> state4/pending5 có thể giữ nguyên. Không kết luận active decoder hang.
> Actual captures: codex-font-title-{controller,wrapper,sequence,movie-gate}.json.
> Pointer discoveries dùng bounded guest heap page scan, mỗi hit được gm_read
> xác nhận; không CDP heap object query. Tất cả helpers build/ là ignored.
>
> Host movie hiện web/host_os.c: MOVIE_FN_BASE mọi slot trả0x4000; constructor
> Extractor/Decoder chỉ cấp bảng failure. Native placeholder runtime/nnmovie.c
> cũng có stub này, NHƯNG native chạy thật thay bằng runtime/nnmovie_mf.c.
> Không coi placeholder là skip thành công. Bước tiếp: triển khai extractor/
> decoder thật cho browser (real WebM/VP9 samples, event/metadata/Buffer ABI),
> hoặc một chế độ skip được chứng minh qua protocol thật; không force EOS,
> readiness, scene state, identity matrix hoặc animation để giả thành công.
> Audit phát hiện Buffer ABI web hiện cũng sai so với subsdk/native: đúng là
> offset0 i32,size4 i32,owns16,capacity24,base40 (native B_* constants).
> Đây là việc phải sửa khi làm movie adapter; chưa sửa production trong lượt này.
> Đã đọc W3C WebCodecs, chưa triển khai/chứng minh decoder browser hỗ trợ.
> Local archive read-only scan xác nhận128 WebM trong2890 stream entries;
> manifest build/codex-movie-streams.json, chưa xác nhận title movie cụ thể.
> Python zstandard cài riêng build/python-deps, không thay Python global.
> **Chưa menu/input hiệu lực/match. Goal vẫn active.**
>
> **MỚI 25/10/2026 17:10 VN — ĐÃ TÌM ĐƯỢC NHÁNH UI BỊ COLLAPSE.**
> Game030a73c0 vẫn live tại5516s/306ctors, ssbu-web; KHÔNG rebuild/reload.
> Không sửa production C hoặc protected headers trong lượt này. Chưa menu/match.
> Dữ liệu mới đổi hướng điều tra: matrix không hỏng đồng loạt. Actual pane set_bg
> 0x804ab6a680 và các parent fade/RootPane có matrix112 identity, local72/76=1/1.
> Nhánh 06_S0x801f74b900 ->pos_06_S0x802eb74880 ->scale0x802eb747a0 ->us0x802eb746c0
> ->mask_root0x805013c1c0 ->inout_null0x802eb74500 ->RootPane0x802eb73d20.
> RootPane local72/76=1/1 và matrix112 identity; inout_null local72/76=0/0,
> matrixXY0. Các con kế thừaXY0; 06_S có local72/76=10.29/0. Pane flags vẫn visible.
> Evidence build/codex-font-pane-chain.json/.log (read-only, parent+24/name176).
> Không force identity/unhide/animation frame. Tìm vì sao sequence chưa kích hoạt in.
>
> Layout owner đã khôi phục từ stack thật:0x80059ea340, root+24=0x802eb73d20.
> Intrusive animation list+8/+16 có4object:in804abceae0,press_any804abceb80,
> out804abcecc0,insert_01804abcec20. Hai lượt đọc vẫn frame32=0/speed60=0 cho cả4;
> applyEnabled36 chỉ insert_01=1. Resource pai1 hợp lệ, frameCount193/120/1/183.
> Fields được xác nhận từ lifted f00410a0(frame+=speed*delta),f003e230/f003f070
> (enabled36),f003e210(resource+24,frameCount+8); tên88 từ f37a04b0.
> codex-font-layout-anims{,-first}.json/.log. Tên gợi ý title/press-any, chưa chốt
> sequence cụ thể hoặc nguyên nhân animation chưa chạy. Focus web=1 giống native;
> chưa có bằng chứng focus notification hay movie là nguyên nhân, không tự sửa giả.
>
> Truy nguồn mới không cần boot lại: Ctx ABI840/flags768/TLS784/fn816 được emcc
> static_assert (build/codex-font-ctx-abi.c/.o). Bounded scan32MiB+64KiB/pthread
> tìm30contexts, main20707944, UI fiber16155184/TLS7800040000(TaskWorker2).
> ShaderUI GPUprogram7812 được chọn bằng full Maxwell control/data equality với
> capture15/25. Op20 C program snapshot chỉ được gọi từ BindProgram. Actual caller
> f00324a0 tail-dispatch qua x3! Vì vậy sampler chỉ x8..x17 trước đó không đủ.
> Probe codex-font-install-ui-record-contexts.cjs dùng x0..x30, actual LR0047858,
> stack004493c ->0060640 ->00594ac ->recursive0059530 ->379c0bc ->004b6e8
> ->37857a0 ->374d328. Captures codex-font-ui-record-contexts{,-stack,-panes}.json.
> Captured guest stack page một lần dưới map_lock để tránh busy nhiều lần.
> Pane pointer khôi phục ở child f0044900 saved parent x20 tại FP-16; caller
> f00604b0 đặt x20=this. Layout khôi phục ở root f0059450 saved parent x21 FP-24;
> caller f004b6a0 đặt x21=thisLayout, root+24. Tất cả probe wrappers đã close.
> backend.perform đã restore prototype sau upload-cache/context probes; game giữ nguyên.
>
> NVN registry static21114224 có517name128-byte, anchored actual QueueSubmit slot58.
> BindProgram379/0x7a00017b00;BindUniformBuffer382;BindUniformBuffers383;
> UpdateUniformBuffer452;CBFenceSync467. Registration không chứng minh invocation.
> Actual guest proc slots:QueueSubmit710593fab0,UpdateUniform7105940700.
> 560-byte uploads phát sinh cả từ draw_range lúc submit, không chỉ NVNFlush;
> transfer-argument probe không match đã bỏ. QueueSubmit context main LR37f0b40
> và FP chain384f51c/37495cc/02c5cf0 đã ghi, chưa phải matrix producer.
>
> Hướng tiếp cụ thể: unique main.elf string insert_01 ởrelative43ce3b5
> (=guest71043ce3b5), xref7101865e30/34 trong f_7101865650(runtime_c0208.c).
> Hàm này có alloc/init/lookup animation, không mặc định coi là per-frame updater.
> press_any string offsets42d59a6/437b456/4453979. Raw locations
> codex-font-title-string-locations.json. Tìm sequence/controller StartAnimation(in),
> các điều kiện readiness/input/movie quanh constructor/caller; giữ chứng cứ actual.
> Nguồn có fallback UpdateUniformBuffer immediate và CBFenceSync immediate đáng audit,
> nhưng chưa chứng minh chúng được gọi ở nhánh lỗi này; không coi là root cause.
> **FONT BUILD ĐÃ PUBLISH VÀ GAME MỚI ĐANG LIVE — 25/10/2026.**
> SHA030a73c0998b986bbb35819038f793a04b4da8ace9a71dd9b2f2327514d40fa5,
> 282880087bytes/100199functions/maxlocals10812. Build xong15:38; hash/4GiB
> shared max65536pages/Node306ctors đều pass đúngSHA. Không còn compiler chạy.
> Coordinator57768 đã kết thúc thành công; after-build16372/browser-followup56984
> đã hoàn tất quan sát và giữ nguyên game. KHÔNG rerun coordinator/observers/boot.
> Sessionssbu-web đã boot bản font mới đúng1lần với7font local. Đã thấy logo loading,
> sau đó canvas đen; CHƯA có menu, input tác động vào game hoặc match chơi được.
> Checkpoint build/codex-font-game-checkpoint.json tại1648s:heap1332477952,
> allocated1286712152/system1291745440,188126draws/8216presents,
> snapshots118551created/156live. Đã vượt lỗi65536 cũ. Screenshot hiện tại:
> output/playwright/codex-font-030a73c0-current.png (đen); không reload để chụp lại logo.
> Actual font arena4267mappedpages, đọc được2SFNT thật (std8256glyphs/upem1024,
> zh-cn28944glyphs/upem1024). Đây chưa phải bằng chứng live glyph/menu.
> Evidence codex-font-live-arena-check.log; helper đã thêm boundedretry cho SFNT đầu.
> Input A476s và Plus1074s: focus/visible=true, C buttons1/1024, phím đã thả;
> không có bằng chứng menu phản hồi. Logs codex-font-game-press-{a,plus}.log.
> Recent512presentation calls:6.43FPS, gap p50=144.14ms/p95=177.34ms;
> present-call p50=1.755ms/p95=2.76ms. Có diagnostic overhead; không phải GPU timing,
> không coi là FPS animation/gameplay hoặc quy nguyên nhân chỉ từ số này.
> Thread helper mới codex-font-read-threads.cjs tự tìm unique array20730080,
> ABI208/name76; read-only/unlocked snapshot, không dùng địa chỉ cũ. 25threads;
> MOVIE_INIT/MoviePlayerThread đều finished1. Không kết luận active decoder treo.
> GameMemory mới20114328, resource registry21376416; địa chỉ chỉ choSHA này.
> Initial8UIsamples:1worldXYidentity lúc frame1243,7zero; late8samples sau frame6465
> worldXY đều0, đủ560byte khớp C gm_read upload staging với GPU. Shaders trùng bản
> đã phân tích. codex-font-{initial,upload}-uniform-analysis.json; raw
> codex-font-upload-pass-probe-refreshed.json. Không sửa matrix/clock/fakeEOS.
> Đã sửa diagnostic pool discovery cạn256attempt trước UIpool361: refresh bounded4096
> sau khi đã tìm registry. Test latepool361 fail old/pass fixed;396a2cd đã push.
> 8d1ae99 chỉ thêm copy JS diagnostic vào publication; Python syntax/hashcopy pass,
> không đổi C/flags/Wasm hoặc rebuild. build_runtime.py source hash hiện khác receipt
> cũ chỉ do bước copy này; KHÔNG rerun observer để so frozen inputs rồi boot lại.
> Bản game hiện tại dùng probe đã refresh thủ công trước fix; đọc đủpool361,8uploads
> đều equal. JS mới đã copy vào served folder cho lần dùng sau, không reload game.
> Tiếp theo: giữ game, tìm guest producer/scene state làm UI worldXY0; font thật chưa
> giải quyết màn hình đen. Mục tiêu vẫn menu thật/input có tác dụng/match hoàn chỉnh.
> **Đã chuẩn bị đo present timing cho lần boot font mới (chưa có số đo).**
> build/codex-font-present-timing.cjs bọc render.present, giữ tối đa512 bản ghi;
> báo average present FPS, p50/p95 khoảng cách khung và thời gian gọi present.
> Không thêm GPU readback; không coi CPU/await duration là GPU execution hoặc
> present FPS là animation/gameplay FPS. Đã kiểm tra cú pháp Node.
> Browser-followup gọi helper sau mỗi live sample, lưu build/codex-font-game-timing.json
> gắn SHA mới. Chỉ collector sau build validate; chưa boot/chưa có kết quả.
> Poll15:34 ngày25/10 xác minh optimizer55572 còn chạy/CPU12883.62s;
> coordinator57768 và after-build16372 vẫn chờ đúng build, không chạy trùng.
> **25/10/2026 14:22 VN — BUILD KHỞI ĐỘNG LẠI SAU WINDOWS CRASH.**
> Đã xác minh không còn coordinator/optimizer/observer cũ. OS boot 23/10 10:45;
> Event41/6008 và Application1001 ghi BlueScreen/P1=116; chưa kết luận nguyên nhân.
> Staging Wasm 0byte (23/10 10:43), publication cũ vẫn281992236byte/f40835ac.
> Không coi state building-font-runtime cũ là process còn chạy.
> Build mới coordinator57768/builder37168/wasm-opt55572, exec61649; flags giữ nguyên,
> --fibers --compact-codegen --jobs8, optimize-level1/normalvalidation, không debuggraph.
> Follow-up mới build/codex-font-after-build.py PID16372/exec48964, state .json tương ứng:
> chờ đúng coordinator và validated publication; KHÔNG mở Chrome khi compiler đang chạy.
> Sau SHA/4GiB/Node306 pass mới mở server8787 + riêng session ssbu-web nếu chưa có,
> rồi chạy codex-font-browser-followup.py (PID coordinator đọc từ state, không hardcode).
> Mọi browser/runtime khác được guard bảo vệ; chỉ boot một lần sau validation.
> Hủy tự mở game bằng build/codex-font-browser.cancel trước khi dừng/pause build.
> Chưa publish font build, chưa có bằng chứng menu/input/match chơi được.
> **ĐÃ CHẠY TIẾP — 23/10/2026 08:15 VN, theo yêu cầu người dùng.**
> Full build mới coordinator169108/builder177332/wasm-opt181088, exec1648.
> --fibers --compact-codegen --jobs8; dùng cache object, chạy lại Asyncify đã bị dừng.
> Browser observer176208/exec82738 theo coordinatorPID từ state, chờ đúng SHA/4GiB/Node306.
> Chỉ sau validate mới boot font build một lần. ssbu-web hiện about:blank/0workers;
> observer chấp nhận riêng trạng thái trống này, bảo vệ mọi trang/runtime khác.
> Server8787 đã mở lại (exec82636). Muốn dừng: cancel observer trước, rồi dừng build.
> Log/state build/codex-font-{full-build.log,build-followup.json,browser-followup.json}.
> Log/state lần bị dừng đã archive build/font-stopped-20261023-081531 (kiểm tra tên thực tế).
> Chưa có publication font mới hoặc bằng chứng menu/gameplay. Không chạy trùng build.
> **ĐÃ DỪNG THEO YÊU CẦU NGƯỜI DÙNG — 23/10/2026 06:01 VN.**
> Đã dừng coordinator178424/builder145152/emcc143808/wasm-opt143728.
> Browser observer đã hủy bằng build/codex-font-browser.cancel; goal đang paused.
> State build/codex-font-build-followup.json = stopped-by-user. Cache object giữ nguyên,
> nhưng Asyncify chưa hoàn tất, khi chạy lại phải làm lại bước tối ưu. Chưa publish font build.
> Trang ssbu-web chỉ còn about:blank/0workers (nguyên nhân mất boot23 chưa rõ).
> Chỉ chạy lại khi người dùng yêu cầu; kiểm tra process/state trước, không dùng các note live cũ.
# Bàn giao AI: SSBU 13.0.4 WebAssembly / WebGPU

> **AUTO FONT BROWSER FOLLOW-UP ĐANG CHỜ:** PID158588/exec75657,
> build/codex-font-browser-followup.py + .json. Không chạycompiler mới.
> Chỉ saubuildSHA/4GiB/Node306/sourcehash pass mới archiveboot23, Stopoldruntime,
> bootfontfix đúng1lần, cài probes và ghi progress/fontarena. Đừng manualboot/
> stop/reload để tránh race observer. Nó heartbeatgiữ sessionssbu-web mỗi180s.
> Hủy observer trước nếu người dùng yêu cầu dừng/pause: tạo
> build/codex-font-browser.cancel hoặc stop riêngPID158588. Hiệngoal vẫnactive.
> build/codex-read-font-arena.cjs đã chuẩn bị; oldbaseline arenaReserved:false.
> Installer guestreader tự tìmGMobject, không dùngstaticaddress cũ choSHA mới.
> Optimizer143728 live(CPU3520.94s), coordinator178424/exec42509 chưaxong.
> Xem statecả2process trước khi hành động; KHÔNG rerun scripts chỉ đểpoll.

> **Chrome font-thread path PASS:** test_font_browser.py dùng C adapter thật,
> local File/WORKERFS + PROXY_TO_PTHREAD/pool3. Đủ1513imports/700glyphcycles,
> sharedfonts/GPOS callbacks pass khi C đọc file trên pthread qua ownerworker.
> Evidence build/font-browser/report.json. ssbu-font đã đóng, ssbu-web giữ nguyên.
> Fullbuild optimizer143728 vẫn live(CPU1582s), exec42509; chưa publish/bootmới.
> Boot23 tại5138s:heap1324744704,snapshots414543tạo/150live,registry242,
> reclaims763096; screenshot vẫn đen. Đây là verifiedwait, không phải buildfailed.

> **FONT ĐÃ NỐI — FULL BUILD ĐANG CHẠY:** host_font.c thay font stub bằng
> dữ liệu7TTF thật + FreeType + guest-safe GlyphMap/OTF/lifetime/errors. UI và
> worker đã có input fonts/WORKERFS; chưa publish bản mới/chưa boot font thật.
> Fixture1513imports/700glyphcycles/2threads + known GPOS callback/reentry pass;
> OS177checks pass,7font raster/input checks pass. Xem STATUS.md chi tiết/phạm vi.
> Coordinator178424/builder145152/emcc143808/optimizer143728, exec42509 live.
> State build/codex-font-build-followup.json, log codex-font-full-build.log.
> KHÔNG rerun coordinator/build mới. --fibers --compact-codegen --jobs8,
> không --debug-graph (actual old full build không dùng), không --trace-asyncify;
> giữ optimize-level1/normalvalidation. Protected headers/graph giữ nguyên.
> Tự checkSHA+4GiB+Node306 rồi ready-for-font-browser-boot, không tự reloadgame.
> Helper build/codex-start-font-integrated.cjs đã chuẩn bị7font/chưa chạy.
> Preserve boot23 tới khi bản mới validate, archive evidence rồi dừng có chủ đích
> để bootfixfont. C staticaddresses cũ không dùng choSHA mới. Chưa chơi được.

> **Tiến độ font thật:** đã thêm web/font_raster.c/.h và fixture chạy lại bằng
> web/test_font_raster.py (pinned Python). FreeType2.14.3, 7TTF local đều pass
> raster/metrics/cmap/CJK khi có/whitespace/rotate/shear/reflection/outline và
> giữ glyph độc lập. Evidence build/font-raster/report.json. Chưa nối guest ABI,
> chưa nạp shared fonts/chưa build runtime mới; host_os.c vẫn glyph stub cũ.
> Tiếp theo adapter guest-safe + lifetime + kerning/GPOS và đường nạp font thật.
> Boot23 tại3844s vẫn live, heap1324744704,311527snapshot tạo/150live,
> registry242/controlReclaims571788; presentation vẫn đen. Preserve run.

> **Cập nhật sau55phút boot23:** vẫn live, heap1324744704;263877snapshot tạo,
> 149live, registry242/controlReclaims483310. HID connected1/styles63 và sampling
> tăng thật. MOVIE_INIT/MoviePlayerThread đều finished1; không kết luận decoder
> đang treo. Glyph buffer thật size40/type8/advance32px nhưng width/height/stride0,
> chứng minh game dùng font stub. Ưu tiên chức năng tiếp: font loading/rasterization
> thật (7TTF local, SDK FreeType2.14.3); chưa viết font backend/chưa build mới.
> Xem STATUS.md và codex-boot23-{hid-state,threads,font-state} artifacts. Skeleton
> OS chỉ để disassembly, bỏdata/unselectedcode, KHÔNG được chạy hoặc publish.

> **BOOT23 VẪN LIVE — bằng chứng mới sau25phút, không reload.**
> SHA f40835ac..., ssbu-web/PID149760. Snapshot đã tạo102234, còn149live;
> heap1324744704bytes giữ nguyên từ1104s tới1504s. Đã vượt ngưỡng lỗi65536 cũ.
> Registry242/controlReclaims117178 ở1104s. Chưa có menu/input/trận đấu chơi được.
> Quan trọng:8mẫu UI khớp đủ560byte giữa GPU và bytes C đã gm_read để upload;
> world X/Y đều0 ngay trong nguồn guest. Source vertex trùng shader đã phân tích.
> Artifact codex-boot23-upload-pass-probe.json và upload-uniform-analysis.json.
> Upload cache chỉ handles348/361,64đoạn<=1024byte; không tranh gm map_lock.
> Direct guest comparisons của late probe đều busy, không coi như khớp dữ liệu.
> Mapping UI361:pool0x8000423fd0,CPUbase0x8010600000,size150994944;348 làquad.
> Z/A đã thử tại1248s, canvas focus/visible, C input buttons1, giữ10frames rồi thả;
> chưa thấy chuyển menu. Draw delta448..1104s đúng24/frame, chỉ gợi ý movie.
> Native thực tế override placeholder bằng nnmovie_mf.c (boot.c), skip mode12sample
> rồi EOS; so với nnmovie.c placeholder trước đây là chưa đủ. Không sửa clock/
> matrix hoặc giả EOS khi chưa có call/state evidence. Xem STATUS.md mới nhất.
> Browser-report hiện có thể là liveUploadPassProbe; archive đúngSHA trước writer.

> **MỚI NHẤT — BOOT23 ĐANG LIVE,23/10 khoảng03:13 Việt Nam.**
> Build control-reuse đã publish SHA f40835ac644406c5a6d43efae4ef3816b5de73e34a1fb826db5c53627855b431,
> 281992236bytes/99138functions/maxlocals10812. Actual hash, shared max65536pages
> và Node306 đều pass đúngSHA. Không còn optimizer/builder/coordinator đang chạy.
> Coordinator kết thúc ở bước browser vì session đã đóng để giảm RAM; đã mở lại
> ssbu-web (PID149760) và guarded boot thành công, target/pass/pool probes ready.
> Receipt build/codex-control-reuse-browser-recovery.json, log boot-start-recovery.
> Preserve game này; KHÔNG rerun coordinator/reload/boot thêm. Các note linker phía
> dưới là lịch sử. Guest reader đã cài,32byte khớp ELF gốc;5pool game thật ánh xạ
> được, không lỗi ở kiểm tra đầu. Capture sắp tới so VS NVN uniform slot0 (shader
> binding3/CB3) với guest prefix<=1024bytes. Chưa có kết quả live comparison/menu/
> input/trận đấu. Archive pass report đúngSHA trước mọi helper ghi /__report.

> **Bổ sung sau Asyncify:** pass Asyncify đã xong4451.41s; optimizer90200 vẫn
> validation, coordinator183220/builder179628/exec8095 còn sống. Preserve build.
> Đã đóng riêng browser session ssbu-web bằng CLI sau khi xác nhận tab idle,
> không game/Worker, để giảm bộ nhớ. Sau compiler hoàn tất cần mở lại session
> trước guarded boot. Nếu coordinator báo thiếu browser thì dùng publication/Node
> reports hợp lệ và boot thủ công; KHÔNG chạy lại coordinator vì nó sẽ build lại.
> Xem build/codex-control-reuse-build-health.json cho CPU/memory mới nhất.
> Font web còn glyph width/height0; source review trong STATUS.md, chưa chứng
> minh là nguyên nhân màn hình đen. Không sửa production C trong linker này.

> **Bổ sung01:28:** vẫn optimizer90200/exec8095, CPU4422.77s; chưa publish/boot.
> Source a3fd877 thêm guest-pool metadata tracing,18JS checks và3502C/WebGPU
> fixture checks pass. Harness tương lai tự cài probe và ghi buffer.guestSource
> vào pass captures; startup mock13checks pass. Không đổi C/flags/graph đang link.
> Sau boot mới kiểm tra self.__ssbuGuestPoolProbe.report(), dùng pool.memory+offset
> để truy uniform guest. Chưa có bằng chứng mapping từ game mới. Xem STATUS.md.

> **HIỆN TẠI — 23/10,01:05 Việt Nam: FULL BUILD CONTROL-REUSE ĐANG CHẠY.**
> Boot21 đã crash khi thử CDP object query; boot22 sau đó dừng đúng lỗi snapshot65536
> tạiheap3.54GB. Đã quan sát565lần reuse control address, sửa vòng đời trong50b1275;
> fixture3502checks/99reclaims pass, registry/snapshots về0. Workerdiagnostic hook5e8c5d6.
> Exec8095 / coordinator183220 / builder179628 / optimizer90200 còn sống; preserve.
> Finalizer8772 đã kết thúc; optimizer CPU474.59s lúc01:11. Chưa publish/boot mới.
> State `build/codex-control-reuse-followup.json`, log `codex-runtime-control-reuse-build.log`.
> Script tự kiểm tra memory65536pages + Node306 đúngSHA rồi guarded boot mới.
> Browser ssbu-web idle '/', Node172852. f37e hiện phục vụ là BẢN CŨ chưa cófix;
> không boot cũ trong lúc link. Không dùng các rawCaddresses f37e cho SHA mới.
> Các live note boot21 bên dưới là lịch sử. Xem STATUS.md mới nhất và báo cáo thật.
> Chưa có menu/input/trận đấu/audio/FPS được xác minh; nguồn world matrix UI0 còn cần tìm.

> **BOOT21 ĐANG LIVE — 23/10,00:30 Việt Nam:** ssbu-web/Node179088, SHA f37e127a...
> đã vượt2GiB cũ: heap2634547200,79444004imports/59707draws tại858s; chưa lỗi.
> Registry69730lists/3552250bindings; allocator2590366984bytes. Logo đã thấy nhưng
> sau loading vẫn đen. Có11capture sớm và10capture muộn sau A; world matrix UI
> có hàng X/Y bằng0, geometry co về1điểm ở các mẫu đó. Chưa biết guest producer
> hoặc lifetime đúng của command storage. Xem STATUS.md và codex-boot21-* reports.
> Preserve run; không restart/link mới. Menu/input/trận đấu chưa xác minh.

> **TRẠNG THÁI MỚI — 23/10/2026, khoảng00:16 Việt Nam:** goal đã được resume
> (tool status active). Build4GiB đã publish SHA f37e127a4124c6637b5ecbae57366fd52e73598d4fdb6c5762f4d3860dd41dd7;
> actual memory max65536pages và Node306 đúngSHA đã pass. Boot21 thật đã bắt đầu
> trên session ssbu-web mới (Node179088), cả target/pass probes đã cài. Không reload
> hoặc khởi động lại game live. Không còn linker. Ghi chú dừng/coordinator/browser
> phía dưới là snapshot lịch sử; coordinator cũ vẫn dừng, followup JSON vẫn stale.
> Menu/input/trận đấu chưa xác minh. Xem STATUS.md mới nhất và report thật.

**BÀN GIAO KHI NGƯỜI DÙNG YÊU CẦU DỪNG — 22/10/2026, 12:27 Việt Nam.**
AI hiện tại dừng công việc mục tiêu; không tự tiếp tục boot hoặc sửa game.
Đã dừng coordinator PowerShell144168 để ngăn tự chạy Node/browser sau build.
Compiler168312 và builder172172 được giữ để không mất kết quả build dài đang gần
hoàn tất. Đã xác nhận cả hai còn sống sau khi dừng coordinator. Người tiếp quản
phải có yêu cầu tiếp tục từ người dùng trước khi chạy game/sửa tiếp.

Cập nhật 22/10/2026 khoảng 12:27 giờ Việt Nam. Đây là snapshot: kiểm tra process,
state và publication thực tế trước khi tiếp tục. Các PID có thể đã kết thúc.

## Checkpoint Git và repository mới

Source đã được đưa vào14commit theo từng subsystem, giữ upstream gốc938e66b và
ngày commit thực tế. Xem [COMMIT_CHECKPOINT.md](COMMIT_CHECKPOINT.md) để tra cứu
các hash, kiểm tra đã chạy và những artifact không nằm trong Git.
Đích upload theo yêu cầu người dùng: `https://github.com/longurara/SSBU-port-web.git`,
nhánh `main`; remote native gốc giữ tên `upstream`. Thao tác Git không resume mục
tiêu chạy game đã tạm dừng. Process bên dưới là snapshot12:27, có thể đã kết thúc;
AI tiếp quản phải kiểm tra lại trạng thái thật. Mọi ghi chú cũ nói `web/` untracked
chỉ là lịch sử, không phải trạng thái sau checkpoint này.

## Mục tiêu và trạng thái

Người dùng yêu cầu làm đến khi game chạy được. **Chưa hoàn thành**: đã thấy logo
loading thật, chưa xác nhận menu đúng, input tác động vào game và trận đấu chơi
được. Constructor pass, fixture GPU, draw count hoặc pixel khác đen không đủ.
Yêu cầu mới nhất: **“dừng lại và viết bàn giao đầy đủ đi”**. Mục tiêu được tạm
dừng theo yêu cầu này; tiêu chí thành công vẫn là menu/input/trận đấu thật.

Repo: `C:\Users\Hikari\Desktop\wasm\ssbu-recomp`. Shell PowerShell, Windows,
RAM16GB. Thư mục ngoài `wasm` không phải repo chính. Server `python web/serve.py`
tại `http://localhost:8787` **phục vụ `build/web-poc/`**, không phải `web/`.

## Ràng buộc phải giữ

- Không sửa `web/guest_memory.h`, `web/runtime_memory.h`, `runtime/runtime.h`.
  Header runtime có thay đổi sẵn của người dùng, phải giữ nguyên.
- Build runtime/advance runtime luôn `--jobs 8`; giữ
  `-sBINARYEN_EXTRA_PASSES=--optimize-level=1` và `BINARYEN_CORES=min(jobs,4)`.
- Không reset/clean repo. Có nhiều thay đổi từ trước; `web/` đang untracked.
- Không chạy linker thứ hai, không reload/restart game live, không kill app người
  dùng hoặc chỉnh paging. Chạy lâu/im lặng không chứng minh treo.
- UTF-8 khi đọc/ghi; JSON PowerShell có thể BOM, Python dùng `utf-8-sig`.
- Approval policy never: không truyền `sandbox_permissions`.
- Sửa production JS ở `web/` phải copy đúng sang `build/web-poc/` để browser dùng.

## Build đang chạy: kiểm tra trước khi làm gì khác

Build bắt đầu `2026-10-22T03:52:16Z`. Lần kiểm tra gần nhất:
`wasm-opt.exe` **PID168312** còn sống, CPU **14922.50 giây**, private memory
**15058853888 bytes**; CPU vẫn tăng. Asyncify đã xong4690.99giây, log đang
`(final validation)`. Chưa publish hoặc boot bản4GiB tại thời điểm kiểm tra.
Build cũ từng tốn khoảng hai giờ, không dùng làm ETA hoặc ngưỡng kill.

- Exec session6115 từng chạy `build/codex-finish-4gb-heap.ps1`; coordinator đã bị dừng
  theo yêu cầu người dùng, không tiếp tục chờ session này như bộ điều phối sống.
- PowerShell144168 đã dừng; Python launcher173924, builder172172 được giữ.
- Finalizer161616 đã kết thúc; optimizer168312 đang chạy.
- Playwright session tên `ssbu-web`; Node167376 **không còn tồn tại** ở kiểm tra bàn giao.
  Không tìm thấy Node Playwright trong inventory process hiện tại. Khi được yêu cầu
  tiếp tục, kiểm tra browser session còn dùng được trước khi boot; đừng giả định sống.
- `build/codex-user-stop-handoff.json` ghi việc dừng thủ công và process được giữ.
- State `build/codex-4gb-heap-followup.json` còn `building-4gb` từ trước khi dừng;
  **không còn được coordinator cập nhật**, không dùng riêng state này để kết luận sống.
- Server Python155412/171544 vẫn chạy `web/serve.py --port 8787` lúc bàn giao.
- Log `build/codex-runtime-4gb-heap-build.log`.
- `build/codex-4gb-build-health.json` là snapshot bổ sung, có thể cũ.

```powershell
# Chạy từ repo
Get-Content build/codex-4gb-heap-followup.json
Get-Process -Id 168312 -ErrorAction SilentlyContinue | Select-Object Id,CPU,PrivateMemorySize64
Get-Content build/codex-runtime-4gb-heap-build.log -Tail 12
```

PID biến mất thì đọc state/log/publication và tìm process liên quan, không kết
luận thất bại ngay. Warning có thể hiện `NativeCommandError`; wrapper kiểm tra
exit code thật. Không sửa C/roots/manifest/header giữa build.

Thiết kế của script coordinator trước khi bị dừng (không còn tự chạy chuỗi này):

1. Build `--fibers --compact-codegen --jobs 8 --trace-asyncify`.
2. Xác nhận root/manifest và hash ba header không đổi, SHA Wasm mới được publish.
3. Parse memory section thật: shared maximum65536pages/4GiB.
4. Node constructor306, đúng SHA publication mới.
5. Guarded boot bằng `build/codex-start-integrated.cjs` và auto-install probes.

Python build riêng vẫn có thể hoàn tất và publish; phần Node/browser của coordinator
đã bị ngăn. Những report followup dưới đây **có thể không được tạo** do dừng thủ công:
`build/codex-4gb-heap-build.json`,
`build/codex-4gb-memory-limits.json`, `build/codex-4gb-heap-node-report.json`,
`build/codex-4gb-heap-boot-start.log`. Không chờ state `browser-running` tự đến.
Không chạy lại script này trong khi process compiler/builder cũ còn sống.

## Module cũ và source mới khác nhau

Wasm đang phục vụ tại lần đọc cuối có SHA:

`2adc6457768e552013c47438b670be9e036010f1485bd50895fe5f046d6e65dc`

281989948bytes, 99134 Wasm functions, max numeric locals10812, shared maximum
32768pages/**2GiB**. Node306 pass đúng SHA tại
`build/codex-post-clone-node-report.json`. Native graph70279functions/1096units.

Source đang link nâng max lên **4GiB**, initial32MiB giữ nguyên; thêm allocator
và command telemetry. Chỉ compile lại `host_allocation_stats.c`,
`host_gpu_commands.c`, reuse phần còn lại. Không gọi bản này đã publish/boot
nếu state/report chưa chứng minh. Không boot module cũ trong lúc link chiếm RAM.

## Bằng chứng game thật và các blocker

| Run | Kết quả | Artifact |
| --- | --- | --- |
| Boot18, 2adc | Logo loading; dừng `Unbound shader texture 1/2`, bắt được shader/CB/packet lỗi | `build/codex-boot18-unbound-capture-report.json` |
| Boot19, cùng Wasm + JS sửa | Qua lỗi texture; 43761draws,958specializeddraws; staging allocation fail vượt2GiB | `build/codex-boot19-staging-allocation-report.json` |
| Boot20, cùng Wasm + diagnostics | 65999693imports,41389draws/2322presents; staging fail tại heap2147483648; sau loading đen | `build/codex-boot20-staging-allocation-report.json` |

Ảnh logo thật đã xem: `output/playwright/codex-boot18-loading-logo.png`.
Boot20 readback trước present khoảng550giây, frame1599/draw24051: window target3
và intermediate87 đều1920x1080, rgba8unorm-srgb, RGB toàn đen/alpha255.
Composite lấy87 đưa vào3. `build/codex-boot20-live-target-probe.json` chứa dữ liệu
thật: đen đã có trước canvas, **chưa xác định nguyên nhân persistent black**.

Ba quad UI đầu draw1973/1975/1981 có màu FS nhưng transform thật đưa chúng ra
bên phải clip volume: `build/codex-early-ui-clip-analysis.json`,
`build/codex-ui-early-records.json`. Có thể đầu animation; không chứng minh lỗi
matrix kéo dài. Không sửa clock/matrix theo suy đoán.
Boot20 pass probe cài muộn chưa lấy được mẫu; report
`build/codex-boot20-live-pass-probe.json` ghi unavailable.

Đã gửi A/KeyZ một lần khi focus trong boot19, không có phản hồi hình ảnh rõ.
Input thật chưa xác nhận. Nâng heap chỉ xử lý giới hạn thấy được, chưa chứng minh
4GiB đủ hoặc xác định/loại trừ memory leak.

## Thay đổi đã làm và validation

### Optional uniform branch cho texture thiếu

`web/spirv-uniform-branches.mjs`, `game-shaders.mjs`, `gpu-draw.mjs`,
`gpu-programs.mjs`: shader gốc compile bình thường. Chỉ khi thiếu texture mới đọc
CB thật (budget64KiB), chứng minh nhánh đơn giản không chạy, specialize SPIR-V.
Không gắn dummy texture. Unknown/nhánh có thể chạy vẫn fail; cache key exact words,
giới hạn256variants. Shader boot18: CB3word1=0, texture2 chỉ dùng khi khác0;
loại8 nhánh không chạy. Boot19/20 qua lỗi cũ trong game thật.

- 289 actual-shader/GPU/production checks:
  `build/codex-uniform-branches-integration-report.json`.
- 26 CPU proof checks: `web/test_spirv_uniform_branches.mjs`.
- 714 GPU checks/20draws:
  `build/codex-uniform-branches-draw-regression-report.json`.
- Bounded failure capture trong gpu-draw giữ shader/options/layout/buffer prefixes,
  budget192KiB, evict capture thành công cũ để giữ lỗi thật.

### Heap và telemetry mới đang link

- `web/host_allocation_stats.c`: `web_allocation_info`, mallinfo ABI khớp bundled
  dlmalloc; allocatedBytes/freeBytes/systemBytes/peakSystemBytes/freeChunks.
- `web/host_gpu_commands.c`: `web_gpu_command_info`, trylock không block Worker;
  list/binding/draw/render/program/sublist counts và modeledCommandBytes.
  Không đổi lifetime, GC hoặc render behavior.
- `runtime-worker.mjs`: heapBytes/allocationInfo/commandInfo trong progress/final,
  tương thích module cũ thiếu exports. `diagnosticFrames=1` expose
  `self.__ssbuDiagnosticGpu`, xóa khi shutdown.
- `app.mjs`: `allocationTimeline`, tối đa128mẫu mỗi10giây, giữ mẫu đầu và127mẫu
  gần nhất; heap/allocator/command/import/draw/present, không giữ frame capture.
  Live tại `window.__ssbuAllocationTimeline`, reset khi start, kèm cả normal/error
  terminal report. Đã `node --check` và copy sang bản phục vụ; chưa xác minh game mới.
- Small shared probe max4GiB: pointer2164332024>2GiB, native write, production GPU
  bridge upload/readback, unsigned tokens, allocator allocate/free: **7checks**,
  `build/codex-large-memory-allocator-report.json`. Không phải game proof hoặc
  4GiB resident RAM.
- NVN fixture **3295checks/2GPUdraws**, registry về0 sau finalize:
  `build/codex-command-diagnostics-report.json`.

Đã xem staging transfer/poolcopy/drawrange/submitdraw/clear/copysource; normal/error
paths được xem có free. Chưa audit mọi allocation. Cần dữ liệu allocator và retained
command metadata ở boot mới; không thêm GC theo tuổi hoặc giảm staging suy đoán.

Giữ các renderer fixes trước: logical compressed sampling đúng mip/layer và
HDR/snorm/sRGB; SPIR-V swizzle; reflected texture dimensions/2DArray selected slice;
ordinary D24S8 sampling qua r32float; bỏ depth state khi không attachment;
negative minLOD hợp lệ clamp0; ViewportIndex/Layer chỉ lower khi chứng minh0;
vertex formats/provoking vertex/strip restart/8bitindices/basevertex/firstinstance.
Shader game dùng Maxwell thật, synthetic WGSL chỉ fixture.

Refill/callback/input đã nằm trong2adc. Shared callback family và clone roots thật
đã vào graph70279. Không bật `--all-pointer-tables` rộng. Root mới chỉ mở rộng
theo target/caller thật quan sát được.

### Kết quả phân tích thêm trong lúc chờ

`build/codex-boot20-memory-trend-analysis.json` tổng hợp bốn report thật. Đoạn
352001..486000ms: GPU buffer liveBytes239562752 và234textures giữ nguyên, heap
capacity tăng302252032bytes, program snapshots tăng11839 qua762presents.
Heap capacity không phải allocated live bytes: chưa được kết luận leak từ số này.

Native `runtime/nvn_gl.c:list_collect` có heuristic hủy handle không dùng120frames;
web giữ registry owner đến FinalizeCommandHandle. **Không copy heuristic tuổi**:
chưa có bằng chứng lifetime hợp pháp. Boot4GiB cần đo registry/allocator để xác định
giữ command metadata hay nguyên nhân khác. Không sửa C/lifetime trong lúc link.

## Theo dõi boot mới và giữ report

Đọc Playwright skill trước khi browser automation trong phiên mới:
`C:\Users\Hikari\.codex\skills\playwright\SKILL.md`.

```powershell
npx.cmd --yes --package @playwright/cli playwright-cli -s=ssbu-web run-code --filename build/codex-poll-compact.cjs
```

CLI exit0 vẫn có thể chứa `### Error`: đọc body. Không gọi start nếu game live.
`codex-start-integrated.cjs` guard `__ssbuRuntimeDone===false`, dùng
`/?runtime=fibers-compact&diagnosticFrames=1`, chọn dữ liệu thật rồi click
“Thử boot game”, cài target/pass probes với retry. Bản plain ở
`build/codex-start-integrated-plain.cjs`. ELF:
`build/native/binaries/13.0.4/main.elf`; ARC:
`build/dump-13.0.4/romfs/data.arc`; exact NRR list xem harness.

- `codex-install-live-target-probe.cjs`: shader/options thật, readback trước present
  sau specialized draw đầu; perform16 trong serialized request, không gọi
  backend.request lồng vào queue.
- `codex-install-live-pass-probe.cjs`: target87, tối đa8modulepairs pha đầu và8pairs
  sau120presentedframes,32KiB/pair; POST ngay mỗi capture, metadata SHA, dưới1MiB.
  Idempotent, lỗi lưu không stop game.
- **Archive `build/web-poc/browser-report.json` trước mọi helper ghi `/__report`**,
  chỉ nhận khi `results.livePassProbe` tồn tại và SHA đúng publication mới.
  File có thể còn fixture cũ; không gắn nhãn nhầm.
- `codex-read-live-target-probe.cjs`: lưu/print target+heap+allocationInfo+commandInfo;
  chạy sau archive pass.
- `codex-follow-visible.cjs`: chờ tối đa50giây, compact telemetry, ảnh live
  `output/playwright/codex-latest-game.png`; xem ảnh trước kết luận.
- `codex-save-live-progress.cjs` cũng ghi đè browser-report, archive trước.
- `codex-read-live-pass-probe.cjs` cần Worker sống; auto-post tránh mất mẫu khi end.
- `build/codex-durable-pass-probe-smoke.json`:16mockcaptures/posts, hai pha,
  gate/dedupe/nonfatalHTTP500/idempotency. Không phải GPU/game proof.

Browser globals: `__ssbuRuntimeDone`, `__ssbuRuntime`, `__ssbuRuntimeProgress`,
`__ssbuRuntimeError`. **Không Worker.evaluate trên Worker đã đóng** (đã từng treo).
Kiểm tra done trước. Cleanup unconfigure canvas, ảnh đen sau terminal không chứng
minh game frame cuối đen.

`/__runtime_report` ghi `build/web-poc/runtime-browser-report.json` khi terminal;
`/__report` ghi `build/web-poc/browser-report.json`. Body cần object `results`,
cap1MiB; kiểm tra response.ok. Archive đúng SHA trước chạy fixtures.

Input: Z/X/C/V=A/B/X/Y; Q/E=L/R; A/S=ZL/ZR; Enter=Plus; Backspace=Minus;
IJKL=Dpad; arrows=leftstick. Canvas/document focus và page visible; blur neutral.
Không có default timeout15phút, có Stop.

## Việc cần làm tiếp

Các bước sau chỉ thực hiện khi người dùng giao AI tiếp quản/tiếp tục:

1. Đọc `build/codex-user-stop-handoff.json`, kiểm tra process/log/publication thật;
   preserve optimizer nếu sống và CPU tiến. Coordinator đã dừng, state cũ có thể stale.
2. Khi builder hoàn thành, đối chiếu SHA mới và memory65536pages rồi chạy Node306
   thủ công theo lệnh dưới đây. Chỉ boot khi kiểm tra xong, browser session đã sẵn sàng.
   Nếu failed, chẩn đoán đúng bước đó trước retry; không relink vì thiếu report followup.
3. Khi game live, archive pass report có SHA mới; thu hai pha target87, readback,
   allocator/command counters và screenshot.
4. So sánh heap/allocated/free/system với registry/draw/present/guest progress;
   phân biệt retained allocations, fragmentation và tăng trưởng hợp lệ bằng dữ liệu.
5. Nếu đen, tìm pass đầu mất nội dung từ shader/CB/vertex/index/viewport/scissor/
   attachments thật qua nhiều thời điểm. Không ép màu, dummy texture, sửa matrix
   hoặc bỏ game logic để giả menu chạy.
6. Lỗi mới phải archive report/console đúng SHA, sửa có bằng chứng, regression đúng
   phạm vi rồi mới build/boot tiếp. Chỉ thêm callback quan sát được.
7. Chỉ báo hoàn thành khi menu thật, input có tác dụng và trận đấu chơi được.

Lệnh kiểm tra thủ công sau khi compiler/builder đã hoàn tất và người dùng yêu cầu
tiếp tục (chưa chạy các lệnh này cho bản4GiB tại thời điểm bàn giao):

```powershell
python web/check_memory_limit.py build/web-poc/runtime-fibers-compact.wasm --pages 65536 --report build/codex-4gb-memory-limits.json
node web/test_runtime_node.mjs fibers-compact
```

Đọc `build/web-poc/runtime-fibers-compact-build.json` và
`build/web-poc/runtime-fibers-compact-node-report.json`: SHA phải khớp nhau, khác
2adc cũ, `results.runtime.constructorsCompleted` phải306. Archive metadata/report
ra các tên4GiB nếu hợp lệ. Xem `codex-finish-4gb-heap.ps1` làm tài liệu các guard;
không chạy nguyên script nếu chỉ cần validate một publication đã hoàn tất, vì nó
sẽ build lại trước. Browser session đã mất process cũ nên phải kiểm tra/mở lại theo
skill trước khi dùng harness; harness tự cài probes nhưng không thay thế Node checks.

## Tài liệu liên quan

- `web/HANDOFF.md`: bản canonical cùng nội dung artifact `handoff.md` ở gốc repo.
- `web/HANDOFF_HISTORY_2026-10-22.md`: nguyên bản handoff trước khi làm gọn.
- `web/STATUS.md`: lịch sử chi tiết, có nhiều PID/live notes đã cũ.
- `build/codex-finish-4gb-heap.ps1`: tài liệu chuỗi build/validate/boot; coordinator đã dừng.
- `build/codex-user-stop-handoff.json`: mốc dừng thủ công và các process được giữ.
- `build/codex-boot20-staging-allocation-report.json`: terminal thật trước build4GiB.
- `build/codex-boot20-live-target-probe.json`: target87/window3 đen trước present.
- `build/codex-unbound-failed-draw.json`, `build/codex-unbound-stage0.glsl`,
  `build/codex-unbound-stage1.glsl`: shader gây lỗi texture cũ.
- `web/check_memory_limit.py`: kiểm tra actual Wasm section, không chỉ build flag.

Ưu tiên report/process/publication mới nhất hơn mọi snapshot trong tài liệu.
