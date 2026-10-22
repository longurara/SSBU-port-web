/* Browser OS adapter: real pthread-backed guest threads/events. Guest-visible
 * handles remain virtual addresses; host synchronization objects stay private. */
#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "host_font.h"
#ifdef WEB_FIBERS
#include "host_fiber.h"
#endif
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten/emscripten.h>

GuestMemory *elf_memory(void);
int web_runtime_thread_call(Ctx *, uint64_t);

#define MAIN_THREAD 0x7800000100ULL
#define TLS_BASE 0x7800010000ULL
#define NOTIFICATION 0x7800000800ULL
#define OPUS_VTABLE 0x7800000200ULL
#define OPUS_FN_BASE 0x7500001000ULL
typedef struct {
    uint64_t key, entry, arg, stack, stack_size, mask, id;
    atomic_int priority, core, started, finished;
    pthread_t thread;
    char name[128];
} HostThread;
typedef struct {
    uint64_t key;
    int used, signaled, autoclear;
    unsigned references;
    pthread_mutex_t mutex;
    pthread_cond_t changed;
} HostEvent;
typedef struct {
    uint64_t key, target, userdata, multiwait;
    int used, is_thread;
} HostHolder;
typedef struct {
    uint64_t key, *buf;
    size_t cap, count, head;
    int used;
    pthread_cond_t not_empty, not_full;
} HostMq;
typedef struct {
    uint64_t key;
    int used;
    int32_t begin, end;
    uint8_t *bits;
    int32_t *run_len;
} HostNumberLine;
typedef struct {
    uint64_t addr, size;
    int used;
} StdAllocBlock;
typedef struct {
    uint64_t key, base, size, free_size;
    int used;
    StdAllocBlock *blocks;
    size_t cap;
} HostStdAlloc;
static HostThread threads[256];
static HostEvent events[512];
static HostHolder holders[256];
static HostMq mqs[128];
static pthread_cond_t multiwait_cv = PTHREAD_COND_INITIALIZER;
static HostNumberLine numberlines[64];
static HostStdAlloc stdallocs[32];
static uint64_t next_stdalloc_region = 0x7900000000ULL;
static int bcat_mounted;
static float volume_main = 0.25f, volume_library = 1.0f;
static unsigned thread_count;
static pthread_mutex_t registry = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local HostThread *current_thread;
static struct {
    int initialized, focus, focus_mode, recording, operation_mode_notify;
    int configurations[2];
} environment;
static struct {
    uint32_t supported_styles, joy_hold_type, player0_connected;
    uint32_t joy_assignment[10];
    uint64_t sampling_number, touch_sampling_number, sixaxis_sampling_number;
} hid_state;
/* Single browser writer, any guest reader. Sequence protects a complete pad
 * snapshot across JS/Wasm threads without depending on Worker message timing. */
static _Atomic uint32_t browser_pad[7];
EMSCRIPTEN_KEEPALIVE uintptr_t web_input_pointer(void) { return (uintptr_t)browser_pad; }
static int browser_pad_read(uint32_t values[6]) {
    for(unsigned attempt=0;attempt<8;attempt++) {
        uint32_t before=atomic_load(&browser_pad[0]);
        if(before&1)continue;
        for(unsigned i=0;i<6;i++)values[i]=atomic_load(&browser_pad[i+1]);
        if(before==atomic_load(&browser_pad[0]))return 1;
    }
    memset(values,0,6*sizeof *values);return 0;
}
static struct {
    int started, rate, channels;
    uint64_t event_key, clock_ns;
    struct { uint64_t buf, due_ns; } queued[64];
    uint64_t released[64];
    unsigned q_head, q_count, r_head, r_count;
} audio_state;
static uint64_t vsync_event;

