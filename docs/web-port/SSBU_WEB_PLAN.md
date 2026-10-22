> Archived planning document; current handoff: [web/HANDOFF.md](../../web/HANDOFF.md).

# Kế hoạch thử port ssbu-recomp lên web

Ngày lập: 18/10/2026. Upstream cố định ban đầu: `938e66bb7be0ba4123e291211895412da421b705`.
Phân tích nền: [SSBU_WEB_FEASIBILITY.md](SSBU_WEB_FEASIBILITY.md).

**Tiến độ 18/10/2026:** đã triển khai nhánh `web-poc`, Emscripten 6.0.12 cục bộ,
server browser và prototype memory gắn vào helper runtime qua tùy chọn build.
Wasm32/Wasm64 ban đầu đều qua 43 kiểm tra trên Node và Chrome; fiber cùng luồng và
WebGPU compute/readback cũng qua. Sau khi có dump, đã thêm kiểm tra mã game thật
và bước khởi tạo trong browser như cập nhật bên dưới. Chưa hoàn tất
scheduler thật hoặc renderer NVN. Kết quả và giới hạn:
[web/STATUS.md](../../web/STATUS.md).

**Cập nhật dữ liệu:** đã có cả XCI base và NSP update 13.0.4 trong Downloads.
Đã ghép/trích xuất thành `ssbu-recomp/build/dump-13.0.4`, kiểm tra hash cả ba
segment của main và SDK đều OK. Differential lifter test: 32.043/32.043 khớp
Unicorn. Build native đã bắt đầu với 2 jobs tại `ssbu-recomp/build/native`,
đã build xong toàn bộ native với 4 jobs và qua SDK/pad/lifter self-test. Lượt
smoke đầu khởi tạo Vulkan, present 6 frame rồi dừng (exit 7) vì thiếu font hệ
thống Switch `nintendo_udsg-r_std_003.ttf`; chưa xác nhận menu/trận đấu. Đã trích đủ 7 font từ Firmware.21.1.0.zip và chạy lại: hiện được title cùng thông báo Milestones lần đầu. Đã sửa kịch bản qua thông báo, đi qua menu/chọn stage/chọn nhân vật và vào phần điều khiển trận theo log; đoạn 300 frame đo được 58,6 FPS native, chưa phải kết quả web. Báo cáo chi tiết ở
`ssbu-recomp/build/dump-validation/summary.json` và `ssbu-recomp/web/STATUS.md`.

## Mục tiêu và phạm vi

**Bản web hiện tại:** 96 hàm lấy từ dump thật (2.688 lệnh) đã chạy bằng
Wasm32/Wasm64, mỗi target khớp 384 case Unicorn AArch64 trên Node và Chrome.
Browser đọc mẫu `data.arc` 17,6 GB qua C stdio/WORKERFS mà không nạp cả file;
loader nạp 3 segment, kiểm tra toàn bộ BSS, bảo vệ read-only, xử lý 337.481
relocation và chạy xong `nninitStartup` với 3 host handler. Core fixture tăng
lên 50 check/target. Chưa chạy constructors/nnMain, scheduler hoặc renderer;
chưa có game chơi được trên web. Các bước kế tiếp: binding SDK, constructor
call graph, module loader, scheduler/exception rồi NVN/WebGPU. Lệnh tái hiện
ở `ssbu-recomp/web/README.md`; báo cáo ở `ssbu-recomp/build/web-poc/`.

Mục tiêu thử nghiệm cuối: Chrome desktop chạy trực tiếp một trận Mario vs CPU trên một stage cố định, điều khiển được, kết thúc bình thường, có số đo frame time và bộ nhớ. Giai đoạn đầu tắt movie/audio; thêm audio sau khi trận mẫu ổn định.

Mục tiêu hiệu năng là 60 fps, chưa phải cam kết. Phải công bố đúng mức đã đạt: compile được, boot được, render được hoặc chơi được. Mỗi mức có log và cách tái hiện riêng.

Chưa làm trong đợt thử: mobile/Safari, online, mod DLL, toàn bộ roster/chế độ chơi, quay video và giao diện launcher hoàn thiện. Đã cài toolchain web và thêm nhánh helper memory opt-in; native dependencies và runtime đầy đủ chưa được port.

## Máy thử hiện tại

