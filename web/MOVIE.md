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

# Browser movie adapter

The currently running game Wasm includes the movie adapter (SHA
`5c88f7a92ccc717481213c2de4b282344f7470890828efb231142fcbcda32e57`). Source routes
Buffer, MediaData, SDK Extractor and Decoder virtual calls through `host_os.c`.
The full build and runtime worker include the real WebM/WebCodecs bridge.
The production SDK path has passed real-video browser fixtures and actual game
video playback through natural EOF/EOS. The title is still blocked by its audio
clock. The production backend and SDK Vorbis path are now wired in source and
have passed an interleaved real-media fixture. The AV game build is running;
the currently loaded 5c88 game does not include that change yet.

The AV build started at 2026-10-25T16:05:41Z, parent PID25188, observed wasm-opt
PID63736, code commit `dd968d5`, with `--fibers --compact-codegen --jobs 8`.
State/logs are `build/codex-av-dd968d5-build-state.json` and
`build/codex-av-dd968d5-build{,.err}.log`. Revalidate the live process before
restarting; unchanged logs are not evidence of termination.

## Actual game result after the integrated build

The build finished at 2026-10-25T13:49:08Z with 100259 functions, 282924589 bytes
and maximum 10812 locals. The game booted with 306 constructors and 33 workers.
Its actual movie is hash40 `0x1dcc80344b`, 108813844 bytes at data.arc offset
1689801320, duration 129.799 seconds, VP9 1280x720 plus two Vorbis stereo tracks.
This differs from the earlier 419-second opening candidate; the new capture
identifies the movie actually used by this boot.

The real game submitted 3890 compressed video packets, acquired/released 3891
outputs including exactly one natural EOS, and copied 1945 decoded frames.
The remaining frames were consumed/released by the game's playback logic.
No video input, frame, output lease or reservation remained after EOS. The
movie player now survives; the old failing placeholder had deleted it.
The canvas remains black, and real A/Plus presses have not changed the scene.

The outer title sequence remains in state2 at `0x80545a3900`. Its readiness
method `f_71036e26c0` checks the global movie manager, then returns the audio
clock when an audio object exists. Actual audio object `0x80487f5480 + 264`
is zero, while actual video object `0x8048845680 + 168` is 129733000 us.
Thus completing video alone does not satisfy this game's title gate. Evidence:
`build/codex-movie-title-gate.json`, `codex-movie-clock.json`, and
`codex-movie-after-a.json`. Do not force that clock, pointer or readiness result.

A separate read-only probe on the active movie's real audio track confirmed
browser AudioDecoder support for `vorbis`, with the actual 4344-byte Xiph
CodecPrivate, 48000 Hz and two channels. It decoded 256 packets to 255 real
interleaved s16 PCM blocks; 163 contained nonzero audio. The first Vorbis packet
primes the decoder and produces no PCM. AudioData.copyTo supports conversion
from the browser's f32-planar output to s16. The first nonzero block is at
1953333 us, hash `ea5a6b25`; last is 5222666 us, hash `6f85525a`.
Report: `build/codex-movie-audio-decode-256.json`. This is not SDK audio support
yet; the next implementation must carry these real samples through the SDK.

## Implemented and checked

`movie-audio.mjs` validates the actual Xiph/Vorbis CodecPrivate headers and
uses WebCodecs AudioDecoder with those headers, sample rate and channel count.
It copies AudioData to actual interleaved s16 PCM, closes every decoded object,
and validates output format/dimensions. Output reservations are kept through
asynchronous decode and client leases, capped at 64. The first Vorbis packet
primes the overlap state without producing PCM; subsequent packets reserve
their real output before decode. Failed decode, format changes, incomplete
drain and invalid ownership are reported rather than inventing samples.

The production adapter's first 255 PCM hashes/timestamps match the independent
raw AudioDecoder probe. The second actual Vorbis track passed 6000 packets,
5999 outputs, 5908 with nonzero audio, with its 64-slot queue repeatedly full
and no reservations after draining. Last decoded timestamp was 105174666 us.
Reports: `build/codex-movie-audio-adapter-256.json` and
`build/codex-movie-audio-adapter-track3-6000.json`. These remain isolated browser
fixtures. MovieBackend multi-track selection and SDK type5/format1 are now wired.

