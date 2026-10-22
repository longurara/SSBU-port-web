# Tài Liệu Bàn Giao (AI Handoff): SSBU 13.0.4 WebAssembly / WebGPU Port (`ssbu-recomp`)

> **4GiB OPTIMIZER LIVE168312**, CPU2213.61sec/private5.82GB at04:07UTC.
> Preserve exec6115 /builder172172 /primary144168. Finalizer161616 is terminal;
> no publication/new boot yet, browser idle. Start harness now auto-installs
> target/pass probes after guarded new boot; active-run guard retained/mock-tested.
> Pass samples first and120frames later, saves immediately to browser-report.json;
> archive before another /__report write. Early UI quads are offscreen by actual
> captured transforms; later persistent black cause unknown. Newest STATUS details
> exact evidence/probe changes. No C/header changes since this linker started.


> **4GiB LINK LIVE, NOT YET PUBLISHED.** Exec6115 /primary144168 /builder172172
> (launcher173924), finalizer161616 verified CPU129.86sec. Preserve; no second link.
> State codex-4gb-heap-followup.json/log codex-runtime-4gb-heap-build.log. Browser
> ssbu-web idle `/` Node167376; served2adc is OLD2GiB. Only2 C units rebuilt;
> protected headers unchanged.7 high-pointer GPU/allocator checks +3295NVN checks
> pass. Script verifies actual65536-page limit/Node306 then boots. Boot20 archived
> at2GiB OOM; real readback shows intermediate87 already black before presentation.
> Black/menu/input/match unresolved. Install ready live probes EARLY after new
> boot; latest STATUS explains artifacts, lifecycle and exact next actions.


> **BOOT19 PASSED OLD TEXTURE ERROR, THEN2GiB OOM; BOOT20 LIVE.** Actual game958
> specialized draws,43761total; archive codex-boot19-staging-allocation-report.json.
> Console confirms2147483648-byte heap limit. Black after loading remains unresolved.
> No C/max-memory change/link yet. Boot20 at03:30UTC same2adc/new Worker diagnostics:
> opt-in self.__ssbuDiagnosticGpu and wasmHeapBytes progress. Preserve ssbu-web
> Node173116; inspect live intermediate targets/heap before terminal. No menu/input/
> match verified. Newest STATUS supersedes all older boot19-live notes below.


> **BOOT19 STARTED WITH PROVED OPTIONAL-BRANCH FIX.** Same2adc Wasm/new JS.
> Boot18 full capture archived:codex-boot18-unbound-capture-report.json.
> Actual shader texture2 is guarded by CB3word1!=0; actual word is0.
> Conservative SPIR-V proof removes8 untaken arms, no replacement texture.
> 289 actual-shader/GPU/production-path checks +714 draw regression checks pass.
> Boot19 at03:13UTC; preserve ssbu-web Node118640, poll only. No linker.
> Passing old stop and menu/input/match unverified. Newest STATUS has details.


> **BOOT17 ENDED / BOOT18 DIAGNOSTIC RUN STARTED.** Same served2adc Wasm.
> Error `Unbound shader texture 1/2`,13979draws/798presents/293textures;
> archive codex-shared-family-unbound-texture-report.json. Missing failing
> shader from first12 captures; added bounded failure capture and boot18 at
> 02:55UTC. Preserve ssbu-web, poll only. No linker. Menu/input/match unverified.
> Latest STATUS supersedes older boot17-live notes below.


> **SHARED-FAMILY BUILD DONE / BOOT17 CONFIRMED LIVE.** Served SHA2adc6457...
> Node306/306 exact SHA; exec13599 finished successfully. No linker pending.
> Preserve ssbu-web run; poll without reload/start. Native graph70279/1096 units,
> Wasm99134 functions/maxlocals10812. New image/menu/input/match and passing the
> old shared-callback stop remain unverified. Latest STATUS supersedes all older
> live-link, high-memory-validation and served-f2d notes below.

> **SHARED-FAMILY ASYNCIFY DONE (4636.45s), VALIDATION LIVE.** Optimizer150676 /
> exec13599 still running with high memory pressure (~19GB private,16GB RAM).
> Preserve it; do not restart or boot old f2d5058 Wasm. Browser is idle. CPU
> advances; publication/Node/new boot are still pending. See newest STATUS.

