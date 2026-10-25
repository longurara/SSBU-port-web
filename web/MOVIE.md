# Browser movie adapter

The current game build still uses the failing movie placeholder in `host_os.c`.
The JavaScript modules below are working decoder components. The source now
also implements Buffer, MediaData, and SDK Extractor virtual calls. Decoder SDK
virtual calls and the runtime wiring remain unfinished. A C/JS bridge and the
Extractor work in separate browser fixtures. These do not yet unblock the title.

## Implemented and checked

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
imports before the remaining Extractor/Decoder placeholder. This source change
has not been published as a new game Wasm build.

`movie-backend.mjs` implements serialized extractor/decoder operations for that
bridge. Source bytes become a Blob; metadata and video packet sizes/keyframes
are read from the actual container. Peek/read/advance preserve the current
sample, and seeking starts at a real preceding keyframe. Decoded outputs remain
owned until release, including after source closure. Unsupported audio is
reported explicitly. SDK event notification currently reaches a fixture
callback; the game's event objects are not connected yet.

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
These remain separate fixtures; the C SDK Decoder and game are not wired yet.

`host_movie_bridge.c/.h` transfers bounded staging buffers from a C pthread to
the JavaScript worker, then completes the waiting pthread with an SDK status.
Every dispatched success or failure completes its wait token. Fixed reply
headers are validated before consuming outputs. This adapter uses
`MAIN_THREAD_ASYNC_EM_ASM`, so compile this translation unit with `gnu11`, as
with `host_gpu_bridge.c`. It is not yet added to the full runtime source list;
that belongs with the upcoming SDK Extractor/Decoder integration.

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
and wrapper objects have different allocators. The codec adapter is not yet
routed through `host_os.c`, linked by the full build, or wired to runtime-worker.

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

1. Complete SDK Decoder virtual calls and route the codec adapter ahead of the
   blanket failure placeholder. Preserve guest callbacks, allocator ownership,
   thread context, and real reader data.
   Use `runtime/nnmovie_mf.c` for the source-verified SDK protocol, rather than
   treating the native placeholder's failure as a successful movie skip.
2. Connect the existing value objects to the new codec adapter. The corrected
   Buffer ABI is offset `+0` i32, size `+4` i32, ownership `+16` u8, capacity
   `+24` u64, base pointer `+40` u64. Keep actual allocator ownership throughout
   decoder input/output buffer lifetimes.
3. Connect source callbacks, track selection, seek, input/output slot lifetime,
   decoder status, and event signaling through the SDK virtual tables. The
   backend protocol is not a substitute for those calls by the game.
4. Connect the verified request/completion bridge to the runtime worker and
   full build. Test real media through the SDK C imports in a small browser
   fixture before spending another full game link/Asyncify cycle. Exercise
   concurrent destruction, backpressure, seek/reset, and exact buffer ownership.
5. Implement or explicitly propagate unsupported audio codecs through the
   game's real track-selection behavior. Vorbis audio, raw H.264, MP4 and VP9
   CodecPrivate configurations are not implemented by these components.
   Laced tracks without container-provided duration are explicitly rejected;
   deriving audio packet durations remains codec-specific work.
6. Boot the complete game and verify the title readiness gate passes, menu
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
