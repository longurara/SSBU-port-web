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

> **25/10/2026: SDK AV đã nối và PASS; full build dd968d5 đang chạy PID25188.**
> Đa-track Extractor giữ cursor/pending chung, SDK Vorbis PCM/audio metadata/events/64
> outputs. Fixture SDK thật: 120video +202PCM từ203audio packets, tất cả hash/time khớp
> direct, giữ đủ48video/64audio, EOS1 mỗi decoder, teardown sạch. Nonzero PCM367279bytes.
> Full actual opening108813844bytes OPEN cả3 tracks PASS; track0+2 routing đúng.
> Node/strictC11/OS182/video-only240+concurrentDtor PASS. codex-movie-sdk-av-120.json.
> Build bắt đầu2026-10-25T16:05:41Z, wasm-opt63736 đang chạy; --fibers --compact-codegen
> --jobs8, optimize-level1/validation giữ nguyên. State/log codex-av-dd968d5-build*.
> Không restart theo log im lặng. Game vẫn5c88/clock0, giữ nguyên đến khi AV publish.
> Tiếp receipt/hash→boot AV→actual audio clock/title/menu/match. Goal chưa hoàn tất.
>
> **25/10/2026: Vorbis JS adapter đã qua stream thật, còn nối backend/SDK audio.**
> movie-audio.mjs giải mã PCM s16 thật, xử lý priming và giữ output reservation64.
> Node bounds/lifetime/errors PASS; 255 PCM hash/time khớp raw AudioDecoder.
> Track3 thật 6000 packets→5999 PCM/5908 có tín hiệu, đầy queue64 rồi chạy tiếp,
> không còn reservation sau drain. Report codex-movie-audio-adapter-track3-6000.json.
> Chưa đổi MovieBackend/Decoder C hoặc rebuild. Tiếp multi-track + SDK audio/events.
> Game5c88 còn giữ nguyên, clock audio0 vẫn chặn title; chưa menu/match.
>
> **25/10/2026: build5c88 xong; actual game video EOF/EOS được, kẹt audio clock0.**
> SHA5c88f7a92ccc717481213c2de4b282344f7470890828efb231142fcbcda32e57; 282924589bytes,
> 100259functions/maxLocals10812. Build PID61344/wasm-opt22848 đã kết thúc, không restart.
> Game mới session ssbu-web/306ctors/33workers đang chạy; KHÔNG reload. Chưa menu/match.
> Actual opening hash0x1dcc80344b,108813844bytes,129.799s,VP9+2Vorbis. Video3890 packets,
> 3891 output acquire/release gồm EOS thật1, COPY1945, không còn held/queued video.
> Player còn sống; title root80545a3900 vẫn state2/callback1865010. f36e26c0 chọn
> audio không-null: audio80487f5480+264=0, trong khi video8048845680+168=129733000us.
> Vì Vorbis unsupported, audio clock không tiến, gate không mở. Không ép clock/pointer.
> AudioDecoder browser hỗ trợ Vorbis với actual private4344bytes. Probe256 packet thật
> →255 PCM s16 blocks/163 có tín hiệu, không lỗi. Evidence codex-movie-audio-decode-256.json.
> Tiếp real Vorbis decode + đa-track Extractor + SDK audio/events, fixture trước rebuild.
> web/HANDOFF.md và web/MOVIE.md có địa chỉ/evidence chi tiết. Goal ACTIVE.
>
> **25/10/2026: SDK Decoder + OS events đã qua 240 frame thật; đã nối source vào runtime.**
> host_movie_decoder.c triển khai vtable 0..18, guest input allocations/output
> lease/NV12/Stop/Reset/Flush/EOS; JS notify dùng key OS event và owner ID tăng dần.
> Production OS router fixture: 240 hash/time khớp direct, 48 output giữ, EOF/EOS=1,
> late events/format/owned buffers/capacity/flush/start/stop và concurrent Dtor khi
> request còn chờ JS đều PASS. Teardown không còn handle/guest allocations/objects.
> Report build/codex-movie-sdk-decoder-240.json. OS probe 182 PASS; C11 syntax PASS.
> Runtime worker và full build đã nối codec/bridge/backend, bridge dùng gnu11.
> Full build PID 61344 đã chạy lúc 2026-10-25T12:29:37Z và chuyển sang link, flags
> --fibers --compact-codegen --jobs 8, giữ optimize-level1/validation. State/log:
> build/codex-movie-dc49f05-build-state.json và build/codex-movie-dc49f05-build{,.err}.log.
> Kiểm tra process thật, không restart theo log im lặng. Sau publish mới boot thật.
> Game hiện vẫn Wasm 030a cũ, chưa menu/match. Vorbis/H264 unsupported.
>
> **Cập nhật 25/10/2026: Decoder JS qua kiểm tra hàng đợi bằng 240 frame thật.**
> Giữ chỗ output trước decode và qua lease, phân tích hidden frame/superframe,
> tách coded frame trước WebCodecs để không mất ảnh trong packet nhiều output.
> CONFIGURE time=8 bật 8 input slot; INPUT_AVAILABLE=29 trả slot khi codec đã nhận,
> STATUS=30 trả lỗi. Output giới hạn 48; EOS drain không chặn Release trong chain.
> Fixture hữu hạn chứa 240 packet VP9 nguyên payload đạt 8 input chờ/48 output giữ,
> 240 hash+timestamp khớp direct decode, EOS thật=1, handle/reservation=0.
> Node queue/lifetime/reset/stale-drain tests PASS; SDK Extractor 12-frame vẫn PASS.
> Reports build/codex-movie-{direct,queue}-240.json; chi tiết web/MOVIE.md.
> CHƯA Decoder C SDK/events/runtime wiring/full build. Game bản 030a vẫn chưa menu/match.
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
# Verified status — 21 October 2026

## Guarded font-build browser follow-up is now active — do not race it

New observer PID158588, exec75657, script build/codex-font-browser-followup.py,
state build/codex-font-browser-followup.json. It watches the existing coordinator
PID178424 through a Windows synchronization handle (no compiler launches).
Initial read-only heartbeat independently confirms boot23 active, expected
f40835ac worker SHA,306ctors/5602s. It sends a bounded page heartbeat every180s
while waiting, keeping the dedicated browser session alive.

Only after coordinator ready-for-font-browser-boot, it independently checks
actual changed Wasm SHA, served metadata, exact4GiB/306Node reports, and frozen
protected/graph/font source hashes. Then archives old live/screenshot/reports,
stops archived boot23 via its existing Stop button, and boots the real font fix
once with all7 local font inputs. Retries diagnostic installation only, never
reboots. Captures progress/arena evidence for twelve20s intervals, preserving
the new runtime for visual/menu/input follow-up. A failure stops the observer;
it does not retry a failed game boot or rebuild. Do not manually boot/stop/reload
while this observer is active. Inspect its current state before the next action.

Prepared build/codex-read-font-arena.cjs uses the existing bounded GuestMemory
reader (installer already discovers the unique object, not a fixed C address).
It scans only the private0x7c00000000/128MiB font arena for checked SFNT headers
and live GlyphMap-shaped allocations. Baseline on old runtime correctly reports
arenaReserved:false. This is not evidence about the pending new game yet.
Evidence build/codex-font-arena-old-baseline.log. Reads are bounded and hold the
existing map_lock only while copying; zero/busy/transient results are not proof
of absent font calls. New actual font-arena observation will be saved separately.

If the human asks to stop/pause, cancel this observer first (create
build/codex-font-browser.cancel or stop only PID158588) to prevent later automatic
browser mutation, then honor the requested compiler/goal handling. Do not rerun
the observer/coordinator just to inspect progress. A cancel file is checked
between stages and every15s while waiting; current goal has no pause request.

Optimizer143728 still live, CPU3520.94s at last check, not a failed/abandoned job.
Coordinator178424, builder145152 and emcc143808 unchanged; exec42509 pending.
The original coordinator still stops after checks; the new observer handles
the subsequent guarded browser transition. These notes supersede earlier
instructions to manually boot immediately when the coordinator becomes ready.

## Actual Chrome font File/WORKERFS + runtime pthread path passes

Added web/test_font_browser.py to compile the same production font C adapter
and host_font_probe into an isolated opt-in browser fixture. System fonts and
the known GPOS variant are selected as local File inputs; the variant remains
under build/font-browser/, outside the served directory. No font uploads.

First owner-worker test passed. Then tested the actual runtime threading path:
PROXY_TO_PTHREAD=1, pool3, C main executes on a pthread, FILE/WORKERFS operations
proxy to the owner worker; two additional pthreads stress glyph lifetimes.
Chrome reports crossOriginIsolated:true and the complete1513-import/700-cycle
fixture passes, including real shared fonts and reentrant GPOS callbacks.
Evidence build/font-browser/{report.json,build.json,build.log,run.log}.
Report hashes local fixture publication and C inputs, not independently fetched
Wasm in the browser. This remains adapter evidence, not a real game boot.
To reproduce: run test_font_browser.py, open the printed explicit index.html URL
in a separate CLI session, run build/font-browser/run.cjs, save output to run.log,
then test_font_browser.py --verify. Server intentionally requires the filename;
/font-probe/ without index.html returns403. Dedicated ssbu-font browser was
closed after verification; game ssbu-web was preserved.

Full font build still live: coordinator178424/builder145152/emcc143808/
optimizer143728. Last optimizer CPU1582.28s, working set643674112bytes;
actual Asyncify command verified, no optional pass-debug enabled. No new
publication/constructor/browser-game result yet. Keep waiting on the same
exec42509/processes; do not restart based on an unchanged log/state file.

Boot23 independently observed at5138s (~85.6min), same f40835ac runtime:
heap1324744704,414543snapshotsCreated/150live, registry242/reclaims763096,
699428draws/29402presents. Screenshot inspected: black. Live observation saved
as build/codex-boot23-font-build-live.json/.log. No menu/match input response proven.

## Real-font guest adapter integrated; full font build currently running

host_font.c now handles real nn::pl inputs and all existing fontll import names
through GuestMemory, replacing host_os.c's shared Font0/empty-glyph stubs.
Shared fonts are local WORKERFS inputs at /fonts. A private 128MiB virtual arena
at0x7c00000000 holds the actual font bytes and individually tracked glyph maps;
ReleasesGlyph decommits pages and reuses addresses. Host font objects remain
private, deduplicated by full bytes/face, with consistent names across engines.
SFNT/TTC length derives from checked guest table directories (32MiB input cap).
All font operations serialize access to FreeType; guest allocator/free callbacks
run after unlocking. Preserves native's known lack of iType hinting/bold matching.

Added bounded GPOS kern/palt extraction (pair/class/single/extension paths),
legacy kern handling, guest OTF table callbacks, work/error/effect state, and
chunked in-place BFTTF decode. UI selects a local fonts directory; worker mounts
the seven required files before boot. advance_runtime.py also selects these files.
No font/ROM bytes are committed or uploaded. Build uses -sUSE_FREETYPE=1 only on
the raster compilation and final link, avoiding guest-wide flag invalidation.

Verified: web/test_host_font.py passes1513 actual production-adapter import calls,
seven local shared fonts, cross-page reads/writes and >32bit guest addresses,
700 repeated/concurrent glyph cycles with unchanged page count after releases,
engine reuse/errors, BFTTF decode, plus a locally generated known-value GPOS
variant testing pair/palt results (-65,5,15), kern-only(-80), allocation/free
callbacks and callback reentry. The seven original system TTFs have no GPOS
table; their empty OTF results are real, not evidence of pair-parser coverage.
Known GPOS variant is generated from the user's font into ignored build/ only.
Class/extension paths exist but are not separately covered by this known fixture.
Evidence build/host-font/report.json; all7 core raster tests and input checks pass.
OS pthread/input regression:177checks,0failures, build/codex-font-os-regression.log.

Full runtime coordinator now PID178424, builder145152, emcc143808, optimizer143728
(verified actual process/command/CPU). Exec session42509. State
build/codex-font-build-followup.json; log build/codex-font-full-build.log.
Uses --fibers --compact-codegen --jobs8, REQUIRED Binaryen optimize-level1 and
normal validation, without optional --trace-asyncify. Previous actual full build
used no --debug-graph; follow that authoritative configuration to reuse its cache.
An initial compile-only command with --debug-graph refreshed253 stale debug units;
it did not change protected headers or publish/link. Correct config compile-only
then passed using cached game objects. No second runtime linker is running.

Coordinator will verify protected/graph hashes, changed publication's actual SHA,
4GiB memory limit and306 Node constructors, then stop at ready-for-font-browser-boot.
Do NOT rerun coordinator while live: that would start another build. New browser
helper build/codex-start-font-integrated.cjs is prepared (not run), selecting all
seven fonts and retaining the active-runtime guard. Preserve boot23 until the
new build validates; archive its final evidence before intentionally stopping it
and booting the actual font fix. Do not reuse old raw C static addresses for new SHA.
Boot23 GuestMemory region table independently rechecked:8regions, enough for arena;
first diagnostic used offset256(page pointer), corrected to actual region_count260.
Evidence build/codex-font-prelink-state.log. No new game/menu/match proof yet.

## Portable real-font raster/metrics core verified, not yet connected to game

Added font_raster.c/.h using the pinned Emscripten FreeType port (2.14.3).
The core copies and owns font inputs, selects the SDK Unicode cmap preference,
reads actual SFNT metrics into the 0x93c SDK layout, and emits independent
0x28-header GlyphMaps with top-down gray coverage. Supports the full 2x2
16.16 scale matrix and native adapter's filled/unfilled outline dilation.
This does not reproduce iType hinting or bold; no claim of pixel equivalence.

Reproducible command (pinned Python): web/test_font_raster.py, optional --font-dir.
All seven local TTF inputs pass in pthread Wasm/Node: actual unitsPerEm and glyph
counts, cmap existence, ASCII/CJK/symbol coverage when present, metrics fields,
whitespace, independent retained maps, doubled size, 90-degree rotation, shear,
reflection, outline extents and filled-minus-unfilled coverage. Compiles with
-Wall -Wextra -Werror. Evidence build/font-raster/{report.json,build.log,run.log}.
No TTF assets are packaged, committed or uploaded. No full runtime link started.

Still required: guest-safe font import adapter (load/set/metrics/glyph lifetimes,
kerning/GPOS/error semantics), actual shared-font input/mounting, build integration,
then an actual game boot. Production host_os.c still uses its old empty-glyph stub.
Do not mistake the standalone fixture for a game fix already deployed.

Boot23 rechecked at3844s: live, heap1324744704/system1222093504,
311527 snapshots created/150 live, registry242/controlReclaims571788,
522830draws/22044presents. Presentation samples remain black. Same live run
preserved, no reload. Evidence build/codex-boot23-font-work-live.log.

## HID polling and actual empty glyphs verified; movie workers exited — boot23

Extracted a non-executable disassembly skeleton from exact f40835ac Wasm using
its unique C GetNpadState prefix address412028; original publication untouched.
web_os_import is function98594. Disassembly identifies supported styles20231920,
player0Connected20231928 and samplingNumber20231976. Actual read:styles63,
joyHold1,connected1,sampling10577->10578 over250ms. C polls the pad; absence of
polling/disconnection is not the demonstrated blocker. This is not a full input trace.
Evidence codex-boot23-hid-state.log and os-skeleton.json/.wat, all under build/.

HostThread ABI208bytes/name76 independently checked by pinned compiler, compile-only.
thread_for absolute function98600 (wasm-dis's generated label98529 is a definition
index; do not confuse it with an absolute function index). Static thread array20119248,
count20233600,normal registry mutex20233604 found in its actual disassembly.
Bounded snapshot under that mutex has25records: MOVIE_INIT and MoviePlayerThread
both started1/finished1. MoviePlayerThread entry0x71036f0890,arg0x802e770000.
Its vtable was initialized (all64slots match) but that only proves constructor
history. Reading its now-finished argument gives qword0=0,extractor+40=0; memory
may be retained/reused, so do not call this a live object or active decoder hang.
Reports codex-boot23-threads.json, movie-vtable.log and movie-object.log.

Original ELF's2112PLT slots were all checked against their GOT relocations:
ExtractorC1=0x71039c6cc0,DecoderC1=0x71039c6c20. Extractor call sites are in
f_71036e7250 and f_71036f1c50. Movie worker uses actual WaitAny and holder userdata,
not an unknown movie method there. Resolver/evidence codex-main-os-imports.json.

Actual shared glyph header at0x7800000c80 has size40,type8,bpp8,advance2097152
(32pixels16.16) but width/height/stride all0. This confirms game calls the empty
font backend. Shared font buffer prefix is zero; that does not prove which font
input the caller requested. Evidence codex-boot23-font-state.log. Real font
loading/rasterization is now the next functional priority; do not claim it alone
explains every zero UI matrix. Seven local user TTFs exist; SDK FreeType port is
pinned to2.14.3. No font implementation or new runtime build started yet.
At3292s still live:354718182imports/441140draws/18641presents,263877snapshots
created/149live,heap1324744704,allocated1219855776/system1222093504. Registry242
lists/controlReclaims483310. The same heap/system size persists across the observed
1104..3292s interval; no menu/input/match completion. Source font metadata readiness
is recorded separately in build/codex-available-font-inputs.json.

## Old snapshot boundary crossed; one actual A press verified — boot23

At1104s:103132603imports/103196draws/4559presents,66737snapshots created but159live,
heap1324744704,allocated1220202608bytes. Registry242lists/13204bindings;
controlReclaims117178, modeled command bytes702944. This crosses the old65536-live
snapshot failure by cumulative creations, with retained snapshots and registry far
below their old counts. Does not prove permanently bounded allocation or gameplay.
Evidence: codex-boot23-live-observation.json and observations.jsonl in build/.

Across samples448..1104s, draw delta82968/present delta3457 is exactly24/frame.
That matches the native sequence's movie range, but draw counts can also differ
because web glyphs are empty: movie state is an inference, not established.
One real focused/visible Z/A press at1248..1250s held10presented frames, then24more
frames observed. Actual C shared input words contained buttons1, sequence222336;
no input timeout. Subsequent draw/present ratio23.9167, no visible menu established.
Receipt build/codex-boot23-press-a.log. Shared input arrival is not proof guest
consumed it or that input controls gameplay correctly.
At1504s the run is still live:148606053imports/164040draws/7095presents,
102234snapshots created/149live. Heap remains1324744704bytes, allocated1220026752,
system1222093504 (same system size as1104s); texturesCreated533 unchanged.
This verifies stable ownership across the old boundary over the observed interval,
not the unfinished UI/input/match. Preserve this run for producer/state diagnosis.

Correction to the earlier media comparison: runtime/boot.c registers
register_movie_mf_imports after register_movie_imports, so native normally replaces
nnmovie.c's failing placeholder unless SSBU_MOVIE=0. tests/DEBUG_MODE.md states
native skip mode lets12actual samples pass before EOS; zero-sample EOS stalls.
Matching web's0x4000 return to the old placeholder alone did NOT establish parity
with the working native movie path. Need actual movie/input/UI state or call
evidence before changing semantics; no runtime C/clock/matrix patches made.

## Actual UI matrices are zero before upload — boot23

Initial11passes archived in codex-boot23-live-pass-probe.json. General pool
diagnostics hit their256-attempt cap before UI handles348/361 appeared. A targeted
upload-time registry read mapped both in1attempt, errors[]:348 CPU memory0x804a64c000,
4096bytes;361 CPU memory0x8010600000,150994944bytes. Only diagnostic metadata changed.
Late10passes (codex-boot23-late-pass-probe.json) have actual addresses, but every
direct guest comparison reports busy mapping lock. Do not reinterpret busy as equal.

Added a bounded live upload observer in codex-boot23-install-upload-source.cjs:
copies completed serialized operation2 staging bytes before C receives its reply,
for handles348/361 only,64entries/max1024bytes each. These bytes come from actual
gm_read in host_gpu_resources.c:transfer/web_gpu_draw_range. Latest overlapping
writes are composed; oversized writes clear history, failed writes are excluded,
destroy/close invalidates it. Eight offline contracts pass; no guest memory writes.

Actual codex-boot23-upload-pass-probe.json contains10passes atframes3934..4057.
Eight UI uniform slot0 captures are complete560bytes and equal the actual C upload
source (one completed write each), with target87 readbacks black. The two fullscreen
passes use handle1, outside this upload observer, explicitly unavailable.
codex-boot23-upload-uniform-analysis.json confirms all8UI vertex sources exactly
match the earlier analyzed Maxwell source. World X/Y rows are all zero, world Z
row[0,0,1,-1], already in guest-derived upload data. This narrows those samples to
the guest UI data/producer/state; it does not explain every draw, establish a font/
clock cause, or prove menu/input/match. The live run must be preserved.

## Control-reuse Wasm published; actual boot23 started — 23 October 2026, 03:13 Vietnam

New exact file SHA f40835ac644406c5a6d43efae4ef3816b5de73e34a1fb826db5c53627855b431,
281992236bytes/99138functions/maxlocals10812. Actual shared-memory maximum65536pages
and306constructors pass for this sameSHA. Build/compiler/coordinator are terminal.
Coordinator stopped only at the deliberately closed browser; manual recovery reopened
ssbu-web/PID149760 and the existing guarded harness confirmed started:true with all
target/pass/guest-pool probes ready. No rebuild. Receipt/logs:
codex-control-reuse-build.json, memory-limits.json, node-report.json,
codex-control-reuse-browser-recovery.json, boot-start-recovery.log (all in build/).

Bounded GuestMemory reader found the new mapping at19503496 (old f37e address invalid).
32bytes at guest0x7100000000 independently match the original ELF PT_LOAD prefix.
Actual game resource table20765584 maps5GPU pools at the initial observation, errors[].
Report codex-boot23-guest-reader-check.log; this is actual game mapping, not fixture data.
No live guest/GPU comparison/menu/input/match result yet. Preserve this run and archive
matching-SHA livePassProbe reports before any other /__report writer. Do not reload.
First observed114s:11537005imports,heap820969472,allocated742350520bytes,
registry1list/controlReclaims6;0draws/0presents/0snapshots. Viewed canvas is black
before any submitted draw. This verifies the new reclaim path executes in the game,
not sustained bounded ownership or a rendered menu. Full observation saved under
build/codex-boot23-live-observation.json; later compact samples append to
build/codex-boot23-observations.jsonl (GPU live/created snapshot counts included).
At324s:11537draws/520presents,heap1022427136,allocated817759048bytes;
10308snapshots created and154live. Actual loading logo visually confirmed and
archived as output/playwright/codex-boot23-loading-logo.png. Early reclamation is
working, but the former65536snapshot boundary and a playable menu/match remain
unverified. No uniform-specialized draw or live pass comparison at this sample.