- Intel Core Ultra 7 155H, 22 logical processors.
- RAM hệ thống khoảng 15,5 GiB.
- Intel Arc Graphics và NVIDIA RTX 4070 Laptop GPU; phải ghi adapter WebGPU thực tế lúc chạy.
- Ổ C còn khoảng 117 GiB tại thời điểm kiểm tra. Kiểm tra lại sau khi có dump, trước khi sinh code/cache.
- Git, Python và Node có trong PATH; Emscripten, clang và CMake chưa tìm thấy trong PATH. Cần dò dependency bằng probe trước khi quyết định cài; không suy ra toàn máy chưa có từ kết quả PATH.
- Bắt đầu native build với `--jobs 2`, đo peak RAM rồi mới tăng. Tránh dùng mặc định 22 jobs trên máy 16 GB RAM vì generated C có những file rất lớn.

## Dữ liệu cần để chạy game

Dump đúng SSBU **13.0.4**, base đã áp dụng update, theo bố cục upstream:

```text
C:\Users\Hikari\Desktop\wasm\local-data\ssbu-13.0.4\
  exefs\
    main
    sdk
    rtld
    subsdk0
    main.npdm
  romfs\
    data.arc
    ... các file RomFS còn lại ...
  sysdata\                 # tùy chọn theo BUILDING.md
    fonts\
    zoneinfo\
```

Có thể giữ dump ở ổ khác và truyền đường dẫn cho build. Ưu tiên ExeFS/RomFS đã giải nén để giảm số bước chẩn đoán. Nếu dữ liệu hiện có là gói NSP/XCI thì trước tiên xác định dạng và phiên bản, rồi chọn đường trích xuất phù hợp; không coi một file base bất kỳ là dump đã sẵn sàng.

Giữ dữ liệu đầu vào ngoài checkout. Các dữ liệu sinh ra, binary chứa game, shader, save và cache nằm trong đường dẫn local/build bị git ignore; trước commit phải kiểm tra file được stage. Không đưa chúng lên repository hoặc hosting thử nghiệm.

## Mốc 0 — Chuẩn bị và kiểm chứng nhỏ, chưa cần dump

- [x] Tạo nhánh làm việc cục bộ, giữ upstream commit đã ghi làm mốc so sánh (`web-poc`).
- [ ] Dò compiler, Windows SDK, native dependencies, browser và công cụ build hiện có bằng các probe nhỏ. Pin phiên bản sau khi probe chạy thành công.
- [x] Chuẩn bị Emscripten trong thư mục dependency cục bộ; thử C → Wasm trên browser, kiểm tra wasm64, pthreads, cross-origin isolation và WebGPU adapter.
- [x] Tạo runner local với COOP/COEP, thu log, lưu browser/compiler/GPU cùng kết quả mỗi lần chạy. Chỉ phục vụ output web và fixture công khai, không expose thư mục dump qua HTTP.
- [x] Tạo fixture CPU tự viết, không lấy code game: load/store 8–128 bit, arithmetic, indirect call và guest stack ở địa chỉ cao qua helper runtime.
- [x] Thử ánh xạ địa chỉ guest cao vào buffer nhỏ, alias, boundary crossing và lỗi truy cập; ghi thời gian đọc/ghi của fixture (chưa suy ra hiệu năng game).
- [x] Tạo phép thử fiber/continuation nhỏ: nested suspend/resume, TLS, typed callback, longjmp và explicit task state đổi worker. Guest C++ unwinder và coroutine thật vẫn chưa được kiểm chứng.

**Điều kiện qua:** compiler/browser probes chạy được, CPU fixture cho kết quả đúng và xác định được một thiết kế scheduler có thể biểu diễn semantics cần thiết. Fixture chỉ là kiểm chứng kỹ thuật, chưa phải game đã chạy.

**Quyết định sớm:** ưu tiên thử wasm64; chỉ đổi wasm32 khi có số đo và audit ABI. Không giữ nguyên base guest làm offset linear memory. Không mặc định thay `SwitchToFiber` bằng Emscripten fiber là đủ: API hiện tại không cho fiber chạy tiếp trên thread khác.

## Mốc 1 — Baseline native, cần dump

