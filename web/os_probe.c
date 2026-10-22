/* Self-authored event tests through the actual OS import adapter. */
#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include <emscripten/emscripten.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int web_os_import(Ctx *,const char *);
int web_os_indirect(Ctx *,uint64_t);
int web_fs_import(Ctx *,const char *);
int web_mutex_import(Ctx *,const char *);
int web_unsigned_sort_import(Ctx *,const char *);
void web_os_reset(void);
uintptr_t web_input_pointer(void);
static GuestMemory memory;
GuestMemory *elf_memory(void){return &memory;}
static _Thread_local jmp_buf *boundary;
static pthread_mutex_t error_lock=PTHREAD_MUTEX_INITIALIZER;
static char error[160],report[256];
static atomic_uint failures,entered,done;
static unsigned checks;
static pthread_t driver;
static const uint64_t EVENT=0x7100001000ULL;
void guest_trap(const char *what,uint64_t address) {
    pthread_mutex_lock(&error_lock);snprintf(error,sizeof error,"%s (%llx)",what,(unsigned long long)address);pthread_mutex_unlock(&error_lock);
    if(boundary)longjmp(*boundary,1);abort();
}
int web_runtime_thread_call(Ctx *c,uint64_t address){(void)c;guest_trap("guest thread execution is outside event fixture",address);return 0;}
static void check_at(int value,int line){checks++;if(!value){atomic_fetch_add(&failures,1);pthread_mutex_lock(&error_lock);size_t n=strlen(error);if(n+16<sizeof error)snprintf(error+n,sizeof error-n,"%sL%d",n?",":"",line);pthread_mutex_unlock(&error_lock);}}
#define check(v) check_at((v),__LINE__)
static uint64_t invoke(const char *name,uint64_t key,uint64_t a,uint64_t b) {
    Ctx c={0};c.x[0]=key;c.x[1]=a;c.x[2]=b;
    if(!web_os_import(&c,name)&&!web_fs_import(&c,name)&&!web_mutex_import(&c,name))guest_trap("import not handled",0);
    return c.x[0];
}
#define INITIALIZE "_ZN2nn2os20InitializeLightEventEPNS0_14LightEventTypeEbNS0_14EventClearModeE"
#define FINALIZE "_ZN2nn2os18FinalizeLightEventEPNS0_14LightEventTypeE"
#define SIGNAL "_ZN2nn2os16SignalLightEventEPNS0_14LightEventTypeE"
#define CLEAR "_ZN2nn2os15ClearLightEventEPNS0_14LightEventTypeE"
#define WAIT "_ZN2nn2os14WaitLightEventEPNS0_14LightEventTypeE"
#define TRY "_ZN2nn2os17TryWaitLightEventEPNS0_14LightEventTypeE"
#define TIMED "_ZN2nn2os19TimedWaitLightEventEPNS0_14LightEventTypeENS_8TimeSpanE"
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec+(double)t.tv_nsec/1e9;}
static int reach(atomic_uint *value,unsigned target,double timeout) {
    double deadline=now()+timeout;struct timespec pause={0,1000000};
    while(atomic_load(value)<target&&now()<deadline)nanosleep(&pause,NULL);
    return atomic_load(value)>=target;
}
static void *waiter(void *argument) {
    (void)argument;jmp_buf stopped;boundary=&stopped;
    if(!setjmp(stopped)) {
        atomic_fetch_add(&entered,1);
        if(invoke(TIMED,EVENT,1000000000,0)!=1)atomic_fetch_add(&failures,1);
    } else atomic_fetch_add(&failures,1);
    atomic_fetch_add(&done,1);return NULL;
}
__attribute__((noinline)) static void publish(void){MAIN_THREAD_ASYNC_EM_ASM({Module['onOsProbeReport'](UTF8ToString($0));},report);}
__attribute__((noinline)) static void inject_input(int mode){MAIN_THREAD_EM_ASM({Module['onInputFixture']($0,$1);},web_input_pointer(),mode);}
static void *run(void *unused) {
    (void)unused;jmp_buf stopped;boundary=&stopped;gm_init(&memory);web_os_reset();
    if(!setjmp(stopped)) {
        const char *sleep="_ZN2nn2os11SleepThreadENS_8TimeSpanE";
        double started=now();invoke(sleep,2000000,0,0);check(now()-started>=0.0019);
        started=now();invoke("nnosSleepThread",10000000,0,0);check(now()-started>=0.0099);
        invoke(sleep,0,0,0);invoke(sleep,UINT64_MAX,0,0);invoke(sleep,UINT64_MAX-1,0,0);check(1);
        invoke(INITIALIZE,EVENT,0,0);check(!invoke(TRY,EVENT,0,0));
        invoke(SIGNAL,EVENT,0,0);check(invoke(TRY,EVENT,0,0)==1&&invoke(TRY,EVENT,0,0)==1);
        invoke(CLEAR,EVENT,0,0);check(!invoke(TRY,EVENT,0,0));
        check(!invoke(TIMED,EVENT,0,0));check(!invoke(TIMED,EVENT,2000000,0));check(!invoke(TIMED,EVENT,UINT64_MAX,0));
        invoke(FINALIZE,EVENT,0,0);invoke(INITIALIZE,EVENT,1,1);
        invoke(WAIT,EVENT,0,0);check(!invoke(TRY,EVENT,0,0));invoke(SIGNAL,EVENT,0,0);
        check(invoke(TRY,EVENT,0,0)==1&&!invoke(TRY,EVENT,0,0));invoke(FINALIZE,EVENT,0,0);
        for(unsigned automatic=0;automatic<2;automatic++) {
            invoke(INITIALIZE,EVENT,0,automatic);atomic_store(&entered,0);atomic_store(&done,0);
            pthread_t a,b;check(!pthread_create(&a,NULL,waiter,NULL));check(!pthread_create(&b,NULL,waiter,NULL));
            check(reach(&entered,2,2));invoke(SIGNAL,EVENT,0,0);
            if(automatic){check(reach(&done,1,0.5));check(atomic_load(&done)==1);invoke(SIGNAL,EVENT,0,0);}
            check(!pthread_join(a,NULL));check(!pthread_join(b,NULL));check(atomic_load(&done)==2);
            check(invoke(TRY,EVENT,0,0)==(automatic?0:1));invoke(FINALIZE,EVENT,0,0);
        }
        jmp_buf expected;boundary=&expected;
        if(!setjmp(expected)){invoke(TRY,EVENT,0,0);check(0);}else{check(1);error[0]=0;}
        boundary=&stopped;
        // More lifetimes than the registry has slots must remain reusable.
        for(unsigned i=0;i<600;i++){invoke(INITIALIZE,EVENT+i*16,0,i&1);invoke(FINALIZE,EVENT+i*16,0,0);}check(1);
        check(gm_reserve(&memory,0x7800010000ULL,65536)==GM_OK);
        check(gm_commit(&memory,0x7800010000ULL,65536)==GM_OK);
        web_memory_bind(&memory);
        check(!invoke("_ZN2nn4nifm10InitializeEv",0,0,0));
        check(!invoke("_ZN2nn4nifm18IsNetworkAvailableEv",0,0,0));
        check(!invoke("_ZN2nn7account10InitializeEv",0,0,0));
        check(!invoke("_ZN2nn7account12ListAllUsersEPiPNS0_3UidEi",0x7800010000ULL,0x7800010010ULL,4));
        check(rd32(0x7800010000ULL)==1&&rd64(0x7800010010ULL)==1&&rd64(0x7800010018ULL)==0);
        check(invoke("_ZN2nn7account22TryOpenPreselectedUserEPNS0_10UserHandleE",0x7800010020ULL,0,0)==1);
        check(!invoke("_ZN2nn7account9GetUserIdEPNS0_3UidERKNS0_10UserHandleE",0x7800010040ULL,0x7800010020ULL,0));
        check(rd64(0x7800010040ULL)==1&&rd64(0x7800010048ULL)==0);
        check(!invoke("_ZN2nn7account11GetNicknameEPNS0_8NicknameERKNS0_3UidE",0x7800010060ULL,0x7800010040ULL,0));
        check(rd8(0x7800010060ULL)=='P'&&rd8(0x7800010065ULL)=='r'&&rd8(0x7800010066ULL)==0);
        invoke("_ZN2nn2oe42SetOperationModeChangedNotificationEnabledEb",1,0,0);
        check(!invoke("_ZN2nn5prepo10InitializeEv",0,0,0));
        check(!invoke("_ZN2nn2ro10InitializeEv",0,0,0));
        check(!invoke("_ZN2nn3hid14InitializeNpadEv",0,0,0));
        check(!invoke("_ZN2nn3hid24SetSupportedNpadStyleSetENS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE",0x3f,0,0));
        wr32(0x7800010080ULL,0);wr32(0x7800010084ULL,0x20);
        check(!invoke("_ZN2nn3hid22SetSupportedNpadIdTypeEPKjm",0x7800010080ULL,2,0));
        check(invoke("_ZN2nn3hid15GetNpadStyleSetERKj",0x7800010080ULL,0,0)==1);
        check(!invoke("_ZN2nn3hid15GetNpadStyleSetERKj",0x7800010084ULL,0,0));
        check(invoke("_ZN2nn3hid19GetPlayerLedPatternERKj",0x7800010080ULL,0,0)==1);
        {
            Ctx c={0};c.x[0]=0x7800010090ULL;c.x[1]=2;c.x[2]=0x7800010080ULL;c.x[3]=1;
            check(web_os_import(&c,"_ZN2nn3hid25GetVibrationDeviceHandlesEPNS0_21VibrationDeviceHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE")&&c.x[0]==2);
            check(rd32(0x7800010090ULL)==1&&rd32(0x7800010094ULL)==2);
            c=(Ctx){0};c.x[0]=0x78000100a0ULL;c.x[1]=8;c.x[2]=0x7800010080ULL;c.x[3]=1;
            check(web_os_import(&c,"_ZN2nn3hid23GetSixAxisSensorHandlesEPNS0_19SixAxisSensorHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE")&&c.x[0]==1);
            check(rd32(0x78000100a0ULL)==1);
        }
        check(!invoke("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj",0x7800010100ULL,0x7800010080ULL,0));
        check(rd64(0x7800010100ULL)==1&&rd32(0x7800010120ULL)==3);
        inject_input(1);
        check(!invoke("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj",0x7800010100ULL,0x7800010080ULL,0));
        check(rd64(0x7800010108ULL)==0x80501&&rd64(0x7800010100ULL)==2);
        check((int32_t)rd32(0x7800010110ULL)==-32767&&(int32_t)rd32(0x7800010114ULL)==32767);
        check((int32_t)rd32(0x7800010118ULL)==-12345&&(int32_t)rd32(0x780001011cULL)==23456);
        check(!invoke("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj",0x7800010100ULL,0x7800010084ULL,0));
        check(!rd64(0x7800010108ULL)&&!rd64(0x7800010110ULL)&&!rd64(0x7800010118ULL)&&!rd32(0x7800010120ULL));
        inject_input(2); // Torn/in-progress input cannot leak partial values or stall the guest.
        check(!invoke("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj",0x7800010100ULL,0x7800010080ULL,0));
        check(!rd64(0x7800010108ULL)&&!rd64(0x7800010110ULL)&&!rd64(0x7800010118ULL));
        inject_input(0);
        check(!invoke("_ZN2nn3hid12GetNpadStateEPNS0_11NpadGcStateERKj",0x7800010100ULL,0x7800010080ULL,0));
        check(!rd64(0x7800010108ULL)&&!rd64(0x7800010110ULL)&&!rd64(0x7800010118ULL)&&!rd64(0x7800010128ULL));
        check(!invoke("_ZN2nn2vi10InitializeEv",0,0,0));
        check(!invoke("_ZN2nn2vi18OpenDefaultDisplayEPPNS0_7DisplayE",0x7800010140ULL,0,0)&&rd64(0x7800010140ULL)==0x7800000900ULL);
        check(!invoke("_ZN2nn2vi11CreateLayerEPPNS0_5LayerEPNS0_7DisplayE",0x7800010148ULL,0x7800000900ULL,0)&&rd64(0x7800010148ULL)==0x7800000910ULL);
        check(!invoke("_ZN2nn2vi15GetNativeWindowEPPvPNS0_5LayerE",0x7800010150ULL,0x7800000910ULL,0)&&rd64(0x7800010150ULL)==0x7800000920ULL);
        check(!invoke("_ZN2nn2vi20GetDisplayVsyncEventEPNS_2os15SystemEventTypeEPNS0_7DisplayE",0x7800010160ULL,0x7800000900ULL,0));
        check(invoke("_ZN2nn2os18TryWaitSystemEventEPNS0_15SystemEventTypeE",0x7800010160ULL,0,0)==1);
        check(!invoke("_ZN2nn2os18TryWaitSystemEventEPNS0_15SystemEventTypeE",0x7800010160ULL,0,0));
        started=now();invoke("_ZN2nn2os15WaitSystemEventEPNS0_15SystemEventTypeE",0x7800010160ULL,0,0);check(now()-started>=0.015);
        invoke("_ZN2nn2os18DestroySystemEventEPNS0_15SystemEventTypeE",0x7800010160ULL,0,0);
        wr32(0x7800010200ULL,0x3052524eu);
        check(!invoke("_ZN2nn2ro18RegisterModuleInfoEPNS0_16RegistrationInfoEPKv",0x7800010220ULL,0x7800010200ULL,0));
        check(rd32(0x7800010220ULL)==1&&rd64(0x7800010228ULL)==0x7800010200ULL);
        check(!invoke("_ZN2nn2ro20UnregisterModuleInfoEPNS0_16RegistrationInfoE",0x7800010220ULL,0,0)&&rd32(0x7800010220ULL)==0);
        wr32(0x7800010200ULL,0);
        check(invoke("_ZN2nn2ro18RegisterModuleInfoEPNS0_16RegistrationInfoEPKv",0x7800010220ULL,0x7800010200ULL,0)==0xc16);
        wr32(0x7800010310ULL,0x304f524eu);wr32(0x7800010338ULL,0x4000u);
        check(!invoke("_ZN2nn2ro13GetBufferSizeEPmPKv",0x7800010350ULL,0x7800010300ULL,0)&&rd64(0x7800010350ULL)==0x4000ULL);
        check(!invoke("_ZN2nn2oe17FinishStartupLogoEv",0,0,0));
        check(!invoke("_ZN2nn2oe25TryPopNotificationMessageEPj",0x7800010360ULL,0,0));
        check(invoke("_ZN2nn4util11ReferSymbolEPKv",0x7800010360ULL,0,0)==0x7800010360ULL);
        invoke("_ZN2nn3mem19NumberLineAllocator10InitializeEPFPvmS2_ES2_PFvS2_S2_ES2_b",0x7800010400ULL,0,0);
        check(rd64(0x7800010400ULL)==1);
        check(invoke("_ZN2nn3mem19NumberLineAllocator8AddRangeEii",0x7800010400ULL,0x100,0x400)==1);
        check(invoke("_ZN2nn3mem19NumberLineAllocator8AllocateEPii",0x7800010400ULL,0x7800010440ULL,16)==1&&rd32(0x7800010440ULL)==0x100);
        check(invoke("_ZN2nn3mem19NumberLineAllocator8AllocateEPii",0x7800010400ULL,0x7800010444ULL,32)==1&&rd32(0x7800010444ULL)==0x110);
        invoke("_ZN2nn3mem19NumberLineAllocator4FreeEi",0x7800010400ULL,0x100,0);
        check(invoke("_ZN2nn3mem19NumberLineAllocator8AllocateEPii",0x7800010400ULL,0x7800010448ULL,16)==1&&rd32(0x7800010448ULL)==0x100);
        invoke("_ZN2nn3mem17StandardAllocator10InitializeEPvm",0x7800010450ULL,0x7800012000ULL,0x4000ULL);
        check(rd8(0x7800010450ULL)==1&&invoke("_ZNK2nn3mem17StandardAllocator16GetTotalFreeSizeEv",0x7800010450ULL,0,0)==0x4000ULL);
        uint64_t sa_p1=invoke("_ZN2nn3mem17StandardAllocator8AllocateEmm",0x7800010450ULL,0x100ULL,64ULL);
        check(sa_p1==0x7800012000ULL);
        invoke("_ZN2nn3mem17StandardAllocator4FreeEPv",0x7800010450ULL,sa_p1,0);
        invoke("_ZN2nn3mem17StandardAllocator8FinalizeEv",0x7800010450ULL,0,0);
        check(rd8(0x7800010450ULL)==0);
        check(invoke("_ZN2nn4lmem15CreateFrameHeapEPvmi",0x7800011000ULL,0x1000,0)==0x7800011000ULL);
        check(invoke("_ZN2nn4lmem21AllocateFromFrameHeapEPNS0_6detail8HeapHeadEmi",0x7800011000ULL,0x80,64)==0x7800011040ULL);
        wr64(0x7800010500ULL,0x7800010520ULL);wr32(0x7800010508ULL,128);
        wr64(0x7800010520ULL,~0ULL);wr64(0x7800010528ULL,~0ULL);
        invoke("_ZN2nn4util8BitArray5resetEv",0x7800010500ULL,0,0);
        check(rd64(0x7800010520ULL)==0&&rd64(0x7800010528ULL)==0);
        {
            static const char fmt[]="%s-%d-%04x",arg[]="ssbu";
            web_write(0x7800010600ULL,fmt,sizeof fmt);
            web_write(0x7800010620ULL,arg,sizeof arg);
            Ctx c={0};c.x[0]=0x7800010640ULL;c.x[1]=32;c.x[2]=0x7800010600ULL;c.x[3]=0x7800010620ULL;c.x[4]=42;c.x[5]=0xbeef;
            check(web_os_import(&c,"_ZN2nn4util8SNPrintfEPcmPKcz")&&c.x[0]==12);
            check(rd8(0x7800010640ULL)=='s'&&rd8(0x7800010645ULL)=='4'&&rd8(0x7800010648ULL)=='b'&&rd8(0x780001064cULL)==0);
        }
        check(invoke("_ZN2nn2os17ConvertToTimeSpanENS0_4TickE",19200000ULL*3,0,0)==3000000000ULL);
        check(invoke("_ZN2nn2os13ConvertToTickENS_8TimeSpanE",2500000000ULL,0,0)==48000000ULL);
        wr64(0x7800010700ULL,1700000000ULL);
        uint64_t cal=invoke("_ZN2nn4time19ToCalendarTimeInUtcERKNS0_9PosixTimeE",0x7800010700ULL,0,0);
        check((uint16_t)cal==2023&&(uint8_t)(cal>>16)==11&&(uint8_t)(cal>>24)==14);
        wr64(0x7800010710ULL,cal);
        check(invoke("_ZN2nn4time18ToPosixTimeFromUtcERKNS0_12CalendarTimeE",0x7800010710ULL,0,0)==1700000000ULL);
        // MultiWait + Event holder
        invoke("_ZN2nn2os19InitializeMultiWaitEPNS0_13MultiWaitTypeE",0x7800010800ULL,0,0);
        invoke("_ZN2nn2os15InitializeEventEPNS0_9EventTypeEbNS0_14EventClearModeE",0x7800010840ULL,0,1);
        invoke("_ZN2nn2os25InitializeMultiWaitHolderEPNS0_19MultiWaitHolderTypeEPNS0_9EventTypeE",0x7800010880ULL,0x7800010840ULL,0);
        invoke("_ZN2nn2os26SetMultiWaitHolderUserDataEPNS0_19MultiWaitHolderTypeEm",0x7800010880ULL,0x12345678ULL,0);
        check(invoke("_ZN2nn2os26GetMultiWaitHolderUserDataEPKNS0_19MultiWaitHolderTypeE",0x7800010880ULL,0,0)==0x12345678ULL);
        invoke("_ZN2nn2os19LinkMultiWaitHolderEPNS0_13MultiWaitTypeEPNS0_19MultiWaitHolderTypeE",0x7800010800ULL,0x7800010880ULL,0);
        check(invoke("_ZN2nn2os10TryWaitAnyEPNS0_13MultiWaitTypeE",0x7800010800ULL,0,0)==0);
        invoke("_ZN2nn2os11SignalEventEPNS0_9EventTypeE",0x7800010840ULL,0,0);
        check(invoke("_ZN2nn2os10TryWaitAnyEPNS0_13MultiWaitTypeE",0x7800010800ULL,0,0)==0x7800010880ULL);
        invoke("_ZN2nn2os21UnlinkMultiWaitHolderEPNS0_19MultiWaitHolderTypeE",0x7800010880ULL,0,0);
        invoke("_ZN2nn2os23FinalizeMultiWaitHolderEPNS0_19MultiWaitHolderTypeE",0x7800010880ULL,0,0);
        invoke("_ZN2nn2os13FinalizeEventEPNS0_9EventTypeE",0x7800010840ULL,0,0);
        invoke("_ZN2nn2os17FinalizeMultiWaitEPNS0_13MultiWaitTypeE",0x7800010800ULL,0,0);
        // MessageQueue
        invoke("_ZN2nn2os22InitializeMessageQueueEPNS0_16MessageQueueTypeEPmm",0x7800010900ULL,0x7800010940ULL,4);
        check(invoke("_ZN2nn2os19TrySendMessageQueueEPNS0_16MessageQueueTypeEm",0x7800010900ULL,0xaaULL,0)==1);
        check(invoke("_ZN2nn2os19TrySendMessageQueueEPNS0_16MessageQueueTypeEm",0x7800010900ULL,0xbbULL,0)==1);
        check(invoke("_ZN2nn2os18TryJamMessageQueueEPNS0_16MessageQueueTypeEm",0x7800010900ULL,0x99ULL,0)==1);
        check(invoke("_ZN2nn2os22TryReceiveMessageQueueEPmPNS0_16MessageQueueTypeE",0x7800010980ULL,0x7800010900ULL,0)==1&&rd64(0x7800010980ULL)==0x99ULL);
        check(invoke("_ZN2nn2os22TryReceiveMessageQueueEPmPNS0_16MessageQueueTypeE",0x7800010980ULL,0x7800010900ULL,0)==1&&rd64(0x7800010980ULL)==0xaaULL);
        check(invoke("_ZN2nn2os22TryReceiveMessageQueueEPmPNS0_16MessageQueueTypeE",0x7800010980ULL,0x7800010900ULL,0)==1&&rd64(0x7800010980ULL)==0xbbULL);
        check(invoke("_ZN2nn2os22TryReceiveMessageQueueEPmPNS0_16MessageQueueTypeE",0x7800010980ULL,0x7800010900ULL,0)==0);
        invoke("_ZN2nn2os20FinalizeMessageQueueEPNS0_16MessageQueueTypeE",0x7800010900ULL,0,0);
        // nn::audio
        invoke("_ZN2nn5audio27InitializeAudioOutParameterEPNS0_17AudioOutParameterE",0x7800010a00ULL,0,0);
        check(rd32(0x7800010a00ULL)==48000&&rd16(0x7800010a04ULL)==2);
        check(!invoke("_ZN2nn5audio19OpenDefaultAudioOutEPNS0_8AudioOutEPNS_2os11SystemEventERKNS0_17AudioOutParameterE",0x7800010a20ULL,0x7800010a60ULL,0x7800010a00ULL));
        check(invoke("_ZN2nn5audio23GetAudioOutChannelCountEPKNS0_8AudioOutE",0x7800010a20ULL,0,0)==2);
        check(invoke("_ZN2nn5audio21GetAudioOutSampleRateEPKNS0_8AudioOutE",0x7800010a20ULL,0,0)==48000);
        {
            Ctx c={0};c.x[0]=0x7800010a80ULL;c.x[1]=0x7800012000ULL;c.x[2]=1920;c.x[3]=1920;
            check(web_os_import(&c,"_ZN2nn5audio21SetAudioOutBufferInfoEPNS0_14AudioOutBufferEPvmm"));
            check(invoke("_ZN2nn5audio28GetAudioOutBufferDataPointerEPKNS0_14AudioOutBufferE",0x7800010a80ULL,0,0)==0x7800012000ULL);
        }
        check(!invoke("_ZN2nn5audio13StartAudioOutEPNS0_8AudioOutE",0x7800010a20ULL,0,0));
        check(invoke("_ZN2nn5audio20AppendAudioOutBufferEPNS0_8AudioOutEPNS0_14AudioOutBufferE",0x7800010a20ULL,0x7800010a80ULL,0)==1);
        check(!invoke("_ZN2nn5audio12StopAudioOutEPNS0_8AudioOutE",0x7800010a20ULL,0,0));
        check(invoke("_ZN2nn5audio25GetReleasedAudioOutBufferEPNS0_8AudioOutE",0x7800010a20ULL,0,0)==0x7800010a80ULL);
        check(!invoke("_ZN2nn5audio13CloseAudioOutEPNS0_8AudioOutE",0x7800010a20ULL,0,0));
        invoke("_ZN2nn2os18DestroySystemEventEPNS0_15SystemEventTypeE",0x7800010a60ULL,0,0);
        // SaveData + writable files
        {
            static const char sv[]="save",fdir[]="save:/dir1",ffile[]="save:/dir1/test.bin",payload[]="hello-save";
            web_write(0x7800010b00ULL,sv,sizeof sv);
            web_write(0x7800010b10ULL,fdir,sizeof fdir);
            web_write(0x7800010b30ULL,ffile,sizeof ffile);
            web_write(0x7800010b60ULL,payload,sizeof payload);
            check(!invoke("_ZN2nn2fs14EnsureSaveDataERKNS_7account3UidEll",0x7800010040ULL,0x10000,0x10000));
            check(!invoke("_ZN2nn2fs18IsSaveDataExistingEPbRKNS_7account3UidE",0x7800010b80ULL,0x7800010040ULL,0)&&rd8(0x7800010b80ULL)==1);
            check(!invoke("_ZN2nn2fs13MountSaveDataEPKcRKNS_7account3UidE",0x7800010b00ULL,0x7800010040ULL,0));
            check(!invoke("_ZN2nn2fs15CreateDirectoryEPKc",0x7800010b10ULL,0,0));
            check(!invoke("_ZN2nn2fs10CreateFileEPKcl",0x7800010b30ULL,sizeof payload,0));
            check(!invoke("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci",0x7800010b90ULL,0x7800010b30ULL,3));
            uint64_t fh=rd64(0x7800010b90ULL);
            Ctx c={0};c.x[0]=fh;c.x[1]=0;c.x[2]=0x7800010b60ULL;c.x[3]=sizeof payload;c.x[4]=0;
            check(web_fs_import(&c,"_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE")&&c.x[0]==0);
            invoke("_ZN2nn2fs9CloseFileENS0_10FileHandleE",fh,0,0);
            check(!invoke("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci",0x7800010b90ULL,0x7800010b30ULL,1));
            fh=rd64(0x7800010b90ULL);
            c=(Ctx){0};c.x[0]=fh;c.x[1]=0;c.x[2]=0x7800010ba0ULL;c.x[3]=sizeof payload;
            check(web_fs_import(&c,"_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm")&&c.x[0]==0);
            check(rd8(0x7800010ba0ULL)=='h'&&rd8(0x7800010ba6ULL)=='s');
            invoke("_ZN2nn2fs9CloseFileENS0_10FileHandleE",fh,0,0);
            check(!invoke("_ZN2nn2fs6CommitEPKc",0x7800010b00ULL,0,0));
            check(!invoke("_ZN2nn2fs13OpenDirectoryEPNS0_15DirectoryHandleEPKci",0x7800010b90ULL,0x7800010b10ULL,3));
            uint64_t dh=rd64(0x7800010b90ULL);
            check(!invoke("_ZN2nn2fs22GetDirectoryEntryCountEPlNS0_15DirectoryHandleE",0x7800010b98ULL,dh,0)&&rd64(0x7800010b98ULL)==1);
            c=(Ctx){0};c.x[0]=0x7800010b98ULL;c.x[1]=0x7800013000ULL;c.x[2]=dh;c.x[3]=4;
            check(web_fs_import(&c,"_ZN2nn2fs13ReadDirectoryEPlPNS0_14DirectoryEntryENS0_15DirectoryHandleEl")&&c.x[0]==0&&rd64(0x7800010b98ULL)==1);
            check(rd8(0x7800013000ULL)=='t'&&rd8(0x7800013000ULL+0x304)==1&&rd64(0x7800013000ULL+0x308)==sizeof payload);
            check(!invoke("_ZN2nn2fs14CloseDirectoryENS0_15DirectoryHandleE",dh,0,0));
            check(!invoke("_ZN2nn2fs10DeleteFileEPKc",0x7800010b30ULL,0,0));
            check(!invoke("_ZN2nn2fs15DeleteDirectoryEPKc",0x7800010b10ULL,0,0));
            check(!invoke("_ZN2nn2fs7UnmountEPKc",0x7800010b00ULL,0,0));
        }
        // HardwareOpusDecoder + web_os_indirect
        check(gm_reserve(&memory,0x7800000000ULL,65536)==GM_OK);
        check(gm_commit(&memory,0x7800000000ULL,65536)==GM_OK);
        invoke("_ZN2nn5codec19HardwareOpusDecoderC1Ei",0x7800010c00ULL,0,0);
        uint64_t vt=rd64(0x7800010c00ULL);
        check(vt==0x7800000200ULL&&rd64(vt+2*8)==0x7500001020ULL);
        {
            Ctx c={0};c.x[0]=0x7800010c00ULL;
            check(web_os_indirect(&c,rd64(vt+2*8))&&c.x[0]==0x2000ULL);
            c=(Ctx){0};c.x[0]=0x7800010c00ULL;c.x[1]=48000;c.x[2]=2;
            check(web_os_indirect(&c,rd64(vt+4*8))&&c.x[0]==0);
            c=(Ctx){0};c.x[0]=0x7800010c00ULL;
            check(web_os_indirect(&c,rd64(vt+3*8))&&c.x[0]==1);
            // 8-byte header (big-endian len=4) + 4 payload bytes
            wr8(0x7800010c40ULL,0);wr8(0x7800010c41ULL,0);wr8(0x7800010c42ULL,0);wr8(0x7800010c43ULL,4);
            wr32(0x7800010c44ULL,0);wr32(0x7800010c48ULL,0);
            c=(Ctx){0};c.x[0]=0x7800010c00ULL;c.x[1]=0x7800010c50ULL;c.x[2]=0x7800010c58ULL;
            c.x[3]=0x7800012000ULL;c.x[4]=960;c.x[5]=0x7800010c40ULL;c.x[6]=12;
            check(web_os_indirect(&c,rd64(vt+8*8))&&c.x[0]==0);
            check(rd64(0x7800010c50ULL)==12&&rd32(0x7800010c58ULL)==960);
            c=(Ctx){0};c.x[0]=0x7800010c00ULL;
            check(web_os_indirect(&c,rd64(vt+5*8))&&rd8(0x7800010c10ULL)==0);
        }
        // ConditionVariable + LanguageCode + nn::pl
        invoke("_ZN2nn2os15InitializeMutexEPNS0_9MutexTypeEbi",0x7800010d00ULL,0,0);
        invoke("_ZN2nn2os27InitializeConditionVariableEPNS0_21ConditionVariableTypeE",0x7800010d40ULL,0,0);
        invoke("_ZN2nn2os9LockMutexEPNS0_9MutexTypeE",0x7800010d00ULL,0,0);
        check(invoke("_ZN2nn2os26TimedWaitConditionVariableEPNS0_21ConditionVariableTypeEPNS0_9MutexTypeENS_8TimeSpanE",0x7800010d40ULL,0x7800010d00ULL,1000000ULL)==0);
        invoke("_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE",0x7800010d00ULL,0,0);
        invoke("_ZN2nn2os23SignalConditionVariableEPNS0_21ConditionVariableTypeE",0x7800010d40ULL,0,0);
        invoke("_ZN2nn2os26BroadcastConditionVariableEPNS0_21ConditionVariableTypeE",0x7800010d40ULL,0,0);
        invoke("_ZN2nn2os25FinalizeConditionVariableEPNS0_21ConditionVariableTypeE",0x7800010d40ULL,0,0);
        invoke("_ZN2nn2os13FinalizeMutexEPNS0_9MutexTypeE",0x7800010d00ULL,0,0);
        check(invoke("_ZN2nn8settings12LanguageCode4MakeENS0_8LanguageE",1,0,0)==0x53552d6e65ULL);
        check(invoke("_ZN2nn2pl8IsVrModeEv",0,0,0)==0);
        // DecompressZlib (RFC 1950 + RFC 1951 fixed Huffman for "hello" + Adler-32 0x062c0215)
        {
            static const uint8_t zhello[]={0x78,0x9c,0xcb,0x48,0xcd,0xc9,0xc9,0x07,0x00,0x06,0x2c,0x02,0x15};
            web_write(0x7800010d80ULL,zhello,sizeof zhello);
            Ctx c={0};c.x[0]=0x7800010da0ULL;c.x[1]=5;c.x[2]=0x7800010d80ULL;c.x[3]=sizeof zhello;
            check(web_os_import(&c,"_ZN2nn4util14DecompressZlibEPvmPKvmS1_m")&&c.x[0]==1);
            check(rd8(0x7800010da0ULL)=='h'&&rd8(0x7800010da4ULL)=='o');
            // Corrupt Adler-32 must fail
            wr8(0x7800010d80ULL+12,0x16);
            c=(Ctx){0};c.x[0]=0x7800010da0ULL;c.x[1]=5;c.x[2]=0x7800010d80ULL;c.x[3]=sizeof zhello;
            check(web_os_import(&c,"_ZN2nn4util14DecompressZlibEPvmPKvmS1_m")&&c.x[0]==0);
        }
        // nn::pl shared font + nn::fontll::ScalableFontEngine
        check(invoke("_ZN2nn2pl22GetSharedFontLoadStateENS0_14SharedFontTypeE",0,0,0)==1);
        check(invoke("_ZN2nn2pl20GetSharedFontAddressENS0_14SharedFontTypeE",0,0,0)==0x7800001000ULL);
        check(invoke("_ZN2nn2pl17GetSharedFontSizeENS0_14SharedFontTypeE",0,0,0)==4096);
        check(invoke("_ZN2nn6fontll18ScalableFontEngineC1Ev",0x7800010e00ULL,0,0)==0x7800010e00ULL);
        check(!invoke("_ZN2nn6fontll18ScalableFontEngine10InitializeEPvj",0x7800010e00ULL,0x7800012000ULL,4096));
        check(invoke("_ZN2nn6fontll18ScalableFontEngine22GetPointerToWorkBufferEv",0x7800010e00ULL,0,0)==0x7800012000ULL);
        check(invoke("_ZN2nn6fontll18ScalableFontEngine15CheckGlyphExistEj",0x7800010e00ULL,'A',0)==1);
        {
            uint64_t gm=invoke("_ZN2nn6fontll18ScalableFontEngine15AcquireGlyphmapEjt",0x7800010e00ULL,'A',2);
            check(gm==0x7800000c80ULL&&rd16(gm+0x10)==16&&rd16(gm+0x26)==2);
        }
        // nn::crypto + nn::mii + movie::Buffer
        {
            wr32(0x7800010ec0ULL,0x34333231u);
            Ctx c={0};c.x[0]=0x7800010ed0ULL;c.x[1]=20;c.x[2]=0x7800010ec0ULL;c.x[3]=4;
            check(web_os_import(&c,"_ZN2nn6crypto16GenerateSha1HashEPvmPKvm")&&c.x[0]==0&&rd32(0x7800010ed0ULL)!=0);
        }
        check(invoke("_ZN2nn3mii9AppendMiiEPi",0x7800010ef0ULL,0,0)==0xe7e);
        check(!invoke("_ZNK2nn3mii8Database8GetCountEPiNS0_10SourceFlagE",0x7800010f00ULL,0x7800010ef0ULL,2)&&rd32(0x7800010ef0ULL)==6);
        invoke("_ZN5movie6BufferC1EPvm",0x7800010f20ULL,0x7800012000ULL,512);
        check(invoke("_ZNK5movie6Buffer4BaseEv",0x7800010f20ULL,0,0)==0x7800012000ULL);
        check(invoke("_ZNK5movie6Buffer8CapacityEv",0x7800010f20ULL,0,0)==512);
        invoke("_ZN5movie6Buffer8SetRangeEii",0x7800010f20ULL,16,256);
        check(invoke("_ZNK5movie6Buffer6OffsetEv",0x7800010f20ULL,0,0)==16&&invoke("_ZNK5movie6Buffer4SizeEv",0x7800010f20ULL,0,0)==256);
        // std::__sort int32 + float
        {
            wr32(0x7800010f80ULL,(uint32_t)-5);wr32(0x7800010f84ULL,10u);wr32(0x7800010f88ULL,(uint32_t)-20);wr32(0x7800010f8cULL,0u);
            Ctx c={0};c.x[0]=0x7800010f80ULL;c.x[1]=0x7800010f90ULL;
            check(web_unsigned_sort_import(&c,"_ZNSt3__16__sortIRNS_6__lessIiiEEPiEEvT0_S5_T_"));
            check((int32_t)rd32(0x7800010f80ULL)==-20&&(int32_t)rd32(0x7800010f84ULL)==-5&&(int32_t)rd32(0x7800010f88ULL)==0&&(int32_t)rd32(0x7800010f8cULL)==10);
            wr32(0x7800010fa0ULL,0x40200000u);wr32(0x7800010fa4ULL,0xbfc00000u);wr32(0x7800010fa8ULL,0x00000000u);wr32(0x7800010facULL,0x3fa00000u);
            c=(Ctx){0};c.x[0]=0x7800010fa0ULL;c.x[1]=0x7800010fb0ULL;
            check(web_unsigned_sort_import(&c,"_ZNSt3__16__sortIRNS_6__lessIffEEPfEEvT0_S5_T_"));
            check(rd32(0x7800010fa0ULL)==0xbfc00000u&&rd32(0x7800010fa4ULL)==0x00000000u&&rd32(0x7800010fa8ULL)==0x3fa00000u&&rd32(0x7800010facULL)==0x40200000u);
        }
        check(!invoke("nnsslInitialize",0,0,0)&&!invoke("nnsslFinalize",0,0,0));
        check(!invoke("_ZN2nn3ssl7Context6CreateENS1_10SslVersionE",0x7800010fc0ULL,1,0));
        check(!invoke("_ZN2nn6socket10InitializeEPvmmi",0x7800012000ULL,0x10000,0x4000)&&!invoke("nnsocketSetLastErrno",0,0,0));
        check(invoke("nnsocketGetLastErrno",0,0,0)==6&&invoke("nnsocketSocket",2,1,0)==(uint64_t)-1LL);
        {
            wr64(0x7800010fd0ULL,0ULL);
            uint64_t tm_ptr=invoke("gmtime",0x7800010fd0ULL,0,0);
            check(tm_ptr!=0&&rd32(tm_ptr+12)==1&&rd32(tm_ptr+16)==0&&rd32(tm_ptr+20)==70);
            wr32(0x7800010fe0ULL,'%');wr32(0x7800010fe4ULL,'d');wr32(0x7800010fe8ULL,0);
            Ctx c={0};c.x[0]=0x7800011000ULL;c.x[1]=16;c.x[2]=0x7800010fe0ULL;c.x[3]=42;
            check(web_os_import(&c,"swprintf")&&c.x[0]==2&&rd32(0x7800011000ULL)=='4'&&rd32(0x7800011004ULL)=='2'&&rd32(0x7800011008ULL)==0);
        }
        web_memory_bind(NULL);
    } else atomic_fetch_add(&failures,1);
    snprintf(report,sizeof report,"{\"ok\":%s,\"checks\":%u,\"failures\":%u,\"error\":\"%s\"}",atomic_load(&failures)?"false":"true",checks,atomic_load(&failures),error);
    publish();return NULL;
}
int os_probe_start(void){return pthread_create(&driver,NULL,run,NULL);}
int os_probe_join(void){return pthread_join(driver,NULL);}
