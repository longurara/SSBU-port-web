/* Native pthread synchronization behind opaque guest mutex addresses. Records
 * are leased across blocking operations; destruction never frees a waiter. */
#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "host_mutex.h"
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct HostMutex {uint64_t key;pthread_mutex_t mutex;pthread_t owner;unsigned references,depth;struct HostMutex *next;} HostMutex;
typedef struct HostCond {uint64_t key;pthread_cond_t cond;struct HostCond *next;} HostCond;
static HostMutex *buckets[1024];
static HostCond *cond_buckets[1024];
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static atomic_uint live,peak;
static unsigned bucket(uint64_t key){return (unsigned)((key>>4)^(key>>16))&1023;}
unsigned web_mutex_live_count(void){return atomic_load(&live);}
unsigned web_mutex_peak_count(void){return atomic_load(&peak);}
static HostCond *cond_for(uint64_t key,int create){
    if(!key)return NULL;
    pthread_mutex_lock(&registry);HostCond *cv=cond_buckets[bucket(key)];
    while(cv&&cv->key!=key)cv=cv->next;
    if(!cv&&create){
        cv=calloc(1,sizeof *cv);
        if(cv){
            if(pthread_cond_init(&cv->cond,NULL)){free(cv);cv=NULL;}
            else{cv->key=key;cv->next=cond_buckets[bucket(key)];cond_buckets[bucket(key)]=cv;}
        }
    }
    pthread_mutex_unlock(&registry);return cv;
}
static void cond_destroy(uint64_t key){
    if(!key)return;
    pthread_mutex_lock(&registry);HostCond **link=&cond_buckets[bucket(key)];
    while(*link&&(*link)->key!=key)link=&(*link)->next;
    HostCond *cv=*link;
    if(cv){*link=cv->next;pthread_cond_destroy(&cv->cond);free(cv);}
    pthread_mutex_unlock(&registry);
}
static HostMutex *acquire(uint64_t key,int create,int kind,int *error){
    *error=0;if(!key){*error=EINVAL;return NULL;}
    pthread_mutex_lock(&registry);HostMutex *record=buckets[bucket(key)];
    while(record&&record->key!=key)record=record->next;
    if(record){
        if(record->references==UINT_MAX){*error=EAGAIN;record=NULL;}else record->references++;
    }else if(!create)*error=EINVAL;
    else{
        record=calloc(1,sizeof *record);
        if(!record)*error=ENOMEM;
        else{
            pthread_mutexattr_t attributes;
            int rc=pthread_mutexattr_init(&attributes);
            if(!rc){
                rc=pthread_mutexattr_settype(&attributes,kind==2?PTHREAD_MUTEX_ERRORCHECK:kind?PTHREAD_MUTEX_RECURSIVE:PTHREAD_MUTEX_NORMAL);
                if(!rc)rc=pthread_mutex_init(&record->mutex,&attributes);
                pthread_mutexattr_destroy(&attributes);
            }
            if(rc){free(record);record=NULL;*error=rc;}
            else{
                record->key=key;record->references=1;record->next=buckets[bucket(key)];buckets[bucket(key)]=record;
                unsigned count=atomic_fetch_add(&live,1)+1;if(count>atomic_load(&peak))atomic_store(&peak,count);
            }
        }
    }
    pthread_mutex_unlock(&registry);return record;
}
static void release(HostMutex *record){
    pthread_mutex_lock(&registry);record->references--;pthread_mutex_unlock(&registry);
}
static int destroy(uint64_t key){
    if(!key)return EINVAL;
    pthread_mutex_lock(&registry);HostMutex **link=&buckets[bucket(key)];
    while(*link&&(*link)->key!=key)link=&(*link)->next;
    HostMutex *record=*link;int rc=0;
    // A never-used static initializer owns no host allocation yet.
    if(record){
        rc=record->references||record->depth?EBUSY:pthread_mutex_destroy(&record->mutex);
        if(!rc){*link=record->next;free(record);atomic_fetch_sub(&live,1);}
    }
    pthread_mutex_unlock(&registry);return rc;
}
static void sdk_error(int rc,uint64_t key,const char *operation){
    if(rc)guest_trap(rc==ENOMEM?"mutex allocation failed":operation,key);
}
static int cond_wait_common(uint64_t cv_key,uint64_t mtx_key,const struct timespec *abstime){
    int rc=0;HostCond *cv=cond_for(cv_key,1);HostMutex *m=acquire(mtx_key,1,0,&rc);
    if(!cv||!m){if(m)release(m);return ETIMEDOUT;}
    pthread_mutex_lock(&registry);if(m->depth)m->depth--;pthread_mutex_unlock(&registry);
    int wrc=abstime?pthread_cond_timedwait(&cv->cond,&m->mutex,abstime):pthread_cond_wait(&cv->cond,&m->mutex);
    pthread_mutex_lock(&registry);if(!m->depth)m->owner=pthread_self();m->depth++;pthread_mutex_unlock(&registry);
    release(m);return wrc;
}
int web_mutex_import(Ctx *c,const char *name){
    uint64_t key=c->x[0];
    if(!strcmp(name,"_ZN2nn2os27InitializeConditionVariableEPNS0_21ConditionVariableTypeE")){cond_for(key,1);c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn2os25FinalizeConditionVariableEPNS0_21ConditionVariableTypeE")||
       !strcmp(name,"_ZNSt3__118condition_variableD1Ev")||!strcmp(name,"_ZNSt3__118condition_variableD2Ev")||
       !strcmp(name,"pthread_cond_destroy")){cond_destroy(key);c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn2os23SignalConditionVariableEPNS0_21ConditionVariableTypeE")||
       !strcmp(name,"_ZNSt3__118condition_variable10notify_oneEv")){
        HostCond *cv=cond_for(key,1);if(cv)pthread_cond_signal(&cv->cond);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2os26BroadcastConditionVariableEPNS0_21ConditionVariableTypeE")||
       !strcmp(name,"_ZNSt3__118condition_variable10notify_allEv")||!strcmp(name,"pthread_cond_broadcast")){
        HostCond *cv=cond_for(key,1);if(cv)pthread_cond_broadcast(&cv->cond);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2os21WaitConditionVariableEPNS0_21ConditionVariableTypeEPNS0_9MutexTypeE")){
        cond_wait_common(key,c->x[1],NULL);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZNSt3__118condition_variable4waitERNS_11unique_lockINS_5mutexEEE")){
        cond_wait_common(key,c->x[1]?rd64(c->x[1]):0,NULL);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2os26TimedWaitConditionVariableEPNS0_21ConditionVariableTypeEPNS0_9MutexTypeENS_8TimeSpanE")){
        struct timespec ts;clock_gettime(CLOCK_REALTIME,&ts);
        uint64_t total=(uint64_t)ts.tv_nsec+c->x[2];
        ts.tv_sec+=(time_t)(total/1000000000ULL);ts.tv_nsec=(long)(total%1000000000ULL);
        c->x[0]=cond_wait_common(key,c->x[1],&ts)==0?1:0;return 1;
    }
    if(!strcmp(name,"_ZNSt3__118condition_variable15__do_timed_waitERNS_11unique_lockINS_5mutexEEENS_6chrono10time_pointINS5_12system_clockENS5_8durationIxNS_5ratioILl1ELl1000000000EEEEEEE")){
        struct timespec ts={(time_t)(c->x[2]/1000000000ULL),(long)(c->x[2]%1000000000ULL)};
        cond_wait_common(key,c->x[1]?rd64(c->x[1]):0,&ts);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"pthread_mutexattr_init")){wr32(key,0);c->x[0]=0;return 1;}
    if(!strcmp(name,"pthread_mutexattr_settype")){
        uint32_t type=(uint32_t)c->x[1];if(type>2)c->x[0]=EINVAL;else{wr32(key,type);c->x[0]=0;}return 1;
    }
    if(!strcmp(name,"pthread_mutexattr_gettype")){if(c->x[1])wr32(c->x[1],rd32(key));c->x[0]=c->x[1]?0:EINVAL;return 1;}
    if(!strcmp(name,"pthread_mutexattr_destroy")){c->x[0]=0;return 1;}
    int posix=!strncmp(name,"pthread_mutex_",14);
    int initialize=!strcmp(name,"pthread_mutex_init")||!strcmp(name,"_ZN2nn2os15InitializeMutexEPNS0_9MutexTypeEbi")||
      !strcmp(name,"nnosInitializeMutex")||
      !strcmp(name,"_ZNSt3__115recursive_mutexC1Ev")||!strcmp(name,"_ZNSt3__115recursive_mutexC2Ev");
    int finalize=!strcmp(name,"pthread_mutex_destroy")||!strcmp(name,"_ZN2nn2os13FinalizeMutexEPNS0_9MutexTypeE")||
      !strcmp(name,"nnosFinalizeMutex")||
      !strcmp(name,"_ZNSt3__15mutexD1Ev")||!strcmp(name,"_ZNSt3__15mutexD2Ev")||
      !strcmp(name,"_ZNSt3__115recursive_mutexD1Ev")||!strcmp(name,"_ZNSt3__115recursive_mutexD2Ev");
    int lock=!strcmp(name,"pthread_mutex_lock")||!strcmp(name,"_ZN2nn2os9LockMutexEPNS0_9MutexTypeE")||
      !strcmp(name,"nnosLockMutex")||
      !strcmp(name,"_ZNSt3__15mutex4lockEv")||!strcmp(name,"_ZNSt3__115recursive_mutex4lockEv");
    int unlock=!strcmp(name,"pthread_mutex_unlock")||!strcmp(name,"_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE")||
      !strcmp(name,"nnosUnlockMutex")||
      !strcmp(name,"_ZNSt3__15mutex6unlockEv")||!strcmp(name,"_ZNSt3__115recursive_mutex6unlockEv");
    int attempt=!strcmp(name,"pthread_mutex_trylock")||!strcmp(name,"_ZN2nn2os12TryLockMutexEPNS0_9MutexTypeE")||
      !strcmp(name,"_ZNSt3__15mutex8try_lockEv")||!strcmp(name,"_ZNSt3__115recursive_mutex8try_lockEv");
    if(!initialize&&!finalize&&!lock&&!unlock&&!attempt)return 0;
    int rc=0;
    if(finalize)rc=destroy(key);
    else{
        int kind=strstr(name,"recursive_mutex")!=NULL;
        if(initialize){kind=posix?(c->x[1]?(int)rd32(c->x[1]):0):strstr(name,"recursive_mutex")?1:c->x[1]!=0;}
        if(kind<0||kind>2)rc=EINVAL;
        else{
            HostMutex *record=acquire(key,!unlock,kind,&rc);
            if(record){
                if(lock)rc=pthread_mutex_lock(&record->mutex);
                else if(attempt)rc=pthread_mutex_trylock(&record->mutex);
                if(unlock){
                    pthread_mutex_lock(&registry);
                    if(!record->depth||!pthread_equal(record->owner,pthread_self()))rc=EPERM;
                    else{rc=pthread_mutex_unlock(&record->mutex);if(!rc)record->depth--;}
                    pthread_mutex_unlock(&registry);
                }else if((lock||attempt)&&!rc){
                    pthread_mutex_lock(&registry);
                    if(!record->depth)record->owner=pthread_self();record->depth++;
                    pthread_mutex_unlock(&registry);
                }
                release(record);
            }
        }
    }
    if(posix)c->x[0]=(unsigned)rc;
    else if(attempt){if(rc&&rc!=EBUSY)sdk_error(rc,key,"mutex try-lock failed");c->x[0]=rc==0;}
    else sdk_error(rc,key,unlock?"unlock of unknown or unowned mutex":finalize?"destroying a busy mutex":initialize?"mutex initialization failed":"mutex lock failed");
    return 1;
}