> **BOOT16 ENDED / NEW SHARED-CALLBACK BUILD LIVE.** Missing71017f3010 at
> actual caller7103725c30 (reported fn stale);39.2M imports/17741 draws. Full
> archive codex-post-clone-callback-report.json. Observed target shared by107
> bounded tables; existing observed discovery yields graph70279/1096 units.
> Finalizer135076 finished; **optimizer150676 is live** (health JSON updated).
> New exec13599, primary119964, builder31884 (launcher145536) alive after stderr
> warning; corrected wrapper works. Log/state codex-runtime-post-clone-build.log /
> codex-post-clone-followup.json. Preserve link; no second linker. Browser idle `/`;
> served f2d5058 is OLD relative to this family. Node/boot run after successful link.
> Older boot16-live and previous-link notes below are history. Menu/input/match
> remain unverified. See newest STATUS for exact evidence and guardrails.

> **CALLBACK BUILD DONE / BOOT16 CONFIRMED LIVE.** Served SHA f2d505827159c1c4...
> Node306/306 exact SHA. Recovery exec30476 completed publication/boot; no linker.
> Primary27080 failed due PowerShell treating native warning as error, not a link
> failure. Recovery state browser-running is authoritative. Preserve Chrome;
> poll without reload. Do not rerun either continuation. Menu/input/match and
> passing old7102363140 failure remain unverified. Newest STATUS supersedes all
> older live-link/served966 notes below. Source native-stderr wrapper fixed/tested
> for future reuse; current boot was not restarted.

> **LINK STILL LIVE: wasm-opt148724**, finalizer104356 finished. Primary119308/
> exec27080 and recovery101536/exec30476 remain alive. Preserve them; do not
> start another linker. Served966 Wasm is old. Browser idle `/`. Newest STATUS
> and build-health JSON record current PIDs; older finalizer/live-boot notes
> below are history. Completion still requires menu/input/playability.

> **BOOT15 ENDED / CALLBACK LINK LIVE.** Visible Smash loading animation was
> verified, then missing7102363140 stopped after14417 draws/823 presents. Full
> archive codex-loading-callback-report.json. Observed vtable group added:
> graph62442; only2 C units recompiled. Exec27080 primary119308/emcc92808/
> finalizer104356 live; preserve link. Builder154480 is missing, so recovery
> watcher exec30476/pwsh101536 waits existing emcc+primary and can validate/
> publish that existing result without relinking. Check BOTH followup/recovery
> state JSON files (newest STATUS). Browser idle `/`; do not boot old966 Wasm.
> Menu/input/match remain unverified. Older live-boot15 notes are history.

> **BOOT15 LIVE: FIRST VISIBLE GAME LOADING LOGO VERIFIED.** Actual canvas
> screenshot output/playwright/codex-first-loading-logo.png shows Smash logo/
> loading arc. Source frame180:7997 nonblack pixels. Do not reload. Continue
> observing via codex-follow-visible.cjs; menu/input/match still unverified.
> Source+progress evidence is in newest STATUS; no linker running.

> **BOOT15 STARTED**, same9668733f... Wasm/new array-view JS. Boot14 passed
> swizzle/logical compressed snapshot, then stopped R8 atlas2D vs shader2DArray.
> Archive codex-refill-array-view-report.json. Shader dimensionality reflection
> now drives a one-layer array view;649 GPU draw checks pass. Preserve active
> tab, poll without reload. No C/linker. Game/menu/input remain unverified.

> **BOOT14 LIVE**, same9668733f... Wasm/new shader swizzle JS. Boot13 archived
> codex-refill-swizzle-report.json:835 draws/167 presents, failed on (1,1,1,R).
> Swizzle lowering:1430 GPU checks,648 draw checks,20 actual shader compilations.
> Preserve tab, poll without reload. No linker/C change. Game pixels/menu/input
> remain unverified. Newest STATUS section supersedes old live notes below.

> **Latest: BOOT13 CONFIRMED LIVE**, same runtime9668733f... . Boot12 ended at
> compressed logical texture bounds after passing the sampler fix. Full archive
> `codex-refill-compressed-bounds-report.json`. JS now decodes padded compressed
> storage to exact logical RGBA32float mip/layer extents:864 sampling checks and
> 518 draw checks/17 fixture draws pass. No C/relink. Preserve current browser,
> poll compactly, do not reload. Visible game/input remain unverified. Older
> boot12/boot11 live notes below are history; see newest STATUS section.