The SDK AV fixture selects video index0 and audio index2 from a three-track
finite excerpt of the actual opening. Original track numbers, codec headers
and compressed bytes are preserved. The C driver goes through production OS
import/indirect routing, guest allocators, SDK Extractor and both Decoders.
It passed 120 video outputs and 202 PCM outputs from 203 Vorbis packets (one
priming packet), with all bytes/hashes/timestamps matching direct decoding.
Both 48 video and 64 audio leases fill; both decoders receive one natural EOS,
and teardown leaves zero handles, completions and guest allocations/objects.
There are 367279 nonzero PCM bytes in this fixture. Report:
`build/codex-movie-sdk-av-120.json`. This remains a fixture, not a game result.

Opening all three tracks of the complete 108813844-byte actual movie also
passes metadata/maximum-packet scanning: video283126 bytes, audio719/716.
Selecting indices0+2 produces the actual interleaved pending sample sequence,
with no fabricated EOF. Report: `build/codex-av-full-source.json`.

`movie-webm.mjs` reads real WebM tracks and compressed blocks from a `Blob`,
`Uint8Array`, or bounded asynchronous reader `{size, read(offset, length)}`.
Reads are cached in 64 KiB windows; it does not require copying a whole movie.
It supports signed block timestamps, SimpleBlock, BlockGroup, Xiph/fixed/EBML
lacing, and unknown-size Segment/Cluster boundaries. Metadata, block sizes,
integer ranges, parent bounds, and returned reader lengths are checked.

`movie-video.mjs` uses actual WebCodecs VP8/VP9 decoding and copies decoded
I420/NV12 frames into the SDK's tightly packed NV12 layout. The decoder has a
bounded output queue, reports unsupported formats/errors, and closes frames
after copying. It does not synthesize video frames or end-of-stream events.

Output capacity is now reserved before decode and remains occupied through
the caller's output lease. WebCodecs `decodeQueueSize` alone is insufficient:
it can reach zero before delayed output callbacks. VP8/VP9 header parsing
counts visible frames, including hidden VP9 reference frames, show-existing
frames and indexed superframes. VP9 superframes are split into individual
coded frames before WebCodecs submission. A real browser test found that
submitting three coded frames with two visible outputs as one chunk produced
only the final image; splitting preserves both original frame hashes.

`host_movie.c` implements Buffer's actual SDK layout and common MediaData
imports. Owned data uses registered guest allocator callbacks; callback context
is preserved and no registry lock is held across guest callbacks. Allocations
retain their original free callback/user pair when the allocator changes.
Owning Buffer copies and metadata strings are deep-copied. Strings returned to
guest code point to real guest storage; host registry tokens are opaque values.
Metadata is limited to forty entries, keys to 39 bytes, strings to 4096 bytes,
and individual owned allocations to 256 MiB. Reset requires stopped workers.
The full runtime build includes this adapter, and `host_os.c` routes its value
imports before the SDK Extractor/Decoder adapter. This source change
has not been published as a new game Wasm build.

`movie-backend.mjs` implements serialized extractor/decoder operations for that
bridge. Source bytes become a Blob; metadata and packet sizes/seek points
are read from the actual container. Peek/read/advance preserve the current
sample across all selected tracks without rewinding on selection changes.
Video seeking starts at a real preceding keyframe; Vorbis-only seeking includes
one actual overlap packet. Decoded outputs remain owned until release, including
after source closure. Unsupported codecs are reported explicitly. Notifications reach a fixture
callback and the production SDK adapter's registered OS event objects.

