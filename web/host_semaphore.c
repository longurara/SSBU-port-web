#define _POSIX_C_SOURCE 200809L
#include "runtime.h"
#include "host_semaphore.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

typedef struct Semaphore {uint64_t key;int count,maximum;unsigned references;pthread_mutex_t lock;pthread_cond_t changed;struct Semaphore *next;} Semaphore;
static Semaphore *buckets[1024];
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static atomic_uint live;
unsigned web_semaphore_live_count(void){return atomic_load(&live);}
static unsigned bucket(uint64_t key){return (unsigned)((key>>4)^(key>>16))&1023;}
static Semaphore *find(uint64_t key){for(Semaphore *s=buckets[bucket(key)];s;s=s->next)if(s->key==key)return s;return NULL;}
static void initialize(uint64_t key,int initial,int maximum){
    if(!key||initial<0||maximum<=0||initial>maximum)guest_trap("invalid semaphore counts",key);
    pthread_mutex_lock(&registry);
    if(find(key)){pthread_mutex_unlock(&registry);guest_trap("semaphore already initialized",key);}
    Semaphore *s=calloc(1,sizeof *s);int rc=s?pthread_mutex_init(&s->lock,NULL):ENOMEM;
    if(s&&!rc){
        pthread_condattr_t attributes;rc=pthread_condattr_init(&attributes);
        if(!rc){rc=pthread_condattr_setclock(&attributes,CLOCK_MONOTONIC);if(!rc)rc=pthread_cond_init(&s->changed,&attributes);pthread_condattr_destroy(&attributes);}
        if(rc)pthread_mutex_destroy(&s->lock);
    }
    if(rc){free(s);pthread_mutex_unlock(&registry);guest_trap("semaphore allocation failed",key);}
    s->key=key;s->count=initial;s->maximum=maximum;s->next=buckets[bucket(key)];buckets[bucket(key)]=s;atomic_fetch_add(&live,1);
    pthread_mutex_unlock(&registry);
}
static Semaphore *retain(uint64_t key){
    pthread_mutex_lock(&registry);Semaphore *s=find(key);
    if(!s||s->references==UINT_MAX){pthread_mutex_unlock(&registry);guest_trap("unknown semaphore or too many waiters",key);}
    s->references++;pthread_mutex_unlock(&registry);return s;
}
static void release(Semaphore *s){pthread_mutex_lock(&registry);s->references--;pthread_mutex_unlock(&registry);}
static void finalize(uint64_t key){
    pthread_mutex_lock(&registry);Semaphore **link=&buckets[bucket(key)];while(*link&&(*link)->key!=key)link=&(*link)->next;
    Semaphore *s=*link;
    if(!s||s->references){pthread_mutex_unlock(&registry);guest_trap("finalizing unknown semaphore or semaphore in use",key);}
    int rc=pthread_cond_destroy(&s->changed);if(!rc)rc=pthread_mutex_destroy(&s->lock);
    if(rc){pthread_mutex_unlock(&registry);guest_trap("semaphore destruction failed",key);}
    *link=s->next;free(s);atomic_fetch_sub(&live,1);pthread_mutex_unlock(&registry);
}
int web_semaphore_import(Ctx *c,const char *name){
    int init=!strcmp(name,"_ZN2nn2os19InitializeSemaphoreEPNS0_13SemaphoreTypeEii")||!strcmp(name,"nnosInitializeSemaphore");
    int finish=!strcmp(name,"_ZN2nn2os17FinalizeSemaphoreEPNS0_13SemaphoreTypeE")||!strcmp(name,"nnosFinalizeSemaphore");
    int acquire=!strcmp(name,"_ZN2nn2os16AcquireSemaphoreEPNS0_13SemaphoreTypeE")||!strcmp(name,"nnosAcquireSemaphore");
    int attempt=!strcmp(name,"_ZN2nn2os19TryAcquireSemaphoreEPNS0_13SemaphoreTypeE")||!strcmp(name,"nnosTryAcquireSemaphore");
    int timed=!strcmp(name,"_ZN2nn2os21TimedAcquireSemaphoreEPNS0_13SemaphoreTypeENS_8TimeSpanE")||!strcmp(name,"nnosTimedAcquireSemaphore");
    int signal=!strcmp(name,"_ZN2nn2os16ReleaseSemaphoreEPNS0_13SemaphoreTypeE")||!strcmp(name,"nnosReleaseSemaphore");
    int many=!strcmp(name,"_ZN2nn2os16ReleaseSemaphoreEPNS0_13SemaphoreTypeEi")||!strcmp(name,"nnosReleaseSemaphoreCount");
    int count=!strcmp(name,"_ZN2nn2os24GetCurrentSemaphoreCountEPKNS0_13SemaphoreTypeE")||!strcmp(name,"nnosGetCurrentSemaphoreCount");
    if(!init&&!finish&&!acquire&&!attempt&&!timed&&!signal&&!many&&!count)return 0;
    uint64_t key=c->x[0];
    if(init){initialize(key,(int32_t)c->x[1],(int32_t)c->x[2]);return 1;}
    if(finish){finalize(key);return 1;}
    Semaphore *s=retain(key);pthread_mutex_lock(&s->lock);int rc=0,ready=0;
    if(signal||many){
        int amount=many?(int32_t)c->x[1]:1;
        if(amount<=0||amount>s->maximum-s->count){pthread_mutex_unlock(&s->lock);release(s);guest_trap("semaphore release exceeds maximum",key);}
        s->count+=amount;pthread_cond_broadcast(&s->changed);
    }else if(count)c->x[0]=(unsigned)s->count;
    else{
        struct timespec deadline;
        int64_t ns=(int64_t)c->x[1];if(ns<0)ns=0;
        if(timed){
            if(clock_gettime(CLOCK_MONOTONIC,&deadline)){pthread_mutex_unlock(&s->lock);release(s);guest_trap("semaphore clock failed",key);}
            deadline.tv_sec+=(time_t)(ns/1000000000);
            deadline.tv_nsec+=(long)(ns%1000000000);if(deadline.tv_nsec>=1000000000){deadline.tv_sec++;deadline.tv_nsec-=1000000000;}
        }
        while((acquire||timed)&&!s->count&&!rc)rc=timed?pthread_cond_timedwait(&s->changed,&s->lock,&deadline):pthread_cond_wait(&s->changed,&s->lock);
        if(s->count){s->count--;ready=1;}
        if(attempt||timed)c->x[0]=ready;
    }
    pthread_mutex_unlock(&s->lock);release(s);
    if(rc&&rc!=ETIMEDOUT)guest_trap("semaphore wait failed",key);
    return 1;
}