> Cập nhật tiếp nối 22/10/2026: xem phần đầu `web/STATUS.md`.
> **BOOT12 ĐANG CHẠY**, cùng Wasm9668733f... với JS sampler mới. Boot11 kết thúc
> tại sampler LOD[-1000,1000]; full report `codex-refill-first-boot-report.json`
> giữ9 capture draw thật. Đã sửa cận dưới LOD âm, **212 GPU pixel checks** qua;
> cận trên âm/reversed/nonfinite vẫn fail rõ ràng. Không cần link C lại.
> Giữ tab boot12, poll compact, không reload. Không có linker đang chạy.
> 14 shader variants thực đã replay sang GLSL/SPV tại build/codex-game-draw-inputs/.
> Vẫn chưa xác minh callback bằng counter, hình/menu hoặc input trong game.
> Các dòng boot11 đang chạy bên dưới là lịch sử; boot11 đã archive trước boot12.
> **REFILL BUILD ĐÃ XONG VÀ BOOT11 ĐANG CHẠY**: SHA
> `9668733fd5cc2e1b75ec0109f6e688d638884b022dc75fd9a05d00d27a411830`;
> Node306/306 đúng SHA. Bao gồm C input + command/control refill. Exec80349,
> builder162384 và wasm-opt152332 đã kết thúc thành công; không còn linker chờ.
> Chrome `ssbu-web` đã boot với diagnosticFrames=1; **không reload/start lại**.
> Poll compact. Nếu lỗi, archive full runtime-browser-report.json và phân tích
> drawDiagnostics/frameDiagnostics. Chưa xác minh callback trong game, hình/menu/
> input. Các dòng “đang link refill” dưới đây đã trở thành lịch sử.
> **INPUT BUILD ĐÃ XONG**, SHA `3cf87b20c979dec0f666c87d7d8fc2f8d90d75c6c4ab96e7f37bfb2925041184`;
> Node306/306 và export web_input_pointer đã xác minh. Báo cáo input-build,
> input-node-report, input-export-report trong build/. Chưa boot game bản này.
> **Đang LINK REFILL**: builder162384/launcher167992, exec80349/pwsh96676 tự nối
> tiếp; **wasm-opt mới152332** đã xong pass Asyncify3.623,06 giây, đang validating.
> Giữ nguyên khi log im lặng; chưa xuất bản/kiểm tra constructors/boot bản refill.
> Log `codex-runtime-refill-build.log`. PID160196 cũ đã kết thúc; phải xem PID
> wasm-opt mới khi poll. Không chạy linker/tác vụ nối tiếp thứ hai. Browser `/`
> đang trống cho boot bản refill khi link và constructors xong. Wasm đang serve
> là SHA3cf87b20... chưa refill; báo cáo game f0a35422... là lịch sử.
> **Đã bổ sung capture đầu vào draw có giới hạn192KiB**, bật theo cùng cờ
> diagnosticFrames; giữ packet, shader Maxwell/options và prefix4KiB từ GPU
> vertex/index/uniform/storage. Có đánh dấu incomplete; không phải replay đầy đủ.
> GPU fixture453 checks/16 draws qua. Lượt boot kế tiếp sẽ lấy dữ liệu thật này.
> Tab vẫn trống, linker160196 còn chạy; không boot Wasm cũ. Không sửa C/header.
> **Boot10 ĐÃ KẾT THÚC: 813 draw /164 presents**, cùng lỗi refill; mẫu pixel
> frame1,2,3,4,60,120 đều RGB0 trong texture nguồn. Archive SHA-tagged
> `build/codex-black-source-report.json`. Chưa xác định nguyên nhân render đen.
> Đã đóng/mở lại riêng phiên Playwright sau khi archive: Chrome giữ14.39GB
> committed memory, gây áp lực RAM/commit nặng cho linker. Tab hiện `/`, chưa boot.
> Giữ tab trống cho follow-up exec80349; không boot Wasm cũ lặp lại trong lúc link.
> Các dòng “boot10 đang chạy” dưới đây là lịch sử. Linker160196 vẫn còn sống.
> **Boot10 đang chạy `diagnosticFrames=1` trên Wasm cũ f0a35422...**, đọc pixel
> texture/crop được present để tìm nguyên nhân hình đen. Chế độ opt-in đã qua147
> GPU checks; off mặc định. Poll compact, không reload. Heap boot9 đã archive.
> Tác vụ nối tiếp exec80349/pwsh96676 vẫn chờ input builder137664/wasm-opt160196.
> Chưa chạy linker refill; start harness kế tiếp cũng bật chẩn đoán pixel.
> **MỚI NHẤT: boot chẩn đoán đạt 948 indexed draw /191 presents**, dừng callback
> cấp thêm command memory tại `8004da1280`. Boot8 trước đó803 draw nhưng ảnh vẫn
> đen; chưa có bằng chứng menu/hình game/input. Đã giữ heap và xác định callback
> `7100031be0`, nhánh command/control `710363eb60`/`710363ebe0`; cả ba có trong graph.
> Source C đã triển khai refill, qua **3.295 fixture checks**, compile-only xong.
> **Chưa tích hợp vào Wasm**: input linker160196 đã bắt đầu trước sửa refill.
> Tác vụ nối tiếp **exec80349** `build/codex-finish-refill.ps1` đang chờ builder
> input137664 kết thúc, rồi kiểm tra constructors -> link refill -> kiểm tra SHA/
> constructors -> boot Chrome có guard. Xem `build/codex-refill-followup.json`.
> Không chạy linker/tác vụ nối tiếp thứ hai; không dùng follow-up tích hợp cũ.
> Các dòng “đang boot8” bên dưới là lịch sử. Heap chẩn đoán hiện còn trong tab,
> bằng chứng cần thiết đã archive; tác vụ mới chỉ reload sau khi build/test xong.
> **Boot7 hoàn tất 7 indexed draw game thật**, dừng Vertex ViewportIndex capability.
> Shader thật ghi InstanceIndex-BaseInstance; đã thêm chứng minh SPIR-V output0
> cho draw1 instance rồi mới bỏ builtin. Nonzero/unproven/alias/branch overwrite
> vẫn fail rõ ràng. Shader thật compile qua; 132 GPU viewport checks +444 draw
> checks qua. Đang boot8; poll `codex-poll-compact.cjs`, không reload. Chưa chứng
> minh frame/menu/input. Link input10084/wasm-opt160196 vẫn phải giữ nguyên.
> **Boot6 đã compile shader gather trong game**, dừng guard depth-test bật khi
> không có depth attachment. Đã sửa đúng native state: bỏ WebGPU depth/stencil
> state khi không có attachment; GPU fixture444 checks/16 draws qua, kể cả NEVER.
> Đang boot7 với JS mới. Poll compact, không reload. Link input10084 vẫn chạy.
> **Gather động đã được tái hiện từ shader game thật và sửa:** 576 GPU pixel
> checks; shader lỗi có 4 gather đã qua WebGPU shader validation; 16 shader cũ
> vẫn qua. Đang boot6 với runtime SHA f0a35422... và JS/compiler mới. Poll
> **`build/codex-poll-compact.cjs`**, không reload. Source capture/report tại STATUS.
> **LINK INPUT đang chạy:** exec **10084**, wasm-opt PID **160196**, log
> `build/codex-runtime-input-build.log`. Không chạy linker thứ hai; không dùng lại
> follow-up cũ hardcode PID builder169548. Browser vẫn dùng Wasm cũ chưa có C input.
> **Đã hoàn tất 2 indexed draw game thật**, xác minh depth sampling fix. Boot4
> dừng shader kế tiếp ở ImageGatherExtended; archive `codex-integrated-two-draws-report.json`.
> Đã mở gate cho gather hằng mà Naga đã hỗ trợ (64 GPU checks), còn mask chưa
> hỗ trợ phải fail rõ ràng, không bị bỏ qua. Đang boot5; nếu compiler lỗi sẽ có
> `lastShaderFailure` chứa control/data thật + options để tái hiện độc lập.
> Chưa có menu/hình game/input được xác minh. Poll, không reload lượt đang chạy.
> **Boot thứ ba vượt lỗi binding42 dư**, dừng ở ordinary float sampling D24S8.
> Đã thêm GPU depth -> r32float snapshot, giữ nguyên shader/sampler và linear
> filtering; 132 GPU checks D32/D24S8, 380 draw checks đều qua. Comparison depth
> giữ view gốc; cần float32-filterable (adapter hiện tại có và runtime đã request).
> Đang boot thứ tư cùng runtime Wasm. Poll `codex-poll-boot.cjs`, không reload.
> **Boot thứ hai đã qua tạo pipeline**, dừng bind group vì cbuf2/binding42 dư.
> Đã thêm reflection binding còn dùng từ Naga/Wasm và lọc bind entries theo đó;
> 4 Rust tests, compute Wasm/GPU, 16/16 shader Maxwell và 380 GPU checks đều qua.
> Archive `codex-integrated-bind42-report.json`. Đang boot thứ ba cùng Wasm
> runtime với compiler/JS mới. Poll `codex-poll-boot.cjs`, không reload.
> **Đã tới draw game đầu tiên:** 2 shader game compile được nhưng pipeline lỗi
> attachment màu không có fragment output; `draws=0`, 3 presents chưa chứng minh
> hình game. Đã archive `codex-integrated-first-draw-report.json`, sửa JS writeMask
> theo output thật, GPU fixture 380 checks / 15 draws. Đang boot lại cùng Wasm
> với JS mới. Poll `codex-poll-boot.cjs`, không reload lượt đang chạy.
> **LINK TÍCH HỢP ĐÃ XONG, BOOT CHROME ĐANG CHẠY (02:26):** SHA
> `f0a354229b00b559b4550675fcc545382ceeb2869665abfb34832db842d5df85`;
> Node 306/306 ctors. Exec 90858 và follow-up 1949 đều đã kết thúc thành công.
> **Không chạy start harness/reload nữa**; poll `build/codex-poll-boot.cjs`
> qua session `ssbu-web`. Bản này có callback + C renderer, chưa có C input mới.
> Chưa có bằng chứng game draw/menu/input. Các ghi chú "link đang chạy" dưới
> đây là lịch sử và đã được thay thế bởi cập nhật này.
> **Tác vụ chờ link -> kiểm tra -> boot đang chạy:** exec **1949**,
> `build/codex-finish-integration.ps1`. Đọc `build/codex-integration-followup.json`
> trước khi tự boot: tác vụ sẽ chạy Node 306 ctors đúng SHA rồi mở Chrome bằng
> harness có guard giữ tab runtime đang chạy. Không tạo linker mới.
> Khi state `browser-running`, poll `codex-poll-boot.cjs`, không reload.
> **Input mới trong source:** bàn phím/gamepad -> shared Wasm -> Npad;
> 177 OS/Wasm checks, 12 mapping contracts, 7 kiểm tra keyboard/focus Chrome.
> Đã bỏ timeout boot mặc định 15 phút; vẫn có nút dừng và `?timeout=...`.
> **Link exec 90858 đang chạy CHƯA chứa sửa C input này**, cần link kế tiếp
> sau khi link hiện tại xong. Không khởi động link thứ hai. JS tương thích
> khi export input chưa có. Chưa xác minh gamepad vật lý hoặc input trong game.
> **Vertex fetch mới:** đã thêm chuyển đổi các định dạng native chưa bind trực
> tiếp được trong WebGPU; 38 CPU contracts và 379 GPU checks / 14 fixture draws.
> Có half3, scaled byte3 lệch hàng, snorm16x3, base vertex và first instance.
> C NVN vẫn qua 3.266 checks. Không có bằng chứng game/menu/input từ fixture.
> PID wasm-opt 157916 vừa được xác minh còn chạy, CPU >4.078 giây; giữ nguyên link.
> **Cập nhật mới nhất:** lượt Chrome source build `ae6b06f8...` đã kết thúc tại
> callback `71017c5540`, 36.837.206 imports, `threadFailed=0`, cùng 234 textures.
> Bằng chứng `build/codex-source-36837206-report.json` xác nhận mapping fix từ C.
> Draw JS đã thêm flat last-vertex, strip/restart, 8-bit indices và index16
> offset2: 312 GPU checks / 8 fixture draws và 11 CPU contract cases.
> C NVN vẫn đạt 3.266 checks. Giữ scope fixture rõ ràng.
> Full link tích hợp vẫn trong exec **90858**, wasm-opt PID **157916**;
> xác minh process/session còn sống trước mọi thao tác link tiếp.
> Khi xong: Node constructors, rồi boot module tích hợp để lấy lỗi/draw game
> tiếp theo. Lượt boot cũ đã hoàn tất, không cần chạy lại module cũ.
>
> **Trạng thái mới nhất 01:11:** link cũ đã xong, source Wasm SHA `ae6b06f8...`
> đạt Node 306/306 và đang boot Chrome ở URL `/` bình thường.
> Link tích hợp mới đang chạy trong exec **90858**, log
> `build/codex-runtime-integrated-build.log` (có `--trace-asyncify`).
> Đã có đường draw thật `host_gpu_draw.c` -> operation 24 -> `gpu-draw.mjs`;
> fixture JS 199 checks, fixture C NVN 3.266 checks/2 GPU draws/1 sampler.
> Shader tự tạo chỉ nằm trong fixture Worker có cờ kiểm thử; runtime game vẫn
> dùng compiler Maxwell thật. Chưa có bằng chứng draw/menu/trận đấu game.
> Không khởi động linker thứ hai vào cùng staging. Poll lượt boot source
> hiện tại bằng `build/codex-poll-boot.cjs` và archive kết quả.
>
> **Lượt scratch boot đã kết thúc** tại `uncompiled indirect target
> 71017c5540`, `lr=7101975594`, `phase=nnMain`, 67.128.644 imports,
> `threadFailed=0`. Báo cáo: `build/codex-scratch-67128644-report.json`.
> Đã đọc trực tiếp vùng scratch dưới map lock: hai mapping đều 64 KB,
> format `%0*d`, output `28`; lỗi swprintf cũ được xác nhận đã vượt qua.
> Đã thêm callback `71017c5540`; prepare **giữ chế độ observed của manifest**
> (`--main --pointer-tables --callback-tables`, không bật broad/all) tự gom
> bảng 7 con trỏ tại `710509e610`. Graph hiện 62.439 hàm / 709 units.
> Cache mới đã compile xong, chỉ `runtime_c01fb.c` và `runtime_table.c` đổi.
> Sau khi linker cũ kết thúc, cần một lượt link mới với graph/cache hiện tại
> để tích hợp callback + presentation + depth clear + draw fail-fast.
> Lượt link cũ đang chạy vẫn dùng graph 62.435 hàm; manifest cũ đã archive ở
> `build/codex-manifest-before-71017c5540.json`. Không chạy hai linker vào cùng staging.
> Live draw guard chưa xác nhận kích hoạt trước khi worker đóng; không dùng
> nó làm bằng chứng. Browser giữ nguyên; chỉ client CLI chờ worker đã đóng bị dừng.
>
> Đã thêm thực thi `ClearDepthStencil` qua operation 25 (210 checks GPU;
> 1.209 checks C NVN, có mip/layer và stencil mask). Object cache mới đã sẵn sàng.
> Lượt boot vừa chạy ở `/scratch.html`, dùng bản Wasm chẩn đoán riêng vá
> đúng 4 hằng số mapping của module cũ, SHA và giới hạn được ghi trong STATUS.
> Poll bằng `build/codex-poll-boot.cjs`; **không reload trang hoặc chạy helper
> fixture cũ trên tab này**. Các fixture phải dùng tab riêng.
> Kết quả hoàn tất của lượt scratch boot đã được archive như ghi phía trên.
> Build sửa scratch mapping đang chạy trong session exec `66734`, log
> `build/codex-runtime-build.log`; xác minh PID hiện tại trước khi build tiếp.
> Link đã bắt đầu trước các sửa C mới của phần xuất hình. Sau khi link xong:
> chạy Node constructors, boot bằng `build/codex-start-boot.cjs` qua session
> Playwright `ssbu-web`, đọc `window.__ssbuRuntimeProgress` / `__ssbuRuntime`
> và lưu báo cáo. Server do phiên này khởi động trong exec session `89820`.
> Không khởi động lại linker chỉ vì chưa có log mới.
>
> Source mới đã nối `nvnQueuePresentTexture` (operation 23) tới canvas game
> qua OffscreenCanvas, có crop và sRGB. GPU fixture xuất hình đạt 121 checks;
> fixture NVN qua C đạt 1.183 checks, 2 presents, không lỗi. Báo cáo nằm tại
> `build/codex-presentation-report.json` và `build/codex-nvn-report.json`.
> Object cache C mới đã compile-only xong; **cần link lượt kế tiếp** để đưa
> thay đổi xuất hình vào Wasm. Draw/dispatch chưa hỗ trợ nay báo lỗi rõ ràng,
> sửa hành vi cũ nhận lệnh rồi bỏ qua (negative test DrawArrays đã xanh).
> Ba header được bảo vệ không thay đổi trong phiên này.