`CONFIGURE time=8` enables the SDK's eight indexed input slots. Submitted
packets are copied into a bounded queue and the slot is returned by the new
`INPUT_AVAILABLE` bitmask operation only after the codec accepts its data.
`STATUS` reports asynchronous decoder failures. Output capacity is 48 including
held leases. Releasing an output pumps pending inputs. Queued EOS starts its
codec drain outside the serialized request chain, allowing output releases
while inputs wait for capacity. EOS is still accepted only at actual extractor
EOF and emitted only after decoding finishes and visible outputs are acquired.
SDK Stop/Flush must use `RESET`, rather than the direct probe's codec drain.
Reset drops pending input/leases; old decoder callbacks/drains cannot notify
the replacement decoder. `CONFIGURE time=0` retains the direct bridge protocol.

The production JS backend passed 240 unchanged real VP9 packets in a finite
test container: eight inputs waiting, 48 held outputs, 240 decoded frames,
one natural EOS, and zero handles/reservations after cleanup. All 240 hashes
and timestamps match direct decoding of the original file. Direct decoding
also filled its 16-frame capacity and resumed under backpressure. Reports:
`build/codex-movie-direct-240.json` and `build/codex-movie-queue-240.json`.
These remain separate fixtures; game behavior must be verified by a new boot.

`host_movie_bridge.c/.h` transfers bounded staging buffers from a C pthread to
the JavaScript worker, then completes the waiting pthread with an SDK status.
Every dispatched success or failure completes its wait token. Fixed reply
headers are validated before consuming outputs. This adapter uses
`MAIN_THREAD_ASYNC_EM_ASM`, so compile this translation unit with `gnu11`, as
with `host_gpu_bridge.c`. It is included in the full runtime source list.

`host_movie_codec.c` implements SDK Extractor constructors and virtual methods:
client reader/path/memory sources, track configuration/selection, sample
peek/read/advance/time/metadata, seek and buffered range, and both destructor
forms. ClientStreamReader calls use real guest scratch storage and preserve
the full guest context. Partial reads are accumulated; handles and scratch are
released before metadata parsing. Metadata links carry the backend extractor
handle and track index for the upcoming Decoder configuration.

Instances serialize operations with a gate, and destruction removes the object
from lookup before waiting for in-flight references. This lifetime mechanism
still needs a concurrent SDK destruction test. Deleting destructors use the
same version-pinned guest delete function as the native adapter; movie buffers
and wrapper objects have different allocators.

`host_movie_decoder.c` implements the Decoder constructor and vtable slots
0..18: configuration from real track metadata, event registration, NV12 format,
Start/Stop/Reset/Flush, input acquisition/buffers/submission, output acquisition,
copy/format/release, errors, and both destructor forms. Eight actual guest
allocations hold input bytes; borrowed Buffer objects cannot free those slots.
Output slots map the backend's handles to SDK indices 0..47. Caller capacity,
slot ownership, byte ranges, unsupported codec/format and EOS are checked.

JS notifications use monotonically assigned owner IDs. The notification mutex
is never held across a JS request or guest callback, and notifications do not
take the Decoder gate. Registered event keys are copied while guest memory is
bound; JS callbacks only signal existing HostEvents through
`web_os_try_signal_event`, without guest memory access or implicit creation.
Unregister/destruction waits for active signals through this mutex; old owner
IDs cannot signal a later decoder's events. SDK destruction removes the object
before waiting for in-flight call references, then closes the backend and frees
its guest inputs. Full reset requires stopped workers.

The SDK Decoder fixture passed 240 real packet/frame hashes and timestamps
against direct decode, including 48 held outputs, actual guest input buffers,
SDK Extractor reads directly into those buffers, real OS event notification,
late event registration, output metadata/capacity checks, single actual EOS,
Flush/Start/Stop and zero live handles/allocations on teardown. A deterministic
two-thread test delays a real STATUS bridge request until the destructor has
unlisted its object, proving the pending call can finish before destruction
frees its implementation. Report: `build/codex-movie-sdk-decoder-240.json`.
This is a finite test container with unchanged real compressed payloads,
not a shortened game movie or proof that the game reaches its menu.

Validation on 25 October 2026:

