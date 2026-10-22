#include "portable_fiber.h"
#include <emscripten.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#if UINTPTR_MAX != UINT32_MAX
#error The static migration adapter is validated only for Wasm32
#endif
static pthread_mutex_t setup_lock=PTHREAD_MUTEX_INITIALIZER;
static char export_signature[65536];
static _Thread_local int installed;
EM_JS_DEPS(portable_fiber_deps,"$Asyncify,$Fibers,$runtimeKeepalivePop,$UTF8ToString,$stringToUTF8,$lengthBytesUTF8");
EM_JS(int,install_rewind_ids,(char *signature,unsigned capacity),{
    if(typeof Asyncify!=='object'||!(Asyncify.funcWrappers instanceof Map)||!Array.isArray(Asyncify.exportCallStack))return 0;
    if(Asyncify.state!==Asyncify.State.Normal||Asyncify.currData)return 0;
    var functions=Array.from(Asyncify.funcWrappers.keys());
    if(!functions.length||functions.length>4096)return 0;
    // instrumentWasmExports visits the same static export section on every
    // Worker. It populates this complete map before any C entry point runs.
    var names=functions.map(function(fn){return fn.name;}).join('|');
    if(!names||lengthBytesUTF8(names)>=capacity)return 0;
    var prior=UTF8ToString(signature);
    if(prior&&prior!==names)return 0;
    if(!prior)stringToUTF8(names,signature,capacity);
    Asyncify.callstackFuncToId.clear();Asyncify.callStackIdToFunc.clear();
    functions.forEach(function(fn,id){Asyncify.callstackFuncToId.set(fn,id);Asyncify.callStackIdToFunc.set(id,fn);});
    Asyncify.callStackId=functions.length;Asyncify.ssbuFiberRoots=functions;
    // A fiber trampoline remains inside the original pthread entry wrapper.
    // callUserCallback's maybeExit would otherwise exit that pthread before
    // invokeEntryPoint gets its turn, causing a second exit with a null self.
    // Keep the stock path for ordinary Asyncify sleeps; fiber root functions
    // must finish with pthread_exit, which unwinds the original JS entry too.
    if(!Asyncify.ssbuRewindInstalled){
        var stockRewind=Asyncify.doRewind;
        Asyncify.doRewind=function(ptr){
            if(!Fibers.trampolineRunning)return stockRewind.call(Asyncify,ptr);
            var original=Asyncify.getDataRewindFunc(ptr),wrapper=Asyncify.funcWrappers.get(original);
            if(!wrapper)throw new Error('Unknown static fiber rewind export');
            runtimeKeepalivePop();return wrapper();
        };
        Asyncify.ssbuRewindInstalled=true;
    }
    if(!Asyncify.ssbuFiberSwitchInstalled){
        var trampoline=Fibers.trampoline;
        Fibers.trampoline=function(){
            try{return trampoline.call(Fibers);}
            catch(error){
                // pthread_exit unwinds the outer JS entry. A later pthread
                // can reuse this Worker, so clear the interrupted trampoline.
                Fibers.trampolineRunning=false;Fibers.nextFiber=0;throw error;
            }
        };
        var finishSwitch=Fibers.finishContextSwitch;
        Fibers.finishContextSwitch=function(fiber){
            var flag=Asyncify.ssbuSuspendFlag;
            Asyncify.ssbuSuspendFlag=0;
            // Publish availability only after every frame has unwound into
            // linear memory, before entering the destination fiber.
            if(flag)Atomics.store(HEAP32,flag>>>2,0);
            return finishSwitch.call(Fibers,fiber);
        };
        Asyncify.ssbuFiberSwitchInstalled=true;
    }
    return 1;
});
EM_JS(int,check_rewind_root,(void),{
    var roots=Asyncify.ssbuFiberRoots,bottom=Asyncify.exportCallStack[0];
    if(!roots||roots.length!==Asyncify.funcWrappers.size||!bottom)return 0;
    var id=Asyncify.callstackFuncToId.get(bottom);
    return Number.isInteger(id)&&roots[id]===bottom&&Asyncify.callStackIdToFunc.get(id)===bottom?1:0;
});
EM_JS(void,set_suspend_flag,(atomic_int *flag),{Asyncify.ssbuSuspendFlag=flag;});
int web_portable_fiber_swap_tracked(emscripten_fiber_t *old_fiber,emscripten_fiber_t *new_fiber,atomic_int *suspended) {
    if(!installed) {
        pthread_mutex_lock(&setup_lock);int ok=install_rewind_ids(export_signature,sizeof export_signature);pthread_mutex_unlock(&setup_lock);
        if(!ok){fprintf(stderr,"fiber: rewind ID installation rejected\n");return 0;}installed=1;
    }
    if(!check_rewind_root()){fprintf(stderr,"fiber: unknown rewind root\n");return 0;}
    set_suspend_flag(suspended);
    emscripten_fiber_swap(old_fiber,new_fiber);return 1;
}
int web_portable_fiber_swap(emscripten_fiber_t *old_fiber,emscripten_fiber_t *new_fiber){return web_portable_fiber_swap_tracked(old_fiber,new_fiber,NULL);}