- [ ] Kiểm tra layout, phiên bản/hash build và file bắt buộc bằng tool upstream trước khi lift.
- [ ] Build nguyên upstream, chưa trộn PR cộng đồng hay thay runtime. Bắt đầu `--jobs 2`; lưu phiên bản tool, lệnh build, peak RAM và log.
- [ ] Chạy các self-test/lifter differential test của upstream. Ghi từng failure hoặc trường hợp không hỗ trợ.
- [ ] Chạy menu → trận 1v1 → kết thúc bằng input script có sẵn. Khóa fighter, stage, seed và phiên bản dữ liệu.
- [ ] Chạy lại cấu hình đó, thu state hash theo frame, CPU/frame time, RAM, loading và ảnh các mốc. Dùng userdata test riêng để việc save không làm lệch hai lượt chạy.
- [ ] Đo reserve so với committed/working-set memory và dải địa chỉ allocator thực sự dùng. Đây là dữ liệu chọn chiến lược memory cho web.

**Điều kiện qua:** native trên chính máy này chạy hết trận mẫu và tái hiện được baseline. Nếu baseline lỗi, giải quyết ở đây trước; không quy mọi lỗi sau đó cho Wasm.

Lệnh dự kiến, sau khi xác định đủ đường dẫn dependency:

```powershell
python scripts/setup.py --dump <dump-folder> --suyu <source-folder> --jobs 2
```

Không coi đây là lệnh đã chạy hoặc đã đủ dependency.

## Mốc 2 — Tách memory và scheduler, kiểm tra native trước rồi Wasm

- [ ] Định nghĩa `GuestAddr` 64-bit, API dịch địa chỉ, copy span, map/reserve/commit/free và chuyển con trỏ host/guest rõ ràng.
- [ ] Inventory toàn bộ cast guest → host: runtime helpers, strings/libc, SDK, FS, loader, exception, renderer và host pointers lưu vào object của game.
- [ ] Có fast path cho vùng liên tục và đường xử lý vùng/trang phân mảnh; không trả raw pointer cho span chưa được chứng minh liên tục.
- [ ] Giữ reserve ảo riêng với backing thật. Theo dõi peak backing, lifetime và lỗi truy cập. Chọn layout từ số đo mốc 1.
- [ ] Chạy native với lớp memory mới, đối chiếu baseline trước khi thêm khác biệt compiler/browser.
- [ ] Kiểm toán memory ordering của instruction lift và atomics/fences trên LLVM mới; tham khảo PR #20 nhưng không merge cả nhánh chỉ để lấy một sửa đổi.
- [ ] Chọn scheduler từ fixture mốc 0 và trace native: có thể giữ task trên worker cố định chỉ khi chứng minh đúng semantics, hoặc biểu diễn continuation/frame trong code lift.
- [ ] Đảm bảo TLS, exception/unwind và callback đúng sau yield/resume. Kiểm tra các task chờ lẫn nhau, không chỉ một coroutine độc lập.
- [ ] Compile các cụm code game vừa đủ sang Wasm, chạy và so state với native. Giữ cùng code generator/config giữa hai bên.

**Điều kiện qua:** memory và task semantics đúng qua test mục tiêu; không deadlock; mọi khác biệt hash được giải thích hoặc sửa. Nếu thiết kế này không đứng vững, sửa thiết kế tại đây trước khi mở rộng renderer.

## Mốc 3 — Boot runtime web và đọc asset

- [ ] Thay Win32 primitives cần cho boot bằng adapter web/pthreads; game loop và các lệnh chờ chạy trong worker.
- [ ] Static link module cần cho đường đi mẫu; guest module load/unload vẫn quản lý dữ liệu/relocation/lifetime đúng. Bổ sung module chung hoặc dependency khi thực tế cần.
- [ ] Adapter FS đọc theo offset từ file người dùng chọn hoặc OPFS; dùng buffer/cache có giới hạn, tránh nạp toàn bộ `data.arc` vào linear memory.
- [ ] Tách save test với asset chỉ đọc. Xác minh commit/persistence qua reload khi bắt đầu bật lưu bền.
- [ ] Bỏ movie qua semantics end-of-stream phù hợp và tắt audio ban đầu; không để vòng chờ movie/audio chặn tiến trình.
- [ ] Dùng display/NVN stub tối thiểu để quan sát mốc boot, event và tiến trình chuẩn bị menu. Bổ sung tín hiệu/fence cần thiết thay vì no-op toàn bộ API.

