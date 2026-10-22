> Archived planning document; current handoff: [web/HANDOFF.md](../../web/HANDOFF.md).

# Khả năng port ssbu-recomp lên trình duyệt

Đánh giá ngày 18/10/2026. Đã đọc mã nguồn tại commit `938e66bb7be0ba4123e291211895412da421b705` của [Denkishi/ssbu-recomp](https://github.com/Denkishi/ssbu-recomp). Bản clone nằm ở `ssbu-recomp/` cạnh báo cáo này; không sửa mã nguồn upstream.

## Kết luận

Có cơ sở kỹ thuật để làm một bản thử nghiệm chạy trực tiếp trên trình duyệt desktop bằng WebAssembly + WebGPU. Tuy nhiên, đây là một dự án port runtime đáng kể, chưa phải việc đổi compiler hoặc thêm HTML. Chưa có bằng chứng rằng toàn bộ game sẽ chạy đúng và đạt 60 fps trên web.

Ba phần cần kiểm chứng trước khi cam kết làm bản đầy đủ: ánh xạ bộ nhớ của game, fiber chuyển giữa các luồng, và chuyển shader sang WebGPU. Mục tiêu đầu tiên hợp lý là một trận 1v1 cố định trên Chrome desktop, bỏ movie và mod động để giảm phạm vi. Mobile nên để sau khi đã đo bản desktop.

Đây là đánh giá từ mã nguồn và tài liệu, không phải kết quả chạy thử game. Workspace chưa có dump game, mã C sinh từ dump hoặc Emscripten trong PATH; chưa build native/Wasm và chưa đo RAM/FPS.

## Những phần có thể tận dụng

- Lifter đã dịch AArch64 sang các hàm C với chữ ký `void f_<addr>(Ctx *c)`. Thanh ghi giữ trong `Ctx`, không phải xây lại bộ giả lập CPU để bắt đầu thử nghiệm. Xem [recomp.py](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/tools/lifter/recomp.py) và [runtime.h](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/runtime.h).
- Có sẵn lớp thay thế thư viện hệ thống, bộ dịch shader Maxwell, giải mã texture, và kiểm tra instruction so với Unicorn.
- Có input script, fixed seed và hash trạng thái theo frame để so sánh hành vi giữa bản native và bản web. Đây là nền tảng xác minh tốt, nhưng không thay thế kiểm tra render/audio.
- Frontend NVN đã có command list và cấu trúc dùng chung giữa hai backend. Có điểm để gắn backend WebGPU, dù frontend vẫn nằm nhiều trong `nvn_gl.c` và còn phụ thuộc Win32/OpenGL qua `nvn_front.h`.

README báo cáo đã chơi được một số chế độ trên Windows; đó là kết quả của tác giả, chưa được xác minh độc lập trong lần nghiên cứu này. [Nguồn](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/README.md).

## 1. Bộ nhớ: rào cản nền tảng

`runtime.h` ghi rõ giả định địa chỉ guest chính là con trỏ host. `rd*`, `wr*`, `rdv`, `wrv` và CAS đều ép địa chỉ 64-bit sang con trỏ rồi đọc/ghi trực tiếp. `hostrt.c` dùng `VirtualAlloc` tại `GUEST_BASE = 0x7100000000`; các vùng module/import/SDK còn nằm ở những base khác.

Trình duyệt không cung cấp cơ chế ánh xạ các vùng địa chỉ này giống Win32. `0x7100000000` là offset 452 GiB nếu giữ nguyên trong linear memory. Con số này **không phải lượng RAM game đang dùng**; nó cho thấy vì sao không thể giữ nguyên mô hình địa chỉ khi chuyển sang Wasm.

Memory64 đã có trên Chrome từ bản 133, và Emscripten hiện có target wasm64 (`-m64`). Vì vậy, kết luận “web chỉ có tối đa 4 GiB nên không thể làm” là không đủ chính xác. Nhưng memory64 không tự tạo vùng ánh xạ thưa kiểu `VirtualAlloc`, cũng không tự chuyển đổi địa chỉ guest thành vị trí dữ liệu trong linear memory. [Chrome](https://developer.chrome.com/release-notes/133), [Emscripten](https://emscripten.org/docs/tools_reference/settings_reference.html#memory64).

Ngoài base cao, `nnsdk.c` dùng reserve/commit/decommit cho allocator của game; `r1_state.c` mô tả vùng reserve từ 16 GB trở lên. Reserve là không gian địa chỉ, chưa chứng minh working set cần chừng đó RAM. Cần đo cả hai, vì Wasm không có cùng mô hình commit từng trang của Windows.

Hướng thử:

1. Giữ địa chỉ guest là `uint64_t`, tạo lớp chuyển địa chỉ và quản lý vùng/trang được backing bởi linear memory.
2. Thay tất cả chỗ chuyển guest pointer sang host pointer, bao gồm string, `memcpy`, libc, FS, shader/texture và exception metadata. Chỉ sửa `rd64/wr64` là thiếu.
3. Xử lý buffer đi qua ranh giới mapping; không mặc định mọi khoảng guest liên tục đều trở thành một con trỏ host liên tục.
4. Cân nhắc wasm64 cho prototype để giảm khác biệt kích thước con trỏ/ABI của runtime. Chỉ chọn wasm32 khi đã đo footprint và kiểm toán các struct/slot con trỏ 64-bit mà game thấy.

[Nguồn bộ nhớ](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/hostrt.h), [loader](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/hostrt.c), [allocator](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/nnsdk.c).

## 2. Thread, fiber và exception

`os_threads.c` dựa vào Win32 threads, condition variables, TLS và `CreateFiberEx`/`SwitchToFiber`. Có thể thay phần thread bằng pthreads trên Web Workers, nhưng fiber cần xử lý riêng.

Đáng chú ý: fiber của game có thể dừng trên một host thread và tiếp tục trên thread khác. [PR #43](https://github.com/Denkishi/ssbu-recomp/pull/43) mô tả lỗi thực tế do hành vi này. Trong khi đó, fiber API hiện tại của Emscripten dùng Asyncify và không cho tiếp tục fiber đã chạy trên thread khác. Vì vậy không thể thay Win32 fiber bằng `emscripten_fiber_swap` một cách máy móc. [Tài liệu fiber](https://emscripten.org/docs/api_reference/fiber.h.html).

Hai hướng cần thử: giữ fiber trên worker cố định nếu tái tạo đúng semantics của task system, hoặc thay đổi lifter để biểu diễn continuation/frame rõ ràng thay vì phụ thuộc host call stack. Hướng sau tác động nhiều hơn nhưng cho quyền kiểm soát scheduler. Cả hai đều cần kiểm tra deadlock, TLS, exception và thứ tự công việc.

`nnunwind.c` đã tự đi qua guest unwind metadata và dùng `setjmp/longjmp` để thoát về handler. Có thể tận dụng logic guest, nhưng phải xác minh với fiber và cơ chế longjmp của Emscripten.

Pthreads trên web yêu cầu cấu hình COOP/COEP phù hợp. Nên chạy game ở worker để các lệnh chờ không chặn UI. [Tài liệu pthreads](https://emscripten.org/docs/porting/pthreads.html).

## 3. Đồ họa: nên thử WebGPU

OpenGL backend hiện dùng compute, image load/store, texture views, GLSL 4.60 và bindless texture. Shader translator phát cả extension desktop/NVIDIA. WebGL 2 dựa trên OpenGL ES 3.0, nên không thể dùng nguyên backend này. [nvn_gl.c](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/nvn_gl.c), [mx_translate.c](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/mx_translate.c), [Emscripten/WebGL](https://emscripten.org/docs/optimizing/Optimizing-WebGL.html).

WebGPU phù hợp hơn vì có compute và storage buffers/textures. Tuy nhiên, nó không nhận nguyên Vulkan API hoặc SPIR-V của backend hiện tại. Cần viết backend NVN → WebGPU và đường shader → WGSL tương thích.

Phần có thể tận dụng là decode Maxwell, texture decoding và phân tích resource của translator. Đường Vulkan hiện đã thay một số bindless handle bằng binding tường minh, hữu ích để tham khảo.

Các điểm phải xử lý:

- GLSL hiện sinh phép toán `uint64_t`/`int64_t`; WGSL chưa có kiểu số nguyên 64-bit tổng quát. Cần hạ các phép này xuống cặp `u32` khi gặp, không chỉ đổi cú pháp shader. [WGSL](https://gpuweb.github.io/gpuweb/wgsl/#host-shareable-memory-layout).
- Kiểm tra số texture, sampler, storage buffer, alignment, texture format, đồng bộ và readback theo khả năng thực tế của adapter. Không mặc định giới hạn Vulkan áp dụng nguyên trên WebGPU. [WebGPU](https://gpuweb.github.io/gpuweb/).
- Thay dirty tracking bằng Windows `GetWriteWatch`. Repo có fallback hash texture và có sẵn `R1_MARK` để đánh dấu guest store, nhưng chưa phải giải pháp web hoàn chỉnh: các ghi từ host cũng phải được theo dõi, cần xử lý concurrency và đo overhead.

Rủi ro render lớn nhất là đúng semantics và hiệu năng đồng bộ CPU/GPU; hiện chưa có benchmark web để kết luận.

## 4. DLL, asset và media

`nnro.c` tải module game từ DLL bằng `LoadLibraryA`/`GetProcAddress`. Bản web cần static link hoặc Wasm side modules. Prototype nên static link những module cần cho trận mẫu và giữ cơ chế guest load/unload ở mức dữ liệu; sau đó mới nghiên cứu tải module theo nhu cầu. Dynamic linking kết hợp pthreads vẫn được Emscripten ghi là experimental. [nnro.c](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/runtime/nnro.c), [dynamic linking](https://emscripten.org/docs/compiling/Dynamic-Linking.html).

Build native được tác giả ghi nhận sinh 112.890 hàm main, 1,3 GB C và DLL main 511 MB. Đây là dấu hiệu cần đo thời gian compile/load và chia module; **không phải ước lượng dung lượng Wasm**. [BUILDING.md](https://github.com/Denkishi/ssbu-recomp/blob/938e66bb7be0ba4123e291211895412da421b705/BUILDING.md).

Asset nên đọc theo offset từ file người dùng chọn hoặc storage trong trình duyệt; tránh nhét toàn bộ `data.arc` vào RAM/MEMFS. Save cần adapter lưu bền và giữ semantics commit. Sync access handle của OPFS hoạt động trong dedicated worker, có thể là một hướng triển khai, nhưng cần nhập/copy dữ liệu và quản lý quota. [FileSystemSyncAccessHandle](https://developer.mozilla.org/en-US/docs/Web/API/FileSystemSyncAccessHandle).

Audio Win32 và movie Media Foundation cần backend khác. Có thể thử libopus biên dịch Wasm với AudioWorklet; movie bỏ qua trước rồi bổ sung WebCodecs hoặc decoder Wasm khi đã có trận mẫu. SDL3 input có thể tận dụng một phần, vẫn phải kiểm tra backend web và tay cầm thật.

Repo không chứa game/assets hoặc generated C. Quy trình thử vẫn cần dump 13.0.4 của người dùng và bước sinh/biên dịch cục bộ; chưa có pipeline nhập dump rồi chơi ngay hoàn toàn trong browser.

## Cập nhật cộng đồng đáng tham khảo

[Issue #40](https://github.com/Denkishi/ssbu-recomp/issues/40) giới thiệu nhánh Linux native: tác giả báo cáo đã boot, chạy menu và đánh trận với âm thanh. Nhánh dùng lớp tương thích Win32 trên POSIX, SDL3, Vulkan, FFmpeg và ucontext. Có thể tham khảo cách tách các phụ thuộc Windows; nó chưa giải quyết mô hình địa chỉ/fiber/renderer cho web. Các kết quả trong issue chưa được kiểm chứng ở đây và chưa phải tính năng đã merge vào main.

[PR #20](https://github.com/Denkishi/ssbu-recomp/pull/20) còn ghi nhận việc chuyển compiler mới và tối ưu mạnh làm lộ thiếu sót memory ordering của lifter. Đây là lý do phải kiểm toán LDAR/STLR/LDXR/STXR và fences khi dùng LLVM/Emscripten mới, kể cả sau khi build thành công. PR đang mở tại thời điểm kiểm tra; không coi các sửa đổi và số FPS đó là kết quả của main hoặc web.

## Thứ tự làm prototype

1. **CPU và memory trước:** biên dịch tập nhỏ code lift sang Wasm; kiểm tra loads/stores, CAS/fences, gọi gián tiếp, guest/host pointers, stack/TLS và kết quả số học với native/Unicorn. Đo chi phí dịch địa chỉ.
2. **Scheduler:** tái hiện task fiber chuyển thread, TLS và throw khi hủy coroutine. Chỉ tiếp tục khi không deadlock và hành vi đúng.
3. **Boot không render:** đọc image/module/asset qua adapter web, chạy tới mốc menu với log. Đo high-water RAM, committed pages, Wasm size và thời gian khởi động.
4. **WebGPU:** đầu tiên một cặp shader và draw thật, sau đó menu, cuối cùng trận 1v1 cố định. So ảnh, texture/compute readback và lỗi validation.
5. **Đo trận:** dùng input script/fixed seed, đối chiếu hash trạng thái theo frame với native; đo frame time, audio underrun, bộ nhớ và phiên chơi dài. Sai biệt float/NaN cũng phải kiểm tra vì Wasm không mặc định tái tạo mọi chi tiết AArch64.

Điều kiện để quyết định làm bản đầy đủ là CPU/memory/fiber đúng, render trận mẫu đúng và chi phí mỗi frame có dư địa cho mục tiêu 60 fps (16,7 ms). Chưa có đủ dữ liệu để hứa thời gian hoàn thành hoặc tốc độ trên máy người dùng. Đánh giá hiện tại: đáng thử proof of concept trên desktop; chưa thể coi là port nhanh hoặc cam kết chạy đầy đủ trên web/mobile.
