# Bàn giao SSBU → Web cho AI tiếp theo

> Tài liệu lịch sử ngày20/10. Bản bàn giao hiện hành: [web/HANDOFF.md](web/HANDOFF.md).
> Thông tin bên dưới về Git/untracked, PID và điểm dừng là snapshot cũ.

**Cập nhật: 20/10/2026, khoảng 05:45, giờ Việt Nam.** Đây là ảnh chụp trạng thái sau khi đẩy `nnMain` và các worker thread trên Chrome/WebGPU vượt qua worker thread `BCATCacheStorage` (`0x71036f9a70`, `628` live mutexes) và phương thức ảo `0x71036f85d0` trong `BootSequenceScene` (`u_71022b7100` -> `f_71036f86e0`) để tiến tới **`0x7100027bf0`** (`phase = guestThread`, `lr = 0x7300000000`, `18,837,755` import calls, **`629` live mutexes**, `348` GPU buffers tạo / `7` hủy / `107.10 MB` ghi, `103` textures / `126` texture views / `172.41 MB` ghi bao gồm `23` lượt `depthUploadPasses` và `11` lượt `stencilUploadWrites` của game thật, `7,844` shader programs đăng ký, `12` command submissions / `12` buffer-to-texture copies / `12` queue finishes), hoàn tất khởi tạo `NintendoWare_Font` / `NintendoWare_Ui2d` trong `f_710375a630`, sửa lỗi thứ tự `SetDevice`-trước-`SetDefaults` trên toàn bộ 6 NVN Builders (`web/host_gpu_resources.c`, `web/host_gpu_sampler.c`, `web/host_gpu_commands.c`, xác minh bởi **1,181/1,181 checks PASS** trong `web/gpu_probe.c`), sửa lỗi nhận diện `rd64` virtual tail-call trong `prune_huge_dispatch` (`web/prepare_runtime.py`, đưa số hàm bị bail về `0` và đạt **13/13 checks PASS** trong `web/test_prepare_runtime.py`), và giữ vững **161/161 checks PASS** trong `web/os_probe.c`; hãy kiểm tra lại file, hash và process trước khi tiếp tục.

## 1. Mục tiêu và tình trạng thật