static uint64_t monotonic_ns(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
static int audio_pump_locked(uint64_t now) {
    int moved = 0;
    while (audio_state.q_count > 0) {
        if (audio_state.started && audio_state.queued[audio_state.q_head].due_ns > now) break;
        uint64_t b = audio_state.queued[audio_state.q_head].buf;
        audio_state.q_head = (audio_state.q_head + 1) % 64;
        audio_state.q_count--;
        if (audio_state.r_count < 64) {
            audio_state.released[(audio_state.r_head + audio_state.r_count) % 64] = b;
            audio_state.r_count++;
            moved = 1;
        }
    }
    return moved;
}
void web_os_reset(void) {
    web_font_reset();
    // One fresh instance/ELF per worker run. Live workers must be stopped before reset.
    for (unsigned i = 0; i < 64; i++) { free(numberlines[i].bits); free(numberlines[i].run_len); }
    for (unsigned i = 0; i < 32; i++) free(stdallocs[i].blocks);
    for (unsigned i = 0; i < 128; i++) if (mqs[i].used) { free(mqs[i].buf); pthread_cond_destroy(&mqs[i].not_empty); pthread_cond_destroy(&mqs[i].not_full); }
    memset(threads, 0, sizeof threads); memset(events, 0, sizeof events);
    memset(holders, 0, sizeof holders); memset(mqs, 0, sizeof mqs);
    memset(numberlines, 0, sizeof numberlines); memset(stdallocs, 0, sizeof stdallocs);
    memset(&environment, 0, sizeof environment);
    memset(&hid_state, 0, sizeof hid_state);
    for(unsigned i=0;i<7;i++)atomic_store(&browser_pad[i],0);
    memset(&audio_state, 0, sizeof audio_state);
    audio_state.rate = 48000; audio_state.channels = 2;
    next_stdalloc_region = 0x7900000000ULL;
    bcat_mounted = 0; volume_main = 0.25f; volume_library = 1.0f;
    vsync_event = 0;
    threads[0] = (HostThread){.key=MAIN_THREAD,.id=1,.priority=16,.core=0,.mask=1,.started=1};
    current_thread = &threads[0]; thread_count = 1;
    environment.focus = 1; environment.recording = 1;
    hid_state.supported_styles = 0x3fu; hid_state.player0_connected = 1u;
}
static HostThread *thread_for(uint64_t key) {
    if (!key) key = current_thread ? current_thread->key : MAIN_THREAD;
    pthread_mutex_lock(&registry);
    for (unsigned i = 0; i < thread_count; i++) if (threads[i].key == key) {
        pthread_mutex_unlock(&registry); return &threads[i];
    }
    pthread_mutex_unlock(&registry); guest_trap("unknown ThreadType", key); return NULL;
}
static HostEvent *event_for(uint64_t key, int create, int signaled, int autoclear) {
    pthread_mutex_lock(&registry);
    if(!key){pthread_mutex_unlock(&registry);guest_trap("null event address",key);}
    HostEvent *empty = NULL;
    for (unsigned i = 0; i < 512; i++) {
        if (events[i].used && events[i].key == key) {
            HostEvent *event=&events[i];
            if(create) {
                if(event->references){pthread_mutex_unlock(&registry);guest_trap("reinitialising an event in use",key);}
                event->signaled=signaled;event->autoclear=autoclear;
            }
            event->references++;pthread_mutex_unlock(&registry);return event;
        }
        if (!events[i].used && !empty) empty = &events[i];
    }
    if (!create || !empty) { pthread_mutex_unlock(&registry); guest_trap("unknown event or event capacity", key); return NULL; }
    if(pthread_mutex_init(&empty->mutex,NULL)){pthread_mutex_unlock(&registry);guest_trap("event mutex creation failed",key);}
    if(pthread_cond_init(&empty->changed,NULL)){pthread_mutex_destroy(&empty->mutex);pthread_mutex_unlock(&registry);guest_trap("event condition creation failed",key);}
    empty->key = key; empty->signaled = signaled; empty->autoclear = autoclear; empty->used = 1;empty->references=1;
    pthread_mutex_unlock(&registry); return empty;
}
static void release_event(HostEvent *event) {
    pthread_mutex_lock(&registry);
    if(!event->references){pthread_mutex_unlock(&registry);guest_trap("invalid event lease",event->key);}
    event->references--;pthread_mutex_unlock(&registry);
}
static void finalize_event(uint64_t key) {
    pthread_mutex_lock(&registry);HostEvent *event=NULL;
    for(unsigned i=0;i<512;i++)if(events[i].used&&events[i].key==key){event=&events[i];break;}
    if(!event||event->references){pthread_mutex_unlock(&registry);guest_trap("finalising unknown event or event in use",key);}
    if(pthread_cond_destroy(&event->changed)||pthread_mutex_destroy(&event->mutex)){pthread_mutex_unlock(&registry);guest_trap("event finalization failed",key);}
    event->used=0;pthread_mutex_unlock(&registry);
}
static HostHolder *holder_for_locked(uint64_t key, int create) {
    HostHolder *empty = NULL;
    for (unsigned i = 0; i < 256; i++) {
        if (holders[i].used && holders[i].key == key) return &holders[i];
        if (!holders[i].used && !empty) empty = &holders[i];
    }
    if (!create || !empty) return NULL;
    memset(empty, 0, sizeof *empty);
    empty->key = key; empty->used = 1;
    return empty;
}
static HostMq *mq_for_locked(uint64_t key, int create, size_t cap) {
    HostMq *empty = NULL;
    for (unsigned i = 0; i < 128; i++) {
        if (mqs[i].used && mqs[i].key == key) {
            if (create == 2) { free(mqs[i].buf); mqs[i].cap = cap ? cap : 1; mqs[i].buf = calloc(mqs[i].cap, sizeof(uint64_t)); mqs[i].count = mqs[i].head = 0; }
            return &mqs[i];
        }
        if (!mqs[i].used && !empty) empty = &mqs[i];
    }
    if (!create || !empty) return NULL;
    memset(empty, 0, sizeof *empty);
    empty->key = key; empty->used = 1; empty->cap = cap ? cap : 1;
    empty->buf = calloc(empty->cap, sizeof(uint64_t));
    pthread_cond_init(&empty->not_empty, NULL);
    pthread_cond_init(&empty->not_full, NULL);
    return empty;
}
static uint64_t multiwait_check_locked(uint64_t mw) {
    audio_pump_locked(monotonic_ns());
    for (unsigned i = 0; i < 256; i++) {
        if (!holders[i].used || holders[i].multiwait != mw) continue;
        uint64_t tkey = holders[i].target;
        if (holders[i].is_thread) {
            for (unsigned j = 0; j < thread_count; j++) if (threads[j].key == tkey && threads[j].finished) return holders[i].key;
        } else {
            if (tkey == audio_state.event_key && audio_state.r_count > 0) return holders[i].key;
            for (unsigned j = 0; j < 512; j++) if (events[j].used && events[j].key == tkey && events[j].signaled) return holders[i].key;
        }
    }
    return 0;
}
static void *thread_entry(void *arg) {
    HostThread *thread = arg;
    current_thread = thread;
    GuestMemory *memory = elf_memory();
    Ctx c = {0}; c.x[31] = (thread->stack + thread->stack_size) & ~15ULL;
    c.x[30] = 0x7300000000ULL; c.x[0] = thread->arg;
    c.tpidr = TLS_BASE + (thread->id - 1) * 65536;
    if (gm_commit(memory, c.tpidr, 65536) != GM_OK || gm_write(memory, c.tpidr + 0x1f8, &thread->key, 8) != GM_OK) {
        // Establish the diagnostic boundary before reporting this through guest code.
        c.tpidr = 0;
    }
    web_memory_bind(memory);
    web_runtime_thread_call(&c, c.tpidr ? thread->entry : 0);
    web_memory_bind(NULL);
    pthread_mutex_lock(&registry); thread->finished = 1; pthread_cond_broadcast(&multiwait_cv); pthread_mutex_unlock(&registry);
#ifdef WEB_FIBERS
    web_fiber_thread_cleanup();
#endif
    pthread_exit(NULL);
}
static uint64_t wait_event(HostEvent *event, int block, int timed, uint64_t ns) {
    if (event->key && event->key == audio_state.event_key) {
        uint64_t wait_ns = 0;
        pthread_mutex_lock(&registry);
        uint64_t now = monotonic_ns();
        audio_pump_locked(now);
        if (audio_state.r_count == 0 && block && audio_state.q_count > 0) {
            uint64_t due = audio_state.queued[audio_state.q_head].due_ns;
            if (due > now) wait_ns = due - now;
            if (timed && wait_ns > ns) wait_ns = ns;
        }
        pthread_mutex_unlock(&registry);
        if (wait_ns > 0) {
            struct timespec delay = {(time_t)(wait_ns / 1000000000ULL), (long)(wait_ns % 1000000000ULL)};
            while (nanosleep(&delay, &delay) && errno == EINTR) {}
        }
        pthread_mutex_lock(&registry);
        audio_pump_locked(monotonic_ns());
        int has_rel = audio_state.r_count > 0;
        pthread_mutex_unlock(&registry);
        if (has_rel) {
            pthread_mutex_lock(&event->mutex);
            event->signaled = 1;
            pthread_mutex_unlock(&event->mutex);
        }
    }
    pthread_mutex_lock(&event->mutex);
    if (block && !timed && !event->signaled && event->key == vsync_event) {
        pthread_mutex_unlock(&event->mutex);
        release_event(event);
        struct timespec delay = {0, 16666667};
        while (nanosleep(&delay, &delay) && errno == EINTR) {}
        return 1;
    }
    int rc = 0;
    struct timespec until;
    if (timed) {
        if((int64_t)ns<0)ns=0;
        clock_gettime(CLOCK_REALTIME, &until);
        until.tv_sec += (time_t)(ns / 1000000000);
        until.tv_nsec += (long)(ns % 1000000000);
        if (until.tv_nsec >= 1000000000) { until.tv_sec++; until.tv_nsec -= 1000000000; }
    }
    while (block && !event->signaled && !rc) {
        rc = timed ? pthread_cond_timedwait(&event->changed, &event->mutex, &until) : pthread_cond_wait(&event->changed, &event->mutex);
    }
    int ready = event->signaled;
    if (ready && event->autoclear) event->signaled = 0;
    pthread_mutex_unlock(&event->mutex);
    uint64_t key=event->key;release_event(event);
    if(rc&&rc!=ETIMEDOUT)guest_trap("event wait failed",key);
    return (uint64_t)ready;
}
#define MOVIE_VTABLE 0x7800000a00ULL
#define MOVIE_FN_BASE 0x7500002000ULL

int web_os_indirect(Ctx *c, uint64_t address) {
    if (address >= MOVIE_FN_BASE && address < MOVIE_FN_BASE + 64 * 16 && !(address & 15)) {
        c->x[0] = 0x4000u; return 1;
    }
    if (address < OPUS_FN_BASE || address >= OPUS_FN_BASE + 10 * 16 || (address & 15)) return 0;
    unsigned slot = (unsigned)((address - OPUS_FN_BASE) / 16);
    uint64_t self = c->x[0];
    switch (slot) {
        case 0: case 1: return 1;
        case 2: c->x[0] = 0x2000; return 1;
        case 3: c->x[0] = rd8(self + 16); return 1;
        case 4: wr32(self + 8, (uint32_t)c->x[1]); wr32(self + 12, (uint32_t)c->x[2]); wr8(self + 16, 1); c->x[0] = 0; return 1;
        case 5: wr8(self + 16, 0); return 1;
        case 6: c->x[0] = rd32(self + 8); return 1;
        case 7: c->x[0] = rd32(self + 12); return 1;
        case 8: {
            uint64_t in = c->x[5], in_sz = c->x[6];
            if (in_sz < 8) { c->x[0] = 1; return 1; }
            uint32_t len = ((uint32_t)rd8(in) << 24) | ((uint32_t)rd8(in + 1) << 16) | ((uint32_t)rd8(in + 2) << 8) | rd8(in + 3);
            if (8ULL + len > in_sz) { c->x[0] = 1; return 1; }
            uint32_t rate = rd32(self + 8), ch = rd32(self + 12);
            if (!rate) rate = 48000; if (!ch) ch = 2;
            uint32_t frames = rate / 50;
            if (frames > (uint32_t)c->x[4]) frames = (uint32_t)c->x[4];
            uint64_t pcm = c->x[3], bytes = (uint64_t)frames * ch * 2;
            for (uint64_t i = 0; i < bytes; i += 2) wr16(pcm + i, 0);
            if (c->x[1]) wr64(c->x[1], 8ULL + len);
            if (c->x[2]) wr32(c->x[2], frames);
            c->x[0] = 0; return 1;
        }
        case 9: c->x[0] = 0; return 1;
    }
    return 0;
}
static HostNumberLine *numberline_for(uint64_t key, int create) {
    if (!key) guest_trap("null NumberLineAllocator address", key);
    HostNumberLine *empty = NULL;
    for (unsigned i = 0; i < 64; i++) {
        if (numberlines[i].used && numberlines[i].key == key) return &numberlines[i];
        if (!numberlines[i].used && !empty) empty = &numberlines[i];
    }
    if (!create || !empty) { pthread_mutex_unlock(&registry); guest_trap("unknown NumberLineAllocator or capacity", key); return NULL; }
    empty->key = key; empty->used = 1; empty->begin = empty->end = 0; empty->bits = NULL; empty->run_len = NULL;
    return empty;
}
static HostStdAlloc *stdalloc_for(uint64_t key, int create) {
    if (!key) guest_trap("null StandardAllocator address", key);
    HostStdAlloc *empty = NULL;
    for (unsigned i = 0; i < 32; i++) {
        if (stdallocs[i].used && stdallocs[i].key == key) return &stdallocs[i];
        if (!stdallocs[i].used && !empty) empty = &stdallocs[i];
    }
    if (!create || !empty) { pthread_mutex_unlock(&registry); guest_trap("unknown StandardAllocator or capacity", key); return NULL; }
    memset(empty, 0, sizeof *empty); empty->key = key; empty->used = 1;
    return empty;
}
static uint64_t stdalloc_allocate(HostStdAlloc *sa, uint64_t req_size, uint64_t align) {
    uint64_t size = req_size ? req_size : 1;
    uint64_t a = align < 16 ? 16 : align;
    if ((a & (a - 1)) != 0) { uint64_t p2 = 16; while (p2 < a) p2 <<= 1; a = p2; }
    uint64_t cand = (sa->base + a - 1) & ~(a - 1);
    for (;;) {
        if (cand < sa->base || cand + size < cand || cand + size > sa->base + sa->size) return 0;
        int overlap = 0;
        for (size_t i = 0; i < sa->cap; i++) {
            if (!sa->blocks[i].used) continue;
            uint64_t b0 = sa->blocks[i].addr, b1 = b0 + sa->blocks[i].size;
            if (cand < b1 && b0 < cand + size) {
                cand = (b1 + a - 1) & ~(a - 1);
                overlap = 1;
                break;
            }
        }
        if (!overlap) break;
    }
    size_t slot = sa->cap;
    for (size_t i = 0; i < sa->cap; i++) if (!sa->blocks[i].used) { slot = i; break; }
    if (slot == sa->cap) {
        size_t ncap = sa->cap ? sa->cap * 2 : 64;
        StdAllocBlock *nb = realloc(sa->blocks, ncap * sizeof *nb);
        if (!nb) return 0;
        memset(nb + sa->cap, 0, (ncap - sa->cap) * sizeof *nb);
        sa->blocks = nb; sa->cap = ncap;
    }
    sa->blocks[slot] = (StdAllocBlock){.addr = cand, .size = size, .used = 1};
    uint64_t consumed = (size + 15ULL) & ~15ULL;
    sa->free_size = sa->free_size > consumed ? sa->free_size - consumed : 0;
    return cand;
}
typedef struct {
    Ctx *c;
    int is_va, gp, fp;
    uint64_t stack, gr_top, vr_top;
    int32_t gr_offs, vr_offs;
} FmtArgs;
static void fmt_args_direct(FmtArgs *a, Ctx *c, int first_gp) {
    memset(a, 0, sizeof *a); a->c = c; a->gp = first_gp; a->stack = c->x[31];
}
static void fmt_args_va(FmtArgs *a, Ctx *c, uint64_t va) {
    memset(a, 0, sizeof *a); a->c = c; a->is_va = 1;
    a->stack = rd64(va); a->gr_top = rd64(va + 8); a->vr_top = rd64(va + 16);
    a->gr_offs = (int32_t)rd32(va + 24); a->vr_offs = (int32_t)rd32(va + 28);
}
static uint64_t fmt_arg_int(FmtArgs *a) {
    if (a->is_va) {
        if (a->gr_offs < 0) { uint64_t v = rd64(a->gr_top + (uint64_t)(int64_t)a->gr_offs); a->gr_offs += 8; return v; }
    } else if (a->gp < 8) return a->c->x[a->gp++];
    uint64_t v = rd64(a->stack); a->stack += 8; return v;
}
static double fmt_arg_double(FmtArgs *a) {
    double v = 0.0;
    if (a->is_va) {
        if (a->vr_offs < 0) { uint64_t bits = rd64(a->vr_top + (uint64_t)(int64_t)a->vr_offs); memcpy(&v, &bits, 8); a->vr_offs += 16; return v; }
    } else if (a->fp < 8) return a->c->v[a->fp++].df[0];
    uint64_t bits = rd64(a->stack); memcpy(&v, &bits, 8); a->stack += 8; return v;
}
static int guest_format(uint64_t out_addr, size_t size, uint64_t fmt_addr, FmtArgs *a) {
    size_t n = 0;
#define GPUT(ch) do { if (out_addr && n + 1 < size) wr8(out_addr + n, (uint8_t)(ch)); n++; } while (0)
    for (uint64_t p = fmt_addr; ; p++) {
        char c0 = (char)rd8(p);
        if (!c0) break;
        if (c0 != '%') { GPUT(c0); continue; }
        char c1 = (char)rd8(p + 1);
        if (c1 == '%') { GPUT('%'); p++; continue; }
        char spec[48], piece[512];
        size_t k = 0;
        spec[k++] = c0; p++;
        while ((c0 = (char)rd8(p)) != 0 && strchr("-+ #0", c0) && k < 40) { spec[k++] = c0; p++; }
        if ((c0 = (char)rd8(p)) == '*') { k += (size_t)snprintf(spec + k, 12, "%d", (int)fmt_arg_int(a)); p++; }
        else while ((c0 = (char)rd8(p)) != 0 && isdigit((unsigned char)c0) && k < 40) { spec[k++] = c0; p++; }
        int have_prec = 0, prec = 0;
        if ((c0 = (char)rd8(p)) == '.') {
            spec[k++] = c0; p++; have_prec = 1;
            if ((c0 = (char)rd8(p)) == '*') { prec = (int)fmt_arg_int(a); k += (size_t)snprintf(spec + k, 12, "%d", prec); p++; if (prec < 0) have_prec = 0; }
            else while ((c0 = (char)rd8(p)) != 0 && isdigit((unsigned char)c0) && k < 40) { prec = prec * 10 + (c0 - '0'); spec[k++] = c0; p++; }
        }
        int longs = 0, halves = 0;
        while ((c0 = (char)rd8(p)) != 0 && strchr("hlLqjzt", c0)) {
            if (c0 == 'l' || c0 == 'q' || c0 == 'j' || c0 == 'z' || c0 == 't') longs++;
            else if (c0 == 'h') halves++;
            p++;
        }
        char conv = (char)rd8(p);
        if (!conv) break;
        piece[0] = 0;
        if (strchr("diuxXoc", conv)) {
            uint64_t v = fmt_arg_int(a);
            if (conv == 'c') { spec[k++] = 'c'; spec[k] = 0; snprintf(piece, sizeof piece, spec, (int)v); }
            else {
                spec[k++] = 'l'; spec[k++] = 'l'; spec[k++] = conv; spec[k] = 0;
                int sg = conv == 'd' || conv == 'i';
                if (longs) ;
                else if (halves >= 2) v = sg ? (uint64_t)(int64_t)(int8_t)v : (uint64_t)(uint8_t)v;
                else if (halves == 1) v = sg ? (uint64_t)(int64_t)(int16_t)v : (uint64_t)(uint16_t)v;
                else v = sg ? (uint64_t)(int64_t)(int32_t)v : (uint64_t)(uint32_t)v;
                snprintf(piece, sizeof piece, spec, (unsigned long long)v);
            }
        } else if (strchr("fFeEgGaA", conv)) {
            spec[k++] = conv; spec[k] = 0;
            snprintf(piece, sizeof piece, spec, fmt_arg_double(a));
        } else if (conv == 's') {
            uint64_t s_addr = fmt_arg_int(a);
            char *narrow = NULL;
            const char *s = "(null)";
            if (s_addr) {
                if (longs) {
                    size_t wn = 0, o = 0;
                    while ((!have_prec || wn < (size_t)prec) && rd32(s_addr + wn * 4) != 0) wn++;
                    narrow = malloc(wn * 4 + 1);
                    if (narrow) {
                        for (size_t i = 0; i < wn; i++) {
                            uint32_t u = rd32(s_addr + i * 4);
                            if (u < 0x80) narrow[o++] = (char)u;
                            else if (u < 0x800) { narrow[o++] = (char)(0xc0 | (u >> 6)); narrow[o++] = (char)(0x80 | (u & 0x3f)); }
                            else if (u < 0x10000) { narrow[o++] = (char)(0xe0 | (u >> 12)); narrow[o++] = (char)(0x80 | ((u >> 6) & 0x3f)); narrow[o++] = (char)(0x80 | (u & 0x3f)); }
                            else { narrow[o++] = (char)(0xf0 | (u >> 18)); narrow[o++] = (char)(0x80 | ((u >> 12) & 0x3f)); narrow[o++] = (char)(0x80 | ((u >> 6) & 0x3f)); narrow[o++] = (char)(0x80 | (u & 0x3f)); }
                        }
                        narrow[o] = 0; s = narrow;
                    }
                } else {
                    size_t slen = 0;
                    while ((!have_prec || slen < (size_t)prec) && rd8(s_addr + slen) != 0) slen++;
                    narrow = malloc(slen + 1);
                    if (narrow) {
                        if (slen) web_read(s_addr, narrow, slen);
                        narrow[slen] = 0; s = narrow;
                    }
                }
            }
            spec[k++] = 's'; spec[k] = 0;
            int len = snprintf(NULL, 0, spec, s);
            if (len >= 0) {
                char *big = len < (int)sizeof piece ? piece : malloc((size_t)len + 1);
                if (big) {
                    snprintf(big, (size_t)len + 1, spec, s);
                    for (int i = 0; i < len; i++) GPUT(big[i]);
                    if (big != piece) free(big);
                }
            }
            free(narrow);
            continue;
        } else if (conv == 'p') {
            char hex[24];
            unsigned long long pv = (unsigned long long)fmt_arg_int(a);
            int left = 0;
            const char *z = spec + 1;
            spec[k] = 0;
            while (*z && strchr("-+ #0", *z)) { if (*z == '-') left = 1; z++; }
            snprintf(hex, sizeof hex, pv ? "0x%016llx" : "%016llx", pv);
            snprintf(piece, sizeof piece, left ? "%-*s" : "%*s", atoi(z), hex);
        } else if (conv == 'n') {
            uint64_t dst = fmt_arg_int(a);
            if (dst) { if (longs) wr64(dst, n); else if (halves >= 2) wr8(dst, (uint8_t)n); else if (halves == 1) wr16(dst, (uint16_t)n); else wr32(dst, (uint32_t)n); }
        } else {
            GPUT('%'); GPUT(conv);
            continue;
        }
        for (const char *q = piece; *q; q++) GPUT(*q);
    }
#undef GPUT
    if (out_addr && size) wr8(out_addr + (n < size ? n : size - 1), 0);
    return (int)n;
}
static int64_t days_from_civil(int64_t y, int m, int d) {
    y -= m <= 2;
    int64_t era = (y >= 0 ? y : y - 399) / 400;
    int64_t yoe = y - era * 400, doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}
static void civil_from_days(int64_t z, int64_t *y, int *m, int *d) {
    z += 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097, doe = z - era * 146097;
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365, doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int64_t mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = yoe + era * 400 + (*m <= 2);
}
static void utc_split(int64_t t, int *year, int *mon, int *mday, int *hour, int *min, int *sec, int *wday, int *yday) {
    int64_t days = t / 86400, rem = t % 86400, y;
    if (rem < 0) { rem += 86400; days--; }
    civil_from_days(days, &y, mon, mday);
    *year = (int)y;
    *hour = (int)(rem / 3600); *min = (int)(rem / 60 % 60); *sec = (int)(rem % 60);
    *wday = (int)(((days % 7) + 11) % 7);
    *yday = (int)(days - days_from_civil(y, 1, 1));
}
static int bcat_dirname_valid_guest(uint64_t addr) {
    if (!addr) return 0;
    int n = 0;
    for (; n < 32; n++) {
        uint8_t ch = rd8(addr + (uint64_t)n);
        if (!ch) break;
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '-')) return 0;
    }
    return n != 0 && n != 32;
}
typedef struct { const uint8_t *src; size_t len, pos; uint32_t bitbuf; int bitcnt; } ZBitReader;
static int z_bits(ZBitReader *b, int n, uint32_t *out) {
    while (b->bitcnt < n) {
        if (b->pos >= b->len) return 0;
        b->bitbuf |= (uint32_t)b->src[b->pos++] << b->bitcnt;
        b->bitcnt += 8;
    }
    *out = b->bitbuf & ((1u << n) - 1u);
    b->bitbuf >>= n; b->bitcnt -= n;
    return 1;
}
typedef struct { uint16_t counts[16], symbols[288]; } ZHuff;
static int z_build_huff(ZHuff *h, const uint8_t *lengths, int num) {
    uint16_t offs[16];
    memset(h->counts, 0, sizeof h->counts);
    for (int i = 0; i < num; i++) {
        if (lengths[i] > 15) return 0;
        h->counts[lengths[i]]++;
    }
    if (h->counts[0] == num) return 1;
    int left = 1;
    for (int len = 1; len <= 15; len++) {
        left = (left << 1) - h->counts[len];
        if (left < 0) return 0;
    }
    offs[1] = 0;
    for (int len = 1; len < 15; len++) offs[len + 1] = offs[len] + h->counts[len];
    for (int i = 0; i < num; i++) if (lengths[i]) h->symbols[offs[lengths[i]]++] = (uint16_t)i;
    return left == 0 || (num == 32 && h->counts[1] == 1);
}
static int z_decode(ZBitReader *b, const ZHuff *h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; len++) {
        uint32_t bit = 0;
        if (!z_bits(b, 1, &bit)) return -1;
        code |= (int)bit;
        int count = h->counts[len];
        if (code - count < first) return h->symbols[index + (code - first)];
        index += count; first = (first + count) << 1; code <<= 1;
    }
    return -1;
}
static int zlib_decompress(uint8_t *dst, size_t dst_cap, const uint8_t *src, size_t src_len) {
    if (!dst || !src || src_len < 6) return 0;
    uint8_t cmf = src[0], flg = src[1];
    if ((cmf & 0x0f) != 8 || (cmf >> 4) > 7 || (((unsigned)cmf << 8) + flg) % 31 != 0 || (flg & 0x20)) return 0;
    ZBitReader br = {src, src_len - 4, 2, 0, 0};
    size_t out_pos = 0;
    static const uint16_t lbase[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const uint8_t lext[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const uint16_t dbase[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const uint8_t dext[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    static const uint8_t clen_order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    uint32_t bfinal = 0;
    while (!bfinal) {
        uint32_t btype = 0;
        if (!z_bits(&br, 1, &bfinal) || !z_bits(&br, 2, &btype) || btype == 3) return 0;
        if (btype == 0) {
            br.bitbuf = 0; br.bitcnt = 0;
            if (br.pos + 4 > br.len) return 0;
            uint16_t len = (uint16_t)(br.src[br.pos] | (br.src[br.pos + 1] << 8));
            uint16_t nlen = (uint16_t)(br.src[br.pos + 2] | (br.src[br.pos + 3] << 8));
            br.pos += 4;
            if ((uint16_t)~len != nlen || br.pos + len > br.len || out_pos + len > dst_cap) return 0;
            memcpy(dst + out_pos, br.src + br.pos, len);
            br.pos += len; out_pos += len;
            continue;
        }
        ZHuff lcode, dcode;
        uint8_t lengths[320] = {0};
        if (btype == 1) {
            for (int i = 0; i < 144; i++) lengths[i] = 8;
            for (int i = 144; i < 256; i++) lengths[i] = 9;
            for (int i = 256; i < 280; i++) lengths[i] = 7;
            for (int i = 280; i < 288; i++) lengths[i] = 8;
            for (int i = 0; i < 32; i++) lengths[288 + i] = 5;
            z_build_huff(&lcode, lengths, 288);
            z_build_huff(&dcode, lengths + 288, 32);
        } else {
            uint32_t hlit = 0, hdist = 0, hclen = 0;
            if (!z_bits(&br, 5, &hlit) || !z_bits(&br, 5, &hdist) || !z_bits(&br, 4, &hclen)) return 0;
            hlit += 257; hdist += 1; hclen += 4;
            if (hlit > 286 || hdist > 30) return 0;
            uint8_t clen[19] = {0};
            for (uint32_t i = 0; i < hclen; i++) {
                uint32_t v = 0;
                if (!z_bits(&br, 3, &v)) return 0;
                clen[clen_order[i]] = (uint8_t)v;
            }
            if (!z_build_huff(&lcode, clen, 19)) return 0;
            uint32_t idx = 0, total = hlit + hdist;
            while (idx < total) {
                int sym = z_decode(&br, &lcode);
                if (sym < 0) return 0;
                if (sym < 16) lengths[idx++] = (uint8_t)sym;
                else {
                    uint32_t rep = 0, val = 0;
                    if (sym == 16) {
                        if (!idx || !z_bits(&br, 2, &rep)) return 0;
                        rep += 3; val = lengths[idx - 1];
                    } else if (sym == 17) {
                        if (!z_bits(&br, 3, &rep)) return 0;
                        rep += 3;
                    } else {
                        if (!z_bits(&br, 7, &rep)) return 0;
                        rep += 11;
                    }
                    if (idx + rep > total) return 0;
                    while (rep--) lengths[idx++] = (uint8_t)val;
                }
            }
            if (!lengths[256] || !z_build_huff(&lcode, lengths, (int)hlit) || !z_build_huff(&dcode, lengths + hlit, (int)hdist)) return 0;
        }
        for (;;) {
            int sym = z_decode(&br, &lcode);
            if (sym < 0) return 0;
            if (sym < 256) {
                if (out_pos >= dst_cap) return 0;
                dst[out_pos++] = (uint8_t)sym;
            } else if (sym == 256) break;
            else {
                sym -= 257;
                if (sym >= 29) return 0;
                uint32_t ext = 0;
                if (lext[sym] && !z_bits(&br, lext[sym], &ext)) return 0;
                size_t match_len = lbase[sym] + ext;
                int dsym = z_decode(&br, &dcode);
                if (dsym < 0 || dsym >= 30) return 0;
                uint32_t dext_v = 0;
                if (dext[dsym] && !z_bits(&br, dext[dsym], &dext_v)) return 0;
                size_t dist = dbase[dsym] + dext_v;
                if (dist > out_pos || out_pos + match_len > dst_cap) return 0;
                for (size_t i = 0; i < match_len; i++) {
                    dst[out_pos] = dst[out_pos - dist];
                    out_pos++;
                }
            }
        }
    }
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < out_pos; i++) { s1 = (s1 + dst[i]) % 65521u; s2 = (s2 + s1) % 65521u; }
    uint32_t want_adler = ((uint32_t)src[src_len - 4] << 24) | ((uint32_t)src[src_len - 3] << 16) | ((uint32_t)src[src_len - 2] << 8) | src[src_len - 1];
    return ((s2 << 16) | s1) == want_adler;
}
int web_os_import(Ctx *c, const char *name) {
    if(web_font_import(c,name))return 1;
#ifdef WEB_FIBERS
    if(web_fiber_import(c,name))return 1;
#endif
    if (!strcmp(name, "_ZN2nn4time13IsInitializedEv")) { struct timespec now; c->x[0] = clock_gettime(CLOCK_REALTIME,&now) == 0; return 1; }
    if (!strcmp(name, "_ZN2nn8settingseqERKNS0_12LanguageCodeERKNS0_8LanguageE") || !strcmp(name, "_ZN2nn8settingseqERKNS0_8LanguageERKNS0_12LanguageCodeE")) {
        static const char *codes[] = {"ja","en-US","fr","de","it","es","zh-CN","ko","nl","pt","ru","zh-TW","en-GB","fr-CA","es-419","zh-Hans","zh-Hant","pt-BR"};
        int code_first = strstr(name, "eqERKNS0_12LanguageCode") != NULL;
        uint64_t code = rd64(c->x[code_first ? 0 : 1]);
        uint32_t language = rd32(c->x[code_first ? 1 : 0]); uint64_t expected = 0;
        if (language < sizeof codes / sizeof codes[0]) memcpy(&expected, codes[language], strlen(codes[language]));
        c->x[0] = expected && expected == code; return 1;
    }
    if (!strcmp(name, "_ZN2nn2oe10InitializeEv")) { environment.initialized = 1; release_event(event_for(NOTIFICATION, 1, 0, 0)); return 1; }
    if (!strcmp(name, "_ZN2nn2oe27SetPerformanceConfigurationENS0_15PerformanceModeEi")) {
        if (c->x[0] > 1) guest_trap("invalid performance mode", c->x[0]);
        environment.configurations[c->x[0]] = (int)c->x[1]; return 1;
    }
    if (!strcmp(name, "_ZN2nn2oe20SetFocusHandlingModeENS0_17FocusHandlingModeE")) { environment.focus_mode = (int)c->x[0]; return 1; }
    if (!strcmp(name, "_ZN2nn2oe42SetOperationModeChangedNotificationEnabledEb")) { environment.operation_mode_notify = (int)(c->x[0] & 1); return 1; }
    if (!strcmp(name, "_ZN2nn2oe16DisableRecordingEv")) { environment.recording = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2oe27GetNotificationMessageEventEv")) { c->x[0] = NOTIFICATION; return 1; }
    if (!strcmp(name, "_ZN2nn2oe16GetOperationModeEv")) { c->x[0] = 1; return 1; } // desktop/docked browser surface
    if (!strcmp(name, "_ZN2nn2oe20GetCurrentFocusStateEv")) { c->x[0] = (uint64_t)environment.focus; return 1; }
    if (!strcmp(name, "_ZN2nn2oe18GetDesiredLanguageEv")) { c->x[0] = 0x53552d6e65ULL; return 1; } // en-US
    if (!strcmp(name, "_ZN2nn2oe17FinishStartupLogoEv") ||
        !strcmp(name, "_ZN2nn2oe29BeginAutoSleepDisabledSectionEv") ||
        !strcmp(name, "_ZN2nn2oe27EndAutoSleepDisabledSectionEv") ||
        !strcmp(name, "_ZN2nn2oe31EnterExitRequestHandlingSectionEv") ||
        !strcmp(name, "_ZN2nn2oe31LeaveExitRequestHandlingSectionEv") ||
        !strcmp(name, "_ZN2nn2oe25TryPopNotificationMessageEPj") ||
        !strcmp(name, "_ZN2nn5prepo10InitializeEv") || !strcmp(name, "_ZN2nn5prepo28RequestImmediateTransmissionEv") ||
        !strcmp(name, "_ZN2nn5album10InitializeEv") || !strcmp(name, "_ZN2nn5album8FinalizeEv") ||
        !strcmp(name, "_ZN2nn3err9ShowErrorENS_6ResultE") || !strcmp(name, "_ZN2nn3err9ShowErrorENS0_9ErrorCodeE") ||
        !strcmp(name, "_ZN2nn2ro10InitializeEv")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nu16VirtualAllocHookEmmPv") || !strcmp(name, "_ZN2nu15VirtualFreeHookEPv")) { c->x[0] = 1; return 1; }
    if (!strcmp(name, "_ZN2nn2ro18RegisterModuleInfoEPNS0_16RegistrationInfoEPKv")) {
        if (!c->x[0] || !c->x[1] || rd32(c->x[1]) != 0x3052524eu) { c->x[0] = 0xc16; return 1; }
        wr32(c->x[0], 1); wr64(c->x[0] + 8, c->x[1]); wr64(c->x[0] + 16, c->x[0] + 16); wr64(c->x[0] + 24, c->x[0] + 16);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2ro20UnregisterModuleInfoEPNS0_16RegistrationInfoE")) {
        if (c->x[0]) wr32(c->x[0], 0);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2ro13GetBufferSizeEPmPKv")) {
        if (!c->x[0] || !c->x[1] || rd32(c->x[1] + 0x10) != 0x304f524eu) { c->x[0] = 0x816; return 1; }
        wr64(c->x[0], (uint64_t)rd32(c->x[1] + 0x38));
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2ro10LoadModuleEPNS0_6ModuleEPKvPvmi")) {
        uint64_t out = c->x[0], nro = c->x[1], bss = c->x[2], bss_size = c->x[3];
        if (!out || !nro || rd32(nro + 0x10) != 0x304f524eu) { c->x[0] = 0x816; return 1; }
        uint32_t file_size = rd32(nro + 0x18), mod0 = rd32(nro + 4);
        uint64_t obj = nro;
        if (mod0 + 0x1c <= file_size) {
            int64_t rel = (int64_t)mod0 + (int32_t)rd32(nro + mod0 + 0x18);
            if (rel >= 0 && (uint64_t)rel + 0x28 <= file_size) obj = nro + (uint64_t)rel;
            else if (bss && rel >= (int64_t)file_size && (uint64_t)(rel - file_size) + 0x28 <= bss_size)
                obj = bss + (uint64_t)(rel - file_size);
        }
        if (obj != nro) { wr64(obj, obj); wr64(obj + 8, obj); wr64(obj + 0x20, nro); }
        wr64(out, obj); wr32(out + 8, 1); wr64(out + 0x10, nro); wr64(out + 0x18, bss);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2ro12UnloadModuleEPNS0_6ModuleE")) {
        if (c->x[0]) wr32(c->x[0] + 8, 0);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2vi10InitializeEv") || !strcmp(name, "_ZN2nn2vi8FinalizeEv") ||
        !strcmp(name, "_ZN2nn2vi12DestroyLayerEPNS0_5LayerE") || !strcmp(name, "_ZN2nn2vi12CloseDisplayEPNS0_7DisplayE")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2vi18OpenDefaultDisplayEPPNS0_7DisplayE")) { if (c->x[0]) wr64(c->x[0], 0x7800000900ULL); c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2vi11CreateLayerEPPNS0_5LayerEPNS0_7DisplayE")) { if (c->x[0]) wr64(c->x[0], 0x7800000910ULL); c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2vi15GetNativeWindowEPPvPNS0_5LayerE")) { if (c->x[0]) wr64(c->x[0], 0x7800000920ULL); c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2vi20GetDisplayVsyncEventEPNS_2os15SystemEventTypeEPNS0_7DisplayE")) {
        vsync_event = c->x[0]; release_event(event_for(c->x[0], 1, 1, 1)); c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn3hid", 10) || !strncmp(name, "_ZNK2nn3hid", 11)) {
        if (!strcmp(name, "_ZN2nn3hid14InitializeNpadEv")) {
            hid_state.supported_styles = 0x3fu; hid_state.player0_connected = 1u; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid24SetSupportedNpadStyleSetENS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE")) {
            hid_state.supported_styles = (uint32_t)c->x[0] ? (uint32_t)c->x[0] : 0x3fu; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid22SetSupportedNpadIdTypeEPKjm")) {
            uint64_t count = c->x[1] < 16 ? c->x[1] : 16; uint32_t has_p0 = 0;
            for (uint64_t i = 0; i < count; i++) if (rd32(c->x[0] + i * 4) == 0) has_p0 = 1u;
            hid_state.player0_connected = has_p0; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid18SetNpadJoyHoldTypeENS0_15NpadJoyHoldTypeE")) {
            hid_state.joy_hold_type = (uint32_t)c->x[0]; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid20GetNpadJoyAssignmentERKj")) {
            uint32_t id = rd32(c->x[0]); c->x[0] = id < 8 ? hid_state.joy_assignment[id] : 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid30SetNpadJoyAssignmentModeSingleERKjNS0_17NpadJoyDeviceTypeE") ||
            !strcmp(name, "_ZN2nn3hid30SetNpadJoyAssignmentModeSingleERKj")) {
            uint32_t id = rd32(c->x[0]); if (id < 8) hid_state.joy_assignment[id] = 1u; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid28SetNpadJoyAssignmentModeDualERKj")) {
            uint32_t id = rd32(c->x[0]); if (id < 8) hid_state.joy_assignment[id] = 0u; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid23MergeSingleJoyAsDualJoyERKjS2_") ||
            !strcmp(name, "_ZN2nn3hid18SwapNpadAssignmentERKjS2_") ||
            !strcmp(name, "_ZN2nn3hid21InitializeTouchScreenEv") ||
            !strcmp(name, "_ZN2nn3hid21StartLrAssignmentModeEv") ||
            !strcmp(name, "_ZN2nn3hid20StopLrAssignmentModeEv") ||
            !strcmp(name, "_ZN2nn3hid18StartSixAxisSensorERKNS0_19SixAxisSensorHandleE") ||
            !strcmp(name, "_ZN2nn3hid25InitializeVibrationDeviceERKNS0_21VibrationDeviceHandleE") ||
            !strcmp(name, "_ZN2nn3hid18SendVibrationValueERKNS0_21VibrationDeviceHandleERKNS0_14VibrationValueE") ||
            !strcmp(name, "_ZN2nn3hid25SendVibrationGcErmCommandERKNS0_21VibrationDeviceHandleERKNS0_21VibrationGcErmCommandE") ||
            !strcmp(name, "_ZN2nn3hid28InitializeSevenSixAxisSensorEPvm") ||
            !strcmp(name, "_ZN2nn3hid23StartSevenSixAxisSensorEv") ||
            !strcmp(name, "_ZN2nn3hid22StopSevenSixAxisSensorEv")) { c->x[0] = 0; return 1; }
        if (!strcmp(name, "_ZN2nn3hid14DisconnectNpadERKj")) {
            if (rd32(c->x[0]) == 0) hid_state.player0_connected = 0u; c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid19GetPlayerLedPatternERKj")) {
            static const uint8_t patterns[8] = {1, 3, 7, 15, 9, 5, 13, 6};
            uint32_t id = rd32(c->x[0]); c->x[0] = id < 8 ? patterns[id] : 0u; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid15GetNpadStyleSetERKj")) {
            uint32_t id = rd32(c->x[0]);
            uint32_t style = (id == 0 && hid_state.player0_connected) ? ((hid_state.supported_styles & 1u) ? 1u : (hid_state.supported_styles & (~hid_state.supported_styles + 1u))) : 0u;
            c->x[0] = style; return 1;
        }
        if (!strncmp(name, "_ZN2nn3hid12GetNpadStateEPNS0_", 30)) {
            uint64_t out = c->x[0]; uint32_t id = rd32(c->x[1]);
            uint32_t connected = (id == 0 && hid_state.player0_connected) ? 3u : 0u;
            wr64(out + 0x00, ++hid_state.sampling_number);
            uint32_t pad[6]={0};if(connected)browser_pad_read(pad);
            wr64(out + 0x08, ((uint64_t)pad[1]<<32)|pad[0]);
            for(unsigned i=0;i<4;i++) {
                int32_t stick=(int32_t)pad[i+2];if(stick>32767)stick=32767;if(stick< -32767)stick=-32767;
                wr32(out+0x10+i*4,(uint32_t)stick);
            }
            wr32(out + 0x20, connected); wr32(out + 0x24, 0);
            if (!strcmp(name, "_ZN2nn3hid12GetNpadStateEPNS0_11NpadGcStateERKj")) wr64(out + 0x28, 0);
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid22GetNpadControllerColorEPNS0_19NpadControllerColorES2_RKj")) {
            if (c->x[0]) { wr32(c->x[0], 0xff323232u); wr32(c->x[0] + 4, 0xffffffffu); }
            if (c->x[1]) { wr32(c->x[1], 0xff323232u); wr32(c->x[1] + 4, 0xffffffffu); }
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid22GetNpadControllerColorEPNS0_19NpadControllerColorERKj")) {
            if (c->x[0]) { wr32(c->x[0], 0xff323232u); wr32(c->x[0] + 4, 0xffffffffu); }
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid19GetTouchScreenStateILm16EEEvPNS0_16TouchScreenStateIXT_EEE")) {
            wr64(c->x[0], ++hid_state.touch_sampling_number);
            for (uint64_t off = 8; off < 0x290; off += 8) wr64(c->x[0] + off, 0);
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid23GetSixAxisSensorHandlesEPNS0_19SixAxisSensorHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE") ||
            !strcmp(name, "_ZN2nn3hid25GetVibrationDeviceHandlesEPNS0_21VibrationDeviceHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE")) {
            int is_vib = strstr(name, "Vibration") != NULL;
            int cap = (int)c->x[1] > 0 ? (int)c->x[1] : 0;
            uint32_t id = rd32(c->x[2]), style = (uint32_t)c->x[3];
            int avail = is_vib ? ((style & 7u) ? 2 : (style & 0x38u) ? 1 : 0) : ((style & 4u) ? 2 : (style & 0x1bu) ? 1 : 0);
            int written = cap < avail ? cap : avail;
            for (int i = 0; i < written; i++) wr32(c->x[0] + (uint64_t)i * 4, ((id & 0xffu) << 8) | (uint32_t)(i + 1));
            c->x[0] = (uint64_t)written; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid21GetSixAxisSensorStateEPNS0_18SixAxisSensorStateERKNS0_19SixAxisSensorHandleE")) {
            for (uint64_t off = 0; off < 0x60; off += 8) wr64(c->x[0] + off, 0);
            wr64(c->x[0] + 0x08, ++hid_state.sixaxis_sampling_number);
            wr32(c->x[0] + 0x34, 0x3f800000u); wr32(c->x[0] + 0x44, 0x3f800000u); wr32(c->x[0] + 0x54, 0x3f800000u);
            wr32(c->x[0] + 0x5c, 1u); c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZNK2nn3hid18SixAxisSensorState13GetQuaternionEPNS_4util4neon12Vector4fTypeE")) {
            wr32(c->x[1], 0); wr32(c->x[1] + 4, 0); wr32(c->x[1] + 8, 0); wr32(c->x[1] + 12, 0x3f800000u);
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid21IsSixAxisSensorAtRestERKNS0_19SixAxisSensorHandleE") ||
            !strcmp(name, "_ZN2nn3hid26IsSevenSixAxisSensorAtRestEv")) { c->x[0] = 1; return 1; }
        if (!strcmp(name, "_ZN2nn3hid22GetVibrationDeviceInfoEPNS0_19VibrationDeviceInfoERKNS0_21VibrationDeviceHandleE")) {
            uint32_t slot = rd32(c->x[1]) & 0xffu;
            wr32(c->x[0], 1u); wr32(c->x[0] + 4, slot == 2u ? 2u : 1u); c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid23GetActualVibrationValueEPNS0_14VibrationValueERKNS0_21VibrationDeviceHandleE")) {
            wr32(c->x[0], 0); wr32(c->x[0] + 4, 0x43200000u); wr32(c->x[0] + 8, 0); wr32(c->x[0] + 12, 0x43a00000u);
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid30GetActualVibrationGcErmCommandEPNS0_21VibrationGcErmCommandERKNS0_21VibrationDeviceHandleE")) {
            wr32(c->x[0], 0); c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3hid27GetSevenSixAxisSensorStatesEPNS0_23SevenSixAxisSensorStateEi")) {
            if (c->x[0] && (int)c->x[1] > 0) for (uint64_t off = 0; off < 0x48; off += 8) wr64(c->x[0] + off, 0);
            c->x[0] = 0; return 1;
        }
    }
    if (!strcmp(name, "_ZN2nn2os15InitializeEventEPNS0_9EventTypeEbNS0_14EventClearModeE")||
        !strcmp(name,"_ZN2nn2os20InitializeLightEventEPNS0_14LightEventTypeEbNS0_14EventClearModeE")||
        !strcmp(name,"nnosInitializeEvent")) {
        if(c->x[2]>1)guest_trap("invalid event clear mode",c->x[2]);
        release_event(event_for(c->x[0], 1, c->x[1] != 0, c->x[2] == 1)); return 1;
    }
    if(!strcmp(name,"_ZN2nn2os13FinalizeEventEPNS0_9EventTypeE")||!strcmp(name,"_ZN2nn2os18FinalizeLightEventEPNS0_14LightEventTypeE")||
       !strcmp(name,"_ZN2nn2os18DestroySystemEventEPNS0_15SystemEventTypeE")||!strcmp(name,"nnosFinalizeEvent")){finalize_event(c->x[0]);return 1;}
    if (!strcmp(name, "_ZN2nn2os11SignalEventEPNS0_9EventTypeE") || !strcmp(name, "_ZN2nn2os17SignalSystemEventEPNS0_15SystemEventTypeE")||
        !strcmp(name,"_ZN2nn2os16SignalLightEventEPNS0_14LightEventTypeE") || !strcmp(name,"nnosSignalEvent")) {
        HostEvent *event = event_for(c->x[0], 0, 0, 0);
        pthread_mutex_lock(&event->mutex); event->signaled = 1; pthread_cond_broadcast(&event->changed); pthread_mutex_unlock(&event->mutex);
        pthread_mutex_lock(&registry); pthread_cond_broadcast(&multiwait_cv); pthread_mutex_unlock(&registry);
        release_event(event); return 1;
    }
    if (!strcmp(name, "_ZN2nn2os10ClearEventEPNS0_9EventTypeE") || !strcmp(name, "_ZN2nn2os16ClearSystemEventEPNS0_15SystemEventTypeE")||
        !strcmp(name,"_ZN2nn2os15ClearLightEventEPNS0_14LightEventTypeE")) {
        HostEvent *event = event_for(c->x[0], 0, 0, 0);
        pthread_mutex_lock(&event->mutex); event->signaled = 0; pthread_mutex_unlock(&event->mutex);release_event(event); return 1;
    }
    if (!strcmp(name, "_ZN2nn2os9WaitEventEPNS0_9EventTypeE") || !strcmp(name, "_ZN2nn2os15WaitSystemEventEPNS0_15SystemEventTypeE")||
        !strcmp(name,"_ZN2nn2os14WaitLightEventEPNS0_14LightEventTypeE") || !strcmp(name,"nnosWaitEvent")) {
        wait_event(event_for(c->x[0], 0, 0, 0), 1, 0, 0); return 1;
    }
    if (!strcmp(name, "_ZN2nn2os12TryWaitEventEPNS0_9EventTypeE") || !strcmp(name, "_ZN2nn2os18TryWaitSystemEventEPNS0_15SystemEventTypeE")||
        !strcmp(name,"_ZN2nn2os17TryWaitLightEventEPNS0_14LightEventTypeE")) {
        c->x[0] = wait_event(event_for(c->x[0], 0, 0, 0), 0, 0, 0); return 1;
    }
    if(!strcmp(name,"_ZN2nn2os14TimedWaitEventEPNS0_9EventTypeENS_8TimeSpanE")||!strcmp(name,"_ZN2nn2os19TimedWaitLightEventEPNS0_14LightEventTypeENS_8TimeSpanE")||
       !strcmp(name,"_ZN2nn2os20TimedWaitSystemEventEPNS0_15SystemEventTypeENS_8TimeSpanE")) {
        c->x[0]=wait_event(event_for(c->x[0],0,0,0),1,1,c->x[1]);return 1;
    }
    if (!strcmp(name, "_ZN2nn2os11YieldThreadEv")) { sched_yield(); return 1; }
    if(!strcmp(name,"_ZN2nn2os11SleepThreadENS_8TimeSpanE")||!strcmp(name,"nnosSleepThread")) {
        int64_t nanoseconds=(int64_t)c->x[0];
        if(nanoseconds<=0){sched_yield();return 1;}
        struct timespec delay={(time_t)(nanoseconds/1000000000),(long)(nanoseconds%1000000000)};
        while(nanosleep(&delay,&delay)){if(errno!=EINTR)guest_trap("guest thread sleep failed",c->x[30]);}
        return 1;
    }
    if (!strcmp(name, "_ZN2nn2os16GetCurrentThreadEv") || !strcmp(name, "nnosGetCurrentThread")) { c->x[0] = current_thread->key; return 1; }
    if (!strcmp(name, "_ZN2nn2os25GetCurrentProcessorNumberEv") || !strcmp(name, "_ZN2nn2os20GetCurrentCoreNumberEv")) { c->x[0] = (uint64_t)current_thread->core; return 1; }
    if (!strcmp(name, "_ZN2nn2os26GetThreadAvailableCoreMaskEv")) { c->x[0] = 7; return 1; }
    if (!strcmp(name, "_ZN2nn2os17GetThreadPriorityEPKNS0_10ThreadTypeE") || !strcmp(name, "_ZN2nn2os24GetThreadCurrentPriorityEPKNS0_10ThreadTypeE") || !strcmp(name, "nnosGetThreadPriority")) { c->x[0] = (uint64_t)(int64_t)thread_for(c->x[0])->priority; return 1; }
    if (!strcmp(name, "_ZN2nn2os20ChangeThreadPriorityEPNS0_10ThreadTypeEi")) { HostThread *thread = thread_for(c->x[0]); int old = thread->priority; thread->priority = (int)c->x[1]; c->x[0] = (uint64_t)(int64_t)old; return 1; }
    if (!strcmp(name, "_ZN2nn2os11GetThreadIdEPKNS0_10ThreadTypeE")) { c->x[0] = thread_for(c->x[0])->id; return 1; }
    if (!strcmp(name, "_ZN2nn2os17GetThreadCoreMaskEPiPmPKNS0_10ThreadTypeE") || !strcmp(name, "nnosGetThreadCoreMask")) {
        HostThread *thread = thread_for(c->x[2]); if (c->x[0]) wr32(c->x[0], (uint32_t)thread->core); if (c->x[1]) wr64(c->x[1], thread->mask); return 1;
    }
    if (!strcmp(name, "_ZN2nn2os17SetThreadCoreMaskEPNS0_10ThreadTypeEim") || !strcmp(name, "nnosSetThreadCoreMask")) {
        HostThread *thread = thread_for(c->x[0]); int ideal = (int)c->x[1]; uint64_t mask = c->x[2];
        if (ideal == -2) { thread->core = 0; thread->mask = 1; }
        else if (ideal == -3) thread->mask = mask;
        else { thread->mask = mask; if (ideal >= 0) thread->core = ideal; else if (mask && !(mask & (1ULL << thread->core))) thread->core = __builtin_ctzll(mask); }
        return 1;
    }
    if (!strcmp(name, "_ZN2nn2os12CreateThreadEPNS0_10ThreadTypeEPFvPvES3_S3_mi") ||
        !strcmp(name, "_ZN2nn2os12CreateThreadEPNS0_10ThreadTypeEPFvPvES3_S3_mii") ||
        !strcmp(name, "nnosCreateThread")) {
        pthread_mutex_lock(&registry);
        if (thread_count == 256) { pthread_mutex_unlock(&registry); c->x[0] = 0x1015; return 1; }
        int has_core = !strcmp(name, "_ZN2nn2os12CreateThreadEPNS0_10ThreadTypeEPFvPvES3_S3_mii") || !strcmp(name, "nnosCreateThread");
        int core = has_core && (int)c->x[6] >= 0 && (int)c->x[6] < 64 ? (int)c->x[6] : 0;
        HostThread *thread = &threads[thread_count];
        *thread = (HostThread){.key=c->x[0],.entry=c->x[1],.arg=c->x[2],.stack=c->x[3],.stack_size=c->x[4],.priority=(int)c->x[5],.id=thread_count+1,.core=core,.mask=1ULL<<core};
        thread_count++; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os13SetThreadNameEPNS0_10ThreadTypeEPKc") || !strcmp(name, "nnosSetThreadNamePointer")) {
        HostThread *thread = thread_for(c->x[0]); size_t i;
        for (i=0;i+1<sizeof thread->name;i++) { thread->name[i] = (char)rd8(c->x[1]+i); if (!thread->name[i]) break; }
        thread->name[sizeof thread->name-1] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os11StartThreadEPNS0_10ThreadTypeE") || !strcmp(name, "nnosStartThread")) {
        HostThread *thread = thread_for(c->x[0]); if (thread->started) guest_trap("thread started twice", thread->key);
        pthread_attr_t attributes; pthread_attr_init(&attributes); pthread_attr_setstacksize(&attributes, 2097152);
        int rc = pthread_create(&thread->thread, &attributes, thread_entry, thread); pthread_attr_destroy(&attributes);
        if (rc) guest_trap("pthread_create failed", (uint64_t)rc); thread->started = 1; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os10WaitThreadEPNS0_10ThreadTypeE") || !strcmp(name, "nnosWaitThread")) { HostThread *thread = thread_for(c->x[0]); if (thread->started && pthread_join(thread->thread, NULL)) guest_trap("join failed", thread->key); return 1; }
    if (!strcmp(name, "pthread_join")) {
        pthread_t pt = 0; int found = 0;
        pthread_mutex_lock(&registry);
        for (unsigned i = 0; i < thread_count; i++) if (threads[i].key == c->x[0] && threads[i].started) { pt = threads[i].thread; found = 1; break; }
        pthread_mutex_unlock(&registry);
        if (found) pthread_join(pt, NULL);
        if (c->x[1]) wr64(c->x[1], 0);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os13DestroyThreadEPNS0_10ThreadTypeE") || !strcmp(name, "nnosDestroyThread")) { return 1; }
    if (!strcmp(name, "_ZN2nn2os19GenerateRandomBytesEPvm") || !strcmp(name, "_ZN2nn6crypto36GenerateCryptographicallyRandomBytesEPvm")) {
        static uint64_t rng = 0x243f6a8885a308d3ULL;
        uint64_t dst = c->x[0], n = c->x[1];
        for (uint64_t i = 0; i < n; i++) {
            uint64_t z = (rng += 0x9e3779b97f4a7c15ULL);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            wr8(dst + i, (uint8_t)((z ^ (z >> 31)) & 0xff));
        }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os19InitializeMultiWaitEPNS0_13MultiWaitTypeE") ||
        !strcmp(name, "_ZN2nn2os17FinalizeMultiWaitEPNS0_13MultiWaitTypeE")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2os25InitializeMultiWaitHolderEPNS0_19MultiWaitHolderTypeEPNS0_10ThreadTypeE") ||
        !strcmp(name, "_ZN2nn2os25InitializeMultiWaitHolderEPNS0_19MultiWaitHolderTypeEPNS0_9EventTypeE")) {
        pthread_mutex_lock(&registry);
        HostHolder *h = holder_for_locked(c->x[0], 1);
        if (h) { h->target = c->x[1]; h->is_thread = strstr(name, "ThreadType") != NULL; h->multiwait = 0; h->userdata = 0; }
        pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os23FinalizeMultiWaitHolderEPNS0_19MultiWaitHolderTypeE")) {
        pthread_mutex_lock(&registry); HostHolder *h = holder_for_locked(c->x[0], 0); if (h) h->used = 0; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os19LinkMultiWaitHolderEPNS0_13MultiWaitTypeEPNS0_19MultiWaitHolderTypeE")) {
        pthread_mutex_lock(&registry); HostHolder *h = holder_for_locked(c->x[1], 1); if (h) h->multiwait = c->x[0]; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os21UnlinkMultiWaitHolderEPNS0_19MultiWaitHolderTypeE")) {
        pthread_mutex_lock(&registry); HostHolder *h = holder_for_locked(c->x[0], 0); if (h) h->multiwait = 0; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os26SetMultiWaitHolderUserDataEPNS0_19MultiWaitHolderTypeEm")) {
        pthread_mutex_lock(&registry); HostHolder *h = holder_for_locked(c->x[0], 1); if (h) h->userdata = c->x[1]; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os26GetMultiWaitHolderUserDataEPKNS0_19MultiWaitHolderTypeE")) {
        pthread_mutex_lock(&registry); HostHolder *h = holder_for_locked(c->x[0], 0); uint64_t u = h ? h->userdata : 0; pthread_mutex_unlock(&registry); c->x[0] = u; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os10TryWaitAnyEPNS0_13MultiWaitTypeE") || !strcmp(name, "_ZN2nn2os7WaitAnyEPNS0_13MultiWaitTypeE")) {
        int block = !strcmp(name, "_ZN2nn2os7WaitAnyEPNS0_13MultiWaitTypeE");
        uint64_t mw = c->x[0], ready = 0;
        pthread_mutex_lock(&registry);
        for (;;) {
            ready = multiwait_check_locked(mw);
            if (ready || !block) break;
            struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 5000000; if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&multiwait_cv, &registry, &ts);
        }
        pthread_mutex_unlock(&registry); c->x[0] = ready; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os22InitializeMessageQueueEPNS0_16MessageQueueTypeEPmm")) {
        pthread_mutex_lock(&registry); mq_for_locked(c->x[0], 2, (size_t)c->x[2]); pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os20FinalizeMessageQueueEPNS0_16MessageQueueTypeE")) {
        pthread_mutex_lock(&registry); HostMq *q = mq_for_locked(c->x[0], 0, 0);
        if (q) { free(q->buf); pthread_cond_destroy(&q->not_empty); pthread_cond_destroy(&q->not_full); q->used = 0; }
        pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os16SendMessageQueueEPNS0_16MessageQueueTypeEm") ||
        !strcmp(name, "_ZN2nn2os19TrySendMessageQueueEPNS0_16MessageQueueTypeEm") ||
        !strcmp(name, "_ZN2nn2os15JamMessageQueueEPNS0_16MessageQueueTypeEm") ||
        !strcmp(name, "_ZN2nn2os18TryJamMessageQueueEPNS0_16MessageQueueTypeEm")) {
        int block = strstr(name, "Try") == NULL, jam = strstr(name, "Jam") != NULL;
        pthread_mutex_lock(&registry);
        HostMq *q = mq_for_locked(c->x[0], 1, 64);
        while (block && q->count >= q->cap) pthread_cond_wait(&q->not_full, &registry);
        int ok = q->count < q->cap;
        if (ok) {
            if (jam) { q->head = (q->head + q->cap - 1) % q->cap; q->buf[q->head] = c->x[1]; }
            else q->buf[(q->head + q->count) % q->cap] = c->x[1];
            q->count++; pthread_cond_signal(&q->not_empty);
        }
        pthread_mutex_unlock(&registry); c->x[0] = ok ? 1 : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os19ReceiveMessageQueueEPmPNS0_16MessageQueueTypeE") ||
        !strcmp(name, "_ZN2nn2os22TryReceiveMessageQueueEPmPNS0_16MessageQueueTypeE") ||
        !strcmp(name, "_ZN2nn2os24TimedReceiveMessageQueueEPmPNS0_16MessageQueueTypeENS_8TimeSpanE")) {
        int block = strstr(name, "Try") == NULL, timed = strstr(name, "Timed") != NULL;
        struct timespec ts;
        if (timed) {
            clock_gettime(CLOCK_REALTIME, &ts);
            uint64_t total = (uint64_t)ts.tv_nsec + c->x[2];
            ts.tv_sec += (time_t)(total / 1000000000ULL); ts.tv_nsec = (long)(total % 1000000000ULL);
        }
        pthread_mutex_lock(&registry);
        HostMq *q = mq_for_locked(c->x[1], 1, 64);
        while (block && !q->count) {
            if (timed) { if (pthread_cond_timedwait(&q->not_empty, &registry, &ts) == ETIMEDOUT) break; }
            else pthread_cond_wait(&q->not_empty, &registry);
        }
        int ok = q->count > 0;
        if (ok) {
            uint64_t val = q->buf[q->head];
            q->head = (q->head + 1) % q->cap; q->count--;
            if (c->x[0]) wr64(c->x[0], val);
            pthread_cond_signal(&q->not_full);
        }
        pthread_mutex_unlock(&registry); c->x[0] = ok ? 1 : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os17ConvertToTimeSpanENS0_4TickE") || !strcmp(name, "nnosConvertTickToNanoSeconds")) {
        uint64_t ticks = c->x[0], sec = ticks / 19200000ULL, rem = ticks % 19200000ULL;
        c->x[0] = sec * 1000000000ULL + (rem * 1000000000ULL) / 19200000ULL; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os13ConvertToTickENS_8TimeSpanE")) {
        uint64_t ns = c->x[0], sec = ns / 1000000000ULL, rem = ns % 1000000000ULL;
        c->x[0] = sec * 19200000ULL + (rem * 19200000ULL) / 1000000000ULL; return 1;
    }
    if (!strcmp(name, "_ZN2nn2os29IsVirtualAddressMemoryEnabledEv")) { c->x[0] = 1; return 1; }
    if (!strcmp(name, "_ZN2nn2os18SetMemoryAttributeEmmNS0_15MemoryAttributeE")) { return 1; }
    if (!strcmp(name, "_ZN2nn4util11ReferSymbolEPKv")) { return 1; }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader12GetAlignmentEv")) { c->x[0] = 1ULL << (rd8(c->x[0] + 0xe) & 63u); return 1; }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader16IsAlignmentValidEv")) {
        uint8_t sh = rd8(c->x[0] + 0xe) & 63u; c->x[0] = (c->x[0] & ((1ULL << sh) - 1ULL)) == 0; return 1;
    }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader16IsSignatureValidEl")) { c->x[0] = rd64(c->x[0]) == c->x[1]; return 1; }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader15IsEndianReverseEv")) { c->x[0] = rd16(c->x[0] + 0xc) == 0xfffe; return 1; }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader11GetFileSizeEv")) { c->x[0] = rd32(c->x[0] + 0x1c); return 1; }
    if (!strcmp(name, "_ZNK2nn4util16BinaryFileHeader11IsRelocatedEv")) { c->x[0] = rd16(c->x[0] + 0x14) & 1u; return 1; }
    if (!strcmp(name, "_ZN2nn4util16BinaryFileHeader13GetFirstBlockEv") || !strcmp(name, "_ZNK2nn4util16BinaryFileHeader13GetFirstBlockEv")) {
        uint32_t off = rd16(c->x[0] + 0x16); c->x[0] = off ? c->x[0] + off : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4util16BinaryFileHeader18GetRelocationTableEv")) {
        uint32_t off = rd32(c->x[0] + 0x18); c->x[0] = off ? c->x[0] + off : 0; return 1;
    }
    if (!strcmp(name, "_ZNK2nn4util17BinaryBlockHeader12GetBlockSizeEv")) { c->x[0] = rd32(c->x[0] + 0x8); return 1; }
    if (!strcmp(name, "_ZNK2nn4util17BinaryBlockHeader12GetNextBlockEv")) {
        uint32_t off = rd32(c->x[0] + 0x4); c->x[0] = off ? c->x[0] + off : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4util15RelocationTable8RelocateEv")) {
        uint64_t rlt = c->x[0], file = rlt - rd32(rlt + 4);
        uint32_t nsec = rd32(rlt + 8);
        uint64_t sections = rlt + 0x10, entries = sections + (uint64_t)nsec * 0x18;
        for (uint32_t s = 0; s < nsec; s++) {
            uint64_t sec = sections + (uint64_t)s * 0x18, base = rd64(sec);
            uint32_t first = rd32(sec + 0x10), count = rd32(sec + 0x14);
            if (!base) base = file;
            for (uint32_t e = first; e < first + count; e++) {
                uint64_t ent = entries + (uint64_t)e * 8, p = file + rd32(ent);
                uint32_t structs = rd16(ent + 4), ptrs = rd8(ent + 6), pad = rd8(ent + 7);
                for (uint32_t k = 0; k < structs; k++) {
                    for (uint32_t j = 0; j < ptrs; j++) {
                        uint64_t v = rd64(p);
                        if (v) wr64(p, v + base);
                        p += 8;
                    }
                    p += (uint64_t)pad * 8;
                }
            }
        }
        wr16(file + 0x14, rd16(file + 0x14) | 1u);
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4util8BitArray5resetEv")) {
        uint64_t words = rd64(c->x[0]); uint32_t bits = rd32(c->x[0] + 8);
        if (words) { uint64_t nw = ((uint64_t)bits + 63ULL) / 64ULL; for (uint64_t i = 0; i < nw; i++) wr64(words + i * 8, 0); }
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4util27ConvertCharacterUtf8ToUtf32EPjPKc")) {
        uint8_t s0 = rd8(c->x[1]); uint32_t cp; int n;
        if (s0 < 0x80) { cp = s0; n = 1; }
        else if ((s0 & 0xe0) == 0xc0) { cp = ((uint32_t)(s0 & 0x1f) << 6) | (rd8(c->x[1] + 1) & 0x3f); n = 2; }
        else if ((s0 & 0xf0) == 0xe0) { cp = ((uint32_t)(s0 & 0x0f) << 12) | ((uint32_t)(rd8(c->x[1] + 1) & 0x3f) << 6) | (rd8(c->x[1] + 2) & 0x3f); n = 3; }
        else if ((s0 & 0xf8) == 0xf0) { cp = ((uint32_t)(s0 & 0x07) << 18) | ((uint32_t)(rd8(c->x[1] + 1) & 0x3f) << 12) | ((uint32_t)(rd8(c->x[1] + 2) & 0x3f) << 6) | (rd8(c->x[1] + 3) & 0x3f); n = 4; }
        else { c->x[0] = 0; return 1; }
        wr32(c->x[0], cp); c->x[0] = (uint64_t)n; return 1;
    }
    if (!strcmp(name, "_ZN2nn4util30PickOutCharacterFromUtf8StringEPcPPKc")) {
        uint64_t s = rd64(c->x[1]); uint8_t s0 = rd8(s);
        int n = s0 < 0x80 ? 1 : (s0 & 0xe0) == 0xc0 ? 2 : (s0 & 0xf0) == 0xe0 ? 3 : (s0 & 0xf8) == 0xf0 ? 4 : 0;
        if (!n) { c->x[0] = 0; return 1; }
        for (int i = 0; i < n; i++) wr8(c->x[0] + (uint64_t)i, rd8(s + (uint64_t)i));
        wr64(c->x[1], s + (uint64_t)n); c->x[0] = (uint64_t)n; return 1;
    }
    if (!strcmp(name, "_ZN2nn4util12GenerateUuidEv")) {
        static uint64_t seq = 1; uint64_t a = 0x4000800000000000ULL | seq, b = 0x8000000000000000ULL | (seq * 0x9e3779b97f4a7c15ULL);
        seq++; c->x[0] = a; c->x[1] = b; return 1;
    }
    if (!strcmp(name, "_ZN2nn4util8SNPrintfEPcmPKcz") || !strcmp(name, "snprintf")) {
        FmtArgs a; fmt_args_direct(&a, c, 3); c->x[0] = (uint64_t)(int64_t)guest_format(c->x[0], (size_t)c->x[1], c->x[2], &a); return 1;
    }
    if (!strcmp(name, "_ZN2nn4util9VSNPrintfEPcmPKcSt9__va_list") || !strcmp(name, "vsnprintf")) {
        FmtArgs a; fmt_args_va(&a, c, c->x[3]); c->x[0] = (uint64_t)(int64_t)guest_format(c->x[0], (size_t)c->x[1], c->x[2], &a); return 1;
    }
    if (!strcmp(name, "sprintf")) {
        FmtArgs a; fmt_args_direct(&a, c, 2); c->x[0] = (uint64_t)(int64_t)guest_format(c->x[0], 1u << 20, c->x[1], &a); return 1;
    }
    if (!strcmp(name, "printf")) {
        FmtArgs a; fmt_args_direct(&a, c, 1); c->x[0] = (uint64_t)(int64_t)guest_format(0, 0, c->x[0], &a); return 1;
    }
    if (!strcmp(name, "fprintf")) {
        FmtArgs a; fmt_args_direct(&a, c, 2); c->x[0] = (uint64_t)(int64_t)guest_format(0, 0, c->x[1], &a); return 1;
    }
    if (!strcmp(name, "swprintf")) {
        uint64_t out = c->x[0], n = c->x[1], wfmt = c->x[2], fmt_buf = 0x7800002100ULL, tmp_buf = 0x7800002200ULL;
        pthread_mutex_lock(&registry);
        size_t k = 0;
        for (; k < 255; k++) {
            uint32_t ch = rd32(wfmt + k * 4);
            wr8(fmt_buf + k, (uint8_t)ch);
            if (!ch) break;
        }
        wr8(fmt_buf + k, 0);
        FmtArgs a; fmt_args_direct(&a, c, 3);
        int len = guest_format(tmp_buf, 1024, fmt_buf, &a);
        if (len < 0 || (uint64_t)len >= n || len >= 1024) {
            if (n && out) wr32(out, 0);
            pthread_mutex_unlock(&registry);
            c->x[0] = (uint64_t)-1LL; return 1;
        }
        for (int i = 0; i <= len; i++) wr32(out + (uint64_t)i * 4, rd8(tmp_buf + (uint64_t)i));
        pthread_mutex_unlock(&registry);
        c->x[0] = (uint64_t)(int64_t)len; return 1;
    }
    if (!strcmp(name, "gmtime") || !strcmp(name, "gmtime_r")) {
        uint64_t dst = !strcmp(name, "gmtime_r") ? c->x[1] : 0x7800002000ULL, utc_str = 0x7800002040ULL;
        int year, mon, mday, hour, min, sec, wday, yday;
        utc_split((int64_t)rd64(c->x[0]), &year, &mon, &mday, &hour, &min, &sec, &wday, &yday);
        wr32(dst + 0, (uint32_t)sec); wr32(dst + 4, (uint32_t)min); wr32(dst + 8, (uint32_t)hour);
        wr32(dst + 12, (uint32_t)mday); wr32(dst + 16, (uint32_t)(mon - 1)); wr32(dst + 20, (uint32_t)(year - 1900));
        wr32(dst + 24, (uint32_t)wday); wr32(dst + 28, (uint32_t)yday); wr32(dst + 32, 0); wr32(dst + 36, 0);
        wr64(dst + 40, 0); web_write(utc_str, "UTC", 4); wr64(dst + 48, utc_str);
        c->x[0] = dst; return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem19NumberLineAllocator10InitializeEPFPvmS2_ES2_PFvS2_S2_ES2_b")) {
        pthread_mutex_lock(&registry);
        HostNumberLine *n = numberline_for(c->x[0], 1);
        free(n->bits); free(n->run_len); n->bits = NULL; n->run_len = NULL; n->begin = n->end = 0;
        wr64(c->x[0], 1);
        for (uint64_t off = 8; off < 64; off += 8) wr64(c->x[0] + off, 0);
        pthread_mutex_unlock(&registry); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem19NumberLineAllocator8AddRangeEii")) {
        pthread_mutex_lock(&registry);
        HostNumberLine *n = numberline_for(c->x[0], 1);
        int32_t b = (int32_t)c->x[1], e = (int32_t)c->x[2];
        if (e > b) {
            if (!n->bits) {
                n->begin = b; n->end = e;
                n->bits = calloc((size_t)(e - b + 7) / 8, 1);
                n->run_len = calloc((size_t)(e - b), sizeof *n->run_len);
            } else {
                int32_t nb = b < n->begin ? b : n->begin, ne = e > n->end ? e : n->end;
                uint8_t *bits = calloc((size_t)(ne - nb + 7) / 8, 1);
                int32_t *runs = calloc((size_t)(ne - nb), sizeof *runs);
                for (int32_t i = n->begin; i < n->end; i++) {
                    if (n->bits[(i - n->begin) / 8] & (1u << ((i - n->begin) % 8)))
                        bits[(i - nb) / 8] |= (uint8_t)(1u << ((i - nb) % 8));
                    runs[i - nb] = n->run_len[i - n->begin];
                }
                free(n->bits); free(n->run_len);
                n->bits = bits; n->run_len = runs; n->begin = nb; n->end = ne;
            }
        }
        pthread_mutex_unlock(&registry); c->x[0] = 1; return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem19NumberLineAllocator8AllocateEPii")) {
        pthread_mutex_lock(&registry);
        HostNumberLine *n = numberline_for(c->x[0], 0);
        int32_t count = (int32_t)c->x[2];
        if (count <= 0) count = 1;
        uint64_t ok = 0;
        if (n->bits && count <= n->end - n->begin) {
            for (int32_t i = n->begin; i <= n->end - count; i++) {
                int32_t j = 0;
                for (; j < count; j++) {
                    int32_t idx = i + j - n->begin;
                    if (n->bits[idx / 8] & (1u << (idx % 8))) break;
                }
                if (j == count) {
                    for (j = 0; j < count; j++) {
                        int32_t idx = i + j - n->begin;
                        n->bits[idx / 8] |= (uint8_t)(1u << (idx % 8));
                    }
                    n->run_len[i - n->begin] = count;
                    if (c->x[1]) wr32(c->x[1], (uint32_t)i);
                    ok = 1;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&registry); c->x[0] = ok; return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem19NumberLineAllocator4FreeEi")) {
        pthread_mutex_lock(&registry);
        HostNumberLine *n = numberline_for(c->x[0], 0);
        int32_t i = (int32_t)c->x[1];
        if (n->bits && i >= n->begin && i < n->end) {
            int32_t len = n->run_len && n->run_len[i - n->begin] > 0 ? n->run_len[i - n->begin] : 1;
            n->run_len[i - n->begin] = 0;
            for (int32_t j = 0; j < len && i + j < n->end; j++) {
                int32_t idx = i + j - n->begin;
                n->bits[idx / 8] &= (uint8_t)~(1u << (idx % 8));
            }
        }
        pthread_mutex_unlock(&registry); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocatorC1Ev")) { wr8(c->x[0], 0); return 1; }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocator10InitializeEPvm")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 1);
        free(sa->blocks); sa->blocks = NULL; sa->cap = 0;
        uint64_t base = c->x[1], size = c->x[2] ? c->x[2] : 0x100000ULL;
        if (!base) {
            uint64_t mapped = (size + GM_PAGE_SIZE - 1) & ~(uint64_t)(GM_PAGE_SIZE - 1);
            base = next_stdalloc_region;
            next_stdalloc_region += mapped + 65536ULL;
            if (gm_reserve(elf_memory(), base, mapped) != GM_OK || gm_commit(elf_memory(), base, (size_t)mapped) != GM_OK)
                guest_trap("StandardAllocator backing reserve failed", base);
        }
        sa->base = base; sa->size = size; sa->free_size = size;
        wr8(c->x[0], 1);
        pthread_mutex_unlock(&registry); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocator8AllocateEm") || !strcmp(name, "_ZN2nn3mem17StandardAllocator8AllocateEmm")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 0);
        uint64_t align = !strcmp(name, "_ZN2nn3mem17StandardAllocator8AllocateEmm") ? c->x[2] : 16ULL;
        uint64_t ptr = stdalloc_allocate(sa, c->x[1], align);
        pthread_mutex_unlock(&registry); c->x[0] = ptr; return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocator4FreeEPv")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 0);
        uint64_t ptr = c->x[1];
        for (size_t i = 0; i < sa->cap; i++) {
            if (sa->blocks[i].used && sa->blocks[i].addr == ptr) {
                uint64_t consumed = (sa->blocks[i].size + 15ULL) & ~15ULL;
                sa->free_size += consumed;
                if (sa->free_size > sa->size) sa->free_size = sa->size;
                sa->blocks[i].used = 0;
                break;
            }
        }
        pthread_mutex_unlock(&registry); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocator10ReallocateEPvm")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 0);
        uint64_t old_ptr = c->x[1], new_size = c->x[2], old_size = 0;
        for (size_t i = 0; i < sa->cap; i++) if (sa->blocks[i].used && sa->blocks[i].addr == old_ptr) { old_size = sa->blocks[i].size; break; }
        uint64_t new_ptr = stdalloc_allocate(sa, new_size, 16ULL);
        if (new_ptr && old_ptr && old_size) {
            uint64_t copy_n = old_size < new_size ? old_size : new_size;
            for (uint64_t k = 0; k < copy_n; k++) wr8(new_ptr + k, rd8(old_ptr + k));
            for (size_t i = 0; i < sa->cap; i++) if (sa->blocks[i].used && sa->blocks[i].addr == old_ptr) {
                uint64_t consumed = (sa->blocks[i].size + 15ULL) & ~15ULL;
                sa->free_size += consumed;
                sa->blocks[i].used = 0;
                break;
            }
        }
        pthread_mutex_unlock(&registry); c->x[0] = new_ptr; return 1;
    }
    if (!strcmp(name, "_ZNK2nn3mem17StandardAllocator16GetTotalFreeSizeEv")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 0);
        uint64_t f = sa->free_size;
        pthread_mutex_unlock(&registry); c->x[0] = f; return 1;
    }
    if (!strcmp(name, "_ZN2nn3mem17StandardAllocator8FinalizeEv")) {
        pthread_mutex_lock(&registry);
        HostStdAlloc *sa = stdalloc_for(c->x[0], 0);
        free(sa->blocks); sa->blocks = NULL; sa->cap = 0; sa->used = 0;
        wr8(c->x[0], 0);
        pthread_mutex_unlock(&registry); return 1;
    }
    if (!strcmp(name, "_ZN2nn4lmem15CreateFrameHeapEPvmi")) {
        uint64_t start = c->x[0], size = c->x[1];
        if (!start || size < 32) { c->x[0] = 0; return 1; }
        uint64_t head = (start + 32ULL + 15ULL) & ~15ULL, tail = start + size;
        wr64(start + 0, start); wr64(start + 8, head); wr64(start + 16, tail); wr64(start + 24, tail);
        c->x[0] = start; return 1;
    }
    if (!strcmp(name, "_ZN2nn4lmem21AllocateFromFrameHeapEPNS0_6detail8HeapHeadEmi")) {
        uint64_t h = c->x[0], size = c->x[1]; int64_t align = (int64_t)(int32_t)c->x[2];
        if (!h) { c->x[0] = 0; return 1; }
        uint64_t head = rd64(h + 8), tail = rd64(h + 16);
        if (align < 0) {
            uint64_t a = (uint64_t)-align; if (!a) a = 4;
            uint64_t p = (tail - size) & ~(a - 1);
            if (p < head || p > tail) { c->x[0] = 0; return 1; }
            wr64(h + 16, p); c->x[0] = p; return 1;
        }
        uint64_t a = align ? (uint64_t)align : 4, p = (head + a - 1) & ~(a - 1);
        if (p + size > tail || p + size < p) { c->x[0] = 0; return 1; }
        wr64(h + 8, p + size); c->x[0] = p; return 1;
    }
    if (!strcmp(name, "_ZN2nn4lmem16DestroyFrameHeapEPNS0_6detail8HeapHeadE")) { return 1; }
    if (!strcmp(name, "_ZN2nn4time10InitializeEv") || !strcmp(name, "_ZN2nn4time8FinalizeEv")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn4time23StandardUserSystemClock14GetCurrentTimeEPNS0_9PosixTimeE") ||
        !strcmp(name, "_ZN2nn4time26StandardNetworkSystemClock14GetCurrentTimeEPNS0_9PosixTimeE")) {
        struct timespec now; clock_gettime(CLOCK_REALTIME, &now);
        if (c->x[0]) wr64(c->x[0], (uint64_t)now.tv_sec);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time19StandardSteadyClock3nowEv")) {
        struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
        c->x[0] = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time19ToCalendarTimeInUtcERKNS0_9PosixTimeE")) {
        int year, mon, mday, hour, min, sec, wday, yday;
        utc_split((int64_t)rd64(c->x[0]), &year, &mon, &mday, &hour, &min, &sec, &wday, &yday);
        c->x[0] = (uint64_t)(uint16_t)(int16_t)year | ((uint64_t)(uint8_t)mon << 16) | ((uint64_t)(uint8_t)mday << 24) |
                  ((uint64_t)(uint8_t)hour << 32) | ((uint64_t)(uint8_t)min << 40) | ((uint64_t)(uint8_t)sec << 48);
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4time18ToPosixTimeFromUtcERKNS0_12CalendarTimeE")) {
        int year = (int16_t)rd16(c->x[0]), mon = (int8_t)rd8(c->x[0] + 2), mday = (int8_t)rd8(c->x[0] + 3);
        int hour = (int8_t)rd8(c->x[0] + 4), min = (int8_t)rd8(c->x[0] + 5), sec = (int8_t)rd8(c->x[0] + 6);
        c->x[0] = (uint64_t)(days_from_civil(year, mon, mday) * 86400 + hour * 3600 + min * 60 + sec); return 1;
    }
    if (!strcmp(name, "_ZN2nn4time14ToCalendarTimeEPNS0_12CalendarTimeEPNS0_22CalendarAdditionalInfoERKNS0_9PosixTimeE") ||
        !strcmp(name, "_ZN2nn4time14ToCalendarTimeEPNS0_12CalendarTimeEPNS0_22CalendarAdditionalInfoERKNS0_9PosixTimeERKNS0_12TimeZoneRuleE")) {
        int year, mon, mday, hour, min, sec, wday, yday;
        utc_split((int64_t)rd64(c->x[2]), &year, &mon, &mday, &hour, &min, &sec, &wday, &yday);
        if (c->x[0]) {
            wr16(c->x[0], (uint16_t)(int16_t)year); wr8(c->x[0] + 2, (uint8_t)mon); wr8(c->x[0] + 3, (uint8_t)mday);
            wr8(c->x[0] + 4, (uint8_t)hour); wr8(c->x[0] + 5, (uint8_t)min); wr8(c->x[0] + 6, (uint8_t)sec); wr8(c->x[0] + 7, 0);
        }
        if (c->x[1]) {
            wr32(c->x[1], (uint32_t)wday); wr32(c->x[1] + 4, (uint32_t)yday);
            wr64(c->x[1] + 8, 0x435455ULL); wr32(c->x[1] + 16, 0); wr32(c->x[1] + 20, 0);
        }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time11ToPosixTimeEPiPNS0_9PosixTimeEiRKNS0_12CalendarTimeE")) {
        int year = (int16_t)rd16(c->x[3]), mon = (int8_t)rd8(c->x[3] + 2), mday = (int8_t)rd8(c->x[3] + 3);
        int hour = (int8_t)rd8(c->x[3] + 4), min = (int8_t)rd8(c->x[3] + 5), sec = (int8_t)rd8(c->x[3] + 6);
        int64_t t = days_from_civil(year, mon, mday) * 86400 + hour * 3600 + min * 60 + sec;
        int n = (int32_t)c->x[2] >= 1 ? 1 : 0;
        if (n && c->x[1]) wr64(c->x[1], (uint64_t)t);
        if (c->x[0]) wr32(c->x[0], (uint32_t)n);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time11IsValidDateEiii")) {
        static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int y = (int32_t)c->x[0], m = (int32_t)c->x[1], d = (int32_t)c->x[2];
        int dim = (m >= 1 && m <= 12) ? (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) ? 29 : mdays[m - 1]) : 0;
        c->x[0] = y >= 1 && m >= 1 && m <= 12 && d >= 1 && d <= dim; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time12GetDayOfWeekEiii")) {
        static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        int y = (int32_t)c->x[0], m = (int32_t)c->x[1], d = (int32_t)c->x[2];
        if (m < 1 || m > 12) { c->x[0] = 0; return 1; }
        if (m < 3) y -= 1;
        c->x[0] = (uint64_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7); return 1;
    }
    if (!strcmp(name, "_ZN2nn4timegtERKNS0_12CalendarTimeES3_")) {
        int ka[6] = {(int16_t)rd16(c->x[0]), (int8_t)rd8(c->x[0] + 2), (int8_t)rd8(c->x[0] + 3), (int8_t)rd8(c->x[0] + 4), (int8_t)rd8(c->x[0] + 5), (int8_t)rd8(c->x[0] + 6)};
        int kb[6] = {(int16_t)rd16(c->x[1]), (int8_t)rd8(c->x[1] + 2), (int8_t)rd8(c->x[1] + 3), (int8_t)rd8(c->x[1] + 4), (int8_t)rd8(c->x[1] + 5), (int8_t)rd8(c->x[1] + 6)};
        for (int i = 0; i < 6; i++) if (ka[i] != kb[i]) { c->x[0] = ka[i] > kb[i]; return 1; }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time13ClockSnapshot29CreateWithStandardSystemClockEPS1_")) {
        uint64_t s = c->x[0];
        struct timespec rt, mt; clock_gettime(CLOCK_REALTIME, &rt); clock_gettime(CLOCK_MONOTONIC, &mt);
        int64_t now = (int64_t)rt.tv_sec, tp = (int64_t)mt.tv_sec;
        int year, mon, mday, hour, min, sec, wday, yday;
        utc_split(now, &year, &mon, &mday, &hour, &min, &sec, &wday, &yday);
        for (uint64_t off = 0; off < 0xd0; off += 8) wr64(s + off, 0);
        for (int k = 0; k < 2; k++) {
            uint64_t ctx = s + (uint64_t)k * 0x20, cal = s + 0x50 + (uint64_t)k * 8, info = s + 0x60 + (uint64_t)k * 0x18;
            wr64(ctx, (uint64_t)(now - tp)); wr64(ctx + 0x10, 0x4000800000000001ULL); wr64(ctx + 0x18, 0x8000000000000001ULL);
            wr64(s + 0x40 + (uint64_t)k * 8, (uint64_t)now);
            wr16(cal, (uint16_t)(int16_t)year); wr8(cal + 2, (uint8_t)mon); wr8(cal + 3, (uint8_t)mday);
            wr8(cal + 4, (uint8_t)hour); wr8(cal + 5, (uint8_t)min); wr8(cal + 6, (uint8_t)sec);
            wr32(info, (uint32_t)wday); wr32(info + 4, (uint32_t)yday); wr64(info + 8, 0x435455ULL);
        }
        wr64(s + 0x90, (uint64_t)tp); wr64(s + 0x98, 0x4000800000000001ULL); wr64(s + 0xa0, 0x8000000000000001ULL);
        wr64(s + 0xa8, 0x435455ULL); wr8(s + 0xcc, 1); wr8(s + 0xcd, 1);
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4time20CalculateSpanBetweenEPNS_8TimeSpanERKNS0_13ClockSnapshotES5_")) {
        int64_t sec = (int64_t)rd64(c->x[2] + 0x90) - (int64_t)rd64(c->x[1] + 0x90);
        if (c->x[0]) wr64(c->x[0], (uint64_t)(sec * 1000000000LL));
        c->x[0] = 1; return 1;
    }
    if (!strcmp(name, "nnResultIsFailure")) { c->x[0] = (uint32_t)c->x[0] != 0; return 1; }
    if (!strcmp(name, "nnResultIsSuccess")) { c->x[0] = (uint32_t)c->x[0] == 0; return 1; }
    if (!strcmp(name, "nnResultGetModule")) { c->x[0] = (uint32_t)c->x[0] & 0x1ffu; return 1; }
    if (!strcmp(name, "nnResultGetDescription")) { c->x[0] = ((uint32_t)c->x[0] >> 9) & 0x1fffu; return 1; }
    if (!strcmp(name, "_ZN2nn3mii6detail15BufferAllocatorC1EPvm")) { wr64(c->x[0], c->x[1]); wr64(c->x[0] + 8, c->x[2]); return 1; }
    if (!strcmp(name, "_ZN2nn3mii6detail15BufferAllocatorD1Ev") || !strcmp(name, "_ZN2nn3mii6detail25CalculateSizeAndAlignmentD1Ev")) { return 1; }
    if (!strcmp(name, "_ZN2nn3mii6detail15BufferAllocator7AlignUpEm")) {
        uint64_t p = rd64(c->x[0]), a = c->x[1] ? c->x[1] : 1, q = (p + a - 1) / a * a;
        wr64(c->x[0] + 8, rd64(c->x[0] + 8) - (q - p)); wr64(c->x[0], q); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mii6detail15BufferAllocator7AdvanceEm")) {
        wr64(c->x[0], rd64(c->x[0]) + c->x[1]); wr64(c->x[0] + 8, rd64(c->x[0] + 8) - c->x[1]); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mii6detail25CalculateSizeAndAlignmentC1Ev")) { wr64(c->x[0], 0); wr64(c->x[0] + 8, 1); return 1; }
    if (!strcmp(name, "_ZN2nn3mii6detail25CalculateSizeAndAlignment7AddSizeEm")) { wr64(c->x[0], rd64(c->x[0]) + c->x[1]); return 1; }
    if (!strcmp(name, "_ZN2nn3mii6detail25CalculateSizeAndAlignment20AddSizeWithAlignmentEmm")) {
        uint64_t a = c->x[2] ? c->x[2] : 1, s = (rd64(c->x[0]) + a - 1) / a * a;
        wr64(c->x[0], s + c->x[1]); if (a > rd64(c->x[0] + 8)) wr64(c->x[0] + 8, a); return 1;
    }
    if (!strcmp(name, "_ZN2nn3mii6detail25CalculateSizeAndAlignment20AddSizeWithAlignmentEmmi")) {
        uint64_t a = c->x[2] ? c->x[2] : 1, s = (rd64(c->x[0]) + a - 1) / a * a;
        wr64(c->x[0], s + c->x[1] * (uint64_t)(int64_t)(int32_t)c->x[3]);
        if (a > rd64(c->x[0] + 8)) wr64(c->x[0] + 8, a); return 1;
    }
    if (!strcmp(name, "_ZNK2nn3mii6detail25CalculateSizeAndAlignment7GetSizeEv")) { c->x[0] = rd64(c->x[0]); return 1; }
    if (!strcmp(name, "_ZN2nn4bcat10InitializeEv")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn4bcat25MountDeliveryCacheStorageEv")) {
        pthread_mutex_lock(&registry);
        uint32_t r = bcat_mounted ? 0x87au : 0u; bcat_mounted = 1;
        pthread_mutex_unlock(&registry); c->x[0] = r; return 1;
    }
    if (!strcmp(name, "_ZN2nn4bcat27UnmountDeliveryCacheStorageEv")) {
        pthread_mutex_lock(&registry); bcat_mounted = 0; pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4bcat31EnumerateDeliveryCacheDirectoryEPiPNS0_13DirectoryNameEi")) {
        if (c->x[0]) wr32(c->x[0], 0); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4bcat24RequestSyncDeliveryCacheEPNS0_21DeliveryCacheProgressE")) { c->x[0] = 0x107au; return 1; }
    if (!strcmp(name, "_ZN2nn4bcat24RequestSyncDeliveryCacheEPNS0_21DeliveryCacheProgressEPKc")) {
        c->x[0] = bcat_dirname_valid_guest(c->x[1]) ? 0x107au : 0x27au; return 1;
    }
    if (!strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectoryC1Ev") || !strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectoryD1Ev") ||
        !strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectory5CloseEv") || !strcmp(name, "_ZN2nn4bcat17DeliveryCacheFileC1Ev") ||
        !strcmp(name, "_ZN2nn4bcat17DeliveryCacheFileD1Ev") || !strcmp(name, "_ZN2nn4bcat17DeliveryCacheFile5CloseEv") ||
        !strcmp(name, "_ZN2nn4bcat21DeliveryCacheProgressC1Ev") || !strcmp(name, "_ZN2nn4bcat21DeliveryCacheProgressD1Ev") ||
        !strcmp(name, "_ZN2nn4bcat21DeliveryCacheProgress6UpdateEv")) { return 1; }
    if (!strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectory4OpenERKNS0_13DirectoryNameE") ||
        !strcmp(name, "_ZN2nn4bcat17DeliveryCacheFile4OpenERKNS0_13DirectoryNameERKNS0_8FileNameE")) { c->x[0] = 0x47au; return 1; }
    if (!strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectory4ReadEPiPNS0_27DeliveryCacheDirectoryEntryEi")) {
        if (c->x[1]) wr32(c->x[1], 0); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn3aoc17CountAddOnContentEv") || !strcmp(name, "_ZN2nn3aoc16ListAddOnContentEPiii") ||
        !strcmp(name, "_ZN2nn3pdm30QueryApplicationPlayStatisticsEPNS0_25ApplicationPlayStatisticsEPKNS_13ApplicationIdEi")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn4pctl32CheckFreeCommunicationPermissionEb") || !strcmp(name, "_ZN2nn4pctl27CheckStereoVisionPermissionEb")) { c->x[0] = 1; return 1; }
    if (!strcmp(name, "_ZN2nn3ngc15ProfanityFilterC1Ev") || !strcmp(name, "_ZN2nn3ngc15ProfanityFilterD1Ev") ||
        !strcmp(name, "_ZN2nn3ngc15ProfanityFilter11SetMaskModeENS1_8MaskModeE") ||
        !strcmp(name, "_ZN2nn3ngc15ProfanityFilter15SkipAtSignCheckENS1_8SkipModeE")) { return 1; }
    if (!strcmp(name, "_ZN2nn3ngc15ProfanityFilter10InitializeEPvm")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn3ngc15ProfanityFilter24MaskProfanityWordsInTextEPiPtj")) { if (c->x[1]) wr32(c->x[1], 0); c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2fs15GetSaveDataSizeEPlS1_RKNS_7account3UidE")) {
        if (c->x[0]) wr64(c->x[0], 0x6000000ULL);
        if (c->x[1]) wr64(c->x[1], 0xa00000ULL);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2oe24GetExpectedVolumeBalanceEPfS1_") || !strcmp(name, "_ZN2nn2oe26GetExpectedVolumeBalanceEPfS0_")) {
        uint32_t vm, vl; memcpy(&vm, &volume_main, 4); memcpy(&vl, &volume_library, 4);
        if (c->x[0]) wr32(c->x[0], vm); if (c->x[1]) wr32(c->x[1], vl);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2oe24SetExpectedVolumeBalanceEff") || !strcmp(name, "_ZN2nn2oe26SetExpectedVolumeBalanceEff")) {
        volume_main = c->v[0].f[0]; volume_library = c->v[1].f[0]; c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2oe21SwitchLcdBacklightOffEv") || !strcmp(name, "_ZN2nn2oe20SwitchLcdBacklightOnEv") ||
        !strcmp(name, "_ZN2nn2oe27SetTvPowerStateMatchingModeENS0_24TvPowerStateMatchingModeE") ||
        !strcmp(name, "_ZN2nn2oe16EnableScreenShotEv") || !strcmp(name, "_ZN2nn2oe17DisableScreenShotEv") ||
        !strcmp(name, "_ZN2nn2oe15SetCpuBoostModeENS0_12CpuBoostModeE")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn2oe25GetProgramTotalActiveTimeEv")) {
        struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
        c->x[0] = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec; return 1;
    }
    if (!strcmp(name, "_ZNK2nn3err18ErrorResultVariant8GetStateEv")) { c->x[0] = rd32(c->x[0]); return 1; }
    if (!strcmp(name, "_ZNK2nn3err18ErrorResultVariantcvNS_6ResultEEv")) { c->x[0] = rd32(c->x[0] + 4); return 1; }
    if (!strcmp(name, "_ZNK2nn3err18ErrorResultVariantcvNS0_9ErrorCodeEEv")) { c->x[0] = rd64(c->x[0] + 4); return 1; }
    if (!strcmp(name, "_ZN2nn5audio27InitializeAudioOutParameterEPNS0_17AudioOutParameterE")) {
        if (c->x[0]) { wr32(c->x[0], 48000); wr16(c->x[0] + 4, 2); wr16(c->x[0] + 6, 0); }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio19OpenDefaultAudioOutEPNS0_8AudioOutEPNS_2os11SystemEventERKNS0_17AudioOutParameterE")) {
        uint32_t rate = c->x[2] ? rd32(c->x[2]) : 0;
        uint16_t ch = c->x[2] ? rd16(c->x[2] + 4) : 0;
        pthread_mutex_lock(&registry);
        audio_state.rate = rate ? (int)rate : 48000;
        audio_state.channels = ch ? (int)ch : 2;
        audio_state.event_key = c->x[1];
        audio_state.q_head = audio_state.q_count = audio_state.r_head = audio_state.r_count = 0;
        pthread_mutex_unlock(&registry);
        if (c->x[1]) release_event(event_for(c->x[1], 1, 0, 1));
        if (c->x[0]) {
            for (uint64_t off = 0; off < 0x38; off += 8) wr64(c->x[0] + off, 0);
            wr32(c->x[0], (uint32_t)audio_state.rate); wr32(c->x[0] + 4, (uint32_t)audio_state.channels);
            wr32(c->x[0] + 8, 2); wr32(c->x[0] + 12, 1);
        }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio39AcquireAudioDeviceNotificationForOutputEPNS_2os11SystemEventE")) {
        if (c->x[0]) release_event(event_for(c->x[0], 1, 1, 1));
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio24GetActiveAudioDeviceNameEPNS0_15AudioDeviceNameE")) {
        static const char devname[] = "AudioTvOutput";
        if (c->x[0]) {
            for (unsigned i = 0; i < 0x100; i += 8) wr64(c->x[0] + i, 0);
            web_write(c->x[0], devname, sizeof devname);
        }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio26SetAudioDeviceOutputVolumeEPNS0_15AudioDeviceNameEf") ||
        !strcmp(name, "_ZN2nn5audio26SetAudioDeviceOutputVolumeEPKNS0_15AudioDeviceNameEf")) { c->x[0] = 0; return 1; }
    if (!strcmp(name, "_ZN2nn5audio23GetAudioOutChannelCountEPKNS0_8AudioOutE")) { c->x[0] = (uint64_t)(audio_state.channels ? audio_state.channels : 2); return 1; }
    if (!strcmp(name, "_ZN2nn5audio21GetAudioOutSampleRateEPKNS0_8AudioOutE")) { c->x[0] = (uint64_t)(audio_state.rate ? audio_state.rate : 48000); return 1; }
    if (!strcmp(name, "_ZN2nn5audio23GetAudioOutSampleFormatEPKNS0_8AudioOutE")) { c->x[0] = 2; return 1; }
    if (!strcmp(name, "_ZN2nn5audio21SetAudioOutBufferInfoEPNS0_14AudioOutBufferEPvmm")) {
        if (c->x[0]) { wr64(c->x[0], 0); wr64(c->x[0] + 8, c->x[1]); wr64(c->x[0] + 16, c->x[2]); wr64(c->x[0] + 24, c->x[3]); wr64(c->x[0] + 32, 0); }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio28GetAudioOutBufferDataPointerEPKNS0_14AudioOutBufferE")) {
        c->x[0] = c->x[0] ? rd64(c->x[0] + 8) : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio13StartAudioOutEPNS0_8AudioOutE")) {
        pthread_mutex_lock(&registry);
        audio_state.started = 1; audio_state.clock_ns = monotonic_ns();
        if (c->x[0]) wr32(c->x[0] + 12, 0);
        pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio12StopAudioOutEPNS0_8AudioOutE") || !strcmp(name, "_ZN2nn5audio13CloseAudioOutEPNS0_8AudioOutE")) {
        pthread_mutex_lock(&registry);
        audio_state.started = 0; audio_pump_locked(UINT64_MAX);
        if (c->x[0]) wr32(c->x[0] + 12, 1);
        pthread_mutex_unlock(&registry); c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio20AppendAudioOutBufferEPNS0_8AudioOutEPNS0_14AudioOutBufferE")) {
        uint64_t buf = c->x[1], data_sz = buf ? rd64(buf + 24) : 0;
        pthread_mutex_lock(&registry);
        int ok = audio_state.q_count < 64 && buf != 0;
        if (ok) {
            uint64_t bps = 2ULL * (uint64_t)(audio_state.channels > 0 ? audio_state.channels : 2);
            uint64_t rate = (uint64_t)(audio_state.rate > 0 ? audio_state.rate : 48000);
            uint64_t frames = bps ? data_sz / bps : 0;
            uint64_t dur_ns = rate ? (frames * 1000000000ULL) / rate : 0;
            if (!dur_ns) dur_ns = 5000000ULL;
            uint64_t now = monotonic_ns();
            if (audio_state.clock_ns < now) audio_state.clock_ns = now;
            audio_state.clock_ns += dur_ns;
            unsigned slot = (audio_state.q_head + audio_state.q_count) % 64;
            audio_state.queued[slot].buf = buf;
            audio_state.queued[slot].due_ns = audio_state.clock_ns;
            audio_state.q_count++;
        }
        pthread_mutex_unlock(&registry); c->x[0] = ok ? 1 : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn5audio25GetReleasedAudioOutBufferEPNS0_8AudioOutE")) {
        pthread_mutex_lock(&registry);
        audio_pump_locked(monotonic_ns());
        uint64_t out = 0;
        if (audio_state.r_count > 0) {
            out = audio_state.released[audio_state.r_head];
            audio_state.r_head = (audio_state.r_head + 1) % 64;
            audio_state.r_count--;
        }
        pthread_mutex_unlock(&registry); c->x[0] = out; return 1;
    }
    if (!strcmp(name, "_ZN2nn5codec19HardwareOpusDecoderC1Ei")) {
        for (unsigned i = 0; i < 10; i++) wr64(OPUS_VTABLE + i * 8, OPUS_FN_BASE + i * 16);
        wr64(c->x[0], OPUS_VTABLE); wr32(c->x[0] + 8, 48000); wr32(c->x[0] + 12, 2); wr8(c->x[0] + 16, 0);
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4util14DecompressZlibEPvmPKvmS1_m")) {
        uint64_t dst_addr = c->x[0], dst_sz = c->x[1], src_addr = c->x[2], src_sz = c->x[3];
        if (!dst_addr || !src_addr || src_sz < 6 || dst_sz > (256ULL << 20) || src_sz > (256ULL << 20)) { c->x[0] = 0; return 1; }
        uint8_t *src = malloc((size_t)src_sz), *dst = malloc(dst_sz ? (size_t)dst_sz : 1);
        int ok = 0;
        if (src && dst) {
            web_read(src_addr, src, (size_t)src_sz);
            ok = zlib_decompress(dst, (size_t)dst_sz, src, (size_t)src_sz);
            if (ok && dst_sz) web_write(dst_addr, dst, (size_t)dst_sz);
        }
        free(src); free(dst);
        c->x[0] = ok ? 1 : 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn2pl8IsVrModeEv") || !strcmp(name, "_ZN2nn2pl11BeginVrModeEv") || !strcmp(name, "_ZN2nn2pl9EndVrModeEv")) {
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn6crypto16GenerateSha1HashEPvmPKvm")) {
        uint64_t out = c->x[0], out_sz = c->x[1], data = c->x[2], sz = c->x[3];
        uint8_t digest[20] = {0};
        uint32_t h0 = 0x67452301u, h1 = 0xefcdab89u, h2 = 0x98badcfeu, h3 = 0x10325476u, h4 = 0xc3d2e1f0u;
        for (uint64_t i = 0; i < sz; i++) {
            uint8_t b = rd8(data + i);
            h0 = ((h0 << 5) | (h0 >> 27)) ^ b; h1 += h0; h2 ^= h1; h3 += h2; h4 ^= h3;
        }
        uint32_t hs[5] = {h0, h1, h2, h3, h4};
        for (int i = 0; i < 5; i++) {
            digest[i*4] = (uint8_t)(hs[i] >> 24); digest[i*4+1] = (uint8_t)(hs[i] >> 16);
            digest[i*4+2] = (uint8_t)(hs[i] >> 8); digest[i*4+3] = (uint8_t)hs[i];
        }
        if (out && out_sz) web_write(out, digest, out_sz < 20 ? (size_t)out_sz : 20);
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn6crypto6detail7Md5Impl", 28)) {
        if (!strcmp(name, "_ZN2nn6crypto6detail7Md5Impl7GetHashEPvm") && c->x[1] && c->x[2]) {
            uint8_t zero[16] = {0};
            web_write(c->x[1], zero, c->x[2] < 16 ? (size_t)c->x[2] : 16);
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn3mii", 10) || !strncmp(name, "_ZNK2nn3mii", 11)) {
        if (!strcmp(name, "_ZN2nn3mii21ParamCharInfoAccessorC1ERKNS0_8CharInfoE") ||
            !strcmp(name, "_ZN2nn3mii16CharInfoAccessorC1ERKNS0_8CharInfoE")) {
            for (uint64_t off = 0; off < 0x58; off += 8) wr64(c->x[0] + off, rd64(c->x[1] + off));
            return 1;
        }
        if (!strcmp(name, "_ZNK2nn3mii21ParamCharInfoAccessor7IsValidEv") ||
            !strcmp(name, "_ZNK2nn3mii16CharInfoAccessor7IsValidEv")) { c->x[0] = 1; return 1; }
        if (!strcmp(name, "_ZNK2nn3mii16CharInfoAccessor11GetNicknameEPNS0_8NicknameEi")) {
            if (c->x[1]) { wr16(c->x[1], 'M'); wr16(c->x[1] + 2, 'i'); wr16(c->x[1] + 4, 'i'); wr16(c->x[1] + 6, 0); }
            return 1;
        }
        if (!strcmp(name, "_ZNK2nn3mii8Database8GetCountEPiNS0_10SourceFlagE")) {
            if (c->x[1]) wr32(c->x[1], (c->x[2] & 2) ? 6u : 0u);
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "_ZN2nn3mii9AppendMiiEPi")) { c->x[0] = 0xe7e; return 1; }
        if (strstr(name, "Color")) {
            c->v[0].f[0] = 0.8f; c->v[1].f[0] = 0.6f; c->v[2].f[0] = 0.5f; c->v[3].f[0] = 1.0f;
            return 1;
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN5movie", 9) || !strncmp(name, "_ZNK5movie", 10)) {
        if (!strcmp(name, "_ZN5movie9ExtractorC1ENS_13ContainerTypeE") ||
            !strcmp(name, "_ZN5movie7DecoderC1ENS_11DecoderTypeENS_19DecoderOutputFormatENS_11DecoderModeEi")) {
            for (unsigned i = 0; i < 64; i++) wr64(MOVIE_VTABLE + i * 8, MOVIE_FN_BASE + i * 16);
            wr64(c->x[0], MOVIE_VTABLE); wr64(c->x[0] + 8, 0);
            return 1;
        }
        if (!strcmp(name, "_ZN5movie6BufferC1Ev")) { for (uint64_t o = 0; o < 0x40; o += 8) wr64(c->x[0] + o, 0); return 1; }
        if (!strcmp(name, "_ZN5movie6BufferC1Em")) { for (uint64_t o = 0; o < 0x40; o += 8) wr64(c->x[0] + o, 0); wr64(c->x[0] + 0x10, c->x[1]); return 1; }
        if (!strcmp(name, "_ZN5movie6BufferC1EPvm")) {
            for (uint64_t o = 0; o < 0x40; o += 8) wr64(c->x[0] + o, 0);
            wr64(c->x[0] + 0x08, c->x[1]); wr64(c->x[0] + 0x10, c->x[2]); wr64(c->x[0] + 0x18, c->x[2]); return 1;
        }
        if (!strcmp(name, "_ZN5movie6BufferC1ERKS0_") || !strcmp(name, "_ZN5movie6BufferaSERKS0_")) {
            for (uint64_t o = 0; o < 0x40; o += 8) wr64(c->x[0] + o, rd64(c->x[1] + o));
            return 1;
        }
        if (!strcmp(name, "_ZNK5movie6Buffer4BaseEv")) { c->x[0] = rd64(c->x[0] + 0x08); return 1; }
        if (!strcmp(name, "_ZNK5movie6Buffer4SizeEv")) { c->x[0] = rd64(c->x[0] + 0x18); return 1; }
        if (!strcmp(name, "_ZNK5movie6Buffer6OffsetEv")) { c->x[0] = rd64(c->x[0] + 0x20); return 1; }
        if (!strcmp(name, "_ZNK5movie6Buffer8CapacityEv")) { c->x[0] = rd64(c->x[0] + 0x10); return 1; }
        if (!strcmp(name, "_ZN5movie6Buffer8SetRangeEii")) { wr64(c->x[0] + 0x20, (uint32_t)c->x[1]); wr64(c->x[0] + 0x18, (uint32_t)c->x[2]); return 1; }
        if (!strcmp(name, "_ZN5movie6Buffer18SetDataAndCapacityEPvm")) { wr64(c->x[0] + 0x08, c->x[1]); wr64(c->x[0] + 0x10, c->x[2]); return 1; }
        if (!strcmp(name, "_ZNK5movie9MediaData9FindInt32EPKcPi") ||
            !strcmp(name, "_ZNK5movie9MediaData9FindInt64EPKcPl") ||
            !strcmp(name, "_ZNK5movie9MediaData10FindStringEPKcPS2_")) { c->x[0] = 0; return 1; }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4time13ClockSnapshotC1Ev")) {
        for (uint64_t off = 0; off < 0xd0; off += 8) wr64(c->x[0] + off, 0);
        return 1;
    }
    if (!strcmp(name, "_ZN2nn4time16LoadTimeZoneRuleEPNS0_12TimeZoneRuleERKNS0_12LocationNameE")) {
        if (c->x[0]) for (uint64_t off = 0; off < 0x4000; off += 8) wr64(c->x[0] + off, 0);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn8settings12LanguageCode4MakeENS0_8LanguageE")) {
        static const char *codes[] = {"ja","en-US","fr","de","it","es","zh-CN","ko","nl","pt","ru","zh-TW","en-GB","fr-CA","es-419","zh-Hans","zh-Hant","pt-BR"};
        uint32_t lang = (uint32_t)c->x[0];
        const char *s = lang < sizeof codes / sizeof codes[0] ? codes[lang] : "en-US";
        uint64_t out = 0; memcpy(&out, s, strlen(s));
        c->x[0] = out; return 1;
    }
    if (!strcmp(name, "_ZN2nn2ro12UnloadModuleEPNS0_6ModuleE") || !strcmp(name, "_ZN2nn6socket8FinalizeEv")) {
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn3aoc17CountAddOnContentEv") || !strcmp(name, "_ZN2nn3aoc16ListAddOnContentEPiii") ||
        !strcmp(name, "_ZN2nn3pdm30QueryApplicationPlayStatisticsEPNS0_25ApplicationPlayStatisticsEPKNS_13ApplicationIdEi")) {
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn4pctl32CheckFreeCommunicationPermissionEb") ||
        !strcmp(name, "_ZN2nn4pctl27CheckStereoVisionPermissionEb")) {
        c->x[0] = 1; return 1;
    }
    if (!strcmp(name, "nnsocketInetHtons") || !strcmp(name, "nnsocketInetNtohs") ||
        !strcmp(name, "_ZN2nn6socket9InetHtonsEt") || !strcmp(name, "_ZN2nn6socket9InetNtohsEt")) {
        uint16_t v = (uint16_t)c->x[0]; c->x[0] = (uint16_t)((v << 8) | (v >> 8)); return 1;
    }
    if (!strcmp(name, "_ZN2nn6socket9InetHtonlEj") || !strcmp(name, "_ZN2nn6socket9InetNtohlEj")) {
        c->x[0] = __builtin_bswap32((uint32_t)c->x[0]); return 1;
    }
    if (!strncmp(name, "_ZN2nn4bcat", 11) || !strncmp(name, "_ZNK2nn4bcat", 12)) {
        if (!strcmp(name, "_ZN2nn4bcat31EnumerateDeliveryCacheDirectoryEPiPNS0_13DirectoryNameEi") ||
            !strcmp(name, "_ZN2nn4bcat22DeliveryCacheDirectory4ReadEPiPNS0_27DeliveryCacheDirectoryEntryEi")) {
            if (c->x[0]) wr32(c->x[0], 0);
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn4nifm", 11)) {
        c->x[0] = strstr(name, "GetResult") ? 0x11c2ULL : 0; return 1;
    }
    if (!strncmp(name, "nnssl", 5) || !strncmp(name, "_ZN2nn3ssl", 10)) {
        if (!strcmp(name, "nnsslContextGetContextId") || !strcmp(name, "nnsslConnectionGetConnectionId")) {
            if (c->x[0]) wr64(c->x[0], 1);
        } else if (!strcmp(name, "_ZN2nn3ssl7Context15ImportServerPkiEPmPKcjNS0_17CertificateFormatE")) {
            if (c->x[1]) wr64(c->x[1], 1);
        } else if (!strcmp(name, "nnsslConnectionGetVerifyCertError")) {
            if (c->x[0]) wr32(c->x[0], 0);
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "nnsocket", 8) || !strncmp(name, "_ZN2nn6socket", 13)) {
        if (!strcmp(name, "_ZN2nn6socket10InitializeEPvmmi") || !strcmp(name, "_ZN2nn6socket8FinalizeEv") ||
            !strcmp(name, "nnsocketSetLastErrno") || !strcmp(name, "nnsocketResolverSetOption") ||
            !strcmp(name, "nnsocketResolverGetOption") || !strcmp(name, "_ZN2nn6socket12FreeAddrInfoEP8addrinfo") ||
            !strcmp(name, "_ZN2nn6socket12FreeAddrInfoEPNS0_8AddrInfoE") ||
            !strcmp(name, "nnsocketGetHostByNameWithOptions")) {
            c->x[0] = 0; return 1;
        }
        if (!strcmp(name, "nnsocketGetLastErrno") || !strcmp(name, "_ZN2nn6socket12GetLastErrnoEv")) {
            c->x[0] = 6; return 1;
        }
        c->x[0] = (uint64_t)-1LL; return 1;
    }
    if (!strncmp(name, "_ZN2nn3ngc", 10)) {
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn3nsd", 10)) {
        if (c->x[0] && c->x[1]) {
            for (uint64_t off = 0; off < 0x100; off += 8) wr64(c->x[0] + off, rd64(c->x[1] + off));
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZNK2nn7account", 15)) {
        if (c->x[1]) wr64(c->x[1], 0);
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn7account12AsyncContext14GetSystemEventEPNS_2os11SystemEventE") ||
        !strcmp(name, "_ZN2nn7friends12AsyncContext14GetSystemEventEPNS_2os11SystemEventE")) {
        if (c->x[1]) {
            release_event(event_for(c->x[1], 1, 1, 1));
            wr8(c->x[1] + 40ULL, 1);
        }
        c->x[0] = 0; return 1;
    }
    if (!strcmp(name, "_ZN2nn7friends17NotificationQueue14GetSystemEventEv")) {
        uint64_t ev = c->x[0] ? c->x[0] : NOTIFICATION;
        release_event(event_for(ev, 1, 0, 0));
        c->x[0] = ev;
        return 1;
    }
    if (!strcmp(name, "_ZN2nn7friends17NotificationQueue3PopEPNS0_16NotificationInfoE")) {
        c->x[0] = 1; return 1;
    }
    if (!strcmp(name, "_ZN2nn3nfp29AttachAvailabilityChangeEventEPNS_2os15SystemEventTypeE") ||
        !strcmp(name, "_ZN2nn3nfp19AttachActivateEventEPNS_2os15SystemEventTypeERKNS0_12DeviceHandleE") ||
        !strcmp(name, "_ZN2nn3nfp21AttachDeactivateEventEPNS_2os15SystemEventTypeERKNS0_12DeviceHandleE") ||
        !strcmp(name, "_ZN2nn3ldn22AttachStateChangeEventEPNS_2os15SystemEventTypeE")) {
        if (c->x[0]) release_event(event_for(c->x[0], 1, 0, 1));
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn3ldn", 10) || !strncmp(name, "_ZN2nn7friends", 14) ||
        !strncmp(name, "_ZNK2nn7friends", 15) || !strncmp(name, "_ZN2nn3nfp", 10) ||
        !strncmp(name, "_ZN2nn2ec", 9) || !strncmp(name, "_ZN2nn3web", 10) || !strncmp(name, "_ZNK2nn3web", 11) ||
        !strncmp(name, "_ZN2nn5swkbd", 12) || !strncmp(name, "_ZN2nn5album", 12) || !strncmp(name, "_ZNK2nn5album", 13) ||
        !strncmp(name, "_ZN2nn5prepo", 12) || !strncmp(name, "_ZNK2nn5prepo", 13)) {
        if (!strcmp(name, "_ZN2nn7friends14GetFriendCountEPiRKNS_7account3UidERKNS0_12FriendFilterE") ||
            !strcmp(name, "_ZN2nn7friends13GetFriendListEPiPNS0_6FriendERKNS_7account3UidEiiRKNS0_12FriendFilterE") ||
            !strcmp(name, "_ZN2nn7friends14GetFriendListEPiPNS0_6FriendERKNS_7account3UidEiiRKNS0_12FriendFilterE") ||
            !strcmp(name, "_ZN2nn7friends18GetBlockedUserListEPiPNS_7account23NetworkServiceAccountIdERKNS2_3UidEii") ||
            !strcmp(name, "_ZN2nn7friends19GetBlockedUserListEPiPNS_7accountNetworkServiceAccountIdERKNS1_3UidEii") ||
            !strcmp(name, "_ZN2nn3nfp11ListDevicesEPiPNS0_12DeviceHandleEi") ||
            !strcmp(name, "_ZN2nn5album16GetImageFileListEPiPNS0_14AlbumFileEntryEmRKNS0_8DateTimeES6_") ||
            !strcmp(name, "_ZN2nn5album21GetExtraMovieFileListEPiPNS0_14AlbumFileEntryEi")) {
            if (c->x[0]) wr32(c->x[0], 0);
        }
        if (!strcmp(name, "_ZN2nn3web18WebPageReturnValueC1Ev") ||
            !strcmp(name, "_ZN2nn3web21ShowWebPageReturnValueC1Ev") ||
            !strcmp(name, "_ZN2nn3web20LobbyPageReturnValueC1Ev") ||
            !strcmp(name, "_ZN2nn3web26OfflineHtmlPageReturnValueC1Ev") ||
            !strcmp(name, "_ZN2nn3web22ShowOfflineHtmlPageArgC1EPKc") ||
            !strcmp(name, "_ZN2nn3web25ShowOfflineHtmlPageArgC1EPKc") ||
            !strcmp(name, "_ZN2nn3web14ShowWebPageArgC1EPKc") ||
            !strcmp(name, "_ZN2nn3web15ShowWebPageArgC1EPKc")) {
            if (c->x[0]) wr64(c->x[0], 0);
        }
        c->x[0] = 0; return 1;
    }
    if (!strncmp(name, "_ZN2nn5image", 12) || !strncmp(name, "_ZNK2nn5image", 13)) {
        if (strstr(name, "GetWorkBufferSize")) { c->x[0] = 64; return 1; }
        c->x[0] = 0; return 1;
    }
    return 0;
}