## 1. Mục tiêu tổng thể
Hoàn thiện bản port **Super Smash Bros. Ultimate (v13.0.4)** chạy trực tiếp trên trình duyệt thông qua **WebAssembly (Emscripten PThreads + Portable Cross-Thread Asyncify Fibers)** và **WebGPU (Maxwell 3D / NVN -> SPIR-V / WGSL)** trong thư mục `c:\Users\Hikari\Desktop\wasm\ssbu-recomp`.

---

## 2. Trạng thái hiện tại (Current Progress)
- **Tập hàm AArch64 đã lift & biên dịch (`build/web-runtime/manifest.json`)**:
  - **62,435 hàm** (`709` compilation units `runtime_c0000.c` .. `runtime_c04d0.c`).
  - **15,677 pointer-table / vtable targets** đã được tự động phát hiện qua [`web/prepare_runtime.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/prepare_runtime.py) và [`web/runtime_roots.json`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/runtime_roots.json).
- **Khởi chạy Constructor (`_runtime_run`)**:
  - **306 / 306** static constructors của `main.elf` chạy thành công 100% (`node web/test_runtime_node.mjs fibers-compact`).
- **Tiến độ thực thi `nnMain` trên Chrome / WebGPU (`runtime-fibers-compact.wasm`)**:
  - Đã vượt qua **38,527,928 lệnh gọi host import** (`importCalls`).
  - Đã biên dịch và đăng ký **7,946 GPU shader programs** (Maxwell -> WGSL).
  - Đã tạo **202 GPU textures**, **251 texture views**, upload **208.92 MB** dữ liệu texture từ `data.arc`.
  - Có **818 fences** (`fencesCreated: 818`, `flushes: 1,638`, `colorClearSubmissions: 4,914`). Báo cáo này **chưa chứng minh frame game hiển thị**: module tại thời điểm đó không thực thi draw và không xuất texture ra canvas (`shadersCompiled: 0`).
- **Lỗi vừa xử lý và trạng thái build hiện tại**:
  - Tại mốc `38,527,928` import calls, hàm `f_71037ad390` (`lr = 0x71037ad414`) gọi `swprintf(ws, 31, L"%0*d", ...)` và gặp lỗi `memory write (unmapped), 1 bytes` tại địa chỉ `0x7800002100`.
  - **Nguyên nhân**: Trong [`web/host_runtime.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_runtime.c#L100-L108), vùng `errno_address` (`0x7800000000`) chỉ được map `1 * GM_PAGE_SIZE` (`4 KB`: `0x7800000000..0x7800000fff`), trong khi `swprintf` và `gmtime` trong [`web/host_os.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_os.c#L1164-L1191) sử dụng scratch buffer tại `0x7800002000..0x7800002600`. Tương tự, vùng `0x7600000000` (`base`) chỉ map `2 * GM_PAGE_SIZE` (`8 KB`) nhưng lại ghi các bảng `CXX_*` và RTTI tại `0x7600008000..0x760000ffff`.
  - **Đã sửa**:
    1. Mở rộng vùng map `errno_address` (`0x7800000000`) và `base` (`0x7600000000`) trong [`web/host_runtime.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_runtime.c#L100-L108) lên `16 * GM_PAGE_SIZE` (`64 KB`).
    2. Bổ sung khóa `pthread_mutex_lock(&registry)` quanh `swprintf` trong [`web/host_os.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_os.c#L1164-L1184) để đảm bảo an toàn đa luồng.
    3. Toàn bộ **709 file `runtime_c*.o`** và các file host `.o` đã được biên dịch xong và lưu cache trong `build/web-runtime/obj/`. Hiện chỉ cần hoàn tất bước link `runtime-fibers-compact.wasm`.

---

## 3. Các Quy Tắc Bắt Buộc (CRITICAL GUARDRAILS)

1. **TUYỆT ĐỐI KHÔNG SỬA 3 FILE HEADER SAU**:
   - [`web/guest_memory.h`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/guest_memory.h)
   - [`web/runtime_memory.h`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/runtime_memory.h)
   - [`runtime/runtime.h`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/runtime/runtime.h)
   - *Lý do*: SHA-256 của 3 file này được đưa vào fingerprint của toàn bộ 709 file `runtime_c*.o` trong [`web/build_runtime.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/build_runtime.py#L107-L117). Chạm vào 1 byte của 3 file này sẽ làm mất toàn bộ cache `.o`!
2. **Luôn truyền `--jobs 8`** khi chạy `build_runtime.py` hoặc `advance_runtime.py`.
3. **Giữ nguyên cấu hình Binaryen trong [`web/build_runtime.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/build_runtime.py#L159-L171)**:
   - Giữ `-sBINARYEN_EXTRA_PASSES=--optimize-level=1` (nếu bỏ cờ này, `wasm-opt` mặc định chạy `-O2` trên module 75 MB sẽ treo hàng giờ).
   - Giữ `BINARYEN_CORES` ở mức `min(args.jobs, 4)`.
4. **Đồng bộ file `.mjs` sang `build/web-poc/`**:
   - Server [`web/serve.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/serve.py) phục vụ thư mục `build/web-poc/`. Nếu chỉ sửa các file `.mjs` trong `web/` mà không chạy lại `build_runtime.py`, phải copy sang `build/web-poc/` (`Copy-Item web\<file>.mjs build\web-poc\<file>.mjs -Force`).
5. **Quy tắc dùng Playwright CLI**:
   - Vì mỗi lệnh `run_command` chạy trong một PowerShell session mới, mỗi khi gọi `playwright-cli` hoặc `advance_runtime.py`, **luôn gộp chung** chuỗi lệnh đóng/mở session trong cùng 1 lệnh `run_command`:
     ```powershell
     npx --yes --package @playwright/cli playwright-cli -s=ssbu-web close; npx --yes --package @playwright/cli playwright-cli -s=ssbu-web open http://127.0.0.1:8787/; npx --yes --package @playwright/cli playwright-cli -s=ssbu-web run-code --filename C:\Users\Hikari\.gemini\antigravity\brain\2b651684-63fb-43cd-af65-2153147fecb7\scratch\inspect_boot.cjs
     ```

---

## 4. Quy Trình Làm Việc Chuẩn (Workflow Cho AI Tiếp Theo)

### Bước 1: Đảm bảo HTTP Server đang chạy ở cổng `8787`
```powershell
python web/serve.py
```
*(Chạy dưới dạng daemon background task).*

### Bước 2: Link & Kiểm tra Module Wasm
Khi sửa C host (`web/*.c`) hoặc thêm hàm vào [`web/runtime_roots.json`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/runtime_roots.json):
1. Nếu có sửa `web/runtime_roots.json` hoặc [`web/prepare_runtime.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/prepare_runtime.py):
   ```powershell
   python web/prepare_runtime.py --main --pointer-tables --all-pointer-tables --callback-tables
   ```
2. Biên dịch & link bản `fibers-compact`:
   ```powershell
   python web/build_runtime.py --fibers --compact-codegen --jobs 8
   ```
3. Kiểm tra nhanh 306 constructors trên Node.js:
   ```powershell
   node web/test_runtime_node.mjs fibers-compact
   ```

### Bước 3: Chạy kiểm thử Boot thực tế trên Chrome / WebGPU
Sử dụng script có sẵn `inspect_boot.cjs` (hoặc tạo lại nếu chạy ở conversation mới):
```javascript
// inspect_boot.cjs
async (page) => {
  await page.goto('http://127.0.0.1:8787/?variant=fibers-compact', { waitUntil: 'networkidle' });
  await page.evaluate(async () => {
    const pick = async (dir, name) => {
      const h = await dir.getFileHandle(name);
      return await h.getFile();
    };
    // Harness tự động mount main.elf và data.arc qua local server / OPFS hoặc file picker
  });
}
```
*(Xem chi tiết nội dung file `C:\Users\Hikari\.gemini\antigravity\brain\2b651684-63fb-43cd-af65-2153147fecb7\scratch\inspect_boot.cjs` hoặc dùng `python web/advance_runtime.py --jobs 8` để tự động hóa vòng lặp phát hiện `indirect call to uncompiled function` / `unhandled host import`).*

### Bước 4: Xử lý các điểm dừng tiếp theo trong `nnMain`
Khi `guestThread` dừng hoặc trap trong `inspect_boot.cjs`:
1. **Nếu báo `indirect call to uncompiled function` tại địa chỉ `0x710xxxxxxx`**:
   - Thêm địa chỉ đó (bỏ tiền tố `0x`) vào mảng `"indirectTargets"` trong [`web/runtime_roots.json`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/runtime_roots.json).
   - Kiểm tra hàm gọi (`guestFunction` / `lr`) trong `build/native/work/gen_eh_o2/c_XXXX.c` xem địa chỉ đó đến từ vtable hay callback table nào để mở rộng quy tắc quét trong [`web/prepare_runtime.py`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/prepare_runtime.py#L307-L355), giúp gom toàn bộ các hàm cùng nhóm trong 1 lần build.
2. **Nếu báo `unhandled host import` hoặc `unhandled indirect host call`**:
   - Cài đặt hàm SDK / OS / FS / GPU tương ứng trong [`web/host_os.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_os.c), [`web/host_fs.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_fs.c), [`web/host_runtime.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_runtime.c), hoặc [`web/host_gpu*.c`](file:///c:/Users/Hikari/Desktop/wasm/ssbu-recomp/web/host_gpu.c).
3. **Nếu báo `memory read/write (unmapped)`**:
   - Kiểm tra `address`, `guestFunction`, `lr`, và các thanh ghi `x0..x30` trong báo cáo JSON của `runtime-worker.mjs` để xác định vùng bộ nhớ hoặc cấu trúc dữ liệu host cần khởi tạo.