**Điều kiện qua:** runtime đi tới mốc chuẩn bị menu trong log, không treo chờ I/O/fence, có số đo startup và high-water RAM. Chỉ gọi là “menu hiện lên” sau khi có render thật ở mốc 4.

## Mốc 4 — WebGPU: shader thật → menu → trận

- [ ] Tách frontend NVN khỏi header Win32/OpenGL đủ để dùng lại command recording, resource và job queue.
- [ ] Thêm backend `nvn_webgpu`, quản lý buffer/texture, bind groups, pipeline, render/compute pass, submit/present và đồng bộ.
- [ ] Giữ decode Maxwell và phân tích resource hiện có; thử sinh WGSL hoặc qua IR được kiểm soát. Hạ integer 64-bit xuống cặp `u32` khi cần, sửa layout/binding/validation theo WebGPU.
- [ ] Dùng shader/resource thật từ dump cục bộ: kiểm tra vertex/fragment trước, compute và readback tiếp theo. So ảnh/kết quả với native.
- [ ] Dirty tracking dùng software notification hoặc hash có giới hạn; bao phủ cả guest stores, host writes và texture aliasing.
- [ ] Render menu, sau đó một stage và hai fighter cố định. Cache pipeline theo phiên bản shader/backend; ghi rõ lỗi unsupported thay vì âm thầm bỏ draw.

**Điều kiện qua:** menu và trận mẫu hiển thị đúng qua ảnh mốc, không có WebGPU validation errors chưa giải quyết, resource lifetime/readback đúng. Renderer clear được canvas chưa tính là mốc này hoàn thành.

## Mốc 5 — Bản chơi thử và quyết định tiếp tục

- [ ] Nối keyboard/gamepad, pause/resume và thoát trận. Chạy manual cùng input script.
- [ ] Thêm Opus/audio output bằng backend web; đo underrun và đồng bộ, kiểm tra khởi động audio từ thao tác người dùng.
- [ ] Chạy trận mẫu 3.600 fight frames, so hash theo từng frame với baseline. Ghi first divergence, không che sai khác bằng cách bỏ trường hash tùy tiện.
- [ ] So hình và âm thanh riêng vì state hash không chứng minh render/audio đúng.
- [ ] Đo cold/warm startup, Wasm download/compile/instantiate, RAM cao nhất, frame time median/p95/p99, stall và chi phí CPU/GPU.
- [ ] Chạy ít nhất 10 phút và chuyển scene/load-unload lặp lại; kiểm tra leak, deadlock, thiết bị GPU bị mất và trạng thái save.

**Đầu ra:** URL local, hướng dẫn chọn dữ liệu, lệnh tái hiện, log, ảnh và bảng kết quả trên máy này. Kết quả có thể là chạy đúng nhưng chưa đạt 60 fps; khi đó dùng profile để quyết định tối ưu hay thu hẹp phạm vi.

## Quy ước nghiệm thu và thứ tự ưu tiên

| Chứng minh | Cần bằng chứng |
|---|---|
| Biên dịch được | Build log + khởi tạo module trên browser |
| CPU/runtime đúng | Differential tests, guest state, memory/TLS/exception tests |
| Boot được | Log đạt mốc xác định, không dùng timeout làm bằng chứng thành công |
| Render được | Draw thật, ảnh so native, kiểm tra compute/readback |
| Chơi được | Input thật, trận hoàn tất, không crash/deadlock |
| Đạt mục tiêu hiệu năng | Frame-time và bộ nhớ đo trên cấu hình ghi rõ |

Ưu tiên đầu tiên khi triển khai: fixture memory + fiber song song về mặt chuẩn bị công việc, sau đó baseline native khi dump sẵn sàng. Không bắt đầu viết toàn bộ WebGPU renderer trước khi hai rủi ro CPU này có hướng giải quyết được kiểm chứng.

Chưa chốt số ngày cho toàn bộ port. Sau mốc 0–2 mới có cơ sở ước lượng phần còn lại. Nhánh Linux cộng đồng có thể tham khảo để tách Win32, nhưng không thay cho kiểm chứng bộ nhớ và scheduler trong browser.