- Unit checks: lacing byte layouts, timestamps, BlockGroup references, unknown
  cluster boundaries, truncation, bad tracks, short reads, odd-size NV12 planes.
- All 128 WebM stream entries in the local 13.0.4 archive: metadata and first
  video sample parsed; all use VP9, with Vorbis audio and no VP9 CodecPrivate.
- One complete 90.670 s file: 5,440 video samples read to actual EOF, from
  3,000 to 90,653,000 microseconds.
- Real Chrome: twelve frames from that file decoded, with sample/output
  timestamps equal; each 1280Ã—720 NV12 frame contains 1,382,400 bytes.
- Real Chrome: twelve frames also decoded from the 419.855 s, three-track
  opening candidate, archive hash40 `0x20e266a6d8`. This matches the duration
  described in the native adapter, but its role in this boot is not proven by
  a captured file-open path. Media SHA256:
  `0fcd53c42de14577b81f8b9934a332da811b7e52f2ea3d772192d9a40d312976`.
- Actual production C imports in a small Wasm fixture: 2,820 checks of raw SDK
  offsets, numeric metadata conversions, guest string lifetimes, owning and
  borrowed copies, assignment/swap/destruction, allocation failure, allocator
  replacement, callback reentry and context preservation, with two concurrent
  Wasm pthreads. All guest allocations/metadata are released at the end.
- OS routing fixture: 182 checks pass, including raw Buffer offsets and actual
  MediaData calls routed through `web_os_import`. Strict runtime C11 syntax
  check of `host_movie.c` passes with `-Wall -Wextra`.
- Real C pthread bridge: twelve frames of the 419.855 s opening candidate
  cross C â†’ JS demux/decoder â†’ C NV12 output. All twelve timestamps and hashes
  match direct browser decoding. 382 checks pass, with zero live handles or
  pending completions. An early EOS request is rejected; no EOS is delivered.
- Actual finite test container: a test-only muxer wraps twelve unchanged real
  VP9 packets in a new WebM container. The C bridge reads that container to its
  real EOF and receives exactly one EOS after twelve frames (139 checks pass).
  This is a protocol fixture, never a shortened game input or a game skip.
- Backend checks cover serialization, selection, keyframe seeking, EOF, handle
  lifetime, and error completion without modifying invalid staging buffers.
- Production SDK Extractor: an actual 80,762,819-byte movie is read through 467
  ClientStreamReader callbacks into guest memory, including partial reads.
  Twelve resulting VP9 packets decode to exactly the same timestamps and NV12
  hashes as direct decoding. The fixture validates metadata links, callback
  context, short/repeated buffer-read failures, and destructor ownership, and
  ends with zero live extractor/value allocations and zero backend handles.
- SDK Extractor finite-container fixture: actual EOF and one EOS after twelve
  unchanged real packets, plus sample metadata, buffered range and SDK seek to
  zero. Decoder calls still use the bridge protocol directly, not the pending
  SDK Decoder implementation. Path/memory sources and nonzero SDK seek need
  additional fixture coverage before claiming all Extractor methods verified.

Local evidence is under ignored `build/`: `codex-movie-webm-metadata.json`,
`codex-real-movie-browser-probe.json`, and
`codex-opening-candidate-browser-decode.json`. No media is committed.
Bridge evidence: `codex-movie-bridge-opening.json` and
`codex-movie-bridge-excerpt-eof.json`.
SDK Extractor evidence: `codex-movie-sdk-extractor-real.json` and
`codex-movie-sdk-extractor-eof.json`.

## Run the checks