## Bounded guest/GPU uniform comparison prepared for next run — 23 October 2026

The future-read codex-start-integrated.cjs and manual live-pass installer now add
guestComparison only for vertex-stage NVN uniform slot0 (shader binding3), up to1024bytes per draw.
They compare the captured GPU prefix against the existing bounded guest reader,
reporting missing reader, busy lock or errors explicitly. Only difference counts
and the first differing byte are added; no extra bulk capture. Timing scope is
after GPU readback: other guest writers may run concurrently, so a mismatch alone
does not prove an upload bug. No guest rendering state is modified.

After the new module is validated and booted, install codex-install-live-guest-reader.cjs
once the diagnostic module/guest mapping is available, and validate its ELF bytes
against the original file. Otherwise captures will explicitly say reader unavailable.
Both probe files pass syntax checks and the existing13startup guard checks still pass.
No real new-game comparison yet, no production C/flags/roots/manifest changes.
Offline check of the exact snippet passes6cases using archived boot22's560-byte
GPU prefix and simulated guest reads (match, one changed byte, busy, missing reader,
unmapped range, invalid length). The initial slot3 selector found no capture and
was corrected: CB3 is shader binding3, mapped to NVN uniform slot0 by this renderer.
Report build/codex-guest-upload-comparison-offline.json; this is not a live guest
comparison or evidence of the new game's matrix values.

## Optional compiler tracing adds validation work — 23 October 2026

