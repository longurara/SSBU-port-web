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