```powershell
node web/test_movie_webm.mjs
node web/test_movie_audio.mjs
python web/test_movie_audio.py --movie C:/path/to/local.webm --track 2 --packets 256
node web/test_movie_video.mjs
node web/test_movie_backend_queue.mjs
python web/test_movie_video.py --movie C:/path/to/local.webm
python web/test_movie_video.py --movie C:/path/to/local.webm --frames 240
python web/test_movie_video.py --movie C:/path/to/local.webm --frames 240 --queued-excerpt
python web/build_movie_values_probe.py
python web/build_os_probe.py
node web/test_movie_backend.mjs
python web/build_movie_bridge_probe.py
python web/test_movie_video.py --movie C:/path/to/local.webm --bridge
python web/test_movie_video.py --movie C:/path/to/local.webm --bridge --excerpt-eof
python web/build_movie_bridge_probe.py --sdk-extractor
python web/test_movie_video.py --movie C:/path/to/local.webm --sdk-extractor
python web/test_movie_video.py --movie C:/path/to/local.webm --sdk-extractor --excerpt-eof
python web/build_movie_bridge_probe.py --sdk-decoder
python web/test_movie_video.py --movie C:/path/to/local.webm --sdk-decoder --excerpt-eof --frames 240
python web/build_movie_bridge_probe.py --sdk-av
python web/test_movie_av.py --movie C:/path/to/local.webm --video-packets 120
node web/test_movie_multitrack.mjs
```

The browser check uses an existing Playwright CLI session `ssbu-web` and the
existing server on port 8787. It opens and closes a temporary page, selects the
local file with a file input, and keeps the game page intact. It publishes only
the movie JavaScript modules into `build/web-poc`; it does not rebuild, boot,
or reload the game. Override `--session` or `--port` as needed. For a movie
inside an archive, pass `--offset` and `--length`. The report hashes that exact
byte range, validates decoded frames and timestamps, and saves under
`build/movie-video-probe/`.

`--frames` defaults to twelve packets and supports up to 3000. Direct probes
also regroup the first two VP9 packets' unchanged coded frames into a legal
superframe and compare its output hashes against the original frames.
`--queued-excerpt` uses a finite test container of the selected packet count,
exercises the production JS backend's SDK queue protocol, and saves under
`build/movie-queue-probe/`. Counts of at least 64 must fill all eight queued
inputs and 48 output leases. This container is never used by the game.

The bridge mode uses a temporary worker and C pthread fixture. It publishes
only fixture files under `build/web-poc/movie-bridge-probe` plus the new movie
JavaScript modules, preserving the game's Wasm and worker script. Reports go
to `build/movie-bridge-probe/`. `--excerpt-eof` constructs its finite test
container in memory and records its separate hash. It does not change the
source file. The worker terminates its pthread pool before returning results.
The SDK mode publishes its fixture under `movie-sdk-extractor-probe` and saves
reports under `build/movie-sdk-extractor-probe/`. It exercises actual SDK
Extractor virtual calls and guest-memory callbacks; its decoder still uses
the previously verified bridge directly. The original game remains intact.

## Integration remaining

1. Follow the current AV full build through publication, verify receipt/hash,
   then boot the real game and confirm the audio clock advances naturally.
   Check the title readiness transition, visual output and effective menu input.
2. Verify nonzero seek followed by SDK Flush/Start on real media. Backend seek
   and reset have separate tests; their combined game sequence is unproven.
   The SDK Extractor's concurrent destruction remains untested.
3. Implement or explicitly propagate unsupported audio codecs through the
   game's real track-selection behavior. Vorbis audio, raw H.264, MP4 and VP9
   CodecPrivate configurations are not implemented by these components.
   Laced tracks without container-provided duration are explicitly rejected;
   deriving audio packet durations remains codec-specific work.
4. Boot the complete game and verify the title readiness gate passes, menu
   input works, and an actual match runs. Current decoder fixtures prove none
   of these game outcomes.

The observed gate is in `f_7101865010`, before clearing title task flag112.
Its movie-player pointer chain ends at null after cleanup, while the outer
sequence keeps updating in state2. See the newest HANDOFF/STATUS entry for
the exact live objects and captures. Do not force this pointer, animation,
matrix, scene state or EOS to manufacture progress.

Format/API references: [WebM container](https://www.webmproject.org/docs/container/),
[Matroska blocks and lacing](https://www.matroska.org/technical/notes.html),
[WebCodecs](https://www.w3.org/TR/webcodecs/).