Người dùng muốn port [Denkishi/ssbu-recomp](https://github.com/Denkishi/ssbu-recomp) để **chạy game trực tiếp trong trình duyệt bằng Wasm/WebGPU**, dùng dump cục bộ. Thành công cuối cùng phải có menu, trận đấu, hình ảnh, điều khiển, âm thanh và đo hiệu năng thực tế. Không thay mục tiêu bằng stream bản native hoặc trang demo giả gameplay.

**Hiện CHƯA chơi được trên web.** Chrome đã chạy đủ 306 constructor, vào `nnMain`, vượt qua `SleepThread`, khởi tạo 4 bộ quản lý bộ nhớ/resource lớn trong `u_710374f360` (`32 MiB + 32 MiB + 192 MiB + 1 GiB`), giải nén ZSTD bảng mục lục `data.arc` (`136,577,040` byte = `130.25 MiB`), khởi tạo `nn::nifm`, `nn::account`, `nn::hid`, `nn::vi`, `nn::ro`, nạp và giải nén các gói tài nguyên trên 11+ luồng pthread/fiber chạy song song (bao gồm `BCATCacheStorage` tại `0x71036f9a70`), khởi tạo `nn::util::BinaryFileHeader`, `nn::mem::StandardAllocator`, `nn::gfx`, `nn::pl` shared fonts, `nn::fontll::ScalableFontEngine`, `NintendoWare_Font`, `NintendoWare_Ui2d` (`f_710375a630`) và `BootSequenceScene` (`u_71022b7100`, vượt qua `0x71036f85d0`), đăng ký **7,844 shader program**, tạo **348 GPU buffer (`107,102,720` byte đã ghi)** và **103 GPU texture / 126 texture views (`172,407,250` byte đã ghi, gồm cả `23` `depthUploadPasses` và `11` `stencilUploadWrites`)**, thực thi **12 command submissions / 12 buffer-to-texture copies / 12 queue finishes**, tạo **629 live mutexes** tại **`0x7100027bf0`** (`importCalls = 18,837,755`). Chưa có bằng chứng menu/trận đấu, draw của game, input/audio hoặc FPS trên web. Các fixture GPU chạy xanh không đồng nghĩa game đã render.

## 2. Workspace, Git và dữ liệu cục bộ

| Mục | Vị trí / trạng thái |
| --- | --- |
| Thư mục cha | `C:\Users\Hikari\Desktop\wasm` |
| Repo đang làm | `C:\Users\Hikari\Desktop\wasm\ssbu-recomp` |
| Branch | `web-poc` |
| Upstream tham chiếu | `938e66bb7be0ba4123e291211895412da421b705` |
| Code port | `web/` — hiện toàn bộ vẫn **untracked** |
| Build/dependencies | `build/`, `deps/` — ignored, không nằm trong Git |
| Server | `http://127.0.0.1:8787/` |

Ngoài `web/`, Git còn thay đổi `.gitignore`, `runtime/build_glslang.bat`, `runtime/runtime.h`, `scripts/setup.py`, `tests/seq_battle_play_fast.txt`, `tools/extract/fw_fonts.py`; các file untracked khác: `tools/extract/inspect_xci.py`, `prepare_dump.py`, `test_packages.py`. Không discard/reset chúng. Chưa tạo commit/PR cho phần port này. Nếu chuyển máy chỉ bằng Git thì **sẽ mất phần chưa add và toàn bộ build cache/evidence**; cần bàn giao workspace riêng hoặc commit source đã kiểm tra.

Dump sử dụng để phát triển là **SSBU 13.0.4**:

- `build/native/binaries/13.0.4/main.elf`: 86,677,632 byte.
- SDK cùng thư mục; SHA-256 SDK đã ghi nhận: `d42578f463806dbd0023752dcecc43fed230c9f35d3a787696c3e92dd08eccdd`.
- `build/dump-13.0.4/romfs/data.arc`: 17,600,869,448 byte; thêm 13 file `.nrr` trong thư mục `.nrr/`, tổng 14 file RomFS cho boot helper.
- `build/native/work/gen_eh_o2/`: C đã lift của native; rất quan trọng cho prepare graph.
- `build/web-nvn-layout/sdk-layout-image.bin`: SDK readonly image cho truy vấn layout (~13.6 MB).
- `build/native/sysdata/fonts/`: font đã trích từ firmware cục bộ.

Người dùng từng cung cấp XCI/extract trong Downloads và firmware/key, có nhắc `C:\Users\Hikari\Downloads\Compressed`, firmware 22.1.0. **Không cần extract lại để tiếp tục mốc hiện tại**: đầu vào đang chạy là các đường dẫn 13.0.4 ở trên. Không in key, đưa key/ROM/asset hoặc C sinh từ game vào commit/public hosting. Chỉ mount file local, không upload `data.arc` 17 GB lên server.

## 3. Trạng thái runtime được xác nhận mới nhất

Nguồn chính: `build/browser-runtime-18837755-report.json` (`build/web-poc/runtime-browser-report.json`), `build/browser-runtime-18836613-report.json`, `build/web-poc/runtime-fibers-compact-node-report.json`, metadata `build/web-poc/runtime-fibers-compact-build.json`, và `build/web-os/report.json`.

| Thuộc tính | Kết quả |
| --- | --- |
| Module | `runtime-fibers-compact` |
| Timestamp build UTC | `2026-10-19T22:34:01.207840+00:00` |
| SHA-256 Wasm | `6e82a0501c677651cfc2069492a405681a5c756a506382ec3ac74875d371ce5e` |
| Kích thước / số function / max locals | `164,570,580` byte / `72,093` (`49,478` guest) / `10,812` |
| Constructor | 306/306 hoàn tất trong Node (`10,945` imports) và Chrome |
| nnMain & guestThread | Vượt qua `0x71036f9a70` (`BCATCacheStorage`, `628` mutexes) và `0x71036f85d0` (`BootSequenceScene`), tiến tới **`0x7100027bf0`** (`importCalls = 18,837,755`) |
| Điểm dừng | `uncompiled indirect target 7100027bf0` |
| Caller / guest function | `lr = 7300000000` (entry point của worker thread được tạo bởi `f_71000007f0` -> `f_7100027c40` -> `f_7100027a70`) / `guestFunction = 0` (`x0 = 0x804ad36080`, `x8 = 0x7100027bf0`, `sp = 0x804ad3b000`) |
| Phase | `guestThread` |
| Mutex / semaphore | 629 live/peak mutex; 2 live semaphore |
| SDK layout queries | 309 (`function = 428760`, `result = c00`) |

Các mốc trung gian đã vượt qua:
- **Từ `0x71036f85d0` (`nnMain`, `628` live mutexes) sang `0x7100027bf0` (`guestThread`, `629` live mutexes, `18,837,755` import calls) trên Chrome/WebGPU**:
  - Sau khi thêm `71036f85d0`, 11 phương thức ảo cấp 1 của `u_71022b7100`, 18 hàm khởi tạo con trực tiếp của `u_71022b7100` và các phương thức ảo của `BootSequenceScene`, `nnMain` vượt qua `0x71036f85d0`, tạo thêm 1 mutex (**`629` live mutexes**) và gọi `f_71000007f0` -> `f_7100027c40` -> `f_7100027a70` để tạo worker thread mới với entry point `0x7100027bf0` (nạp từ GOT `0x71052a2000 + 632`) và hàm callback bên trong `0x7100027da0` (nạp từ GOT `0x71052a2000 + 640`).
- **Từ `0x71036f9a70` (`guestThread`) sang `0x71036f85d0` (`nnMain`, `628` live mutexes, `threadFailed = 0`) trên Chrome/WebGPU**:
  - Sau khi thêm `71036f9a70`, `71022b7100` (`BootSequenceScene` body), `7101760c10` / `71017616d0` và các worker thread functor, luồng nền `BCATCacheStorage` (`0x71036f9a70`) chạy hoàn tất phần khởi tạo hàng đợi mà không gặp lỗi (`threadFailed = 0`), tăng số mutex sống từ `627` lên **`628`**, trong khi `nnMain` tiếp tục chạy qua `u_71022b7100` tới `0x71022b8f80` (`f_71036f86e0`) và gọi phương thức ảo `0x71036f85d0` qua bảng vtable `0x710523aa00`.
- **Từ `18,826,267` lên `18,837,085` import calls trên Chrome/WebGPU**:
  - Sau khi thêm `710007add0` và các đại diện vtable của `NintendoWare_Font` / `NintendoWare_Ui2d` (`f_710375a630`), `nnMain` hoàn tất toàn bộ hàm khởi tạo cuối cùng `f_710375a630` của `u_7103741990`, đăng ký **7,844 shader programs** (`+6` shader UI2D/Font), tạo **348 GPU buffers / 7 destroyed (`8,206` lượt ghi / `107,102,720` byte)**, **103 GPU textures / 126 texture views (`+23` texture views, `125` lượt ghi / `172,407,250` byte, trong đó có `23` `depthUploadPasses` và `11` `stencilUploadWrites`)**, và **12 command submissions / 12 buffer→texture copies / 12 queue finishes**. **`shadersCompiled = 0`, `colorClears = 0` trong boot game thật.**
- **Tối ưu hóa cắt tỉa `L_dispatch` (`prune_huge_dispatch`) & xử lý `rd64` virtual tail-call trong `web/prepare_runtime.py`**:
  - Sửa lỗi nhận diện virtual tail-call (`{ jt = c->x[R]; goto L_dispatch; }`) trong `prune_huge_dispatch()` khi thanh ghi đích `c->x[R]` được nạp bằng `rd64(` nhưng trong cửa sổ 20 dòng phía trước có lệnh `(int32_t)rd32(a)` không liên quan, giúp cắt tỉa hoàn toàn `f_71032f2db0` (`576 -> 0` cases) và `f_71022f49a0` (`176 -> 0` cases), đưa số hàm bị bail trên toàn bộ 461 đơn vị `runtime_c*.c` về **0**. Unit tests trong `web/test_prepare_runtime.py`: **PASS 13/13** (`23/23` toàn thư mục `web/`).
- **Sửa lỗi tràn stack pthread `64 KB` trong `web/host_fs.c`, sửa `nn::nifm` trong `web/host_os.c`, và mở rộng `web/os_probe.c` (`161/161 PASS`)**:
  - Chuyển hai mảng `unsigned char staging[65536]` trong `read_file` và nhánh `WriteFile` của `web_fs_import` (`web/host_fs.c`) thành `static unsigned char staging[65536]` (dưới khóa `fs_lock`), giảm kích thước stack frame của `web_fs_import` từ `66,592` byte xuống `< 2 KB`.
  - Kiểm tra đơn vị: `python web/build_os_probe.py` **PASS 161/161 checks (`failures = 0`)** (`build/web-os/report.json`).

## 4. Điểm dừng tại `0x7100027bf0` (`lr = 0x7300000000`, `phase = guestThread`)

Sau khi vượt qua `0x71036f85d0`, `nnMain` tiếp tục khởi tạo hệ thống con tiếp theo trong `u_71022b7100` (`f_71000007f0` -> `f_7100027c40` -> `f_7100027a70`), tạo một worker thread mới qua `nn::os::CreateThread`. Vì `f_7100027a70` và `f_7100027c40` nạp địa chỉ hàm entry `0x7100027bf0` (`0x71052a2000 + 632`) và hàm callback bên trong `0x7100027da0` (`0x71052a2000 + 640`) qua bảng GOT (`ADRP` + `LDR` `rd64(a)`) thay vì `ADRP` + `ADD`, `computed_addresses()` chưa tự động thu thập hai hàm này. Cần bổ sung `7100027bf0`, `7100027da0` và các mục con trỏ hàm GOT liên quan vào `web/runtime_roots.json`.

## 5. Depth/stencil & NVN Builder đã hoàn tất và có đầy đủ bằng chứng

- **C NVN Depth & Builder Fixture (`web/gpu_probe.c`)**: Đã cấp `DEPTH_POOL_OBJECT` (`0x710000c200`) với 65,536 byte backing riêng tại `DEPTH_STORAGE` (`0x7100080000`) và bổ sung 5 kiểm tra hồi quy cho thứ tự `SetDevice`-trước-`SetDefaults` trên `MemoryPoolBuilder`, `BufferBuilder`, `TextureBuilder`, `SamplerBuilder`, `QueueBuilder`. Chrome WebGPU chạy với SDK thật **PASS 1,181 checks, 0 failures** (`depthUploadPasses = 16`, `depthReadPasses = 8`, `stencilUploadWrites = 12`). Bằng chứng: `build/browser-nvn-depth-report.json`.
- **Direct WebGPU Depth/Stencil & Negative Batch Validation (`web/test-gpu-depth.mjs`, `web/test_gpu_textures.mjs`)**: Đã bổ sung kiểm tra negative mid-batch bảo đảm `validate()` chặn toàn bộ batch trước khi ghi bất kỳ texel nào lên GPU. Kết quả: **PASS 652 checks, 0 failures** trên Chrome (`build/browser-depth-transfers-report.json`) và **PASS 12 checks** trên Node (`web/test_gpu_textures.mjs`).
- **Full UI (“Chạy kiểm tra”)**: Đã cập nhật `web/build_gpu_probe.py`, `web/app.mjs`, và `build/check-browser-clear-ui.cjs`. Kết quả chạy UI trên Chrome **PASS** (`render.checks = 3704`, `textures.checks = 1181`, `depth.checks = 652`). Bằng chứng: `build/browser-depth-ui-report.json`.

| NVN | WebGPU | Encoding nguồn |
| --- | --- | --- |
| `0x31` | stencil8 | 1 byte |
| `0x32` | depth16unorm | 2 byte |
| `0x33` | depth24plus | uint32 normalized |
| `0x34` | depth32float | float32 |
| `0x35` | depth24plus-stencil8 | depth high24, stencil low8 |
| `0x36` | depth32float-stencil8 | float32 + uint32 chứa stencil low8 |

Giới hạn cần giữ rõ:

- Nguồn synthetic theo encoding của `runtime/tex_decode.c`, chưa so pixel với native GL hoặc draw game thật.
- Depth24plus có precision opaque. Test chấp nhận sai số `0x33 <= 512` đơn vị U32; `0x35 <= 1` D24 LSB, stencil phải exact. Không gọi đây là bit-exact native parity.
- Float depth chỉ nhận finite `[0,1]`; NaN/out-of-range trap rõ ràng.
- Depth24/32 upload dùng fragment shader `frag_depth`, staging `r32float`, explicit unfilterable-float binding.
- Partial stencil update đọc toàn aspect về CPU, merge, ghi lại toàn aspect; đúng trong fixture nhưng có chi phí readback cần theo dõi.
- `depth32float-stencil8` là feature tùy chọn, worker và main-page UI đều yêu cầu khi adapter hỗ trợ.

File liên quan: `gpu-depth.mjs`, `gpu-textures.mjs`, `gpu-render.mjs`, `host_gpu_texture_objects.c`, `host_gpu_resources.c`, `runtime-worker.mjs`, `gpu-probe-worker.mjs`, `gpu_probe.c`, `test-gpu-depth.mjs`, `test_gpu_textures.mjs`. Ba builder `build_runtime.py`, `build_gpu_probe.py`, `build_maxwell.py` đều copy `gpu-depth.mjs` và `test-gpu-depth.mjs`.

Ràng buộc WebGPU tham khảo nguồn chính thức: [depth formats](https://gpuweb.github.io/gpuweb/#depth-formats), [copying depth/stencil](https://gpuweb.github.io/gpuweb/#copying-depth-stencil). Depth24 opaque và giới hạn aspect/copy là lý do có đường shader/readback riêng.

## 6. Toolchain và lệnh tiếp tục

Chạy từ repo root trong PowerShell; dùng SDK có sẵn, không cài lại vô cớ:

```powershell
Set-Location C:\Users\Hikari\Desktop\wasm\ssbu-recomp
$taskPython = '.\deps\dump-tools\Scripts\python.exe'
$taskNode = '.\deps\emsdk\node\24.19.0_64bit\node.exe'
git status --short
Get-Content build/runtime-semaphore-state.json
Get-Content build/web-poc/runtime-fibers-compact-build.json
Get-NetTCPConnection -LocalPort 8787 -State Listen
```

Pinned: Python 3.12; Emscripten **6.0.12**; Node **24.19.0**; Binaryen **133-60-g93d6e9de7**; native LLVM9/MSVC; glslang 16.6.0 (`e1b562a8bed273a02f30b59b66a5d499793cede5`); Naga 29.0.4 có patch cục bộ. `SSBU_EMSDK` có thể override SDK; kiểm tra trước khi dùng SDK khác.

Kiểm tra nhanh sau sửa source tương ứng:

```powershell
& $taskPython web/test_prepare_runtime.py
& $taskPython web/test_build_runtime.py
& $taskPython web/build_os_probe.py
& $taskNode web/test_gpu_textures.mjs
& $taskPython web/build_gpu_probe.py --with-sdk
```

Browser fixture dùng Playwright CLI session đã có. Các file `build/check-browser-*.cjs` là function cho `run-code`, **không phải script chạy trực tiếp bằng Node**:

```powershell
npx.cmd --yes --package @playwright/cli playwright-cli -s=ssbu-gpu run-code --filename build/check-browser-gpu-depth.cjs
npx.cmd --yes --package @playwright/cli playwright-cli -s=ssbu-gpu run-code --filename build/check-browser-nvn-clear.cjs
```

Nếu session chưa có, mở bằng CLI (đọc skill Playwright nếu dùng workflow này):

```powershell
npx.cmd --yes --package @playwright/cli playwright-cli -s=ssbu-gpu open http://127.0.0.1:8787/ --headed --browser=chrome
```

Helper reload page và POST báo cáo `/__report`; `browser-report.json` sẽ bị ghi đè. Archive từng bằng chứng trước lần chạy kế tiếp. `check-browser-clear-ui.cjs` còn hardcode counts cũ (3704/1053), phải cập nhật sau khi depth fixture hoàn chỉnh, không sửa expected để che lỗi.

Sau khi các sửa đang dở đã ổn, build runtime một lần và boot:

```powershell
& $taskPython web/prepare_runtime.py --main --pointer-tables
& $taskPython web/build_runtime.py --debug-graph --fibers --compact-codegen --jobs 2
& $taskNode web/test_runtime_node.mjs fibers-compact
```

Node test trên **chỉ constructor**, không chạy nnMain/GPU. Với browser `ssbu-web` đã mở và server có sẵn:

```powershell
& $taskPython web/advance_runtime.py --steps 10 --session ssbu-web --fibers --compact-codegen --pointer-tables --jobs 2
```

Lệnh advance boot thật với main + 14 file RomFS, lưu `build/runtime-progress/<timestamp>/`, chỉ thêm callback main đã quan sát và có native definition. Dừng ở lỗi SDK/GPU/host service. `--steps 1` chỉ quan sát boot, không tự thêm root ở bước cuối. Nếu cần mở session `ssbu-web`, dùng cùng cú pháp `open` như trên. Không dùng đồng thời hai script điều khiển cùng session.

Server chỉ cần khởi động nếu port chưa có listener:

```powershell
& $taskPython web/serve.py --port 8787
```

Server loopback, chỉ public `build/web-poc/` theo suffix cho phép, có COOP/COEP và no-store. Không đổi sang serve toàn repo/dump. URL `/` chọn `fibers-compact`; `?runtime=debug` là variant khác. UI chọn `main.elf`, RomFS local rồi “Thử boot game”; dùng WorkerFS/File, đọc một phần archive với offset 64-bit, không nạp cả 17 GB vào RAM.

## 7. Vì sao build chậm và các bẫy không được làm mất

Máy khoảng 16 GB RAM; graph hiện **45,163 guest function / 212.67 MB C** (sau khi cắt tỉa bảng nhảy `L_dispatch` giảm `-27.52 MB` C từ `240.19 MB`), Wasm trước Asyncify **103,146,504 byte (`98.36 MB`)**, Wasm sau Asyncify ~120–146 MB. Binaryen Asyncify toàn module là đoạn chậm, có thể nhiều phút kể cả object cache đầy đủ. Không báo ETA chắc chắn khi chưa đo.

- Dùng `--jobs 2` trên máy này. `build_runtime.py` mặc định vẫn 4, nên phải truyền rõ. `BINARYEN_CORES` được cap 2. Tăng compiler jobs không làm Asyncify song song tương ứng và dễ gây paging.
- Guest codegen dùng `-O1 -Xclang -disable-llvm-passes`; cache guest object chia sẻ giữa variant, host fiber tách riêng.
- `-sBINARYEN_EXTRA_PASSES=--optimize-level=1` cần thiết để coalesce local trước Asyncify. Bản optimize0 từng ra 117 MB/max 92,748 locals và Chrome từ chối; bản hiện tại max 1,742. Không bỏ guard numeric-local ≤50,000.
- Không dùng `ASYNCIFY_REMOVE`/`IGNORE_INDIRECT` tùy tiện để build nhanh: dễ sai luồng async/fiber.
- Fiber C/Asyncify/root stack phải `aligned_alloc(16, size)`. Emsdk malloc 8-byte từng gây assertion `readEmAsmArgs` sau migration. Pool pthread đã tăng lên 16 (`-sPTHREAD_POOL_SIZE=16`).
- Không khai báo mảng lớn (ví dụ `64 KB`) trên stack của các hàm được gọi từ pthread phụ: stack mặc định của pthread Wasm là `64 KB`; `web/host_fs.c` đã chuyển `staging[65536]` sang `static` dưới khóa `fs_lock` để giữ stack frame `< 2 KB`.
- `portable_fiber.c/.h`: pin Asyncify wrapper IDs xuyên worker, trampoline/keepalive và cleanup khi throw; TLS getter riêng translation unit để tránh cache sai TLS address khi chuyển fiber.
- Không thay Emsdk tùy tiện; `libasync.js` đang được adapter kiểm tra theo SHA `a2c2f495ab5f850d61e045b5c4dd4db66a0b4390b9b3e98d33059c38d6f00329`.
- Link ở `build/web-runtime/linking/<variant>/`, validate trước publish. Metadata chỉ xác nhận compiler/locals; phải boot riêng. Cặp JS/Wasm publish không atomic đối với fetch đồng thời: đợi build xong rồi reload.
- Browser timeout không chứng minh linker chết. Kiểm tra PID, CPU, log; không restart một linker còn chạy. Không kill Chrome/Node/Python theo tên chung.

Batch callback gần đây: `prepare_runtime.py` tìm contiguous relocation tables chứa callback **đã quan sát**, tối đa 256 entry, không recursive kéo mọi bảng, đồng thời dùng `prune_huge_dispatch()` cắt tỉa `L_dispatch` theo bảng nhảy `.rodata`. `--pointer-tables` đã gồm logic này. **Không bật `--all-pointer-tables` mặc định**: thử rộng từng kéo 75,923 function/~882 MB C.

## 8. Bản đồ code và bằng chứng đã có

| Nhóm | File / vai trò |
| --- | --- |
| Chuẩn bị / build graph | `web/prepare_runtime.py`, `runtime_roots.json`, `build_runtime.py`, `advance_runtime.py` |
| UI / worker | `web/app.mjs`, `runtime-worker.mjs`, `assets-worker.mjs`, `serve.py` |
| Guest memory / ELF / archive / string / sort | `web/guest_memory.c`, `runtime_memory.c`, `asset_io.c`, `guest_string.c`, `guest_sort.c`; các module ELF/loader trong `web/` |
| OS / FS / fiber / sync | `web/host_os.c`, `host_fs.c`, `host_runtime.c`, `host_fiber*`, `portable_fiber*`, `host_mutex*`, `host_semaphore*` |
| C NVN bridge | `web/host_gpu*.c/.h`, `host_nvn_layout.c` |
| GPU owner | `gpu-textures.mjs`, `gpu-depth.mjs`, `gpu-render.mjs`, các module `gpu-*.mjs` |
| Shader translator | `web/maxwell/`, `web/shader-compiler/`, `build_maxwell.py` |
| Báo cáo nền | `web/README.md`, `web/STATUS.md`; chi tiết có thể là lịch sử, phải đối chiếu timestamp/hash |

Các mốc dưới đây là bằng chứng thành phần đã đạt:

| Thành phần | Bằng chứng / giới hạn |
| --- | --- |
| Native baseline | 1,161 C unit, 98 NRO DLL; lifter 32,043/32,043, pad 49/49. Smoke ~150 s exit0, có input chiến đấu ~58.6 FPS native; không gán FPS này cho web, chưa xác nhận hết trận/audio |
| Game leaf Wasm | 96 lifted function / 384 A64 oracle case, Node+Chrome |
| Guest memory, OS, FS & Sort (`os_probe.c` / `probe.c`) | **161 checks** trong `os_probe.c` (`build/web-os/report.json`) + 57 checks trên `core32`/`core64` (TLB 131,072 trang `g_tlb_idx` + single-page fast-path cho `gm_read`/`gm_write`/`gm_span`, lazy zero-page commit 1 GiB, `gm_os_commit`/`gm_os_decommit` idempotent, `nn::nifm`/`account`/`hid`/`vi`/`ro`, `nn::util::DecompressZlib`, `nn::pl` shared fonts, `nn::fontll`, `nn::crypto::GenerateSha1Hash`, `nn::mii`, `movie::Buffer`, `std::__sort`) |
| Loader/archive | 3 segment+BSS ~33.6 MiB, 337,481 relocation; partial archive read qua 2/4 GiB và EOF |
| Host fiber | 324 checks Node+Chrome, variadic EM_ASM trước/sau migration; control sai alignment fail đúng |
| Mutex | 46,383 checks Node+Chrome, 1,536 key; adapter động thay hard limit 512, game thực đã tới 572 |
| Semaphore | 1,426 checks Node+Chrome; bounded count, wait/timeout/finalize; game thực 2 live |
| Runtime diagnostic | 68,444 formatter/canary checks; heap fault frame/TLS, không printf callback gây Asyncify lỗi |
| Sampler | 156 analytic check trên GPU, 17 sampler; các mode chưa hỗ trợ vẫn báo lỗi |
| Program snapshot/bind | Immutable snapshot, queue retain, null unbind; captured shader test có pipeline validation, **không dispatch/draw** |
| Color clear JS | 3,704 Chrome checks: mask/scissor, kiểu float/sint/uint, sRGB, RGB implicit alpha, layer/mip; `build/browser-color-clear-report.json` |
| Color clear + Depth C NVN | **1,181 Chrome checks, 0 failures** (`depthUploadPasses = 16`, `depthReadPasses = 8`, `stencilUploadWrites = 12`); `build/browser-nvn-depth-report.json` |
| Depth JS + Negative Batch | **652 Chrome checks, 0 failures** + 12 Node guard checks; `build/browser-depth-transfers-report.json` |
| Full UI (“Chạy kiểm tra”) | **PASS** (`render.checks = 3704`, `textures.checks = 1181`, `depth.checks = 652`); `build/browser-depth-ui-report.json` |

Color clear hiện có SetRenderTargets/Scissor/ClearColor variants, giữ resource lifetime và thứ tự copy/clear. Chỉ origin upper-left và scissor explicit; các trường hợp chưa hỗ trợ trap. Packet operation22 đọc được legacy56 byte và bản60 byte có view format. Chưa có đường draw game đầy đủ.

Shader translator: startup corpus 20/20 module chuyển được qua Maxwell→glslang→SPIR-V→Naga→WGSL; fixture tạo 5 compute pipeline+1 graphics diagnostic pipeline, chưa thực thi game draw. Corpus rộng64 chỉ35 thành công,29 unsupported rõ ràng (27 ViewportLayer,2 ImageQueryLod). Bind sampler=256+image để tránh trùng stage. Native có khoảng9,574 capture `.mxd`; không tuyên bố đã hỗ trợ hết.

## 9. Thứ tự tiếp tục được đề xuất

1. Đọc tài liệu này, kiểm tra Git/process/hash/report; không rebuild từ đầu hoặc extract lại dump. Lưu ý không chạm `web/guest_memory.h` hoặc `web/runtime_memory.h` nếu không thực sự cần thiết vì `web/build_runtime.py` đưa chúng vào hash stamp của toàn bộ `runtime_c*.c`. Luôn truyền `--debug-graph --fibers --compact-codegen --jobs 4` (hoặc `--jobs 2`) khi gọi `web/build_runtime.py` để tái sử dụng các file `runtime_c*-debug-compact.o` đã được lưu đệm.
2. Tại mốc `0x7100027bf0` (`build/browser-runtime-18837755-report.json`, sau khi worker thread `BCATCacheStorage` `0x71036f9a70` và phương thức ảo `0x71036f85d0` trong `BootSequenceScene` đã chạy thành công, tăng số mutex sống lên **`629`**), điểm dừng là `uncompiled indirect target 7100027bf0` (`lr = 0x7300000000`, `phase = guestThread`). Cần thêm `7100027bf0`, `7100027da0` và các con trỏ hàm GOT liên quan vào `web/runtime_roots.json`, chạy `prepare_runtime.py --main --pointer-tables`, `build_runtime.py --debug-graph --fibers --compact-codegen --jobs 4`, rồi chạy `advance_runtime.py --steps 1` trên Chrome/WebGPU.
3. Ghi nhận điểm dừng kế tiếp với hash/timestamp/caller/phase. Nếu gặp `uncompiled indirect target`, thêm definition thật vào `web/runtime_roots.json` (cùng bảng vtable liên quan nếu có); nếu gặp import chưa port hoặc lỗi GPU/SDK, implement đúng ngữ nghĩa native/SDK và bổ sung test vào `web/os_probe.c` hoặc `web/gpu_probe.c`.
4. Tiếp tục SDK services và lazy NRO loading (98 native DLL chưa được port đầy đủ), resource banks, command state, graphics/compute pipelines và draw/dispatch thật. Đây còn là phần lớn công việc.
5. Chứng minh frame game, menu rồi trận đấu; hoàn thiện presentation/input/audio/save; chỉ đo FPS sau khi có gameplay thực. Mỗi mốc lưu report và ảnh tương ứng.

Không có cơ sở hứa ngày hoàn tất full game. Hãy trao đổi theo mốc kiểm chứng và blocker cụ thể; người dùng ưu tiên làm thật, không chỉ lập kế hoạch hoặc báo “gần xong”.

## 10. Checklist bàn giao kết quả cho người dùng

- Phân biệt source mới, module đã build và module thực đã boot; ghi SHA Wasm cho mọi kết luận runtime.
- Nêu rõ fixture/Node/native/browser game; không cộng số checks khác nhau rồi coi như gameplay.
- Giữ log lỗi thật, fail fast với unsupported API; không dùng shader/asset/gameplay giả để che phần chưa port.
- Cập nhật `STATUS.md` sau mốc thực tế và liên kết bằng chứng trong ignored `build/`. Khi bàn giao máy khác cần chuyển evidence riêng.
- Chỉ commit source hợp lệ sau kiểm tra; không stage toàn thư mục build/dump/key.

**Điểm tiếp tục cụ thể nhất: thêm `7100027bf0`, `7100027da0` cùng các con trỏ hàm GOT liên quan vào `web/runtime_roots.json`, chuẩn bị và liên kết `runtime-fibers-compact.wasm`, xác minh `306/306` constructor trên Node, và đẩy `nnMain` / worker thread vượt qua mốc `0x7100027bf0` (`18,837,755` import calls, `629` live mutexes) trên Chrome/WebGPU.**
