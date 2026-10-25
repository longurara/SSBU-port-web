"""Compile the locally prepared constructor graph into a browser worker module."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import sys
from wasm_stats import local_counts
from prepare_gpu_states import prepare as prepare_gpu_states
from prepare_nvn_layout import prepare as prepare_nvn_layout
from fiber_toolchain import verify as verify_fiber_toolchain

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'web'
GEN = ROOT / 'build/web-runtime'
OUT = ROOT / 'build/web-poc'
EMCC = ROOT / 'deps/emsdk/upstream/emscripten/emcc.py'


def publish_module(staged: Path, target: Path):
    """Validate before replacing the served pair; failed links keep old output.

    This is a local development publication step, not an atomic deployment
    across concurrent browser fetches. Reload the page after a successful build.
    """
    if staged.suffix!='.mjs' or target.suffix!='.mjs' or staged.name!=target.name:
        raise ValueError('Staged and published module basenames must match')
    if staged.resolve()==target.resolve():raise ValueError('Link into a separate staging directory')
    module=staged.with_suffix('.wasm')
    for source in (staged,module):
        if not source.is_file():raise FileNotFoundError(source)
    counts=local_counts(module)
    if max(counts,default=0)>50000:raise RuntimeError('Linked module exceeds browser numeric-local limit')
    digest=hashlib.sha256()
    with module.open('rb')as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    report={'timestamp':datetime.now(timezone.utc).isoformat(),'wasmSha256':digest.hexdigest(),
            'bytes':module.stat().st_size,'functions':len(counts),'maximumLocals':max(counts,default=0),
            'scope':'Compiler output and numeric-local guard only; execution requires separate constructor/browser checks'}
    target.parent.mkdir(parents=True,exist_ok=True)
    pending=[]
    # Copy both complete files before replacing either served file. A partial
    # extension is not served by the local HTTP harness.
    for suffix in ('.wasm','.mjs'):
        destination=target.with_suffix(suffix);temporary=destination.with_name(destination.name+'.partial')
        shutil.copy2(staged.with_suffix(suffix),temporary);pending.append((temporary,destination))
    for temporary,destination in pending:temporary.replace(destination)
    metadata=target.with_name(target.stem+'-build.json');temporary=metadata.with_name(metadata.name+'.partial')
    temporary.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');temporary.replace(metadata)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--debug-graph', action='store_true', help='compile guest units without optimisation for bring-up; output runtime-debug')
    parser.add_argument('--fibers', action='store_true', help='experimental full Asyncify build with cross-pthread guest fibers')
    parser.add_argument('--trace-asyncify',action='store_true',help='log top-level Binaryen pass timings during linking')
    parser.add_argument('--compact-codegen',action='store_true',help='use O1 code generation without LLVM IR optimization for all guest units')
    parser.add_argument('--jobs',type=int,default=4,help='parallel C compiler processes (1..16)')
    parser.add_argument('--compile-only',action='store_true',help='prepare cached objects without linking/publishing a module')
    args = parser.parse_args()
    if not 1<=args.jobs<=16:parser.error('--jobs must be between 1 and 16')
    if args.fibers:verify_fiber_toolchain(ROOT)
    flavor=('runtime-fibers' if args.fibers else 'runtime-debug' if args.debug_graph else 'runtime')+('-compact' if args.compact_codegen else '')
    staging=GEN/'linking'/flavor;staging.mkdir(parents=True,exist_ok=True)
    manifest = json.loads((GEN / 'manifest.json').read_text())
    objects = GEN / 'obj'
    objects.mkdir(exist_ok=True)
    flags = ['-std=c11', '-O1', '-Wno-unused-label', '-fno-strict-aliasing', '-ffp-contract=off',
             '-DRECOMP_WEB_MEMORY', '-DWEB_FULL_RUNTIME', '-pthread', '-I', str(WEB), '-I', str(ROOT / 'runtime'), '-I', str(GEN)]
    # Lifted guest code calls the same host ABI in both variants. Fiber host
    # implementation flags belong to the adapters, not the guest instruction
    # bodies; share their objects across otherwise identical runtime variants.
    guest_flags=flags.copy()
    sources = [GEN / unit for unit in manifest['units']]
    sources += [GEN / 'runtime_table.c', GEN / 'runtime_imports.c', GEN / 'runtime_sdkdata.c']
    sources += [ROOT / 'build/web-sdk/sdk_leaf.c']
    sources += [prepare_gpu_states()]
    sources += [WEB / 'guest_sort.c',WEB/'host_allocation_stats.c']
    sources += [WEB / 'host_gpu_programs.c']
    sources += [prepare_nvn_layout(),WEB/'host_nvn_layout.c',WEB/'host_gpu_texture.c']
    sources += [WEB/'host_gpu_texture_objects.c',WEB/'host_gpu_texture_views.c',ROOT/'runtime/tex_decode.c']
    sources += [WEB/'host_gpu_copy.c',WEB/'host_gpu_render.c',WEB/'host_gpu_draw.c']
    sources += [WEB/'runtime_frame.c',WEB/'runtime_trap.c',WEB/'host_mutex.c',WEB/'host_semaphore.c']
    sources += [WEB/'host_font.c',WEB/'font_raster.c',WEB/'font_otf.c',*[WEB/name for name in ('host_movie.c','host_movie_codec.c','host_movie_decoder.c','host_movie_bridge.c')]]
    if args.fibers:
        flags += ['-DWEB_FIBERS','-sSUPPORT_LONGJMP=emscripten']
        sources += [WEB/name for name in ('host_fiber.c','host_fiber_tls.c','portable_fiber.c')]
    sources += [WEB / name for name in ('asset_io.c', 'elf_loader.c', 'guest_memory.c', 'runtime_memory.c', 'startup.c', 'host_runtime.c', 'guest_string.c', 'host_os.c', 'host_fs.c', 'host_gpu.c', 'host_gpu_bridge.c', 'host_gpu_resources.c', 'host_gpu_commands.c', 'host_gpu_sampler.c')]
    def compile_source(source):
        guest=source.name.startswith('runtime_c')
        debug = args.debug_graph and guest
        unit_flags=guest_flags if guest else flags
        local_flags = ['-O0' if flag == '-O1' else flag for flag in unit_flags] if debug else unit_flags
        if args.compact_codegen and guest:
            local_flags=[*unit_flags,'-Xclang','-disable-llvm-passes']
        if source.name == 'font_raster.c':
            local_flags=[*local_flags,'-sUSE_FREETYPE=1']
        if source.name in ('host_runtime.c','host_gpu.c','host_gpu_bridge.c','host_movie_bridge.c','portable_fiber.c','runtime_trap.c'):
            local_flags = ['-std=gnu11' if flag == '-std=c11' else flag for flag in local_flags]
        target = objects / (source.stem + ('-debug' if debug else '') + ('-fibers' if args.fibers and not guest else '') + ('-compact' if args.compact_codegen else '') + '.o')
        compact = target.with_suffix('.compact')
        if debug and (compact.exists() or (target.exists() and max(local_counts(target), default=0) > 50000)):
            compact.write_text('O1 code generation, IR optimisation disabled, to bound Wasm locals\n')
            local_flags = [*unit_flags, '-Xclang', '-disable-llvm-passes']
        def fingerprint_for(chosen_flags):
            return hashlib.sha256(source.read_bytes() + json.dumps(chosen_flags).encode() +
                                     ((GEN / 'runtime_funcs.h').read_bytes() if source.name == 'runtime_table.c' else b'') + (ROOT / 'runtime/runtime.h').read_bytes() +
                                     (WEB / 'guest_memory.h').read_bytes() + (WEB / 'runtime_memory.h').read_bytes() + ((WEB/'runtime_frame.h').read_bytes() if source.name in ('host_runtime.c','runtime_frame.c','runtime_trap.c','host_fiber.c') else b'') +
                                     ((WEB/'runtime_diagnostics.h').read_bytes() if source.name in ('runtime_memory.c','runtime_trap.c') else b'') +
                                     ((WEB/'host_mutex.h').read_bytes() if source.name in ('host_runtime.c','host_mutex.c') else b'') +
                                     ((WEB/'host_semaphore.h').read_bytes() if source.name in ('host_runtime.c','host_semaphore.c') else b'') +
                                     (b''.join((WEB/header).read_bytes() for header in ('host_font.h','font_raster.h','font_otf.h')) if source.name in ('host_font.c','font_raster.c','font_otf.c','host_os.c') else b'') +
                                     (b''.join((WEB/header).read_bytes() for header in ('host_movie.h','host_movie_bridge.h','host_event.h')) if source.name.startswith('host_movie') or source.name=='host_os.c' else b'') +
                                     (b''.join(header.read_bytes() for header in sorted(WEB.glob('*fiber*.h'))) if args.fibers and not guest else b'') +
                                     ((ROOT/'runtime/tex_decode.h').read_bytes() if source.name in ('host_gpu_texture_objects.c','host_gpu_copy.c','tex_decode.c') else b'') +
                                     (b''.join(header.read_bytes() for header in sorted(WEB.glob('host_gpu*.h'))) if source.name.startswith(('host_gpu','host_nvn')) else b'')).hexdigest()
        fingerprint = fingerprint_for(local_flags)
        stamp = target.with_suffix('.stamp')
        if target.exists() and stamp.exists() and stamp.read_text() == fingerprint:
            return target
        temporary = target.with_suffix('.partial.o')
        subprocess.run([sys.executable, str(EMCC), *local_flags, '-c', str(source), '-o', str(temporary)], check=True)
        if debug and max(local_counts(temporary), default=0) > 50000:
            compact.write_text('O1 code generation, IR optimisation disabled, to bound Wasm locals\n')
            local_flags = [*unit_flags, '-Xclang', '-disable-llvm-passes']
            subprocess.run([sys.executable, str(EMCC), *local_flags, '-c', str(source), '-o', str(temporary)], check=True)
            fingerprint = fingerprint_for(local_flags)
        temporary.replace(target)
        stamp.write_text(fingerprint)
        print(f'Compiled {source.name}', flush=True)
        return target
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        built = list(pool.map(compile_source, sources))
    (GEN/f'{flavor}-objects.json').write_text(json.dumps({'objects':[str(path) for path in built],'scope':'Compiled objects only; execution equivalence requires separate runtime checks'},indent=2)+'\n')
    if args.compile_only:
        print(f'Prepared objects for {flavor}; not linked',flush=True);return
    command = [sys.executable, str(EMCC), '-O1', *map(str, built), '--no-entry', '-pthread', '-sUSE_FREETYPE=1', '-lworkerfs.js', '-lnodefs.js',
               '-sMODULARIZE=1', '-sEXPORT_ES6=1', '-sENVIRONMENT=worker,node',
               '-sINITIAL_MEMORY=33554432', '-sALLOW_MEMORY_GROWTH=1', '-sMAXIMUM_MEMORY=4294967296',
               '-sSTACK_SIZE=2097152', '-sPTHREAD_POOL_SIZE=32', '-sASSERTIONS=1',
               '-sEXPORTED_FUNCTIONS=["_elf_load","_elf_info","_elf_unload","_elf_relocate","_elf_relocation_info","_runtime_run","_runtime_start","_runtime_join","_runtime_main","_runtime_info","_web_next_prime","_gpu_complete","_web_nvn_layout_query_count","_web_nvn_layout_info","_web_mutex_live_count","_web_mutex_peak_count","_web_semaphore_live_count"]',
               '-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","NODEFS","ccall","HEAPU8","PThread"]',
               '-o', str(staging / f'{flavor}.mjs')]
    if args.fibers:
        # Asyncify flattens large lifted functions. Its internal local coalescing
        # must run before instrumentation extends live ranges; otherwise three
        # functions exceed the browser's 50,000 numeric-local limit.
        asyncify_remove = [
            "f_7101f45620", "f_71020fbb90", "u_71022b7100", "f_7102123d40",
            "f_710038eb40", "f_71002c9900", "f_710225b990", "u_71038b7ce0",
            "f_710037d530", "f_710040a5e0", "u_71022cd350", "f_71016d9e10",
            "f_71025b3550", "f_7100351630", "f_71023db060", "f_7101f89310",
            "f_71025bb1c0", "f_710034b310", "f_710036c1c0", "f_7102389c10",
            "f_71017d2170", "f_71017b3640", "f_710226f500", "f_710067fc50",
            "f_7100389170", "f_71033afe40", "f_710214a020", "f_71006cdd20",
            "f_7100378ec0", "f_71003719c0", "f_71021d37a0", "f_7101fd17f0",
            "f_71002c5cf0", "f_71035a4130",
        ]
        command += ['-sASYNCIFY=1','-sASYNCIFY_STACK_SIZE=1048576','-sSUPPORT_LONGJMP=emscripten',
                    '-sASYNCIFY_REMOVE=' + json.dumps(asyncify_remove, separators=(',', ':')),
                    '-sBINARYEN_EXTRA_PASSES=--optimize-level=1']
    command += ['--embed-file',str(ROOT/'build/web-nvn-layout/sdk-layout-image.bin')+'@/sdk-layout-image.bin']
    # Windows has a 32 KiB process command-line limit; constructor/vtable units
    # exceed it. Response files are parsed by emcc, never by a shell.
    response = GEN / f'link-{flavor}.rsp'
    response.write_text('\n'.join(json.dumps(argument) for argument in command[2:]) + '\n', encoding='utf-8')
    link_environment=os.environ.copy()
    if args.fibers:link_environment.setdefault('BINARYEN_CORES',str(min(args.jobs,4)))
    # Level 2 copies the entire module to WAT for every pass and serializes
    # function passes. That is impractical for this 70+ MB module.
    if args.trace_asyncify:link_environment['BINARYEN_PASS_DEBUG']='1'
    subprocess.run([sys.executable, str(EMCC), '@' + str(response)], check=True,env=link_environment)
    publish_module(staging/f'{flavor}.mjs',OUT/f'{flavor}.mjs')
    for name in ('runtime-worker.mjs', 'index.html', 'app.mjs', 'game-input.mjs', 'font-input.mjs','movie-webm.mjs','movie-video.mjs','movie-backend.mjs', 'gpu-guest-pool-diagnostics.mjs', 'gpu-backend.mjs','gpu-programs.mjs','gpu-textures.mjs','gpu-samplers.mjs','gpu-render.mjs','gpu-draw.mjs','gpu-depth-sampling.mjs','gpu-logical-sampling.mjs','spirv-uniform-branches.mjs','gpu-depth.mjs','gpu-program-inspect.mjs','wgsl-interface.mjs','spirv-compat.mjs','maxwell-pipeline.mjs','game-shaders.mjs'):
        shutil.copy2(WEB / name, OUT / name)
    print(f'Linked {flavor}.mjs', flush=True)


if __name__ == '__main__':
    main()
