# Bàn giao AI: SSBU 13.0.4 WebAssembly / WebGPU

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