Installed wasm-opt reports133/version_133-60-g93d6e9de7. The matching upstream
[pass runner](https://github.com/WebAssembly/binaryen/blob/93d6e9de7/src/passes/pass.cpp#L863)
confirms BINARYEN_PASS_DEBUG=1 adds per-pass and pass-group validation, excluded
from printed pass timings. Our --trace-asyncify sets that variable. Normal
[wasm-opt input/output validation](https://github.com/WebAssembly/binaryen/blob/93d6e9de7/src/tools/wasm-opt.cpp#L319)
exists separately. For the next necessary build, prefer the existing default
without optional tracing, preserving normal validation, jobs8, the required
Binaryen optimization option and all exact-SHA execution checks. No speedup was
measured and no invocation changed during the active build. Source receipt:
build/codex-binaryen-trace-source-review.json. Current build has completed Asyncify
and its pass validation; optimizer90200/exec8095 remains in final validation.

## Font backend gap confirmed from source — 23 October 2026

Control-reuse build remains live (optimizer90200, coordinator183220, exec8095);
CPU12301s at the source-review check. No new Wasm or real-game run yet.
Read-only findings saved in build/codex-web-font-source-review.json. Web shared-font
imports return a fixed address/4096 bytes, and AcquireGlyphmap returns a cleared
0x28-byte header with advance/type but zero width/height and no glyph pixels.
Metrics and glyph existence are synthetic. Native nnpl.c/nnfontll.c load actual
TTFs and rasterize outlines; seven extracted fonts exist in build/native/sysdata/fonts.
Native rasterization depends on Windows GDI and cannot be linked unchanged into Wasm.
No font binaries copied/committed, and no production build inputs changed.

This establishes an incomplete text backend, not the cause of the observed black
target or collapsed non-text geometry. No actual font-call trace tied to those
passes was collected. Validate the pending lifetime fix on the real game first;
real font loading/glyph rendering remains required before claiming faithful text.

## Same optimizer verified; media source limits recorded — 23 October 2026, 01:44 Vietnam

Previous goal turn was verified wait on live90200/exec8095. This turn revalidated
the same process and advancing CPU; no publication or new boot. Current health JSON
is authoritative for the latest CPU/private-memory observation. No second linker.

Read-only media review saved to build/codex-media-path-review.json. Movie virtual
dispatch in web_os_indirect returns explicit0x4000failure, matching the native
nnmovie.c placeholder; a blanket-success decoder stall was not established.
Named buffer/constructor wrappers have different semantics; no actual boot call trace
was collected for this hypothesis. AudioOut implements timed buffer release, not a
browser PCM output bridge. Real audio remains part of the unfinished completion audit.
No C/media/clock/matrix changes during the active control-reuse build. Rendering and
guest uniform data must still be checked on the actual new Wasm after publication.

## Portable guest-source tracing prepared for next boot — 23 October 2026, 01:28 Vietnam

Previous turn verified the existing optimizer and screened clock callers. This turn
verified SAME optimizer90200/coordinator183220/builder179628/exec8095 remains live,
CPU4422.77s/private8127930368bytes at01:28. No new publication/boot, browser idle.
Frozen runtime C/Worker/header hashes, roots and manifest still match build input receipt.
No C/clock/matrix/graph/compiler changes during this linker. Do not start another link.

Added gpu-guest-pool-diagnostics.mjs (commit a3fd877, local) to identify the pinned
Wasm32 Resource registry by hashed-chain validation and actual backend GPU handle/size
matches. No hardcoded static address; metadata reads only. Production operation2 comes
only from host_gpu_resources.c:transfer while resource_lock remains held until reply,
so the optional hook observes stable metadata there. Captured guest buffer addresses
use Resource.memory+offset, never the separate GPU address; memory0 is explicitly absent.
Candidate/scan/chain budgets are bounded and diagnostic failures leave requests running.

Resource ABI checked with pinned compiler (compile-only object, no second linker).
18 synthetic JS contracts pass. Actual already-built C/NVN/WebGPU fixture with this
inspection enabled still passes3502checks and maps pool0x7100006000 to guest
0x7100020000,size81920, plus the recreated GPU-only pool memory0/address0x7b00000000.
Report build/codex-guest-pool-fixture-report.json, no errors. This is fixture evidence,
not proof of game uniform contents or rendering. Initial harness expectation65536 was
corrected after reading the fixture's actual81920-byte allocation; no implementation
change was needed to obtain the correct result.

Updated future-read codex-start-integrated.cjs to install guest-pool probe after target/
pass probes; lazy heap getter waits naturally until actual C upload. Existing pass
capture now annotates buffer.guestSource from the observed pool metadata. The helper
module is copied into build/web-poc already; current publisher does not remove extra
files. New startup mock13checks confirms one boot, active-runtime refusal and nonfatal
optional probe failure: build/codex-guest-pool-startup-smoke.json. No old-game boot.

After publication/Node/new boot: check diagnosticProbes.guestPools.ready and
self.__ssbuGuestPoolProbe.report(). The report should eventually map actual vertex/UBO
handles; captured guestSource then identifies the real guest upload address for the
next UI producer investigation. If unavailable, preserve explicit error/scope and do
not assume raw addresses. Existing bounded guest reader can be installed separately
after ABI validation; old f37e command/list raw offsets are NOT valid for a new SHA.
UI producer, menu, working input, playable match/audio/FPS remain unverified.

## Control-reuse optimizer live; clock screening did not establish a bug — 23 October 2026, 01:11 Vietnam

Previous goal turn made progress: observed actual control reuse, fixed ownership,
passed3502C/GPU checks and launched guarded build. This continuation verified the same
exec8095/coordinator183220/builder179628. Finalizer8772 completed; optimizer90200 now
live, CPU474.59375s/private5446713344bytes at18:11:18UTC. No publication/new game boot.
build/codex-control-reuse-build-health.json holds the current live optimizer snapshot.
Browser ssbu-web remains idle; preserve existing link, no second runtime builder.

Screened actual GetSystemTick callers in compact generated units while waiting.
Pinned PTHREADS emscripten_get_now uses synchronized timeOrigin+performance.now, unlike
native QPC origin. That difference alone is not proof of broken animation. The21 nearby
floating-point candidates were screened; both zero-nearby-subtraction candidates reviewed
use unrelated saved/configuration values, and the observed time difference path subtracts
integers before conversion. build/codex-tick-caller-screen.json records scope and findings.
No demonstrated absolute-tick float32 cancellation or identified UI producer. No C/clock/
matrix/source edits during this build. New game still must verify reclaimed counts,
bounded registry/snapshots, UI, input and playable match after link/Node checks.

## Control-storage lifetime fix verified in fixture; full build live — 23 October 2026, 01:05 Vietnam

BOOT21 ENDED: page crashed during optional CDP Runtime.queryObjects inspection; CLI
confirmed reset to about:blank. Cause not established; do not call it4GiB OOM.
Archive build/codex-boot21-browser-crash-report.json and corresponding crash.log.
Replaced that approach with opt-in self.__ssbuDiagnosticModule in runtime-worker.mjs,
deleted on shutdown. Boot22 restarted on SAME f37e Wasm, no relink at that time.
Direct module access verified. Bounded64KiB guest reader uses the existing gm.map_lock;
unique GuestMemory signature found at19503480 for f37e. Read32bytes matched local ELF.

Pinned Wasm32 ABI assertions compiled before changing C, and actual pthread/JS mutex
interop7checks passed (normal mutex lock word offset4, EBUSY10, C waiter wakes and
native mutex works after JS release). Export-body disassembly of a nonexecuted Wasm
skeleton located f37e commands_lock20782432, lists20782456, buffers20782464.
These addresses/old CommandList layout are SPECIFIC TO f37e; do not reuse on a new SHA.
No protected headers changed and no heap-wide DevTools object queries repeated.

Actual command storage sampled under that normal mutex (nonblocking, synchronous
bounded traversal). Registry has678 command-buffer objects, many not configured.
build/codex-boot22-command-storage4.json stores32 samples; derived
build/codex-boot22-control-reuse-analysis.json records565 resets across235 configured
buffers: same actual buffer pointer/key/control address, consumption32->0.
Old control tracking never invalidated the previous list when those bytes were reused.

BOOT22 TERMINAL at1222s: GPU program snapshot space exhausted (65536 live snapshots),
error invalid NVN program for command binding.128098405imports, heap3542089728,
allocated3439215672, registry116860lists/5982818bindings/4141450drawRecords/
840482programBindings. Full115-sample report archived:
build/codex-boot22-snapshot-exhaustion-report.json. Actual boot22 early pass capture
also archived after checking timestamp against boot22 start (same Wasm SHA as boot21).

SOURCE FIX commit50b1275: record each list's actual32-byte control slot. EndRecording
retires registry ownership of overlapped old slots, using the same release_list refcount
path as explicit finalization. Adding storage alone leaves handles valid; selected/nested
references keep immutable metadata alive. No age limit, snapshot-cap increase or shader
substitution. controlReclaims telemetry added. Refill fixture now supplies distinct
control blocks when it intends to keep older handles alive.

Actual C/NVN/bridge/WebGPU fixture passes3502checks/2draws,99controlReclaims and final
registry/bindings/snapshots all0: build/codex-control-reuse-fixture-report.json.
New tests cover exact reuse, declaration without overwrite, adjacent slots, cross-buffer
partial overlap, stale submission rejection,96reuse cycles and retained nested metadata.
Opt-in Worker module hook committed separately as5e8c5d6. These new commits are local;
last confirmed remote checkpoint6760975 remains unchanged. Game validation still pending.

Dedicated browser recycled ONLY AFTER boot22 terminal + fixture; ssbu-web idle '/',
Node172852. New FULL BUILD exec8095, coordinator183220, launcher182812/builder179628,
finalizer8772 confirmed live; no second linker. State/log:
build/codex-control-reuse-followup.json / codex-runtime-control-reuse-build.log.
Coordinator build/codex-finish-control-reuse.ps1 retains --jobs8, BinaryenO1/cores4,
graph70279 and max4GiB; hashes protected headers, roots/manifest and changed source.
After link it checks changed SHA, actual max65536pages, Node306 exact SHA, then guarded
boot with target/pass probes. Preserve this existing process. Served f37e is OLD relative
to this fix; do not boot it during build. UI matrix producer/blackness remains unresolved;
menu/input/match/audio/performance are not verified.

## Boot21 passed old2GiB limit; persistent UI collapse captured — 23 October 2026 (Vietnam)

Previous turn progressed by validating actual4GiB publication and starting guarded boot21.
This turn verified live ssbu-web throughout. At678001ms heap2231500800 exceeded old2GiB
without the former staging allocation error. At858000ms:79444004imports/59707draws,
heap2634547200, allocated2590366984/free171480, registry69730lists/3552250bindings/
2456010drawRecords/443697renderRecords/499610programBindings, modeledCommandBytes197126480.
Still live on SHA f37e127a..., Node179088; no linker or runtime restart.

Logo visually confirmed around326s (latest screenshot later overwritten by black frames).
After loading, source/window readback remains black. Archived11 actual pass captures
at frames785..907 to build/codex-boot21-live-pass-probe.json BEFORE target reader POST.
Target capture archived build/codex-boot21-live-target-probe.json, target5 and87 black.
Focused canvas visible/document.hasFocus true, A/KeyZ held1200ms at620..622s;
no menu/input correctness claim. Later textures increased293->439; attribution unknown.

Actual Maxwell->GLSL outputs generated by build/codex-compile-boot21-passes.mjs.
build/codex-boot21-ui-clip-analysis.json approximately evaluates actual GLSL position
prefix from full captured float32x2 vertices/CB3. Most UI draws' CB3 world matrix
has zero X/Y rows, all four clip positions collapse to(0,0,0.0033333,1) in both phases.
Fullscreen black quad has identity world matrix and covers screen, FS tint black.
This identifies geometry-level absence for those sampled UI draws; does not identify
the guest producer or prove all shaders/frames. Do not force game matrices/clock.

Installed independent later probe AFTER A via build/codex-install-late-pass-probe.cjs,
namespace __ssbuLatePassProbe, report results.liveLatePassProbe (same /__report endpoint).
Archived10 captures frames2018..2141 to build/codex-boot21-late-pass-probe.json.
All sampled target87 readbacks still zero RGB; approximate clip analysis again has
collapsed UI geometry: build/codex-boot21-late-pass-probe-clip-analysis.json.
Original early report and later report have DIFFERENT result keys; archive proper SHA
before another POST. read-heap-timeline helper does not POST, so preserves reports.

Current resume state and bounded allocation timeline updated. No C/lifetime edits made.
Registry grows rapidly with real allocated bytes; lifetime/ownership still needs a correct
solution rather than native120-frame age heuristic. Continue live boot to observe any
new boundary; investigate actual guest uniform producer and command storage ownership.
Menu, working controls, playable match/audio/performance remain unverified.

## Boot21 actual heap and registry telemetry verified — 23 October 2026 (Vietnam)

New4GiB boot21 remains live on ssbu-web. At206001ms: 20915739imports,
283draws/58presents, heap1022427136, allocator allocated924016320/free15248416,
registry1363lists/65041bindings/44346drawRecords/8731renderRecords/9250programBindings,
modeledCommandBytes3752944. At224000ms:563draws, allocator961989112/free1564904,
liveProgramSnapshots1136, commandInfo busy:true (expected nonblocking trylock).
Actual canvas screenshot viewed: black at this early stage, no visible menu claim.

build/codex-boot21-heap-timeline.json holds21 compact samples with exact new SHA;
build/codex-4gb-resume-state.json is the current resume snapshot. Read timeline with
build/codex-read-heap-timeline.cjs via CLI; its result is saved locally without POSTing
/__report, so pass-probe reports are not overwritten. CLI log is UTF8 with possible BOM.
No actual target87 specialized pass yet; preserve existing auto-installed probes and
archive browser-report when results.livePassProbe and f37e SHA appear. Do not restart.

## Goal resumed; actual 4GiB Wasm validated and boot21 started — 23 October 2026 (Vietnam)

Goal tool returned active after prior explicit pause; inspected current worktree first.
Git main remains clean at6760975 (14 checkpoint commits already pushed). Old builder/
optimizer ended while paused and published actual SHA
f37e127a4124c6637b5ecbae57366fd52e73598d4fdb6c5762f4d3860dd41dd7,
281991683bytes/99137functions/maxlocals10812 at2026-10-22T05:54:47Z.
Actual memory parser and independent file hash pass: shared minimum512/max65536pages.
Actual Node306 constructors pass on this exact new SHA; archived build metadata,
memory limits and node report under codex-4gb-heap-* / codex-4gb-memory-limits.json.

Opened fresh headed Playwright ssbu-web (Node179088) because prior session process
was absent. Guarded codex-start-integrated.cjs confirmed started:true and both target/
pass probes installed at about2026-10-22T17:15:50Z (23Oct00:15 Vietnam).
This is the first real new4GiB boot (boot21); preserve it, do not reload/restart.
Prior coordinator144168 remains intentionally stopped and old followup JSON stale.
No linker active. No menu/input/match proof yet; early progress capture pending.

## USER REQUESTED STOP / FULL HANDOFF — 22 October 2026, 05:27 UTC

User: "dừng lại và viết bàn giao đầy đủ đi". Goal work stops at this handoff.
Stopped coordinator PowerShell144168 after verifying its exact command line, preventing
automatic Node checks/browser boot. Preserved expensive in-flight builder172172 and
optimizer168312; both verified alive after coordinator stop (CPU14922.50s,
private15058853888). Asyncify4690.99s completed; log final validation, no new publication.
Server155412/171544 remains. Old Playwright Node167376 absent; do not assume session live.
build/codex-user-stop-handoff.json records manual stop. Original followup state remains
building-4gb and is stale after coordinator stop; no automatic browser-running transition.
Compiler can still finish/publish; resume validation manually only when user resumes work.

Canonical web/HANDOFF.md and root handoff.md refreshed identically with stop state,
exact process ownership, manual validation commands, evidence, diagnostics, missing
menu/input/match proof and next actions. Previous full handoff is preserved as
web/HANDOFF_HISTORY_2026-10-22.md. Earlier live-coordinator notes below are history.

## Archived heap trend narrows next measurements — 22 October 2026

Previous turn made progress (bounded telemetry history) and verified live validation.
This continuation revalidated session6115/optimizer168312 and derived
build/codex-boot20-memory-trend-analysis.json from four archived actual observations.
During352001..486000ms, GPU buffer liveBytes239562752 and liveTextures234 stay fixed,
but Wasm heap capacity grows302252032bytes and liveProgramSnapshots grows11839 across
762presents. This correlation is not proof of a leak: old reports lack allocator
live/free counts and command registry counts, supplied only by the pending4GiB build.

Source review found native runtime/nvn_gl.c:list_collect expires unused lists after
120frames, while web registry ownership is explicitly released by FinalizeCommandHandle.
Do not copy native's age heuristic as a fix: legal command/resource lifetime still
needs evidence. No C/roots/headers/lifetime changes made during the active build.
New boot should compare allocator allocated/free/system and registry lists/bindings/
drawRecords with these snapshot counters before choosing a lifecycle change.

## 4GiB Asyncify pass completed; validation live — 22 October 2026, 05:19 UTC

Actual build log now reports asyncify4690.99seconds followed by `(validating)`.
Optimizer168312 remains alive and advancing (CPU14476.80seconds/private17750343680
at the subsequent check). No new publication or boot yet. Preserve exec6115 and
the existing build; this is verified wait, not a hung-process inference.
Current health JSON records the latest live validation snapshot.

## Bounded allocation history for the next boot — 22 October 2026, 05:17 UTC

Previous handoff turn made progress by publishing the current handoff and preserving
the original history. This continuation revalidated live optimizer168312 (CPU14345.625s,
private15157309440 bytes), session6115/parent144168/builder172172 still active.
No new Wasm publication or boot; served2adc remains the old2GiB module.

Added opt-in allocationTimeline in app.mjs using existing diagnostic progress fields:
one compact sample per10seconds, at most128 entries, preserving first and recent127.
Includes heap/allocator/command counts, imports/draws/presents; excludes frame captures.
Available live as window.__ssbuAllocationTimeline, reset at start, attached to both
normal and error terminal reports. No additional C calls/timers or guest behavior.
Node syntax check passed; served build/web-poc/app.mjs matches source. Real telemetry
and end-to-end game verification await the currently running4GiB linker.


## Live memory readers include new telemetry; staging paths reviewed — 22 October 2026

4GiB exec6115/optimizer168312 remains live, CPU advancing (latest actual snapshot
build/codex-4gb-build-health.json). No C changes/restart/new boot during this wait.

Reviewed temporary staging in transfer/web_gpu_pool_copy/web_gpu_draw_range,
submit_draw, color/depth clear packets and copy-source preparation. These inspected
normal/error paths release their temporary blocks. This is NOT an all-allocation
leak audit and does not identify the actual2GiB growth cause. Retained command
metadata and allocator free/allocated counts still need live4GiB evidence; no
heuristic GC or speculative staging-size reduction was added.

Updated codex-read-live-target-probe.cjs to save/print allocationInfo and commandInfo
alongside heapBytes, and codex-follow-visible.cjs to print these fields for live or
terminal results. Previously the new C telemetry existed in the Worker but the
compact target reader dropped it. Both scripts syntax-checked; backward compatible
when old module has no exports. Archive immediate pass report before running
another /__report writer. Actual new-game verification remains pending publication.


## Durable probe smoke verified; 4GiB optimizer remains live — 22 October 2026

Previous goal turn made concrete progress in diagnostic capture/startup control;
this turn verified the actual captureInputs path with mocked GPU readbacks and
server responses. build/codex-durable-pass-probe-smoke.json records16 captures/
16 immediate posts, two phases of8, duplicate suppression, optional-resource gate,
nonfatal HTTP500 and idempotent install. No GPU/game claim from this mock.

Actual optimizer168312 alive at 2026-10-22T04:11:55.8929910Z, CPU3351.50sec,
private5872099328bytes; parent exec6115/builder172172 unchanged.
No new linker, publication or game boot. Preserve this live process; source2adc
remains old2GiB served Wasm. Current health JSON contains this snapshot.

After eventual guarded new boot, inspect browser-report.json BEFORE other
/__report helpers. Only archive as live-pass evidence when results.livePassProbe
is present and its metadata-associated SHA matches the new published build.
The old fixture/browser report is not evidence from a new boot. Auto-installed
probes are ready; no manual repeat startup should occur while runtime is active.


## 4GiB optimizer LIVE; guarded next boot now auto-installs durable probes — 22 October 2026

Preserve exec6115. Finalizer161616 finished and handed off to wasm-opt168312,
verified alive at04:07UTC CPU2213.61sec/private5.82GB. Builder172172 and primary
144168 alive; no build failure/publication yet. Log still running passes; silence
is not terminal. Do not start another linker or boot the old served2adc module.
Browser remains idle. Old finalizer PID in previous section is historical.

Improved diagnostic tooling while the existing link continues (no further C or
protected-header changes): codex-start-integrated.cjs now wraps the archived exact
plain start harness, then optionally installs both live target/pass probes once
backend is ready. The already-running PowerShell script invokes that same filename
after successful publication/65536-page check/Node306, so new boot will auto-install
without a human needing to race the Worker closure. Optional probe failure is
reported in diagnosticProbes; it does not restart or stop a successfully started
runtime. Active-run refusal/reload guard remains first. Node control-flow mock
verified one boot, both probes installed and a second start refuses active run;
this is not actual game boot verification. Plain original preserved in
build/codex-start-integrated-plain.cjs. New generated harness syntax checked.

Pass probe now waits until actual uniformSpecializedDraws>0, samples up to8 module
pairs immediately and up to8 again after120 presented frames. Each capture budget
32KiB (16 total <1MiB HTTP limit); this compares initial/settled UI transforms.
Each successful readback/capture immediately POSTs /__report and records save status,
so data survives a later Worker termination. Latest saved file is
build/web-poc/browser-report.json; ARCHIVE IT before another /__report helper or
fixture overwrites it. Runtime terminal report remains a different file. New boot
metadata SHA tags the pass report, with explicit metadata-association scope.

Offline real-input analysis: codex-ui-early-records.json was extracted from boot20
full runtime archive. Actual Maxwell->GLSL outputs are codex-ui-<draw>-<stage>.glsl.
Three early solid-color quads (1973,1975,1981) have valid nonblack FS uniform colors,
but their captured vertex transform places every corner beyond x>w (minimum NDC
x1.009375/1.039583). CPU approximate arithmetic archived in
codex-early-ui-clip-analysis.json. This explains why those FIRST quads cannot paint;
it does not prove why persistent later frames remain black, or authorize altering
game matrices. Two-time pass probe is intended to inspect their later transforms.
Do not treat the first12 captures as settled UI. No menu/input/match verified.


## 4GiB build LIVE after verified >2GiB GPU bridge; preserve linker — 22 October 2026

NEW build exec6115, primary powershell144168, Python launcher173924/actual172172,
finalizer161616 verified alive CPU129.86sec/private6.76GB. Health snapshot:
build/codex-4gb-build-health.json. Only host_allocation_stats.c and host_gpu_commands.c
compiled; remaining objects reused. Corrected native-stderr wrapper kept builder
alive after harmless warning. Do not restart due silent validation or formatted
NativeCommandError warning. No second linker. Logs codex-runtime-4gb-heap-build.log;
state codex-4gb-heap-followup.json is building-4gb. Finalizer may hand off to a NEW
wasm-opt PID; inspect current processes/exec handle, not a stale PID alone.

Source build_runtime.py maximum now4294967296, initial32MiB unchanged. Graph still
70279/1096 units; no roots/manifest/protected-header edits. Existing Binaryen flags
and --jobs8 preserved. Script build/codex-finish-4gb-heap.ps1 checks roots/manifest/
protected-header hashes, actual shared memory maximum65536 pages in the new binary
(web/check_memory_limit.py), publication SHA, Node306 exact SHA, then guarded boot.
Expected output codex-4gb-heap-build.json/codex-4gb-memory-limits.json/
codex-4gb-heap-node-report.json and boot-start.log. No manual boot while link lives.
Served2adc is still OLD2GiB binary; actual parser confirmed32768-page shared maximum.

Browser ssbu-web was recycled AFTER boot20 archive/fixtures, now idle `/`, dedicated
Node167376, preserving memory for linker. Avoid game boots during heavy validation.

Boot20 exact final:65,999,693 imports,41,389 draws/2322 presents,723 specialized
actual game draws/5784 removed arms, stopped graphics staging allocation failed
at wasmHeapBytes2147483648. Archive codex-boot20-staging-allocation-report.json.
Heap progression snapshots: codex-boot20-heap-early.json (1.022GB208sec),
heap-loading (1.325GB352sec),heap-before-ui (1.425GB416sec),heap-post-loading
(1.627GB486sec). codex-boot20-live-target-probe.json at550sec1.829GB stores actual
before-present readback: frame1599/24051draws, BOTH window3 and intermediate87
are1920x1080 RGBA8sRGB,0nonblack/allalpha255. Last composite samples87 into3.
It proves black originates before canvas presentation, not why. Captured real
composite shader sources/options/layout/packet are in this same probe archive.
Separate per-pipeline pass probe was installed late but runtime ended before
collection; codex-boot20-live-pass-probe.json explicitly says unavailable. Do NOT
claim it contains draw records. No menu/input/match verified.

4GiB fixture web/large_memory_probe.c + build_large_memory_probe.py allocates a
padding block without touching it, obtains pointer2164332024 (>2GiB), writes native
bytes, uploads/reads real WebGPU through production dispatchGpuRequest, checks
unsigned callback token.7checks PASS, including allocator telemetry before/after
free: allocated2164266584 ->1608 bytes. Report codex-large-memory-allocator-report.json.
This verifies shared pointer handling/ABI, not full game footprint or playable game.
Pinned Emscripten settings.js explicitly supports opting into >2GiB via max4GiB.

New host_allocation_stats.c exposes pinned dlmalloc size_t mallinfo ABI as JSON;
verified by above large-block allocation/free fixture. New web_gpu_command_info in
host_gpu_commands.c reports registry lists/bindings/record counts using trylock,
skips when busy (owner Worker must not deadlock waiting on a guest GPU lock).
Worker progress/final includes these only under diagnosticFrames, with typeof
compatibility for old Wasm. C NVN integration3295 checks/2fixture GPU draws PASS,
all registry/program/resource counts zero after finalization, archived
codex-command-diagnostics-report.json. Capacity alone is not a claim that growing
command lifetimes or black rendering are fixed; new metrics are for that diagnosis.

Ready live diagnostic harnesses for NEXT boot, install EARLY while Worker alive:
- codex-install-live-target-probe.cjs exposes/captures real textures immediately
  before first presentation after a specialized draw; codex-read-live-target-probe.cjs
  saves via /__report. Uses existing opt-in self.__ssbuDiagnosticGpu.
- codex-install-live-pass-probe.cjs captures successful target87 draws by actual
  module pair (max8); codex-read-live-pass-probe.cjs MUST run before Worker closes.
  For reliability, add bounded immediate server save inside its capture callback
  or integrate final/progress capture before the next end. Its data was lost in20.
- No active game now; do not call any installer on a terminated Worker (evaluate
  can hang). Check window.__ssbuRuntimeDone first. Existing start harness is plain,
  does NOT auto-install these probes; after new guarded boot install them early.


## Boot19 passed texture stop, then hit 2GiB heap limit; boot20 diagnostics live — 22 October 2026

Boot19 ended at graphics staging allocation failed after64,728,369 imports,
43,761 draws/2,239 presents (exact final counters in archived report),958 successful
uniform-specialized draws /7,664 removed branches. Same2adc Wasm. Full report:
build/codex-boot19-staging-allocation-report.json. Prior live success evidence:
build/codex-boot19-passed-unbound-progress.json. This proves the old texture stop
was passed in the actual game, not just fixtures. Presentation source stayed black
after loading; no menu/input/match verified. A/KeyZ was sent once with canvas/document
focused at~560sec; no clear visible response or resource progression was observed.

Browser console has explicit allocation failures requesting2147508224/2147545088
bytes against limit2147483648. The current build_runtime.py max is2GiB; allocation
limit is confirmed. Root memory composition/lifetime still needs investigation;
do not merely reduce staging size, add arbitrary command-list age GC or claim a
4GiB build fixes rendering. Program snapshot lifetime remains34675 live at stop.
Native backend's 120-frame list GC is a heuristic, not faithful lifetime evidence.
No maximum-memory/config/C/header change or linker has been started yet.

The attempted read-only Worker instrumentation was requested after the runtime
had already ended; Worker.evaluate remained pending on its closed context. Separate
page status confirmed done=true and report was archived. Dedicated browser recycled;
this is not a reason to restart any live runtime. Old CLI94761 may resolve on close.

Added opt-in self.__ssbuDiagnosticGpu while diagnosticFrames=1, removed at shutdown;
progress/final reports now include wasmHeapBytes. It allows serialized readback of
actual render targets from the live Worker without waiting for another trap.
Production behavior/rendering unchanged; no raw Wasm memory is exposed by this field.

**BOOT20 STARTED03:30UTC**, same2adc Wasm/branch fix + new Worker diagnostics;
ssbu-web dedicated Node173116. Preserve run, poll and inspect the live Worker early.
Next inspect intermediate target pixels vs presented5 to locate persistent black,
and heap growth to characterize2GiB exhaustion before choosing C/config changes.
No linker pending. Source proof guard now also invalidates CB knowledge on possible
memory-writing calls/copies/atomics/ExtInst pointer outputs;26 self-authored CPU
proof checks passed, actual shader still removes8 arms with dependency0/43/1.


## Actual optional texture branch proved; boot19 started with JS fix — 22 October 2026

Boot18 reproduced Unbound shader texture1/2 at40,142,126 imports,
16,601 draws/906 presents,293 textures/19 pipelines. Archived exact served2adc
SHA in build/codex-boot18-unbound-capture-report.json. New bounded failure capture
worked: it retained BOTH actual Maxwell stages, layouts/active bindings, immutable
packet and complete small vertex/index/CB buffers after evicting12 early captures.
Actual loading canvas separately visually verified/saved in
output/playwright/codex-boot18-loading-logo.png; metadata-associated progress
build/codex-boot18-loading-progress.json. No menu/input/match verified.

Actual fragment GLSL (build/codex-unbound-stage1.glsl) reads texture2 only under
p0 = (float(CB3 word1) != 0). Captured CB3 word1 is0. Other real slots0/1 stay
sampled. This is a disabled optional branch requiring a static WebGPU binding,
not evidence of a missing upload. No replacement texture was added.

New web/spirv-uniform-branches.mjs conservatively evaluates scalar constants,
read-only uint CB loads, local predicates and simple untaken single-block ifs.
It only removes proven false arms; unknown branches stop the proof walk;
loops/Phi refuse optimization, calls/unknown pointer stores invalidate mutable
state. Validated zero offset/stride4/NonWritable CB layout; unknown values retain
texture requirements. Actual shader removes8 disabled arms and reflected binding
42/298, with proof dependency0/43/1. Original shader instructions outside removed
arms stay intact. game-shaders.mjs applies proof before existing lowering.

Production gpu-draw first compiles normally. Only if a used texture slot is
missing, it reads real bound CB ranges in the serialized GPU queue (64KiB budget)
and recompiles with those exact words. Active missing resources still fail.
Cache keys include the actual words; specialized variant cache bounded256.
New successful-draw counters uniformSpecializedDraws/uniformBranchesRemoved.
All3 builder scripts copy the production helper. No C/header/Wasm change.

Validation: build/codex-uniform-branches-integration-report.json,289 checks:
actual captured fragment GPU pixel parity (zero,-zero,enabled), NaN/nonzero/
unknown retain binding; production packet with missing2 renders correct pixels,
changing uniform to enabled rejects without drawing, restoring zero succeeds,
and binding2 executes the original enabled effect. Existing full GPU draw suite
714 checks/20 draws passed: codex-uniform-branches-draw-regression-report.json.
Failure capture late-record/budget smoke check passed. Initial fixture entry name
was corrected to actual Naga main; final tests above succeeded.

Boot19 started03:13UTC on SAME2adc Wasm with new JS. Dedicated ssbu-web Node118640.
Preserve this run, poll without reload/start. No linker pending. Passing the old
unbound-texture stop and menu/input/match remain unverified for this new run.


## Boot17 archived at unbound texture; boot18 diagnostic run started — 22 October 2026

Boot17 on served 2adc6457768e552013c47438b670be9e036010f1485bd50895fe5f046d6e65dc
ended at `Unbound shader texture 1/2`, nnMain fn71002c48f0/LR71037f0b40,
37,935,280 imports,13,979 draws/798 presents,293 textures/19 pipelines.
Full SHA-tagged archive: build/codex-shared-family-unbound-texture-report.json.
It passed the former shared-callback stop; menu/input/match remain unverified.
Frame diagnostics show loading-logo-sized pixels through frame540, black at600+.
The prior first12 successful draw captures do not include failing program13731.
Its packet binds fragment units0/1, but required unit2 is absent; cause unknown.

Added bounded failing-draw capture (same192KiB budget, evicts early successful
captures on failure) including actual Maxwell source/options/layout/active bindings
and buffer prefixes. No substitute texture or game behavior change. JS copied to
build/web-poc. Recycled only terminal dedicated ssbu-web after archive; boot18
started02:55UTC, same Wasm/new diagnostic JS, CLI session8832. Preserve active
run, poll without reload/start. No C rebuild/linker pending. Older boot17-live
notes below are historical.


## Shared-family runtime published; boot17 CONFIRMED LIVE — 22 October 2026

Exec13599 completed successfully, including publication, exact-SHA Node306/306 and
guarded Chrome startup. **No linker remains pending** from this build. Served SHA:
`2adc6457768e552013c47438b670be9e036010f1485bd50895fe5f046d6e65dc`,
281989948 bytes,99134 Wasm functions,max numeric locals10812. It includes the shared
71017f3010 callback family (native graph70279 /1096 units) and prior renderer/input.
Archives `build/codex-post-clone-build.json`, `codex-post-clone-node-report.json`.
State codex-post-clone-followup.json is browser-running; terminal exec13599 exit0.
Old optimizer150676 /builder31884 /primary119964 are historical; do not restart them.

**Boot17 is confirmed LIVE** in ssbu-web:306 constructors,1,307,267 imports at36sec,
threadFailed0. Preserve this run. Do NOT reload or invoke start harness again.
Use codex-follow-visible.cjs /codex-poll-compact.cjs. No visible image/menu/input/match
from this new module has yet been verified, nor passing the former shared callback
failure. Previous loading-logo screenshots prove only their explicitly tagged runs.

Asyncify took4636.45 seconds; per-pass and final validation completed. Peak observed
optimizer private memory exceeded20GB on16GB RAM, with Windows expanding paging;
no user applications were closed and no system settings changed. Corrected native
stderr handling worked through the entire real build, with no orphan recovery needed.
The builder rebuilt257 objects and reused the remaining cache; protected headers
were not changed. Older validation/link-live notes below are historical.


## Shared-family Asyncify finished; post-pass validation live — 22 October 2026

Optimizer150676 completed Asyncify in4636.45 seconds, then entered validation.
This is a real pass transition in codex-runtime-post-clone-build.log, not a
publication/constructor/game success. Primary119964 and builder31884 remain live
in exec13599. Do not restart the linker. Wasm is still old f2d5058... .

Validation currently has high memory pressure: optimizer private memory about19.1GB,
physical RAM16GB, free physical16MB, virtual commit free about2GB with Windows
having expanded commit capacity to~42GB. CPU continues advancing. Only the dedicated
idle test browser remains; no large stale test Chrome process was found. Preserve
the existing run, keep browser idle, and avoid concurrent GPU/game boots. No user
applications were closed or system settings changed. Await actual exit/publication;
slow/silent validation is not terminal. Guarded script will check Node306 +SHA and
boot only after successful publication. Menu/input/match remain unverified.


## Boot16 archived; shared virtual callback family prepared and new build live — 22 October 2026

Boot16 on f2d505827159c1c4... ended at missing callback71017f3010 after39,236,695
imports,17,741 actual draws/964 presents,255 textures. Full SHA-tagged archive:
`build/codex-post-clone-callback-report.json`. Its actual loading logo was separately
captured/visually inspected in `output/playwright/codex-boot16-loading-logo.png`;
live metadata-associated archive `codex-boot16-loading-progress.json`. No menu/input/
match verified. Prior clone trap7102363140 did not stop this run; no per-callback
counter is present, so do not invent a direct callback counter claim.

Reported guestFunction7103723360 is stale from its initializer. LR7103725c7c identifies
the ACTUAL call in f_7103725c30 at7103725c78:load object[0] then vtable+16 and call.
Missing f_71017f3010 is a real virtual trampoline loading vtable+136 and tail calling.
Source/relocation evidence `codex-post-clone-callback-discovery.json` and
`codex-post-clone-actual-caller.c`; this is not a substituted stub.

The observed target appears in107 bounded relocation groups, largest21 entries,
664 distinct targets. Added only observed71017f3010 to runtime_roots.json; existing
--main --pointer-tables --callback-tables prepare completed in OBSERVED mode,
not --all-pointer-tables. Graph62442 ->70279 functions,1079 ->1096 units,
source370460388 ->494114202 bytes through real direct dependencies and discovered
tables;377 callback groups/2438 targets, no missing definitions. Prior manifest
`codex-manifest-before-71017f3010.json`. No protected header change.

Boot16 was archived before recycling only the dedicated ssbu-web browser; it is
now idle `/` to preserve memory. Served Wasm is STILL f2d5058..., without this new
family. Do not boot the old module during the current link.

**NEW BUILD LIVE**: exec13599, primary pwsh119964, Python launcher145536 and actual
builder31884 both confirmed alive AFTER the native stderr warning. Native invocation
wrapper correction is working in the real build (not merely its3 fixture checks).
Script `build/codex-finish-post-clone.ps1`, state `codex-post-clone-followup.json`,
log `codex-runtime-post-clone-build.log`, required --jobs8/trace-asyncify. Source
compilation rebuilt257 objects; finalizer135076 completed and wasm-opt150676 is live. Preserve current process;
no second linker or recovery watcher was started. Original loading-callback scripts
and exec27080/30476 are HISTORICAL and terminal; do not rerun them.

Current script will verify unchanged roots/manifest, publication SHA/new module,
Node306 constructors against exact SHA, then guarded boot. Expected artifacts:
codex-post-clone-build.json,codex-post-clone-node-report.json and boot-start.log.
Check state/process before manual work; once browser-running poll without reload.


## Callback Wasm published and boot16 CONFIRMED LIVE — 22 October 2026

Existing emcc92808 finished successfully. Recovery exec30476/pwsh101536 completed
validation/publication, Node constructors and guarded browser boot. **No linker is
pending** from this build; both exec27080 and30476 are terminal. Served runtime:
`f2d505827159c1c4f9c0081a07ec24a27819c2d47330f41983803f426f50e8b1`,
215296600 bytes,87047 functions,max numeric locals10812. Metadata:
`build/codex-loading-callback-build.json`. Node306/306 for EXACT same SHA:
`build/codex-loading-callback-node-report.json`. This module includes observed
callback7102363140/vtable group, prior refill+input, and current shader/view JS.

**Boot16 confirmed live**,306 constructors and317408 imports at18 seconds. Preserve
Playwright session ssbu-web; do not reload/start the harness again. Poll compact or
codex-follow-visible.cjs. Real menu/input/match and passing the loading callback
are unverified. Boot15's actual loading screenshot remains preserved separately.
All old notes saying linker148724 is live /served966 are now historical.

Primary exec27080 ended with Windows PowerShell NativeCommandError on a harmless
emcc stderr warning (-pthread+ALLOW_MEMORY_GROWTH). This explains the missing
build_runtime parent while emcc survived; it was not an emcc link failure.
Recovery confirmed emcc exit0 before publishing the existing staged pair. State
`codex-loading-link-recovery.json` is browser-running; primary failed state is
historical and must not trigger another link. Publication log is retained.

For future reuse, codex-finish-loading-callback.ps1 now wraps native invocations
with temporary Continue, checks actual exit code, restores Stop. Original script
archived as codex-finish-loading-callback-original.ps1. Windows PowerShell fixture
passes3 checks:stderr warning exit0 accepted,exit7 rejected,preference restored.
Evidence codex-native-stderr-policy-report.json. The edited script was NOT rerun;
only its function was extracted for tests. Do not rerun historical continuations.
The live-progress helper now obtains published SHA metadata instead of hardcoding
old966; its SHA association scope is explicit. No C/header change in this step.


## Callback link moved to wasm-opt148724 — 22 October 2026

Finalizer104356 finished; optimizer148724 started2026-10-21T23:15:44.7409453Z.
Confirmed alive with CPU172.625 seconds at23:17:07Z; primary119308 and recovery
101536 also live. Health snapshot `build/codex-loading-build-health.json`.
Preserve exec27080 and30476; keep polling the same handles despite silent logs.
Original build_runtime154480 remains missing; recovery watcher holds existing
emcc92808 and primary119308 handles and can publish ONLY after both terminate
and emcc exit0. Served Wasm is still9668733f..., not the callback build.
Browser remains idle at `/`; do not boot the old module again while linking.
No other linker was started. Menu/input/match remain unverified.


## Boot15 archived at missing callback; callback link live — 22 October 2026

Boot15 ended at `uncompiled indirect target7102363140`, caller7103726120,
LR7103726294,36,568,949 imports,14,417 actual draws/823 presents,255 textures.
Full SHA-tagged archive: `build/codex-loading-callback-report.json` (runtime9668733f...).
Loading logo/animation was actually visible before failure, but menu/input/match
remain unverified. Preserve `output/playwright/codex-first-loading-logo.png`.
Browser ssbu-web has been recycled AFTER terminal archival and is idle at `/`
to reclaim memory for linking. Older notes saying boot15 is live are history.

Caller loads a vtable slot+32. Actual relocation group71050b7cc0 contains five
callbacks:7102363050,7102363120,71017f5230,7102363130,7102363140; target's clone body
also constructs an object with this same table. Evidence:
`build/codex-loading-callback-discovery.json` plus extracted f_*.c in build/.
Added only observed7102363140 to runtime_roots.json. Prepare completed with
--main --pointer-tables --callback-tables (no broad/all scan):62442 functions,
1079 emitted units, observed mode,270 callback groups/1774 targets. Previous
manifest archived in codex-manifest-before-7102363140.json. Only runtime_c02f2.c
and runtime_table.c recompiled. Protected headers unchanged.

**Existing full link must be preserved.** Started --jobs8 --trace-asyncify via
`build/codex-finish-loading-callback.ps1`, exec27080, primary pwsh119308.
Emcc92808 (launcher156332), current finalizer104356 confirmed live; CPU141 seconds
at last observation. Log: `build/codex-runtime-loading-callback-build.log`.
Asyncify/validation can be silent for a long time; do not start another linker.
Current served module is STILL old9668733f...; callback fix not published yet.

Important orchestration evidence: original build_runtime PID154480 is missing,
although its emcc child92808 and primary119308 remain live. Recovery watcher
`build/codex-recover-loading-link.ps1`, exec30476/pwsh101536 holds emcc/primary
process handles and waits for BOTH. State `build/codex-loading-link-recovery.json`.
It never compiles/links: if emcc succeeds but primary did not publish, it validates
and publishes the existing staged pair using build_runtime.publish_module, checks
root/manifest hashes, Node306 constructors+SHA, then starts guarded Chrome boot.
If primary already published it preserves primary result. Primary state:
`build/codex-loading-callback-followup.json`. Check BOTH states before manual work;
never run either continuation a second time. If an execution observation times
out, poll the same handle/process; no restart based on a silent log.

After success, archives will be codex-loading-callback-build.json and
codex-loading-callback-node-report.json, and boot-start.log in build/.
Only after state browser-running poll the browser without reload.


## FIRST VISIBLE GAME LOADING LOGO verified in boot15 — 22 October 2026

Boot15 is live; do not reload it. At230 seconds,1185 actual draws/180 presents,
source frame180 had7997 nonblack pixels at bounds[1759,955,119,111]. Actual canvas
screenshot was captured while worker remained live and visually inspected:
`output/playwright/codex-first-loading-logo.png` shows the Smash logo with a loading
arc in the lower-right. This is the first verified game image, not a menu or match.
Live SHA-tagged progress archive `build/codex-first-loading-logo-progress.json`
(saved at252 seconds) is separate from any terminal report.

Actual runtime confirms new paths:24 array sampling views and46 logical compressed
snapshots by frame180,30 compiled shaders,15 pipelines. No current error observed.
Keep following the live run to menu and verify input/playability before completion.
The observer `build/codex-follow-visible.cjs` waits at most50 seconds, returns a
compact status and captures `output/playwright/codex-latest-game.png` while live.
It preserves the first loading screenshot. Default startup has no time limit.

Additional selected mip/layer pixel checks pass through the promoted array view:
714 checks/20 draws; `build/codex-array-view-selected-slice-report.json`.
Shader-lowering draw checks are opt-in via testGpuDraw(device,{shaderLowering:true})
so the dump-free C fixture compiler import does not require shader Wasm assets.


## Boot15 live with shader-declared single-layer array views — 22 October 2026

Boot14 passed swizzle and submitted one real logical compressed sampling snapshot
(counter1/subresource1). It then failed binding an R8 atlas 2048x4096 as2D when the
shader declares2DArray. Archive `build/codex-refill-array-view-report.json` contains
761 draws/152 presents,26 compiled shaders, actual texture117/view166 with swizzle
[1,1,1,2]. Presentation-source samples still black; no menu/input proof.

SPIR-V reflection now retains image dimensionality in layout.textureDimensions.
The draw path creates a2DArray view of the original selected single2D layer when
required by the shader, preserving format/aspect/mip/layer range. Other mismatches
still fail explicitly. The adapted descriptor also reaches logical/depth snapshot
helpers. Counter arraySamplingViews records actual adaptations.

GPU draw fixture now exercises a real shader-declared2DArray with native2D packet
and (1,1,1,R) mapping:649 checks/19 draws, arraySamplingViews1, logical copy1. Report
`build/codex-array-view-draw-report.json`. Existing SPIR-V contracts still pass.
The new array GLSL fixture is included in build_shader.py. JS copied to web-poc;
no C/header/relink. Boot15 started successfully via guarded harness in ssbu-web.
Preserve current run and poll compactly. No linker running; game unverified.


## Boot14 live after shader component-swizzle lowering — 22 October 2026

Boot13 stopped at NVN texture swizzle needs shader lowering after835 indexed draws,
167 presents,23,024,744 imports. Texture129/view178 requests [1,1,1,2] = (1,1,1,R).
Full SHA-tagged archive: build/codex-refill-swizzle-report.json, runtime9668733f... .
All source samples remain black. Logical copy count0 does not verify in-game logical
compressed snapshots; removal of the former bounds guard alone is not that proof.

gpu-draw.mjs now forwards per-stage descriptor maps as immutable compiler variants.
spirv-compat.mjs maps typed sample/fetch results and gather component selectors.
Identity, signed/unsigned constants, invalid maps and unresolved/comparison forms
have explicit handling. No texel substitution or guessed shader replacement.
Validation:1430 GPU checks/22 variants,648 draw checks/19 draws including the actual
packet/compiler/cache path and identity reversion,20 actual captured Maxwell sources
compile to validated WGSL with mappings. Reports in build/:codex-swizzle-pixels-report,
codex-swizzle-draw-report,codex-game-swizzle-compile-report (all .json).
Reference: https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html
New swizzle-test.frag fixture is generated by build_shader.py.

Logical sampling fixture harness had used a wrong report envelope; the returned
864 checks were real but archived file was stale. Fixed results envelope, checked
HTTP save status and reran; codex-logical-sampling-report.json now holds864 checks.
No C/protected header change or relink. JS is copied to web-poc.

Boot14 confirmed live in Playwright ssbu-web on same Wasm/new swizzle JS. Preserve
active tab; poll without reload. No linker running. Visible game/input unverified.


## Boot13 live with logical compressed-texture sampling — 22 October 2026

Boot12 on runtime `9668733fd5cc2e1b75ec0109f6e688d638884b022dc75fd9a05d00d27a411830`
passed the negative minimum sampler LOD case, then stopped at **Compressed texture
logical bounds need sample-coordinate lowering** (785 indexed draws /157 presents).
Full SHA-tagged archive: `build/codex-refill-compressed-bounds-report.json`.
Presentation-source samples were still black; no visible game or input verified.

New `gpu-logical-sampling.mjs` decodes padded compressed GPU storage into exact
logical mip/layer extents in RGBA32float before sampling, preserving decoded signed
values, sRGB conversion and the guest sampler/coordinates. It is wired into real
`gpu-draw.mjs`, copied to web-poc and included in the three builder copy lists.
No C change or relink. GPU fixtures: **864 logical sampling checks**, plus
**518 draw checks /17 draws** including the real draw path with a 3x3 BC1 source.
Reports: `build/codex-logical-sampling-report.json`, `build/codex-logical-draw-report.json`.
These fixtures do not establish game parity. Cube/HDR cases are not yet pixel-tested.

**Boot13 is confirmed live** through Playwright session `ssbu-web`: 306 constructors,
16,789,257 imports at144 seconds in nnMain. Preserve this run; poll compactly without
reload. No linker was started. Completion still requires visible game and input.


## Boot11 retained real draw inputs; negative minimum sampler LOD fixed — 22 October 2026

The first refill-enabled boot ended at `WebGPU graphics draw failed`, with
GPU error **Sampler LOD range requires unsupported clamp semantics**, 22,519,718
imports. SHA-tagged full archive: `build/codex-refill-first-boot-report.json`,
runtime9668733f... . It retains **9 real draw-input records**, including a later
flat-color rectangle draw788, plus the failing draw packet (6 indexed vertices,
texture123/view172). All sampled presentation-source pixels remain black.
Do not claim callback execution, a visible game or actual input from this report
alone; it does not include a refill-event counter.

The actual failing sampler is min3 /mag1 /nearest mip with LOD **[-1000,1000]**.
`gpu-samplers.mjs` now accepts finite negative lower clamps when the upper clamp
is nonnegative, mapping the lower endpoint to0 while retaining min/mag/mip
filters and all positive LOD behavior. Reversed/nonfinite ranges and negative
upper clamps still reject explicitly. A GPU counterexample showed that blindly
normalizing a negative upper clamp can change min/mag selection at positive
explicit LOD; that attempted generalization was removed, not shipped.
Counterexample: `build/codex-negative-upper-lod-counterexample.json`.

Validation: CPU descriptor/cache contracts pass, and **212 actual GPU pixel
checks** pass, covering the game's interval, mixed min/mag filters, negative
input LOD and fractional lower clamps, together with existing mip/wrap/compare
cases. Evidence `build/codex-negative-lod-report.json`. Primary LOD/filter
reference reviewed: Khronos Vulkan textures.adoc sections LOD Operation,
Image Level(s) Selection and Texel Filtering:
https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/chapters/textures.adoc
No C/header/linker change was needed; source sampler JS was copied to web-poc.

Actual draw sources can now be inspected independently: Node helper
`build/codex-extract-draw-inputs.mjs` replayed captured Maxwell inputs/options
through the real compiler and emitted **14 unique GLSL/SPIR-V variants** plus
`captures.json` in `build/codex-game-draw-inputs/`. Buffer decoding is in
`decoded-buffers.jsonl`; ranges retain explicit completeness flags. This is
input/compiler evidence, not a complete textured draw replay or pixel parity.

**Boot12 is live** on the same9668733f... Wasm with the sampler JS correction.
The completed browser session was recycled after full archival, then the guarded
harness started the new run. Poll compactly without reload. No linker is live.

## Refill-enabled Wasm published; real Chrome boot11 is live — 22 October 2026

Refill builder162384 /wasm-opt152332 and continuation exec80349 have completed
successfully. **No linker is currently pending from this continuation.** Served
runtime SHA `9668733fd5cc2e1b75ec0109f6e688d638884b022dc75fd9a05d00d27a411830`,
215,293,168 bytes, 87,044 functions, max numeric locals10,812. It includes C
input and command/control-memory refill. Metadata `build/codex-refill-build.json`;
Node **306/306 constructors** pass for that exact SHA in
`build/codex-refill-node-report.json`. Continuation state is `browser-running`:
`build/codex-refill-followup.json`.

The guarded harness started Chrome at `?runtime=fibers-compact&diagnosticFrames=1`.
Boot11 has passed constructors and is in nnMain startup. **Do not reload or run
the start harness again.** Poll `build/codex-poll-compact.cjs` through session
`ssbu-web`; it omits capture base64 data. On failure archive the full
`build/web-poc/runtime-browser-report.json` before another attempt, then inspect
`drawDiagnostics` and `frameDiagnostics`. Actual callback execution, nonblack
presentation, menu and game input remain to be verified; the prior game reports
were from old f0a35422... and do not verify this new module.

## Input module published and constructors verified; refill link started — 22 October 2026

The previous input build finished successfully; old builder137664 and
wasm-opt160196 are terminal. Published Wasm SHA:
`3cf87b20c979dec0f666c87d7d8fc2f8d90d75c6c4ab96e7f37bfb2925041184`,
215,292,029 bytes, 87,043 functions, maximum numeric locals 10,812.
Metadata: `build/codex-input-build.json`. Node **306/306 constructors** pass for
that exact SHA (`build/codex-input-node-report.json`). Binary export inspection
also verifies `web_input_pointer` exists as a function export, index86494:
`build/codex-input-export-report.json`. Export presence/constructors do not
establish actual browser input or visible gameplay.

Continuation exec80349 /pwsh96676 automatically moved to **linking-refill**.
New builder PID162384 (launcher167992), same required `--jobs 8`, log
`build/codex-runtime-refill-build.log`. It links the tested refill C source SHA
`1534F068096ACAB56228F904EC95D35EB6727DD1619075F7428D11DD80E970FA`.
Finalizer108740 completed; current **wasm-opt152332** runs the refill Asyncify
validation. Its Asyncify pass completed in **3,623.06 seconds**; post-pass
validation is live. This is the only linker; preserve it during long silent
periods. Publication and constructor/browser checks have not happened yet.
Do not start another linker or re-run the continuation. Inspect actual current
wasm-opt PID rather than reusing old160196. State:
`build/codex-refill-followup.json`. It will verify the new publication/SHA and
constructors, then boot Chrome with frame/draw-input diagnostics enabled.

**Served module is now the input SHA3cf87b20..., still without refill.** Historical
game reports from f0a35422... must not be attributed to it. Browser remains idle
at `/` to avoid repeating the large memory pressure during this new link.

## Bounded actual draw-input capture ready for next boot — 22 October 2026

Opt-in `diagnosticFrames=1` now also records input evidence for the first 12
distinct pipeline keys. It retains the parsed immutable actual draw packet,
actual retained Maxwell control/data (at most 32 KiB per stage), exact compiler
options/metadata, and up to 4 KiB prefixes read back from used vertex/index and
active uniform/storage GPU ranges. Each range records its original bounds,
captured size and whether it is complete. Total serialized capture is capped
at 192 KiB, leaving room for shader-failure data in the 1 MiB runtime report.
It is explicitly **not** a complete replay (textures and omitted range tails
are not captured). Diagnostic failures do not stop game drawing; feature is
off by default. No protected header or C source changed in this step.

Physical GPU draw fixture now passes **453 checks /16 draws**, including exact
vertex and unaligned-uniform byte capture, oversized-range prefix labeling,
report budget enforcement and injected capture failure. Report:
`build/codex-draw-input-capture-report.json`. Served JS was updated; compact
poll prints capture metadata only, not base64 bytes. Next guarded boot already
uses the opt-in query. Do not reboot the old module while the linker is live.

Source buffer staging review found `web_gpu_draw_range` uploads CPU-visible
pool data at submission. Winding review found no evidence warranting a face
flip: WebGPU's polygon-area formula and native GL_UPPER_LEFT both negate the
ordinary framebuffer-coordinate shoelace sign; existing strip/back-cull GPU
tests also pass. Primary references inspected:
https://raw.githubusercontent.com/gpuweb/gpuweb/main/spec/index.bs
(rasterize polygon), and Khronos ARB_clip_control sections 13.6/14.6.1:
https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/extensions/ARB/ARB_clip_control.txt
These checks do not identify the source of the black game textures. The next
actual draw-input capture is intended to provide evidence for that analysis.

## Black presentation sources confirmed; completed browser session recycled — 22 October 2026

Boot10 terminated at the same unported command-memory callback after **813
indexed draws /164 presentations**, 23,519,585 imports. SHA-tagged archive:
`build/codex-black-source-report.json`. Real GPU source readbacks of the full
1920x1080 crop at presentations 1,2,3,4,60,120 all have RGB minimum/maximum zero
and zero nonblack pixels. Frame60 was sampled after 288 actual draws. Thus
black pixels already exist upstream of the canvas presentation step. This does
not yet identify whether startup content, rendering state, data or shaders are
responsible; do not invent a culling/shader fix without further evidence.

After terminal confirmation and archival, the dedicated Playwright Chrome
session was closed/reopened at `/` with no runtime. Its renderer171440 had
14.39 GB private committed memory; Windows had only ~484 MB physical RAM and
~2.83 GB commit headroom left while wasm-opt privately held ~10.28 GB.
The renderer belonged to the isolated `playwright_chromiumdev_profile-jqKiWA`
profile; its sole tab was this completed test. Recycling freed commit headroom
to ~9.87 GB immediately, without touching the linker or user browser sessions.
**Do not reboot the old Wasm again while linking** without a concrete need;
leave the fresh `ssbu-web` session ready for the guarded refill-build follow-up.
Input linker160196, builder137664 and follow-up96676 /exec80349 remain live.

## Opt-in presentation-source pixel diagnostics — 22 October 2026

`?diagnosticFrames=1` enables bounded raw GPU readbacks of the actual presented
texture/crop on frames 1..4 and every 60th presentation. Reports retain at most
8 samples (first plus recent): RGB extrema at native 8/10-bit precision,
nonblack pixel count/bounds and raw cropped-byte hash. Alpha alone does not
count as nonblack. These are source pixels, not proof of canvas visibility or
gameplay. Diagnostic readback failures are reported separately and do not fail
normal presentation; the feature is off by default.

Actual GPU validation passes **125 presentation checks +22 diagnostic checks**,
covering crop/sRGB display pixels, matching source crops/hashes, alpha-only
black, cropped-out color, BGRA ordering, RGB10 precision and injected readback
failure with continued presentation. Evidence:
`build/codex-frame-diagnostics-fixture-report.json`. Source modules were copied
to the served directory. Existing color-clear cancellation test also passes.

A tenth boot is live with `diagnosticFrames=1` on the same old f0a35422... Wasm,
to investigate black presentation sources while the input linker continues.
Poll `build/codex-poll-compact.cjs` without reload. The guarded start harness now
uses this query for the future refill-enabled boot too. The diagnostic heap
from boot9 was archived before starting boot10; it is no longer in the page.
Sequential continuation exec80349 / pwsh96676 remains live and waiting on input
builder137664 / wasm-opt160196. No second linker has started.

## Command-memory refill implemented; diagnostic boot reached 948 draws — 22 October 2026

Boot8 completed 803 actual indexed game draws / 162 presentations, but its
captured canvas was black (`output/playwright/codex-first-game.png`). The
subsequent opt-in diagnostic boot completed **948 indexed game draws**, 191
presentations, 12 compiled shaders and 8 pipelines before the same terminal
`NVN command memory refill callback not ported` at command buffer
`8004da1280`, guest function `71000322b0`, LR `71000567d8`, 24,454,361 imports.
Neither draw counts nor presentations establish a visible game or usable input.
Archive: `build/codex-diagnostic-refill-runtime-report.json`, still runtime SHA
`f0a354229b00b559b4550675fcc545382ceeb2869665abfb34832db842d5df85`.

The diagnostic shared heap was preserved after failure, without reloading the
page. `build/codex-refill-memory-report.json` identifies the live host command
buffer with command size 512 / used 496, callback `7100031be0`, user
`8004da1268`. Guest page-table lookup additionally found command/control
callbacks `710363eb60` / `710363ebe0`; all three already exist in
`runtime_table.c`, so no graph widening is needed. Their lifted bodies confirm
event 0/1 and the pointer to the requested minimum size passed by the wrapper.
Lookup evidence: `build/codex-refill-targets-report.json`.

`host_gpu_commands.c` now invokes the guest refill callback on exhausted command
or control space, releases the command registry mutex during reentry, restores
the pending call's registers while retaining exclusive-monitor effects, and
validates the returned capacity. Command-callback disabling does not disable
control refill. Recursive exhaustion, finalization/recording lifecycle changes
inside callbacks, callback traps and insufficient replenishment stop explicitly.
The same path covers all six previously guarded recording branches and control
space at EndRecording.

Validation: SDK-backed C/Worker/WebGPU fixture **3,295 checks**, zero failures,
2 fixture draws, including callback reentry without deadlock, command/control
refill, rounded minimum size, argument restoration, callback-disabled behavior,
insufficient memory and callback failure cleanup. Report:
`build/codex-refill-probe-report.json`. Compile-only with `--jobs 8` succeeded:
`build/codex-refill-compile.log`. Protected headers were not changed. This source
change is **not yet in the served runtime or the currently linking input build**.

Sequential continuation is live in exec **80349**, script
`build/codex-finish-refill.ps1`, state `build/codex-refill-followup.json`. It
verified input builder PID 137664 with exact start time and waits for that
existing build to exit. It then checks the input publication and 306 constructors,
links the refill source with `--jobs 8 --trace-asyncify`, checks its publication
and constructors by SHA, and starts the guarded Chrome boot. **Do not launch a
second linker or rerun this continuation while it is live.** Old follow-up
`codex-finish-integration.ps1` is obsolete. Poll the new continuation and actual
processes; after `browser-running`, use `codex-poll-compact.cjs` without reload.
Last observed input wasm-opt PID 160196 remains live, CPU >8,072 seconds.

## Seven real draws; proven viewport-zero output lowered — 22 October 2026

The seventh boot completed **7 real indexed game draws**, 10 compiled shaders,
7 pipelines and 2 depth snapshots, then stopped at the next vertex shader's
`ShaderViewportIndexLayerEXT` capability. Archive
`build/codex-integrated-seven-draws-report.json`, 19,963,284 imports, runtime
SHA f0a35422... . Present count is still the 3 pre-draw presentations, so this
does not yet demonstrate a visible rendered game frame or menu.

The actual captured shader (`game-layer-failure.json`, source/replay in
`build/codex-game-layer-shader.json` and `.vert`) writes
ViewportIndex = InstanceIndex - BaseInstance. Its packet draws one instance
at base 0 into one layer/viewport. The SPIR-V lowerer now proves all Layer/
ViewportIndex stores are exactly zero before removing those outputs and their
capability. It uses the fixed single-instance draw context, constant expression
evaluation and CFG reachability to select an unoverwritten dominating local
store; pointer escapes, branch overwrites, unknown/nonzero outputs, uninitialized
outputs and proof-budget exhaustion remain explicit errors. No blanket
viewport/layer suppression or multi-viewport emulation is claimed.

Validation: CPU contracts include unsafe branch writes and pointer aliases;
**132 GPU checks** cover actual pixels at firstInstance 0 and 9, plus rejection
of nonzero/unproven output (`build/codex-viewport-report.json`). The exact
captured game vertex shader passes Wasm compilation and WebGPU shader
validation with the zero proof (`build/codex-game-layer-compiled-report.json`).
Existing draw fixture still passes 444 checks / 16 draws.

An eighth real boot is starting with the proven specialization. Poll compactly
without reload. C input link exec 10084 / wasm-opt160196 is still separate and
live; do not start another linker. Game frame/menu/input remain unverified.

## No depth/stencil attachment state corrected — 22 October 2026

The sixth boot compiled the formerly failing dynamic-gather shader in-game
(4 shaders compiled) and reached a later renderer guard: depth/stencil test
enabled with **no depth attachment**. Archive:
`build/codex-integrated-no-depth-attachment-report.json`, 19,831,791 imports,
same f0a35422... runtime, 2 completed indexed draws.

The renderer no longer rejects that native state or invents a depth buffer:
when the attachment is absent, it omits WebGPU depth/stencil state. Native
OpenGL semantics make those unattached tests pass. The GPU fixture verifies
actual color writes with both depth and stencil NEVER enabled and no matching
attachment. It now passes **444 checks / 16 draws**. The fixture's preserved
original program token array was also separated from mutable draw state so
this test really uses its intended shader pair, not a previous flat-color pair.
Report `build/codex-draw-report.json`.

A seventh real boot is starting with this JS correction. Poll compactly,
without reload. Input C link exec 10084 / wasm-opt160196 remains live; do not
start a second linker. A game frame/menu/input remains unverified.

## Actual dynamic-gather shader reproduced and lowered — 22 October 2026

The fifth boot retained the **actual failing fragment source**: 1,824 control
bytes / 2,388 data bytes, shader stage 5, four dynamic offset gathers. It stops
at the explicit `0x10` guard after the same 2 completed draws. Archive:
`build/codex-integrated-gather-dynamic-report.json`. Local replay capture:
`build/web-poc/game-gather-failure.json`; generated GLSL/SPIR-V evidence:
`build/codex-game-gather-shader.json` and `.frag`.

`spirv-compat.mjs` now lowers signed/unsigned 32-bit 2D/2D-array gather offsets
into the same base-mip gather footprint. It queries the real view's mip-0
extent, computes floor(uv*extent - 0.5) + offset, and centers the new gather
coordinate on that texel quad; array layer is retained. The original gather
and sampler preserve component ordering and address modes. ConstOffset uses
the same path, avoiding WGSL's narrower constant-offset range. Unknown masks,
including ConstOffsets arrays, still reject explicitly. SPIR-V ImageQuery
capability/imports/types are added only as needed. No unsupported offset is
silently dropped by Naga.

Validation: **576 GPU pixel checks** for constant/dynamic alpha gather, signed
offsets including (31,-32), clamp-to-edge and non-power-of-two extents, in
`build/codex-gather-report.json`. The exact captured game shader now compiles
all 4 gathers and passes WebGPU shader validation (126,072 WGSL bytes):
`build/codex-game-gather-compiled-report.json`. Existing 16 captured shader
compilations and Wasm compute/readback tests still pass. These prove compiler
and fixture behavior, not a complete game frame. The sixth real boot is now
starting; poll **`build/codex-poll-compact.cjs`** to avoid printing shader bytes.

## Input integration link running — 22 October 2026

Exec **10084** runs `build_runtime.py --fibers --compact-codegen --jobs 8
--trace-asyncify`, log `build/codex-runtime-input-build.log`. The previous link
and follow-up were terminal before it started; this is the only live linker.
Latest verified wasm-opt PID **160196**, CPU >321 seconds. It integrates the
already tested C input snapshot/export; no protected cache header changed.
**Do not start another linker into this staging path.** Served runtime remains
SHA f0a35422... until this build publishes. The browser currently boots that
earlier module with the updated JS/compiler. Do not attribute its reports to
the future input-enabled Wasm. The old follow-up script hardcodes the previous
builder PID and must NOT be rerun for this build without updating its identity.

## First two actual game draws completed — 22 October 2026

The fourth integrated boot completed **2 actual indexed game draws**, with
3 compiled game shaders, 2 pipelines, 2 samplers and 1 real depth-sampling copy.
It then stopped compiling the next fragment shader at unsupported capability
`ImageGatherExtended`. Evidence `build/codex-integrated-two-draws-report.json`,
19,845,595 imports, same runtime SHA. The ordinary filtered depth adaptation
is therefore verified through actual game draw submission. 3 presentations
precede these draws; a rendered game menu/image is still not proven.

SPIR-V lowering now admits the ImageGatherExtended declaration only with
gather operands already supported by Naga (constant components and constant
offsets). Dynamic offsets and other unsupported image operand masks fail
explicitly before Naga, whose parser otherwise ignores unknown masks.
GPU readback verifies the four gathered alpha texels, ordering and constant
offset in **64 checks**, including the explicit capability declaration:
`build/codex-gather-report.json`. CPU negative operand contracts also pass.
The GLSL fixture itself does not require that capability, so this does NOT
claim dynamic gather offsets are implemented or that the game shader passed.

Future translator failures now include bounded actual retained Maxwell
control/data captures (base64, <=512 KiB combined) plus compile options in
`lastShaderFailure`. This makes a failing game shader reproducible without
rebooting for each compiler experiment. Larger captures are explicitly omitted
to preserve the local report's 1 MiB limit.

A fifth real boot is starting with the gather gate and shader capture. Poll
without reload. The goal still requires an actual visible game and input;
fixture tests or two submitted draws are not completion.

## Ordinary filtered depth sampling adapted — 22 October 2026

The third boot passed the unused-binding gate and reached a real D24S8 texture
binding. WebGPU rejected the native ordinary float sampler's expected Float
sample type against Depth/UnfilterableFloat. Evidence:
`build/codex-integrated-depth-sampling-report.json`, 19,775,101 imports,
2 compiled shaders / 1 pipeline, still 0 completed game draws.

`gpu-depth-sampling.mjs` now copies ordinary sampled depth views on GPU into
`r32float`, keeping the original shader and sampler. The copy uses textureLoad
and r32float storage, preserving the sampled f32 depth value; it copies each
selected mip/layer before draw, with temporaries released afterward. Repeated
bindings of the same view within a draw share one snapshot. Shadow/comparison
image types are identified by SPIR-V reflection and keep their original depth
view. `float32-filterable` is required (available and requested on this adapter)
instead of silently replacing linear filtering or reducing depth precision.
The new module is in runtime, GPU probe and Maxwell builder copy lists.

A prototype direct-depth type rewrite exposed that noncomparison depth
sampling requires a nonfiltering binding; that prototype was removed from
production rather than changing the guest's filtering. The actual GPU test
uses the retained ordinary GLSL/SPIR-V shader with simultaneous color/depth
sampling and verifies linear interpolation across depth 0.25/0.75. Both
D32 and D24S8 pass **66 checks each**; report
`build/codex-depth-sampling-report.json`. Existing draw fixture remains
**380 checks / 15 draws**; CPU interface/SPIR-V contracts also pass.

A fourth boot of the same runtime Wasm is starting with these JS changes.
Poll `codex-poll-boot.cjs` without reload. Game draw/menu/input remain unverified.

## Automatic-layout binding usage reflected from Naga — 22 October 2026

The second integrated boot passed pipeline creation (graphicsPipelines = 1)
but stopped before submission because metadata cbuf2 produced binding 42 while
WebGPU's actual automatic layout contained only 41 and 43. Evidence:
`build/codex-integrated-bind42-report.json` (19,942,123 imports, same source SHA,
2 compiled game shaders, 0 completed draws). The absent-fragment-output fix
therefore passed the actual pipeline gate, not yet the full game draw.

The Wasm Naga compiler now exports active resource bindings from validated
entry-point call-graph global usage. Existing code-only compile remains
compatible; `compileWithBindings` supplies code plus reflection to the real
Maxwell compiler. Draw binding creation intersects native declarations with
that usage for constant/storage buffers, texture/sampler halves, push constants
and draw parameters. Unused metadata no longer creates bind group entries or
requires an unused guest range. Automatic WebGPU layout is retained.

Validation: 4 Rust compiler tests, including an unused binding 42 beside a
helper-function read of 41 and a write to 43; real Wasm compiler reflection
and WebGPU compute readback; **16/16 captured Maxwell shader compilations**.
Report `build/codex-shader-bindings-report.json`. GPU draw fixture still passes
380 checks / 15 draws and deliberately declares unused binding 42 in both
WGSL/native metadata while reflecting it inactive. No runtime C relink needed;
`build_shader.py` rebuilt the small independent compiler module successfully.

A third real boot is now starting with the updated renderer/compiler. Poll
`codex-poll-boot.cjs` without reload. No game/menu/input success is claimed.

## First actual game draw reached; absent fragment output fixed — 22 October 2026

The integrated module's first boot finished at **19,939,376 imports**, nnMain,
`WebGPU graphics draw failed`, guest `71002c48f0`, LR `71037f0b40`.
Two actual game shaders compiled; pipeline creation rejected a bound color
target whose fragment entry point had no corresponding color output while
its writeMask was nonzero. 3 presentation calls occurred, but no game draw
completed (draws = 0). Do not claim a menu/image from those presentations.
SHA-tagged evidence: `build/codex-integrated-first-draw-report.json`, containing
the real graphics packet and GPU validation error.

`wgsl-interface.mjs` now reflects fragment output locations (direct returns,
return structs, builtin-only and void returns). `gpu-draw.mjs` disables color
writes and blending for attachments without an output, retaining their load/
store contents. This matches an unused native attachment without inventing a
color output. CPU interface reflection checks pass; GPU fixture now passes
**380 checks / 15 draws**, including an empty fragment shader that preserves
a bound color target. `build/codex-draw-report.json`.

A second real boot of the same source Wasm is starting with these JS changes.
Use `codex-poll-boot.cjs`; do not reload an active boot. The previous boot is
terminal and archived. No second C link was necessary for this pipeline fix.

## Integrated source module published and booting — 22 October 2026, 02:26 Asia/Saigon

The full link completed successfully (exec 90858), including the observed
callback graph and graphics C implementation. New SHA:
`f0a354229b00b559b4550675fcc545382ceeb2869665abfb34832db842d5df85`,
215,291,688 bytes, 87,041 functions, maximum 10,812 locals.
Metadata: `build/codex-integrated-build.json`. Node passed **306/306 actual
constructors**, same SHA (`build/codex-integrated-node-report.json`).

The completion follow-up (exec 1949) has also exited successfully after
starting the Chrome boot in the original `ssbu-web` page. State file says
`browser-running`; **do not run the start harness again or reload**. Poll with
`build/codex-poll-boot.cjs`. Latest poll archive is
`build/codex-integrated-boot-poll.log`. At 4 seconds: 306 constructors, 32,389
imports, no failed guest threads, 20 programs, 2 textures, no compiled shaders
or draw/present yet. This is a live boot, not a completion claim.

The module still predates the C input change. Its object cache is prepared for
a later link, but prioritize observing the integrated boot's next real failure
before deciding further C changes. Source JS vertex conversion, failure packet
archival and input/UI lifecycle fixes are served. No second linker is running.

## Full input UI lifecycle checked — 22 October 2026

`build/codex-test-input-ui.cjs` additionally verifies the whole UI -> runtime
Worker -> posted shared pointer -> input writer connection with a routed,
self-authored module. Two boot/keyboard/focus/release/stop/reboot cycles pass
6 assertions. This complements the real C Npad test; it is not a live game
input response. Report `build/codex-input-ui-report.json`. The first harness
attempt tried to focus the disabled ELF input and timed out; using the enabled
Stop button verifies actual canvas blur. The fixture tab was closed after
each attempt, preserving the original page. Cancellation reports were archived
separately and the real source report restored only while the build follow-up
was still waiting.

## Worker GPU failure evidence preserved — 22 October 2026

Async backend/Worker errors previously bypassed the normal result-report path,
losing `lastGraphicsPacket` and GPU statistics when the Worker closed. The
Worker catch now sends a plain diagnostic snapshot, and the UI archives it
under `phase: workerError` (or `browserError`/`cancelled` for those paths).
Each run clears stale progress and diagnostics. A recorded packet alone still
does not imply submission, rendering or game completion.

The full UI test `build/codex-test-worker-error.cjs` uses a separate tab with
a deliberately incomplete graphics packet and a routed fixture module.
The real Worker/backend rejects it, the UI stores the same packet/error in
the server report, and runtime controls recover. It also exposed and fixed
an input variable declared in the file-verifier scope instead of startRuntime;
that ReferenceError would have prevented the next real boot. Report archived
as `build/codex-worker-error-report.json`; the global report was restored to
the archived real source boot only while the follow-up was still waiting.
These are JS-only fixes available to the pending integrated build's boot.

## Integration completion follow-up running — 22 October 2026

Exec session **1949** runs `build/codex-finish-integration.ps1`. It waits on
the existing Python builder PID **169548** (start UTC 18:11:01.2149977), then
requires newly published build metadata, runs actual Node constructors and
checks the tested SHA matches that module before starting Chrome. It preserves
any active browser runtime: the guarded `codex-start-integrated.cjs` fails
before navigation if `__ssbuRuntimeDone === false`. It does not build/link,
restart a failed build, or start a second linker.

Watch `build/codex-integration-followup.json` for `waiting-build`,
`constructors`, `starting-browser`, `browser-running` or `failed`. New outputs:
`codex-integrated-build.json`, `codex-integrated-node-report.json`,
`codex-integrated-node.log`, `codex-integrated-boot-start.log`. Once browser is
running, poll `codex-poll-boot.cjs` without reload. Do not issue competing
browser navigation while the follow-up is transitioning. This is an immediate
local process dependency, not a recurring automation. Server HTTP 200 verified.

The follow-up explicitly copies `game-input.mjs`, because the current builder
loaded its old Python copy list before that source change. The current link
still predates the C input export; the JS handles its absence. At last check,
wasm-opt PID 157916 and the full builder tree were live; old module remains
served until validated publication. No game completion is claimed.
The input C change also passed a separate `--fibers --compact-codegen --jobs 8
--compile-only` run (`build/codex-input-objects.log`): only `host_os.c` rebuilt,
with no second link. That object is ready for a later link; it does not alter
the already-running linker input or add input support to its pending module.

## Browser input connected in source — 22 October 2026

`host_os.c` now exposes a shared, atomic seven-word pad snapshot through
`web_input_pointer`. The browser is the sole input writer; guest HID readers
use a bounded sequence retry and return neutral input on a torn snapshot.
`GetNpadState` uses the snapshot for connected player 0, clamps sticks to
[-32767,32767], and keeps other/disconnected players neutral. The guest state
layout and keyboard bindings follow `runtime/nnhid.c`. Browser standard pads
use Nintendo physical button positions, a 15% rescaled stick deadzone and
native stick-direction button bits.

`game-input.mjs` writes directly through the shared Wasm memory posted by the
owner Worker. Keyboard input is restricted to the game canvas, and blur,
visibility change and teardown release input. RAF samples standard gamepads
only while the canvas/window is focused. The UI lists bindings. The default
15-minute boot timeout has been removed: a running game must not be killed
just because it stays in nnMain. Explicit `?timeout=<milliseconds>` remains
available, while constructor-only runs retain their bounded timeout.

Validation: **177 OS/Wasm checks**, including actual JS shared-memory writes
read through Npad imports in a pthread, torn snapshots, stick clamps and
non-player-0 isolation. Report `build/codex-os-input-report.json`. Pure mapping
contracts: **12** (`test_game_input.mjs`), browser keyboard/focus/release checks:
**7** (`build/codex-browser-input-report.json`). No physical gamepad or live
game/menu response is verified yet.

**The currently running full link predates this C input change.** The JS is
backward compatible when the export is absent, but a subsequent full link is
required for game input. Do not start it while exec 90858 is live. That handle
was polled successfully again, and wasm-opt PID 157916 is still advancing
(CPU >5,653 seconds). Current served Wasm remains SHA ae6b06f8... . The new
input module was added to the builder's publication list and served files.

## Native vertex fetch conversion — 22 October 2026

The full integration linker is still live: wasm-opt PID 157916 was verified
again with CPU time advancing beyond 4,078 seconds. No second linker started.
The prior turn that only read handoff documents supplied no new runtime evidence;
this turn revalidated the actual process before continuing implementation.

`gpu-draw.mjs` now describes all vertex formats accepted by native `vfmt_get`
and converts unsupported WebGPU fetch encodings from actual GPU buffer bytes.
These include scalar/three-channel byte/half formats, normalized 16-bit values,
scaled integers and signed/unsigned packed 10:10:10:2. Integer shader input
classification remains intact. Converted streams retain original vertex and
instance indices, base vertex, stride-zero semantics and implicit (0,0,0,1)
components. Reads and expanded uploads are bounded to 64 MiB; unsupported
instance divisors still fail explicitly. Direct WebGPU formats keep their
existing path. No guest/C source or protected cache header changed.

Validation: 38 CPU boundary/default/type contracts in
`test_gpu_vertex_fetch.mjs`; 379 GPU checks / 14 fixture draws, including
float3 default w, half3, unaligned byte3 scaled fetch, signed normalized short3,
indexed base vertex and first instance. Report `build/codex-draw-report.json`.
The C NVN integration fixture still passes 3,266 checks / 2 actual draws
(`build/codex-c-draw-report.json`). These remain synthetic renderer tests;
there is still no verified game image, menu, match or input.

## Source boot confirmed and triangle conversion — 22 October 2026

The mapping/mutex fix is now verified in the **source-built** module, not only
the earlier binary diagnostic. Chrome boot completes 306 constructors and
stops at the same next callback `0x71017c5540` / `lr = 0x7101975594`, with
`threadFailed = 0`, 7,949 programs and 234 textures / 283 views. Import count
is 36,837,206 in this scheduling, versus 67,128,644 in the earlier diagnostic;
counts alone are not a stable boot milestone. The archived report includes
source-build SHA `ae6b06f80d8873e14dbe52ce47da45fd6d9ca72515e562462b9e790ff4eb367a`:
`build/codex-source-36837206-report.json`. This boot is finished; preserve it
until the integrated build completes before booting that newer module.

The JavaScript draw path additionally lowers NVN last-provoking-vertex flat
interpolation, indexed triangle strips, restart and 8-bit indices into real
32-bit triangle lists. Cyclic rotations preserve winding and original vertex
IDs/base vertex/instance parameters; strip restart discards incomplete groups
and resets parity. Actual source index bytes are read from the GPU buffer,
including unaligned 8-bit offsets. A bounded 64 MiB conversion cap fails clearly.
No shader storage writes are accepted, so additional vertex invocations cannot
silently change guest storage. Unsupported shader features still fail explicitly.

Validation: **11 CPU contract cases** in `test_gpu_draw_indices.mjs` and
**312 real GPU checks** in `test-gpu-draw.mjs`, now including last-vertex colors,
strip winding with back-face culling, 8-bit restart and a legitimate vertex 255
when restart is disabled, and a 16-bit index buffer starting at a 2-byte offset.
The latter is converted to an aligned physical index buffer without changing
the original vertex IDs. GPU report: `build/codex-draw-report.json` (8 actual
fixture draws). These are self-authored shader fixtures, not game screenshots.
This work is JavaScript only and will be copied by the currently running full
integration build, with no additional guest object rebuild.
The existing C NVN draw fixture still passes 3,266 checks after these changes
(`build/codex-c-draw-report.json`).

## Current integration — 22 October 2026, 01:11 Asia/Saigon

The original source link has completed: SHA
`ae6b06f80d8873e14dbe52ce47da45fd6d9ca72515e562462b9e790ff4eb367a`,
215,267,430 bytes, 87,020 functions, maximum 10,812 locals. The source-built
module passes **306/306 constructors in Node**, archived in
`build/codex-mapping-source-node-report.json`. It predates the new callback and
renderer work. A real Chrome boot of this source build is now running in the
existing `ssbu-web` page; poll it without reload (`codex-poll-boot.cjs`).

A second full source build is now running in exec session **90858**, log
`build/codex-runtime-integrated-build.log`, command
`build_runtime.py --fibers --compact-codegen --jobs 8 --trace-asyncify`.
This integrates the 62,439-function callback graph plus presentation,
depth/stencil clears, and the new graphics recorder/executor. Check its live
process/session before starting any other link into the same staging path.

`host_gpu_draw.c` now records vertex layouts/streams, retained GPU ranges,
uniform/storage bindings, combined/separate texture handles, pipeline states,
viewports, depth ranges and draw arguments. Submission snapshots live buffer
contents, resolves physical descriptors, preserves program snapshots and
serializes operation 24 to `gpu-draw.mjs`. Small immutable records allocate only
their actual payload (rather than a maximum-sized range array per state change),
to avoid unnecessary memory growth over thousands of boot-loop submissions.

The real WebGPU draw path builds shader/pipeline/bind groups and executes
triangle and indexed-triangle calls. It copies unaligned storage ranges into
aligned GPU buffers, handles push constants/draw parameters, color masks,
standard blending, scissor and supported depth/stencil states. The runtime
includes the last graphics packet in failure reports for further bring-up.

Validation is explicitly synthetic: the fixture Worker alone injects
self-authored WGSL in place of the compiler, while production continues to
compile retained Maxwell sources with the existing real compiler. The JS draw
fixture passes **199 checks** (ordinary/indexed/textured draws, blending,
scissor, storage alignment, push-constant groups, invalid resources/restart
semantics). The C NVN fixture passes **3,266 checks**, including two real GPU
draws, a physical sampler, metadata mutation after recording, buffer writes at
submission and program finalization before replay. Reports:
`build/codex-draw-report.json`, `build/codex-c-draw-report.json`.
These do not yet prove a game draw, native pixel parity, menu or match.

Unsupported draw features fail explicitly: compute, graphics storage writes
and images, indexed strips/restart lowering, 8-bit indices, last-provoking-
vertex flat interpolation, unsupported vertex-fetch encodings, complex raster
states, texture swizzles and compressed logical-bound sampling. These must be
implemented against observed game packets as execution reaches them. Production
has no synthetic shader or gameplay fallback.

## Latest boot evidence — 22 October 2026, 00:31 Asia/Saigon

The separate scratch-mapping diagnostic completed its boot attempt at
**67,128,644 imports**, stopping at **uncompiled indirect target
`0x71017c5540`**, `phase = nnMain`, `lr = 0x7101975594`, with
`threadFailed = 0`. It registered 7,949 programs, created 234 textures / 283
views and uploaded 209,066,994 texture bytes. No game shaders were compiled or
frames presented. Report: `build/codex-scratch-67128644-report.json`.

The original `swprintf` failure is directly verified as passed in this binary
diagnostic: inspection under the guest map lock finds both mappings at 65,536
bytes and the scratch page containing format `%0*d` and formatted output `28`.
The inspection locates the GuestMemory object from the exported `elf_unload`
disassembly and validates its layout before reading. This is stronger evidence
than comparing import counts from differently scheduled boots. The full source
link is still pending, so this does not certify that newer source integration.

Caller inspection finds a virtual call at `0x7101975590` via vtable slot +48.
The original relocation table at `0x710509e610` contains seven related targets
`71017c5460`, `71017c5470`, `71017c5480`, `71017c5500`, `71017c5520`,
`71017c5530`, `71017c5540`. The observed callback is now in `runtime_roots.json`;
`prepare_runtime.py --main --pointer-tables --callback-tables` discovers the
whole group and increases the graph from 62,435 to **62,439 functions**,
keeping 709 units. This preserves the existing observed-table mode; the earlier
handoff command with `--all-pointer-tables` is not the current manifest mode.

An attempted live Draw/Dispatch guard did not return before the worker ended.
Its activation is unconfirmed and is not used as evidence of draw rejection.
The pending CLI client was stopped only after the completed runtime report and
closed worker confirmed that its target was gone; the browser was preserved.

## Continuation — 22 October 2026, depth/stencil clear and scratch diagnostic

`ClearDepthStencil` is now recorded and executed rather than silently ignored.
Operation 25 snapshots the clear depth, stencil value/write mask and scissor;
the retained depth target view selects its mip/layers. Full clears use attachment
load operations, while partial/masked clears use a real depth/stencil pipeline.
All six supported depth/stencil formats are covered. The standalone real GPU
fixture passes **210 checks**, including exact stencil mask preservation and
all-or-nothing invalid packet validation. The C NVN fixture passes **1,209
checks**, including clears of mip 1/layer 1, clipped negative scissors and
immutable source metadata. Reports: `build/codex-depth-clear-report.json` and
`build/codex-nvn-depth-clear-report.json`. Updated host objects are cached; these
C changes still require a full link before they become part of the game module.

The source linker remains active (check PID/log before another link). To make
boot progress while waiting, `build/codex-patch-scratch.py` pins the old Wasm
SHA `1a03f4c049177939fb358d0e19050adfa6000e9c2df7d4c670d8be4212de86e6`
and changes exactly four constant/call pairs inside `runtime_run`: reserve and
commit sizes for the two scratch mappings, identified with `llvm-objdump`.
The separate `runtime-scratch-patch.wasm` has SHA
`1b9dbaa10f8d1e45fb46322be19a5446c824a868c0ac5073a8fbe60a44a1c407`
and passes **306/306 constructors** (`build/web-poc/runtime-scratch-patch-node-report.json`).
This is explicitly a binary diagnostic, not a new source-linked integration.
Boot uses `/scratch.html` and `build/codex-start-scratch.cjs` in Playwright
session `ssbu-web`; collect progress/result without reloading that page.
The first invocation was inadvertently reloaded by the old NVN helper after
18 million imports, so it was restarted. Run fixtures in isolated tabs using
`build/codex-check-nvn-isolated.cjs`, never the old reload helper on a live boot.

An additional diagnostic `runtime-scratch-guard.wasm` (SHA
`e8c17007427653d1ed43420dd21645fd3f520036997f92be82889cd382ed11cd`)
only rejects the 13 previously ignored Draw/Dispatch names, matching the source
fail-fast fix. It has not been booted yet; preserve the current scratch run
before trying it. Actual game draw execution remains unimplemented.

## Current continuation — 21–22 October 2026

Read `web/HANDOFF.md` for the newer build/cache instructions; the sections
below describe older milestones. The stored browser report at continuation
start reaches 38,527,928 imports and stops in `swprintf` at unmapped
`0x7800002100`. The source now maps both the errno scratch region and SDK
constant region to 64 KiB, with `swprintf` protected by `registry`.
The next build is being linked with `--fibers --compact-codegen --jobs 8`.

Code inspection confirms that the reported 818 fences and 4,914 color clears
are **not evidence of 818 displayed game frames**. `nvnQueuePresentTexture`
currently sends backend operation 5, which only submits an empty GPU command
list; no window texture reaches a canvas. Many draw/state command names are
accepted by `host_gpu_commands.c` without recording or executing them. The
stored report also has `shadersCompiled = 0`. Actual draw execution and canvas
presentation remain required before claiming visible game rendering.

The continuation now implements operation 23 for presentation: the C queue
retains the selected physical window texture and sends its crop; the runtime
Worker renders that texture into a transferred game canvas. The presentation
shader preserves top-left coordinates and uses an sRGB attachment view to
encode sampled linear color correctly. Repeated boots create fresh canvases.
`test-gpu-present.mjs` passes 121 real GPU checks (actual presented pixels,
cropping, sRGB, invalid inputs, destroyed textures); evidence:
`build/codex-presentation-report.json`. The C NVN fixture passes 1,183 checks
with two actual presents and no leaked textures/program snapshots; evidence:
`build/codex-nvn-report.json`. These synthetic tests do not prove game draws.

The existing negative `DrawArrays` test exposed silent acceptance of unsupported
draw/dispatch calls. Those names now fall through to the explicit unimplemented
NVN diagnostic until a real execution path is added. The full module currently
linking predates these C presentation/draw changes; it is intended to verify
the scratch mapping fix first. A subsequent link is required to integrate
the newer C changes. None of the three protected cache headers was edited.

## Handoff update — 05:45 Asia/Saigon

Read [the detailed AI handoff](../README_AI_HANDOFF.md) before continuing.
This update supersedes the earlier 19 October 2026 handoff notes below.

1. **Published `runtime-fibers-compact` & Chrome `nnMain` / worker progression past `0x71036f9a70` and `0x71036f85d0` to `0x7100027bf0` (`629` live mutexes, `18,837,755` import calls, `threadFailed = 0`)**:
   - Verified `runtime-fibers-compact` (`6e82a0501c677651cfc2069492a405681a5c756a506382ec3ac74875d371ce5e`, **164,570,580 bytes / 72,093 functions / maximum 10,812 locals / 49,478 guest functions**, `2026-10-19T22:34:01.207840+00:00`, `build/web-poc/runtime-fibers-compact-build.json`) in Node (`306/306` constructors, `10,945` imports) and Chrome via `advance_runtime.py --steps 1` (`build/browser-runtime-18837755-report.json`, `build/web-poc/runtime-browser-report.json`).
   - After passing the `BCATCacheStorage` guest worker thread (`0x71036f9a70`, `628` live mutexes, `build/browser-runtime-18836613-report.json`) and adding `71036f85d0` along with the `T1` virtual methods (`71036f80f0`, `71036f86e0`, `71037003e0`, `7103720870`, `7101754420`, `71017546f0`, `7101755800`, `71016802b0`, `710158bb70`, `710133dba0`, `71006abe50`), the 18 direct subsystem initializers of `u_71022b7100`, and the `BootSequenceScene` virtual methods (`71022cd350`, `71022d23e0`, `71022d6fd0`, `71022d8050`, `71022d8880`, `71022d9a80`, `71022da6e0`), `nnMain` passed `0x71036f85d0`, created another mutex (**`629` live/peak mutexes**), and spawned a new worker thread via `f_71000007f0` -> `f_7100027c40` -> `f_7100027a70`, stopping at **uncompiled indirect target `0x7100027bf0`** (`phase = "guestThread"`, `importCalls = 18,837,755`, `guestFunction = 0`, `lr = 0x7300000000`, `sp = 0x804ad3b000`, `x0 = 0x804ad36080`, `x8 = 0x7100027bf0`).
   - At this boundary, the runtime has completed all 306 constructors, created **629 live/peak mutexes**, **2 live semaphores**, **309 SDK layout queries**, **7,844 registered shader programs**, **348 GPU buffers created / 7 destroyed (`8,206` writes / `107,102,720` bytes written, `80,465,920` live bytes)**, **103 GPU textures / 126 texture views (`125` writes / `172,407,250` texture bytes written, including `23` real game `depthUploadPasses` and `11` `stencilUploadWrites`)**, and **12 command submissions / 12 buffer-to-texture copies / 12 queue finishes**. Evidence: `build/browser-runtime-18837755-report.json`.
   - `0x7100027bf0` (loaded from GOT `0x71052a2000 + 632` in `f_7100027a70`) is the generic thread entry wrapper for threads created via `f_7100027a70`, which immediately dispatches to the inner callback stored at `c->x[0] + 448` (`0x7100027da0`, loaded from GOT `0x71052a2000 + 640` in `f_7100027c40`).

2. **NVN Builder `SetDevice`-Before-`SetDefaults` Fix (`1,181/1,181` Checks PASS)**:
   - Fixed all 6 NVN Builder `SetDefaults` implementations (`MemoryPoolBuilder`, `BufferBuilder`, `TextureBuilder`, `TextureView` in `web/host_gpu_resources.c`, `SamplerBuilder` in `web/host_gpu_sampler.c`, and `QueueBuilder` in `web/host_gpu_commands.c`) so `SetDefaults` preserves the `device` pointer at offset `+16` if `SetDevice` was called before `SetDefaults` (matching the `nn::gfx` initialization order in `f_71022ee2b0`, where `SetDevice` is called before `SetDefaults` on a 1,000-descriptor pool at offset `30,000`).
   - Added 5 regression assertions in `web/gpu_probe.c`: Chrome NVN C fixture (`check-browser-nvn-clear.cjs`) now passes **1,181/1,181 checks (`failures = 0`)** (`build/browser-nvn-depth-report.json`), and full UI **Chạy kiểm tra** (`check-browser-clear-ui.cjs`) passes (`render.checks = 3704`, `textures.checks = 1181`, `depth.checks = 652`).

3. **`rd64` Virtual Tail-Call Fix in `prune_huge_dispatch()` (`web/prepare_runtime.py`)**:
   - Fixed `prune_huge_dispatch()` in `web/prepare_runtime.py` so virtual tail calls (`{ jt = c->x[R]; goto L_dispatch; }`) whose target register `c->x[R]` is loaded via `rd64(` are recognized as non-jump-table tail calls even when an unrelated signed 32-bit load (`(int32_t)rd32(a)`) appears earlier in the 20-line window. This eliminated the last 2 bailed functions (`f_71032f2db0` `576 -> 0` cases, `f_71022f49a0` `176 -> 0` cases, `0` bailed functions across all `461` `runtime_c*.c` units). Unit tests: **13/13 PASS** in `web/test_prepare_runtime.py` (`23/23` total in `web/`).

4. **64 KB Pthread Stack Overflow Fix (`web/host_fs.c`), `nn::nifm` Return Fix (`web/host_os.c`), and `os_probe` (`161/161 PASS`)**:
   - Fixed a 64 KB pthread stack overflow in `web/host_fs.c` (`static unsigned char staging[65536]` under `fs_lock`), fixed `_ZN2nn4nifm*` in `web/host_os.c`, and verified **161/161 checks PASS (`failures = 0`)** in `web/os_probe.c` (`build/web-os/report.json`).
   - No menu/match, actual game draw, input/audio or browser FPS has been verified yet. The older sections below are retained as historical record.

## Latest web integration result

The latest published full module includes the semaphore adapter and completes
306 constructors in Node and Chrome. The observed Chrome boundary is now
**uncompiled indirect target 7103545f50**, called from **71037cccb0** in a guest
thread, with **550 live/peak mutexes** and **2 live semaphores**. Evidence:
`build/runtime-progress/1792373536138784400/report-02.json` and
`build/runtime-callback-node-report.json`.
The module is **60,022,715 bytes / 46,158 functions / maximum 548 locals**,
SHA-256 `2aaacc18a7c4d7d01ddbbd3af34689be1273af9d04645f460139256dc2482997`.
The bounded continuation has added this observed callback's real native
definition and is rebuilding it with referenced pointer-table methods and the
new NVN color-clear frontend. Earlier callbacks 710354a120 and 7103546af0 have
been passed. The real boot now confirms semaphore creation; no menu/match,
draw, input/audio or FPS verification exists yet.

The previous full module independently passed the former 512-mutex limit with
547 live/peak mutexes and stopped at `InitializeSemaphore`, caller 71035468ac,
after 26,765 imports. Its evidence remains at
`build/runtime-progress/1792372563072165500/report-00.json`.

The ordinary `/` page now selects `fibers-compact` by default, with the older
non-fiber module still selectable through `?runtime=debug`. Chrome verification
loaded `runtime-fibers-compact.wasm` from `/` and completed all 306 constructors.
The cancellation control was also verified to clear the active run and unlock
the inputs. Evidence: `build/browser-default-runtime-report.json`. The worker
reports the Wasm loading/compilation phase before importing the large module.

### Recorded program bindings

`nvnCommandBufferBindProgram` now captures immutable shader source versions
through the owner-Worker snapshot API. Recorded command handles and submitted
queue stage state retain those versions independently of `SetShaders` changes
or program finalization. Stage masks update only the selected stages; a null
program unbinds them. Queue finalization releases its remaining snapshots.

Chrome fixtures pass **124 checks** without SDK layout and **331 checks** with
the real SDK layout functions. They include source replacement/finalization
before submission, command-handle finalization after submission, independent
stage updates, null unbinding, rejected masks/stale programs, and zero snapshots
remaining after queue destruction. Evidence:
`build/browser-program-command-report.json`. Real-GPU retained shader compilation
is recorded separately in `build/browser-program-snapshots-report.json`; these command tests
do not compile shaders, dispatch compute or draw game pixels. This change is
linked into the published full runtime, but the observed game boundary has
not yet exercised a program-binding command.

### Physical color-clear renderer

`gpu-render.mjs` and owner-Worker operation 22 now execute ordered color clears
on physical WebGPU texture subresources. Full clears use render-pass load/clear;
channel masks or scissor rectangles use a typed fullscreen shader that preserves
other pixels/channels. Pipeline caching is bounded to 256 entries. All packet
records are validated before rendering, and closing during asynchronous pipeline
creation prevents submission to destroyed resources.

Chrome passes **3,704 pixel/guard checks**, including every RGBA mask on
normalized, signed and unsigned integer targets, float values outside [0,1],
linear-to-sRGB encoding through storage formats and view overrides, preservation
of implicit alpha on expanded RGB targets,
scissor, mip/layer isolation, ordered clears, invalid
batch rejection and texture cleanup. The full page's **Chạy kiểm tra** run is
also green with this fixture. Evidence: `build/browser-color-clear-report.json`
and `build/browser-color-clear-ui-report.json`. Node cancellation/input-lifetime
checks and existing GPU bridge/resource/queue tests pass.

These particular tests are analytic physical-target packets. The NVN frontend
is tested separately below. Depth/stencil, 1D/3D targets and multisampling remain
unported.

### NVN color-clear command frontend

`SetRenderTargets`, `SetScissor(s)` and `ClearColor(i/ui)` now capture immutable
guest metadata and retain target textures. Queue target/scissor state survives
command-handle finalization and later submissions. Mixed copy/clear submissions
execute each action in recorded order; copy-only submissions retain their batch
upload path. Targets are released on unbinding or queue destruction. Upper-left
origin, explicit scissors and matching-format 2D/array/cube subresources are
supported. Compatible linear/sRGB view formats select the actual attachment
format; incompatible format reinterpretations, lower-left origin and implicit
scissor defaults stop explicitly pending validation. The owner accepts legacy
56-byte clear records and new 60-byte records with the view format, preserving
compatibility with the previous full runtime.

The SDK-backed NVN fixture passes **1,053 checks** in Chrome, including full clear
→ buffer-to-texture copy → masked clear pixel ordering, cross-submission state,
clipped negative scissors, captured array/view/color/scissor input lifetimes,
texture finalization guards, RGB float/uint clears retaining implicit alpha,
and captured sRGB render-target view overrides.
All texture/program-snapshot resources are released. Evidence:
`build/browser-nvn-color-clear-report.json`. The full page also passes with both
this fixture and the physical clear fixture. This is a self-authored NVN stream;
the real game has not yet submitted a clear/draw at the observed boundary.

### Constructor/runtime continuation

The real constructor graph now completes **306/306 constructors in Node and
Chrome 155**, with 10,945 handled import calls. Browser evidence:
`build/web-poc/runtime-browser-report.json`; the `Khởi tạo runtime` control
selects the same local `main.elf` and runs it in a dedicated worker. This is
still not a rendered menu/match.

`prepare_runtime.py` builds real dependency graphs, retains EH/resume bodies,
and prunes only instruction labels proved unreachable by AArch64 control flow
(six CFG preservation tests pass). It includes actual pointer-table leaves,
ADRP+ADD callback targets, and the allocator/runtime code range. The latest
`--main` graph has 33,879 functions / 90,866,886 generated C bytes, including the
graphics initialization range. Missing address buckets are permitted during
optional range discovery, but a missing required callee still fails preparation.
Fifteen Python tests pass, including that distinction. Stable address
buckets and per-unit cache keys avoid rebuilding everything for one new target.
All extracted code remains under ignored build/.

An optional `prepare_runtime.py --main --pointer-tables` discovers consecutive
relative code-pointer relocations at data addresses computed by explicitly
observed callbacks (including their unwind bodies). It also reverse-indexes
complete contiguous code-pointer groups containing observed callback targets.
Groups longer than 256 entries are skipped whole, and newly discovered methods
do not recursively select further groups. Targets still require real native
definitions; discovery does not replace execution.

The callback-table candidate added **65 functions / 148,797 C bytes**, from
33,810 / 90,714,226 to 33,875 / 90,863,023. All 33,810 prior function bodies
were hash-identical, none missing; 21 tables / 175 targets had native definitions.
Evidence: `build/callback-table-comparison.json`. Eleven preparation tests pass,
including non-recursive reverse selection and whole-group size bounds. The
next observed callback expanded the active graph to 33,879 functions and 22
tables / 181 targets; that graph is currently building.

The earlier address-computation-only probe had **33,808 functions / 90,681,048
source bytes**, versus **33,799 / 90,664,859**: nine added functions, 16,189 added
source bytes, no missing or changed existing function bodies. In particular,
the actual table at 710522e9a8 referenced by callback 710353cd70 has six further
methods absent from that earlier graph. Evidence:
`build/observed-pointer-table-comparison.json`; that graph has now been built
and booted through the callback group to the mutex boundary. Nine preparation tests pass, including contiguous relocation walks,
gaps, bounds and constant clobbering. `advance_runtime.py --pointer-tables`
can use this discovery on its next observed callback rebuild.

The initial broad experiment discovered 75,923 functions / 882,426,125 source
bytes by walking tables throughout the transitive graph. It remains available
as `--all-pointer-tables` for investigation; it was prepared separately, not
compiled or substituted for the live runtime. Normal preparation retains its
existing graph unless the caller opts into either discovery mode.

`build_runtime.py --debug-graph` compiles guest units at O0 for bring-up; a
large-function fallback uses O1 code generation with LLVM IR passes disabled
to stay below practical browser local-count limits. Generated modules are
checked for numeric-local counts. The failed experimental Binaryen coalescing
pass was stopped after this alternative instantiated and ran successfully.
No FPS claim is made for this debug build. Windows link uses an emcc response
file to avoid the command-line length limit.

Host services now handle the observed constructor path: memory/string
operations, C++ guards, pthread mutex/TLS state, virtual address reservation and
page commit/decommit, main thread identity/core/priority, clocks, SDK constants,
and libc++ strings. String buffers are allocated/freed by the actual game
allocator, so inlined guest code can exchange them safely. 91 self-contained
real SDK leaves are compiled too; the remaining SDK graph is prepared in
`build/web-sdk` but not yet bound. The next-prime handler matches 12 independent
Unicorn runs of the real SDK, including input zero (which differs from the
older native host shortcut). Concurrent page-table mutation is now serialized
with scalar/vector/CAS access; borrowed spans still require stable lifetime.

`runtime_main` now enters nnMain on a real pthread. The observed OE notification
thread uses actual pthread/event waits; clocks and read-only RomFS mounting
work in the browser. WORKERFS mounts the selected directory's File objects,
including nested .nrr files, without loading the archive into Wasm memory.
The owner Worker stays available to serve filesystem and WebGPU proxies.

Chrome boot reaches NVN bootstrap, resolves the function registry, creates the
compatibility device against a real WebGPU device, and sets clip modes. It
now passes all 98 device capability queries and creates memory pools,
texture/sampler descriptor pools, command buffers and a queue. It currently
records texture/sampler pool bindings, registers actual shader programs,
creates a 16x16 RGBA8 texture and a six-face cubemap, and executes seven real
buffer-to-texture copies in seven submissions. After the LightEvent adapter
was added, the ordinary module reaches `nn::os::InitializeFiber` after about
22,700–22,800 host calls. The optimized full Asyncify module now passes that
initialization. The first build stopped at callback **710353cd70**, called from
**710392acb0**. After adding that actual native definition, the next build
passes it and stops at **710353cef0**, called from **710353cb90** (22,692–22,711
imports in the saved runs). This is another method of the same relocated
callback table. The scoped-table build now passes both callbacks and stops in
a guest thread at **mutex not initialised or table full**, key **8005406110**,
last guest function **7103957670**. The original message does not distinguish
an exhausted fixed table from an unknown unlock. A new mutex adapter and
import attribution are being integrated to resolve/diagnose that boundary.
These results do not yet prove a game fiber
switch/migration or a frame. Earlier boot evidence is retained under
`build/runtime-progress/1792354243195715200` and
`build/runtime-progress/1792354299127181800`.
The capability table uses
the actual requested WebGPU device limits for physical limits, explicit
frontend ABI sizes/alignment, and zero for unsupported feature flags. It does
not copy a Switch's hardware capability table. Unsupported commands stop explicitly; optional
native fast-clear registration reports failure. Evidence:
`build/web-poc/runtime-browser-report.json`. Node constructor evidence remains
`build/web-poc/runtime-node-report.json`; Node has no attached WebGPU device.
Game fibers, full SDK/NRO loading, GPU textures/rendering, input/audio and a
browser match remain incomplete. The full port goal remains active.

### Mutex adapter refresh

The previous mutex registry used 512 fixed records and conflated a full table
with an unknown unlock. `host_mutex.c` replaces it with a dynamically allocated
hash registry, operation leases, native-thread ownership and recursion depth.
Destroy rejects held locks and waiters; unknown/wrong-owner unlocks do not
create a new mutex. POSIX calls return their errors; SDK/C++ operations trap on
invalid operations. The root cause of the observed game error is still to be
established by the refreshed full-module boot, rather than assumed to be capacity.

`python web/build_mutex_probe.py` passes **46,383 checks** in Node and Chrome
155: 1,536 simultaneous guest keys, recursive/error-check modes, actual
cross-pthread exclusion, a 20,001-update protected counter, wrong-owner unlock,
busy destroy, destruction/reuse and SDK entry points. All test mutexes are
released. Evidence: `build/web-mutex/report.json` and
`build/browser-mutex-report.json`, `results.mutexes`. This is standalone
production-adapter coverage, not a successful game boot past that error.

The host-import wrapper now records the current symbol before dispatching a
known handler and restores the previous logical-frame symbol on return.
Failures inside implemented handlers therefore identify the import too.
The heap frame pointer remains attached to the logical stack across fiber
migration. The runtime worker also reports live/peak mutex counts when the
new module exports are present. The first mutex-refresh module passed Node
constructors but hit a C-stack ABI assertion on Chrome while sending a report.
The aligned-stack module now returns a valid browser report past 512 mutexes,
at the semaphore boundary described above.

### Fiber stack ABI and semaphore continuation

Wasm32 malloc in the pinned SDK guarantees eight-byte alignment. The fiber
initializer installs the supplied C stack top directly, whereas C/JS variadic
argument buffers require sixteen-byte stack alignment. Fiber C/Asyncify
stacks now use `aligned_alloc(16, ...)`. The initial pthread pool is eight,
covering the additional threads observed during bring-up.

The expanded production-adapter fixture passes **324 checks in Node and
Chrome** (108 per round), including mixed i32/f64/i64 EM_ASM arguments before
and after migration. A deliberately eight-byte-aligned allocator control
fails at `readEmAsmArgs`, matching the game's previous assertion. It is never
published. Evidence: `build/web-host-fiber/report.json`,
`build/browser-fiber-stack-alignment-report.json`, and
`build/web-host-fiber-unaligned/control.log`.
Run `python web/build_host_fiber_probe.py --unaligned-control` for that control.
The independent 79-check migration probe also still passes after alignment.

`host_semaphore.c` provides real pthread-backed counting permits, try/timed
acquire, single/count release, current-count queries and finalization.
Records are leased while operations block; invalid counts/overflow do not
silently alter the semaphore. Timeouts use a monotonic absolute deadline.
**1,426 checks pass in Node and Chrome**, including two actual pthread
consumers, empty/available/zero/negative/timed acquisition, count bounds,
overload/C aliases and 700 simultaneous objects with complete cleanup.
Evidence: `build/web-semaphore/report.json` and
`build/browser-semaphore-report.json`, `results.semaphores`.
This is isolated adapter evidence; the full game integration is currently
building under `build/runtime-semaphore-state.json` / `build/runtime-semaphore-build.log`,
then will run Node constructors and the bounded browser continuation.

A separate diagnostic copy of the preceding Wasm/JS module helped observe
the semaphore boundary without removing its ABI assertion. It is not a
production fix. Source hash/scope: `build/runtime-observer-metadata.json`;
observation: `build/runtime-error-observer-report.json`.

### NVN resource/queue bridge

`WebGpuPrograms` now retains immutable source revisions separately from the
current program handle. Replacing or finalizing a program does not invalidate
an outstanding snapshot; releasing a snapshot rejects stale tokens, and close
prevents late compilation/cache commits. The owner-Worker bridge reserves
operations 20/21 for retain/release and serializes snapshot compilation with
other device error scopes. The default snapshot count limit is 65,536 and can
be configured for a registry. This prepares recorded program bindings; the
NVN command recorder does not yet use those operations.

`node web/test_gpu_program_snapshots.mjs` checks source ownership, replacement,
finalization, shared variants, bounds, stale tokens, failure recovery and close
during compilation with a mock compiler/device. Chrome 155 additionally passes
**10 checks using actual captured compute/vertex sources**: a finalized source
revision still produces matching WGSL through Maxwell/glslang/Naga, accepted
fresh WebGPU modules, and an accepted compute pipeline. No dispatch/draw or
pixel comparison occurs. Evidence:
`build/browser-program-snapshots-report.json`, `results.gpuProgramSnapshots`.
The optional browser fixture is published by `build_maxwell.py` and included
in **Chạy kiểm tra** when actual captures are present.

`gpu-samplers.mjs` now decodes the frontend's 48-byte sampler values into real
WebGPU samplers. `WebGpuBackend.materializeSampler` snapshots the descriptor
before queuing, shares device error-scope ordering, and uses a bounded 512-entry
LRU; cache eviction preserves samplers already retained by live bind groups.
The parser supports nearest/linear filters, mip nearest/linear, repeat,
mirror-repeat, edge clamp, eight depth comparisons and compatible integer
anisotropy values. Non-mip filters currently require a one-level bound texture.
Border/mirror-clamp modes, LOD bias/snap, min/max reduction, negative LOD clamps
and incompatible anisotropy explicitly require further shader lowering.

Chrome 155 passes **156 analytic color/depth sample comparisons** through the
actual backend and 17 actual GPUSamplers: min/mag, mip interpolation, address
wrapping, LOD clamps and all depth comparison functions. The fixture also
creates an anisotropic sampler; anisotropic pixel equivalence is not proved.
Evidence: `build/browser-sampler-integration-report.json`, `results.gpuSamplers`.
`node web/test_gpu_samplers.mjs` separately verifies guards, byte offsets,
cache bounds/eviction, queued input snapshots, failure recovery and late close
with a mock device. The browser fixture is included in **Chạy kiểm tra** when
published by `build_gpu_probe.py`. This is a renderer component; game command
recording/bind groups still need to call it before actual NVN draws can run.

`host_gpu_resources.c` implements CPU-backed and GPU-only pool/buffer storage,
builder copies, range validation, descriptor pool metadata and reference
lifetimes. Physical pools support 4 KiB allocations (the game's observed pool
size 0x54000 is not a multiple of the 64 KiB virtual mapping page). GPU storage
is allocated lazily; CPU-backed uploads stream through a bounded 1 MiB staging
buffer. Flush/invalidate respect buffer offsets and aligned GPU transfers.
Virtual/sparse pool remapping remains unsupported. Texture/image descriptor
registration now snapshots source state and creates validated WebGPU views.

`host_gpu_bridge.c` sends requests from the guest pthread to its owner Worker,
then waits on a condition variable until async WebGPU validation/completion.
`gpu-backend.mjs` serializes requests so per-device error scopes cannot cross
between guest callers. Allocation, upload, readback, destroy, queue flush,
queue completion and error status use actual WebGPU operations. A failed
request does not poison the request queue; stale resource handles are rejected.

`host_gpu_commands.c` implements command-buffer lifecycle, callback metadata,
memory accounting, pool retention, unique command handles and queue builders.
Empty recordings and ordered texture/sampler pool bindings can finish. Handles
own their binding records and retain descriptor pools until handle finalization;
later recordings do not overwrite earlier handles. The frontend reserves 16
bytes per binding in its own command representation, not native NVN packets.
Buffer-to-texture copies also record immutable view/region/stride metadata.
SubmitCommands accepts these implemented records and pool bindings; draw and
other unported copy commands still stop explicitly. Queue Finish/
Finalize waits for actual WebGPU completion. Sync fences capture actual queue
completion promises; poll, finite/infinite CPU wait, server wait, refencing and
release are implemented. Completion waits leave the bridge available to other
callers. Queue submissions/flush/finish serialize per queue without holding the
global command registry during owner-Worker waits. Command/control byte counters
measure the frontend representation; no compute commands are accepted yet.

`host_gpu_copy.c` reads source bytes at submission, including captured row/image
strides and array/3D offsets. Live resource leases cover each in-flight
submission, rather than pinning one-shot upload pools for the lifetime of an
old command handle. After completion the game may finalize its source pool;
replay validates the source address and original texture identity again. Copies
currently stage bytes through CPU memory and WebGPU writeTexture. A direct GPU
buffer-copy fast path and rendered-game performance remain unverified.

Pure state builders for eight NVN families share 79 native entrypoints through
generated guest-memory adapters. Sampler builders preserve float-register ABI,
LOD/filter/wrap/compare/border values, and descriptor registration copies the
sampler state. Physical WebGPU samplers and draw-time descriptor binding are
still pending.

The dump-free C integration fixture uses these same NVN functions through a
real pthread/Worker/WebGPU path: **107 checks pass on Chrome**. It checks copied
builders, descriptor metadata, a non-64-KiB physical pool, CPU->GPU flush,
GPU->guest invalidate, 65,536-byte content comparison, invalid ranges,
GPU-only addresses with no CPU mapping, finalization/stale handles, recovery,
queue flush/finish, recording/handle
lifetimes, ordered bindings across independent command handles, descriptor
retention, program initialization/source registration/replacement/finalization,
sync/fence behavior, state copies across guest pages, narrow boolean
and 64-bit output writes, sampler registration copies, and explicit rejection
of an unsupported DrawArrays call. It creates
and releases three real GPU buffers (81,928 bytes uploaded, 65,556 read back),
leaving zero live buffer bytes. Browser evidence:
`results.webgpu.gpuResources` in `build/web-poc/browser-report.json`.

This resource fixture is not a game draw. The latest real game boot allocates
five physical WebGPU buffers (two subsequently released), uploads 67,187,456
buffer bytes in 82 transfers, creates two textures and two views, and performs
14 texture uploads totaling 14,336 bytes, including seven submitted copies.
No game frame/menu/match is rendered yet.
Node sync/dispatch tests separately exercise controlled promises and mock
modules; their timeout/cancellation checks are not physical GPU timing evidence.

The unsigned 32/64-bit libc++ array sort adapters use checked guest-memory
reads/writes and bounded-stack heap sort. **88 Wasm cases match actual SDK
AArch64 execution in Unicorn**, including extrema, duplicates, sorted/reversed
arrays, unaligned 64-bit data, page crossings and surrounding canaries. The
oracle maps/relocates the user's SDK, not a reimplementation of its sort.
Reproduce with `python web/check_sdk_sort.py`. The observed libc++ substring
constructor is also bound; its buffers still use the game's allocator. Exception
throwing for an out-of-range substring remains an explicit unsupported boundary.

### Browser shader compiler

`build_shader.py` builds a standalone ~1.1 MB Wasm compiler using pinned Naga
29.0.4. Its private dependency copy has a scoped fallthrough emission patch,
following Naga's HLSL strategy; the original IR still passes the normal
validator. It never modifies the global Cargo cache. Three Rust tests cover
fallthrough with function results, emitted-WGSL revalidation and invalid input.

The browser's check button now tests SPIR-V -> Wasm compiler -> WGSL -> WebGPU
execution. A self-authored four-case fallthrough shader reads back [7,6,4,99],
and malformed input followed by recompilation on the same instance succeeds.
`build/web-poc/browser-report.json` also records two actual native-cache vertex
shaders converted by the Wasm compiler and accepted by Chrome's WebGPU shader
compiler (336,876 / 1,034,892 WGSL bytes). This is shader compilation evidence,
not a game pipeline, correct pixels, or rendering performance evidence.

The optional `/?inspectPrograms=1` boot now inspects the programs actually
registered in the live WebGPU backend. The latest browser report records
**20/20 shader modules accepted**, **five actual compute pipelines created**,
and **one graphics pipeline created** from registered vertex/fragment handles
13/14. The graphics pair is selected by compatible interface masks, not an
observed BindProgram command; it uses a synthetic 48-byte float vertex layout
at locations 0/2/8 and an RGBA8 target. No compute dispatch, graphics draw,
native pixel comparison or game frame is implied. The explicit probe options
select triangles and float resource interpretation. Evidence:
`results.runtime.diagnosticProgramInspection` in the runtime browser report.
Diagnostic pipelines have separate statistics; their creation does not mean
the game submitted them. With the inspection window, the snapshot has 20
programs and 87 buffer writes (67,216,128 bytes), rather than the earlier fast
halt's 15 programs / 82 writes.

Shader lowering now assigns a deterministic sampler namespace:
`samplerBinding = 256 + imageBinding`, with image bindings below 256 and
conflicts rejected. Previously a fragment sampler could take binding 0 and
collide with a vertex texture compiled independently. A real Chrome WebGPU
negative control rejects the old pair with “Binding types differ (texture vs
sampler)”; the stable 0/256/40/296 interface creates a pipeline. Evidence:
`results.webgpu.samplerStageBindings`. SPIR-V tests check the independent-stage
mapping and reserved-namespace rejection. The 20 modules and five compute /
one graphics pipeline above were also rechecked with the new mapping.
`test_wgsl_interface.mjs` covers the bounded Naga input-interface reader;
`test_gpu_pipeline_checks.mjs` checks queue recovery and closing during async
pipeline creation with a mock device, separately from the real GPU evidence.

The earlier first-eight cached-SPIR-V probe converts 2/8. `shader_probe.py` applies
diagnostic triangle-only interface changes, moves push constants to an explicit
uniform binding and lowers f32 NaN/Inf classification using IEEE integer bits.
Three Python tests cover scalar/vector classification, special values and
rejection of other widths. Remaining samples stop on unsupported demote-helper
and viewport-index capabilities. Point primitives require a separate path;
the diagnostic PointSize varying is not point-size emulation.

The new `maxwell_compiler.c` and `web/maxwell/CMakeLists.txt` compile the existing
portable Maxwell translator plus pinned glslang 16.6.0 into a ~2 MB Wasm module.
glslang targets Vulkan/SPIR-V 1.0, preserving GLSL discard as OpKill rather than
introducing the native backend's SPIR-V 1.6 demote extension. A timed native
diagnostic run exited 0 after 156 seconds and captured 9,574 MXD1 stage files
under ignored build/maxwell-capture. They are compiler inputs only; the web
path does not call a native compiler or use a precompiled shader cache.

`game-shaders.mjs` now connects Maxwell -> Wasm translation/glslang -> SPIR-V
interface lowering -> Wasm Naga -> WGSL. First 16 captures pass this whole path
and Chrome WebGPU shader validation, including vertex and textured fragment
stages. The same 16 also pass through the new program registry and variant
compiler, with real WebGPU validation and zero live programs after release.
Evidence: results.webgpu.gamePrograms in
`build/web-poc/browser-report.json` (16/16). This still does not prove pipeline
creation, correct draw pixels, a rendered game frame, or gameplay.

`host_gpu_programs.c` snapshots bounded raw control/code/constant data during
SetShaders/SetShadersExt. The Worker owns the copies after the C request returns.
Invalid replacements preserve the old source; finalization releases handles.
`gpu-programs.mjs` caches validated shader variants by the renderer's explicit
topology and integer resource/attribute options. The game boot currently
registers 15 programs but compiles zero variants: draw/pipeline creation has not
yet been reached. There is no assumed triangle topology in the live game path.
Controlled mock tests separately check ownership, cache keys, invalid replacement,
validation failure/recovery and release during compilation.

### Source texture layout

`host_gpu_texture.c` implements texture-builder configuration, copied builder
state, swizzles and packaged layout data through checked guest-memory accesses.
Its 96-byte portable state matches the native frontend; the device occupies
the unused tail of the 128-byte object. The dump-free GPU fixture checks copies
across a page boundary, device preservation, dimensions/storage, 64-bit stride
and bounded packaged-layout output. `host_gpu_texture_views.c` preserves the
native view override flags and copies, including signed level counts and
32-bit output writes. Texture/image descriptor registration copies view state,
creates actual WebGPU views, replaces/unregisters entries and releases views
when pools are finalized. Draw-time binding and sampling remain incomplete;
view component swizzles still require shader-side handling.

`host_gpu_texture_objects.c` owns texture lifetimes, retains source pools and
uses the existing portable `runtime/tex_decode.c` for Tegra layout/deswizzling.
Initial texels come from the actual memory pool, including GPU readback for
CPU_NO_ACCESS storage. Packaged-data pointers remain metadata, matching the
native frontend; they do not replace pool storage. Source bounds, strides and
128 MiB staging limits are checked before copying. Recorded copies retain
immutable metadata; source pools are not kept alive merely by old handles.

`gpu-textures.mjs` validates binary subresource packets and creates actual
WebGPU textures, uploads every mip/layer/slice, and supports bounded readback.
RGB formats that need four components are expanded with the correct alpha
bits; integer and floating data is not converted through RGBA8 previews.
Format features are requested only when the adapter exposes them. Unsupported
depth/stencil uploads, multisampling, texture buffers, sparse storage, flag-0x40
render-target layouts and compressed dimensions requiring emulation still stop
explicitly. Buffer-to-texture commands now update GPU contents at submission;
automatic refresh from later direct CPU writes to texture storage remains pending.

The optional SDK-backed integration fixture passes **314 checks in Chrome**:
9 physical textures, 49 uploads and 49 readbacks, 8,836 texture bytes uploaded,
17 created/released views and zero live textures/views after release. It checks 2D/odd row sizes, arrays with
mips, 3D mip volumes, BC1 blocks, explicit linear stride, RGB32 float/uint
expansion, short-stride rejection, stale handles and a GPU-only source pool.
It executes 16 copies in 9 submissions, including metadata/stride snapshots,
source changes after recording, partial regions, array/volume/BC copies, replay
after source-pool recreation, finalized-handle rejection and recovery. View tests
cover sRGB reinterpretation, copied descriptor state, failed replacement
preserving the old view, replacement/unregistration and pool cleanup.
Two fixed GOB byte-offset checks supplement the library round-trip preparation.
These are self-authored texels, not a native-vs-web game-image comparison.
Evidence: `results.webgpu.gpuTextures` in browser-report.json. Build with
`python web/build_gpu_probe.py --with-sdk` and call
`testGpuResources({withSdk:true})`; the default probe remains dump-free.
Separate CPU/mock tests cover format precision, malformed packets, feature
gates and validation-failure cleanup, not physical GPU execution.

`prepare_nvn_layout.py` selects **30 real SDK functions**, including the chip
method targets used by storage-kind selection, with no host-service shortcuts.
It prepares a relocated SDK snapshot from the locally verified 13.0.4 SDK
(SHA-256 pinned in the preparer). Both artifacts stay under ignored build/;
the runtime embeds the snapshot and lazily maps it read-only into guest memory.
The source device/chip model follows runtime/nvn_sdk.c. It describes Switch
source storage and does not replace the real WebGPU capability table.

`host_nvn_layout.c` executes that graph for storage size/alignment/class and
sparse layout, using guest-stack scratch space and cloned registers. **459
configurations / 2,295 queries match actual AArch64 execution in Unicorn**,
including all known formats, mip chains, arrays, 3D, cubes, multisampling,
linear stride, sparse flags and packaged block heights. Buffer-target byte
sizes and flag-0x40 alignment also have independent expected-value checks.
Reproduce with `python web/check_nvn_layout.py`; results:
`build/nvn-layout-check.log`. This verifies SDK layout rules, not image upload,
deswizzling or rendered pixels. The live Chrome boot records `sdkLayoutQueries:6`
and its most recent inputs/result in runtime-browser-report.json.

The embedded snapshot raises the linker's minimum static allocation to about
17.5 MB, so this runtime starts with 32 MiB of Wasm memory and retains its
existing 512 MiB maximum. This is separate from the reserved guest address space.

### OS event continuation and callback bring-up

`host_os.c` now handles LightEvent initialize/finalize, signal/clear, wait,
try-wait and timed wait, using pthread mutexes/condition variables. Manual and
auto-clear behavior is preserved. Event operation leases prevent a slot from
being finalized/reused during a wait; finalized slots can be initialized again.
`python web/build_os_probe.py` passes **26 checks with real Wasm pthreads in
Node**, including two concurrent waiters, one-signal auto-clear behavior,
timeouts, signed negative timeout handling, finalized-event rejection and 600
event lifetimes through the 512-slot registry. This is not a game scheduler or
fiber test. Evidence: `build/web-os/report.json`.

With the local server and Playwright CLI session open,
`python web/advance_runtime.py --steps 10` runs bounded boot/build iterations.
It adds only observed main-image callbacks with existing lifted definitions,
requires a fresh browser report, saves per-step logs under
`build/runtime-progress/`, and stops at host/GPU failures. It never generates
stub handlers. Recent runs added callbacks 0x7103717c10 and 0x7103549e20 and
stopped first at LightEvent, then at InitializeFiber after the event adapter.

The original same-thread fiber probe does not cover migration. Emscripten's
[fiber API documentation](https://emscripten.org/docs/api_reference/fiber.h.html)
identifies thread-specific rewind IDs as the current cross-thread limitation.
The project-local `portable_fiber.c` adapter assigns canonical rewind IDs for
the complete static export set on each Worker. It guards the export signature,
preserves ordinary Asyncify sleep handling, and prevents a fiber rewind from
exiting the pthread before its outer entry wrapper. No Emscripten source is
patched. Because this uses private internals, `fiber_toolchain.py` pins the
SHA-256 of Emscripten 6.0.12's libasync.js and rejects another version. Wasm64
and dynamic-module changes are explicitly outside this adapter's scope.

`python web/build_fiber_migration.py` passes **79 checks in Node and Chrome
155** with actual pthread A → B → A migration, opposite prior rewind-ID
histories, nested stack cookies and setjmp/longjmp. Its stock-API negative
control fails. `python web/build_host_fiber_probe.py` passes **315 checks in
Node and Chrome** (105 per round, three pthread lifetimes on the same two
Workers) through the actual Initialize/Switch/GetCurrent/Finalize
imports. These check virtual guest registers/SP, destination thread TLS,
diagnostic longjmp on a different pthread, root-frame restoration, rejection
of running/unknown fibers, finalization of a suspended stack, and 4,200
initialize/finalize cycles per round reusing slots. Worker IDs prove pool
reuse; interrupted trampolines are cleared at pthread exit. These use self-authored callbacks,
not lifted game fibers. Evidence: `build/web-host-fiber/report.json` and
`results.hostFibers` / `results.fiberMigration` in the browser report.

`host_fiber.c` allocates C/Asyncify stacks lazily at first use. A fiber becomes
available to another worker only after its full stack has unwound; the JS
context-switch hook publishes this with an atomic store. TLS state is read
through separate translation units after migration, avoiding cached source
pthread TLS addresses. Each guest thread/fiber owns its diagnostic frame; it
follows the fiber instead of using one jmp_buf per physical pthread. The fiber
loop's frame covers both callbacks and switches, and finalization discards its
suspended diagnostic chain along with the two stacks.

The observed game call sets InitializeFiber flags to 1. The adapter accepts
flag values 0 and 1 and rejects other bits: SDK bit 0 skips optional stack-alias
allocation, rather than changing virtual register preservation. The supplied
guest stack is used directly. `python web/check_sdk_fiber_flags.py` executes
the actual SDK prefix for flags 1 and for flags 0 with a refusing optional
alias allocator, verifies its state/arguments/stack fields, and stops before
platform context setup. It does not claim SDK native context assembly runs
in Wasm. The 315-check fixture starts with the game's flag 1 and tests both
valid values plus rejection of flag 2.

`python web/build_runtime.py --debug-graph --fibers` builds a separate
`runtime-fibers` module. The ordinary debug build retains its previous
InitializeFiber boundary and has been booted again after the diagnostic-frame
change: 306 constructors complete. The full optimized Asyncify integration
now also completes **306/306 constructors in Node and Chrome**, and passes the
InitializeFiber boundary. Its first successful module is **59,980,555 bytes /
46,125 functions / maximum 548 locals**, SHA-256
`d3ffd63134973d2eb5c5b3b1764647791b302af7e18b548bb22cd0b85016972e`.
Node evidence: `build/web-poc/runtime-fibers-compact-node-report.json`; Chrome
boot evidence: `build/runtime-progress/1792354243195715200/report-00.json`.
That first build stops at callback 710353cd70. The following build passes that
callback and stops at 710353cef0; it is **59,982,395 bytes / 46,126 functions /
maximum 548 locals**, SHA-256
`7fb2ef05f82819254129841392f66d6b94c4a3aadf30eeede0451703f7c38a39`.
Its publication metadata is `build/web-poc/runtime-fibers-compact-build.json`
(overwritten by subsequent builds), and its browser evidence is
`build/runtime-progress/1792354299127181800/report-01.json`.
The current scoped-table build/boot loop is logged in
`build/runtime-fibers-pointer-advance.log` and
`build/runtime-progress/1792368936341494600/`.
It completed at the mutex boundary. The published scoped-table module is
**59,991,328 bytes / 46,135 functions / maximum 548 locals**, SHA-256
`1e9e755b4c2015eea4a620e72b9b0b05e966703112e2f4640081e435204249d3`.
The mutex refresh is recorded in `build/runtime-mutex-refresh-state.json`,
`build/runtime-mutex-refresh-build.log` and, after linking,
`build/runtime-mutex-refresh-node.log` / `build/runtime-mutex-refresh-advance.log`.
There is no menu/match or proven game fiber migration yet.
Earlier attempts were intentionally stopped
after source fixes and after discovering that Binaryen debug level 2 makes
full WAT copies. They are not evidence of an unsupported Wasm operation.
`--trace-asyncify` uses level 1 only; the successful full build enabled it and
recorded 821.552 seconds for the Asyncify pass (separate from validation and
link time). The callback rebuild leaves tracing off. Use `/?runtime=fibers-compact`, or
`python web/advance_runtime.py --steps 1 --fibers --compact-codegen`.

### Compact code generation and synchronous diagnostics

`--compact-codegen` applies O1 backend code generation with LLVM IR passes
disabled to every guest unit, rather than only the single bucket that exceeded
the browser local limit. The initial controlled comparison retained 46,089
linked functions while reducing the module from **77,104,030 to 54,108,540
bytes**, and maximum numeric locals from **44,403 to 25**. It is still a bring-up
build, not an FPS measurement. The actual browser graph completes 306
constructors and reaches the same InitializeFiber boundary with this variant.
`python web/check_compact_codegen.py` checks 96 actual lifted leaves against
384 existing AArch64 oracle states: all pass. Guest objects share the same
host ABI between variants; the 942 guest object paths are identical for compact
fiber/non-fiber builds. Only host adapters receive the fiber implementation
flags. `--jobs` controls C compiler concurrency and `--compile-only` prepares
cached objects without publishing a module.

An isolated Asyncify advice probe identified a further false-positive path:
stdio callbacks in fault formatting caused `read_word` to be classified as
suspending through `web_memory_fault`. Production traps now live in
`runtime_trap.c`; bounded callback-free formatters preserve the messages,
and the unframed fatal path uses a direct synchronous JS console call.
The memory fault path no longer references stdio in full-runtime builds.
No Asyncify remove-list/unsafe override is used: the compiler proves these
paths cannot suspend. The genuine fiber switch remains a positive analysis
control.

`python web/build_diagnostic_probe.py` passes **68,444 byte/canary comparisons
against snprintf**, including capacities zero through 240, truncation, UTF-8
and 64-bit hex addresses. It also exercises the actual production trap/TLS
frame with mapped reads/writes, recovered unmapped/readonly faults, long-message
truncation and parent-frame preservation. Evidence:
`build/web-diagnostics/report.json` and its build advice log. The compact
runtime was rebuilt after this change; `node web/test_runtime_node.mjs compact`
again completes **306/306 actual constructors** in Node. Its binary hash and
result are in `build/web-poc/runtime-debug-compact-node-report.json`.
The first complete no-I/O Asyncify transformation finished but failed the
numeric-local guard: 117,967,313 bytes / 46,125 functions, maximum 92,748 locals,
with three functions above 50,000 (`build/runtime-fibers-noio-build.log`).
Compact backend code generation alone does not bound locals after Asyncify
flattens the functions. The new full integration enables Binaryen's internal
optimization level 1, which coalesces locals before suspension instrumentation.
For one isolated actual guest bucket this reduced maximum locals from 41,932
to 15; a second bucket dropped from 47,048 to 15. These are compiler stress
checks with unresolved calls, not game execution. The largest bucket's
optimization exceeded its deliberately bounded 90-second benchmark, so no
compile-time estimate is inferred for the full module.

The optimized configuration passes the 79-check migration adapter, the
315-check host fiber adapter, all 68,444 diagnostic formatting comparisons,
and `python web/check_compact_codegen.py --asyncify` (384 real AArch64 leaf
oracle states). The leaf fixture uses the full runtime's emscripten longjmp
mode because Wasm exception mode cannot be mixed with Asyncify. These gates
do not establish whole-game scheduling correctness. The full build uses at
most two Binaryen workers by default. Full-module validation, Node constructors
and Chrome boot now pass the former InitializeFiber boundary, as recorded
above; actual game fiber switching and a menu/match remain unverified.

Subsequent runtime builds link into `build/web-runtime/linking/<variant>/`
and validate numeric locals before replacing the served `.mjs`/`.wasm` pair.
A failed or over-limit output therefore preserves the last published build.
Successful publication records a Wasm hash, bytes/function/local counts and
timestamp in `<variant>-build.json`; this describes compiler output, not game
execution. Two publication tests cover a rejected 50,001-local module,
accepted 50,000-local boundary, incomplete pairs and unchanged prior output.
Reload after publication; the two served files are not an atomic deployment
for requests arriving during that short replacement interval. The
first successful full build was launched before this staging change; the
subsequent callback rebuild uses staging.

The optimized 79- and 315-check fiber fixtures also pass in Chrome 155
(`build/browser-sampler-integration-report.json`, `fiberMigration`/`hostFibers`). The page
also passes the 384-state optimized Asyncify game-leaf oracle in Chrome; its
`game32Asyncify` report records the Wasm hash and check timestamp. The page
now prevents concurrent runtime launches and offers a stop control. A real
browser cancellation followed by a fresh compact runtime launch completes
306/306 constructors. Historical nnMain boundary evidence remains in
`build/runtime-progress/` as the root report is overwritten on later checks.

`spirv-compat.mjs` splits combined image/samplers into distinct bindings,
publishes the remapping, relocates push constants, preserves real draw parameters
through a 16-byte uniform block (baseVertex/baseInstance/drawIndex), and lowers
f32 NaN/Inf and unsigned 32-bit carry/borrow without losing their semantics.
Node tests compare scalar/vector carry/borrow with BigInt and IEEE classification
with independently reinterpreted float bits. Input/interface/topology guards
also pass. The game compiler suppresses only WGSL's derivative-uniformity
diagnostic because the reconstructed per-invocation PC loop hides original warp
reconvergence. Implicit derivatives remain in the program; their pixel parity
is unverified and must be included in native-vs-web draw comparison.

A broader Node probe samples 64 captures across the full run: 35/64 currently
convert to WGSL. 27 stop on shader-selected viewport capability and two on
ImageQueryLod. They are not replaced by viewport zero or a constant LOD. The
first-16 fixture is a compiler smoke set, not evidence for all game shaders.
The compiler is still not connected to the NVN boot renderer; resource and
command execution remains required.

Commands for this stage:

```
deps/dump-tools/Scripts/python.exe web/prepare_sdk.py
deps/dump-tools/Scripts/python.exe web/prepare_runtime.py --main
deps/dump-tools/Scripts/python.exe web/build_runtime.py --debug-graph
deps/dump-tools/Scripts/python.exe web/build_shader.py
deps/dump-tools/Scripts/python.exe web/build_maxwell.py
deps/dump-tools/Scripts/python.exe web/build_gpu_probe.py
deps/dump-tools/Scripts/python.exe web/prepare_shader_fixtures.py --count 16
# Broader local compiler diagnostic (no GPU draw):
deps/emsdk/node/24.19.0_64bit/node.exe web/probe_maxwell.mjs 64 --spread
deps/emsdk/node/24.19.0_64bit/node.exe web/test_spirv_compat.mjs
deps/emsdk/node/24.19.0_64bit/node.exe web/test_gpu_capabilities.mjs
# PowerShell, constructor + nnMain diagnostic:
$env:SSBU_WEB_RUNTIME='runtime-debug'
deps/emsdk/node/24.19.0_64bit/node.exe web/run-runtime.mjs --main
```

Earlier kernel/bootstrap results follow as history. Current kernel fixtures
pass 54 checks on each pointer width (including concurrent page-table growth,
protection/decommit and cross-page reads), the 384-state game fixtures and eight
ELF malformed-input/recovery scenarios still pass after memory changes.

The browser build now runs code from the local validated dump, beyond the
original synthetic probes. `web/prepare_game.py` selects 96 bounded leaf
functions from the native lift (2,688 instructions; 88 functions use memory),
then creates independent Unicorn AArch64 register/flags/data-window oracles.
All 384 states match on both Wasm32 and Wasm64 in Node and Chrome 155.
This is a bounded subset, not whole-game equivalence.

`guest_memory.c` now grows its page table up to an explicit configured limit;
the default remains 256 pages. New tests cover growth beyond that limit,
aliasing across growth, capacity failure without partial mapping and cleanup.
The two core fixtures now pass 50 checks each; the fiber fixture still passes.
Mapping mutations still require stopped accessors. The reservation array still
has 16 slots; a concurrent allocator/scheduler is not implemented.

The dedicated browser worker mounts local File objects read-only via WORKERFS.
C stdio range reads validate six 64 KiB samples of the 17,600,869,448-byte
`data.arc`, including crossing 2/4 GiB and the file tail. A short read at EOF is
detected. The asset Wasm heap starts at 16 MiB; the entire archive is neither
uploaded nor copied into a browser filesystem. Sample hashes do not establish
whole-archive integrity.

The actual main ELF loads three LOAD segments at virtual 0x7100000000:
86,672,224 file bytes plus 35,252,880 BSS bytes, in 29,767 pages. Every BSS byte
is checked for zero; text/rodata write protection is exercised. The loader then
applies 306,168 relative, 31,297 imported-symbol and 16 undefined weak-symbol
relocations (0 own symbols), matching the native counts. Counts and selected
relocated values also match an independently prepared Python reference.
Imported-symbol addresses are unmapped symbolic slots at 0x7400000000, not
working SDK services. Unsupported relocation types fail and discard the image.

The real lifted `nninitStartup` body runs in that mapped image with a 1 MiB
guest stack. Three actual handlers supply no CLI arguments and retain the
guest FS allocator callbacks. The expected callback addresses, an image-global
pointer, stack restoration and return link are verified. All other imports,
indirect functions, SVCs and nnMain explicitly stop. No constructors or nnMain
are run; there is no NVN renderer, menu, match or browser game FPS result yet.

Node also passes eight malformed ELF input/recovery scenarios. Chrome passed
the combined runtime/game-function/asset/loader/bootstrap flow. The final runtime
rerun after clearing earlier development errors logged 0 errors/0 warnings.
The HTTP server's five tests pass, including separate report endpoints and
origin checks. Evidence:

- `build/web-poc/{node-report,browser-report}.json`
- `build/web-poc/{elf-node-report,elf-browser-report,asset-browser-report}.json`
- `build/web-game/selection.json` (function addresses and original-byte hashes)
- `output/playwright/ssbu-web-bootstrap-full.png`

Run commands and limitations are in [README.md](README.md). All game-derived
sources, vectors and compiled outputs remain in ignored build directories.
Next gate: extend import binding, constructor call graph and guest module loading;
then scheduler/exception semantics and an NVN/WebGPU backend. Bootstrap success
does not establish that those remaining gates are feasible or complete.

## Latest native result

Update after the font input: extracted all seven shared fonts from the user's
`Downloads/Compressed/Firmware.21.1.0.zip` into ignored
`build/native/sysdata/fonts`. The archive is version 21.1.0 (its filename),
although the user described it as 22.1.0. The supplied 22.1.0 key archive's
prod.keys matches the existing local file; no global key file was changed.
The rerun loaded the fonts, decoded the opening movie, reached the title and
rendered the first-launch Milestones notice. Visually verified screenshot:
`build/native/captures/frame_600.png`. The original script waits for the main
menu and cannot dismiss this extra first-launch notice. A local script variant
uses the runtime's existing `@until` input mode to dismiss notices. At that
point the first run had not reached a match.

The subsequent `smoke-firstboot.log` run passed the notices, main menu, rules,
stage and character selection, and reached the battle input section (step 19,
poll 2963). Later scripted movement/jump/attack steps ran while the renderer
reported fight frames. Frames 4200–4500 averaged 17.06 ms (58.6 FPS), median
16.63 ms, p99 35.07 ms; this is one native interval, not a browser benchmark.
The `battle_120.bmp` filename is misleading: its trigger also fired on a reward
screen before the actual battle. Visual inspection showed a Thanatos reward;
do not cite that screenshot as proof of combat. Audio was muted. The fast
run completed its 150-second test window and exited 0 after 154 seconds; this
was a timed stop, not a verified match result. All 81 script steps were reached.
The fast sequence now dismisses first-launch notices with `@until`, following the
existing reward-aware sequence pattern. No native rebuild was needed.

The following font-blocked result describes the earlier smoke run.

Native build has completed all 11 setup steps: 1,161 main objects, 98 module
DLLs, linked `build/native/work/ssbu_native.exe`. SDK self-test: 0 failures.
Controller self-test: 49/49. Lifter differential check: 32,043 identical, 0
different. The old `compile.log` string `failed: 0` is a successful summary,
not a compiler error.

A first scripted smoke run selected the NVIDIA RTX 4070 Laptop GPU and presented
6 Vulkan frames with 0 Vulkan call failures, then exited with code 7 after 66
seconds because `nintendo_udsg-r_std_003.ttf` is missing. It did not reach a
verified title/menu or match. Validation layers were off, and no game screenshot
was captured. This is not evidence of playable gameplay or a web game port.

Log: `build/native/logs/smoke.log`. The required font check is implemented in
`runtime/nnpl.c`; despite the older optional-font wording in build documentation,
the current runtime explicitly stops when a requested shared font is absent.
No Nintendo fonts/firmware were found in the searched Downloads/.switch paths.
Next input needed: the user's Switch shared-font folder or firmware dump from
which `tools/extract/fw_fonts.py` can extract those fonts. Set `SSBU_FONTS` or
place them under `build/native/sysdata/fonts`, then repeat the scripted run.

### Firmware input provided afterward

Located `Downloads/Compressed/Firmware.23.0.1.zip` (238 NCA files). Read its NCA
headers locally and extracted only the five system-font data NCAs (titles
0100000000000810 through 0814) to `build/firmware-font-ncas`. Archive entries were
read through ZipFile; input archive and keys were not modified.

Font decoding is currently blocked: all five NCAs require the local entry named
`key_area_key_application_16`, which is absent from the current
`~/.switch/prod.keys`; the corresponding master key is absent too. No fonts were
written. No other key file was found in Downloads or inside the firmware ZIP.
The user was asked for a compatible local prod.keys path or an already extracted
Switch font folder. No game rerun was started while this input remains missing.

Updated `fw_fonts.py` to report missing key **names**, return a failing exit code
when no fonts can be produced, and close skipped NCA file handles. The former
behavior returned success after silently writing zero fonts. Verified the current
input now reports the missing name, writes zero fonts, and exits 1; key values
were never logged.

The progress paragraphs below describe earlier stages and are retained as history.

Branch: `web-poc`, based on upstream
`938e66bb7be0ba4123e291211895412da421b705`.

Implemented and run locally, without a game dump:

| Check | Result |
|---|---|
| Pinned Emscripten 6.0.12 build, strict C warnings | pass |
| Default runtime memory helper branch, compile-only | pass |
| Wasm32 CPU/memory/runtime-helper/pthread fixture | 43 checks, 0 failures |
| Wasm64 CPU/memory/runtime-helper/pthread fixture | 43 checks, 0 failures |
| Nested same-thread Asyncify fiber + longjmp fixture | pass |
| Two live workers resuming explicit continuation state | pass, part of core fixture |
| Local HTTP server tests | 4 tests, pass |
| Browser COOP/COEP and SharedArrayBuffer | pass |
| WebGPU compute and 64 expected readback words | pass |
| WebGPU canvas clear/present | pass |
| Browser console after final reload/run | 0 errors, 0 warnings |

Core and fiber checks passed under SDK Node **24.19.0** and the Playwright-driven
headed **Chrome/155.0.0.0** on Windows. Browser GPU information reported `intel`,
architecture `xe-lpg`, `fallback=false`. This run did not select/test the NVIDIA
adapter. User-agent and available GPU details are retained in the browser report.

The opt-in runtime helper adapter uses sparse guest pages and preserves 64-bit
guest addresses under both pointer widths. It is bounded to 256 mapped page
entries for this experiment. Mapping changes and dirty reset require stopped
accessors; a production allocator/concurrent page table has not been built.

Evidence remains locally in ignored build/output directories:

- `build/web-poc/manifest.json`
- `build/web-poc/node-report.json`
- `build/web-poc/browser-report.json`
- `output/playwright/ssbu-web-probes.png`

Reports are overwritten on rerun. They describe synthetic fixtures only. The
small Wasm output sizes and memory-loop timings cannot predict a full game build
or its FPS.

## Still unverified (original web-probe stage)

- Full Windows native build and playable baseline on this machine.
- Game dump/version validation, generated code and module loading in Wasm.
- Complete guest/host pointer audit and real allocator working set.
- Real fiber migration/task-system semantics and guest exception unwinding.
- Full lifter atomic memory ordering and numerical equivalence.
- NVN backend and Maxwell shader translation to WebGPU/WGSL.
- Asset loading, saves, input, audio, movies and a real match in a browser.

## Input preparation and native build — 18 October 2026

Found `Super Smash Bros Ultimate.xci` in the user's Downloads directory
(15,262,199,296 bytes). Read-only HFS0/NCA metadata inspection found base display
version **1.0.0** and embedded patch display version **2.0.1**. Both Program NCAs'
`main` and `sdk` text sizes fail the upstream 13.0.4 size checks. No game content
was extracted, no input file was modified, and full payload hashes were not read.
Metadata report: `build/xci-inspection.json` (ignored). Local inspection used the
existing keys without logging key values.

The user then supplied a separate 13.0.4 NSP update in Downloads. Its control
version and main/SDK text sizes match 13.0.4. Applied that update over the base
Program NCA from the XCI, ignoring its older embedded patch, into
`build/dump-13.0.4/`. Prepared ExeFS and all 14 RomFS files; `data.arc` is
17,600,869,448 bytes. All three segments of main and SDK passed the SHA-256 checks
in upstream `nso2elf.py`. This does not claim full NCA/payload hash verification.

The native lifter differential test ran against Unicorn using the actual
instruction samples: **32,043 identical, 0 different, 0 reference faults**.
Evidence: `build/native/logs/lifter-early-check.log`. Input validation summary:
`build/dump-validation/summary.json`.

Native dependencies were prepared locally: LLVM 9.0.0, SDL3 3.4.18, glslang
16.6.0, Vulkan-Headers v1.4.347, Python packages in `deps/dump-tools`, and
libopus 1.6.1 built with MSVC after checking Xiph's published SHA-256. Open-source
Mii test references and their revision are recorded in
`deps/yuzu-reference/provenance.json`. No permanent PATH modifications were made.

The installed Visual Studio 2026 instance is an Insiders installation. Added
`-prerelease` to the setup and glslang-shim vswhere queries; the native compile/
link dependency probe then passed. Runtime memory helpers still use their default
native branch for this build; the web adapter is not enabled.

The full native pipeline was started with two compile jobs under `build/native`.
At the user's request, it was restarted with four jobs, retaining 740 completed
objects and discarding one output that was in flight during the stop. The resumed
compile identified 421 remaining translation units. Restart details are recorded
in `build/restart-4jobs.json`; future reruns of `build/run-native.cmd` use four jobs.
Image conversion, module extraction, table generation, main lift and SDK lift
completed. Main lift produced 1,161 C translation units. Main compilation is
still in progress at this status update; full native linking, SDK/pad self-tests,
boot and a scripted match have **not** completed. See `build/native/logs` and
`build/native/state.json` for progress; `build/run-native.cmd` reruns/resumes the
pipeline with the local tool paths.

The next verification gate remains the native scripted match and memory/task
measurements. See [README.md](README.md) to rerun the dump-free web probes.
