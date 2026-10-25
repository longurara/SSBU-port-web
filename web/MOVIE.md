# Browser movie adapter

The current game build still uses the failing movie placeholder in `host_os.c`.
The JavaScript modules below are working decoder components. The source now
also implements Buffer and MediaData imports, but the Extractor/Decoder adapter
and its runtime bridge have not been connected. They do not yet unblock the title.

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

Validation on 25 October 2026:

- Unit checks: lacing byte layouts, timestamps, BlockGroup references, unknown
  cluster boundaries, truncation, bad tracks, short reads, odd-size NV12 planes.
- All 128 WebM stream entries in the local 13.0.4 archive: metadata and first
  video sample parsed; all use VP9, with Vorbis audio and no VP9 CodecPrivate.
- One complete 90.670 s file: 5,440 video samples read to actual EOF, from
  3,000 to 90,653,000 microseconds.
- Real Chrome: twelve frames from that file decoded, with sample/output
  timestamps equal; each 1280×720 NV12 frame contains 1,382,400 bytes.
- Real Chrome: twelve frames also decoded from the 419.855 s, three-track
  opening candidate, archive hash40 `0x20e266a6d8`. This matches the duration
  described in the native adapter, but its role in this boot is not proven by
  a captured file-open path. Media SHA256:
  `0fcd53c42de14577b81f8b9934a332da811b7e52f2ea3d772192d9a40d312976`.
- Actual production C imports in a small Wasm fixture: 2,819 checks of raw SDK
  offsets, numeric metadata conversions, guest string lifetimes, owning and
  borrowed copies, assignment/swap/destruction, allocation failure, allocator
  replacement, callback reentry and context preservation, with two concurrent
  Wasm pthreads. All guest allocations/metadata are released at the end.
- OS routing fixture: 182 checks pass, including raw Buffer offsets and actual
  MediaData calls routed through `web_os_import`. Strict runtime C11 syntax
  check of `host_movie.c` passes with `-Wall -Wextra`.

Local evidence is under ignored `build/`: `codex-movie-webm-metadata.json`,
`codex-real-movie-browser-probe.json`, and
`codex-opening-candidate-browser-decode.json`. No media is committed.

## Run the checks

```powershell
node web/test_movie_webm.mjs
python web/test_movie_video.py --movie C:/path/to/local.webm
python web/build_movie_values_probe.py
python web/build_os_probe.py
```

The browser check uses an existing Playwright CLI session `ssbu-web` and the
existing server on port 8787. It opens and closes a temporary page, selects the
local file with a file input, and keeps the game page intact. It publishes only
the two new JavaScript modules into `build/web-poc`; it does not rebuild, boot,
or reload the game. Override `--session` or `--port` as needed. For a movie
inside an archive, pass `--offset` and `--length`. The report hashes that exact
byte range, validates twelve frames and timestamps, and saves under
`build/movie-video-probe/`.

## Integration remaining

1. Replace the blanket movie failure handling with an SDK adapter. Preserve
   guest callbacks, allocator ownership, thread context, and real reader data.
   Use `runtime/nnmovie_mf.c` for the source-verified SDK protocol, rather than
   treating the native placeholder's failure as a successful movie skip.
2. Connect the existing value objects to the new codec adapter. The corrected
   Buffer ABI is offset `+0` i32, size `+4` i32, ownership `+16` u8, capacity
   `+24` u64, base pointer `+40` u64. Keep actual allocator ownership throughout
   decoder input/output buffer lifetimes.
3. Implement track selection, source callbacks, seek, input
   and output buffer lifetimes, decoder status, and actual event signaling.
   The browser decoder alone does not implement the SDK's slot/event protocol.
4. Connect a request/completion bridge to the runtime worker. Test real media
   through the production C imports in a small browser fixture before spending
   another full game link/Asyncify cycle. Exercise concurrent destruction,
   backpressure, seek/reset, and exact buffer ownership.
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
