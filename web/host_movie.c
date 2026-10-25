/* SDK movie value objects. Guest pointers remain guest addresses; allocations
 * use the registered SDK allocator, never a host malloc pointer masquerading
 * as guest storage. ABI verified against subsdk Buffer and nnmovie_mf.c. */
#include "host_movie.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_BYTES 64
#define BUFFER_LIMIT (256u*1024u*1024u)
enum { B_OFFSET=0,B_SIZE=4,B_I32=8,B_U32=12,B_OWNS=16,B_CAP=24,B_I64=32,B_BASE=40,B_DEV=48,B_SYNC=56 };
typedef struct {uint64_t alloc,release,user;} Allocator;
typedef struct Allocation {uint64_t address,size;Allocator allocator;struct Allocation *next;} Allocation;
static pthread_mutex_t movie_lock=PTHREAD_MUTEX_INITIALIZER;
static Allocator allocator;
static Allocation *allocations;
enum { MD_I32=1,MD_I64,MD_FLOAT,MD_POINTER,MD_STRING };
typedef struct {char key[40];unsigned type;uint64_t value;} MetadataEntry;
typedef struct Metadata {uint64_t token;unsigned count;MetadataEntry entries[40];struct Metadata *next;} Metadata;
static Metadata *metadata;
static uint64_t next_metadata=1;
uint64_t web_movie_guest_call(Ctx *c,uint64_t function,uint64_t a,uint64_t b,uint64_t d,uint64_t e,uint64_t f){
    Ctx saved=*c;c->x[0]=a;c->x[1]=b;c->x[2]=d;c->x[3]=e;c->x[4]=f;
    call_indirect(c,function);uint64_t result=c->x[0];*c=saved;return result;
}
static uint64_t guest_call(Ctx *c,uint64_t function,uint64_t a,uint64_t b,uint64_t d){return web_movie_guest_call(c,function,a,b,d,0,0);}
static uint64_t guest_allocate(Ctx *c,uint64_t size){
    if(size>BUFFER_LIMIT)return 0;
    pthread_mutex_lock(&movie_lock);Allocator owner=allocator;pthread_mutex_unlock(&movie_lock);
    if(!owner.alloc||!owner.release)return 0;
    Allocation *record=malloc(sizeof *record);if(!record)return 0;
    uint64_t address=guest_call(c,owner.alloc,size?size:16,16,owner.user);
    if(!address){free(record);return 0;}
    *record=(Allocation){address,size,owner,NULL};
    pthread_mutex_lock(&movie_lock);record->next=allocations;allocations=record;pthread_mutex_unlock(&movie_lock);
    return address;
}
static void guest_release(Ctx *c,uint64_t address){
    if(!address)return;
    pthread_mutex_lock(&movie_lock);Allocation **link=&allocations;
    while(*link&&(*link)->address!=address)link=&(*link)->next;
    Allocation *record=*link;if(record)*link=record->next;
    pthread_mutex_unlock(&movie_lock);
    if(!record)guest_trap("unowned movie allocation",address);
    Allocator owner=record->allocator;free(record);
    guest_call(c,owner.release,address,owner.user,0);
}
unsigned web_movie_live_allocations(void){
    pthread_mutex_lock(&movie_lock);unsigned count=0;
    for(Allocation *p=allocations;p;p=p->next)count++;
    pthread_mutex_unlock(&movie_lock);return count;
}
unsigned web_movie_live_metadata(void){
    pthread_mutex_lock(&movie_lock);unsigned count=0;
    for(Metadata *m=metadata;m;m=m->next)count++;
    pthread_mutex_unlock(&movie_lock);return count;
}
void web_movie_reset(void){
    pthread_mutex_lock(&movie_lock);
    while(allocations){Allocation *next=allocations->next;free(allocations);allocations=next;}
    while(metadata){Metadata *next=metadata->next;free(metadata);metadata=next;}
    next_metadata=1;
    allocator=(Allocator){0};pthread_mutex_unlock(&movie_lock);
}
static void buffer_release(Ctx *c,uint64_t object){
    uint64_t address=rd64(object+B_BASE);unsigned owns=rd8(object+B_OWNS);
    wr64(object+B_BASE,0);wr8(object+B_OWNS,0);
    if(owns)guest_release(c,address);
}
static void copy_bytes(uint64_t destination,uint64_t source,uint64_t size){
    uint8_t bytes[4096];
    for(uint64_t offset=0;offset<size;offset+=sizeof bytes){size_t n=(size-offset)>sizeof bytes?sizeof bytes:(size_t)(size-offset);web_read(source+offset,bytes,n);web_write(destination+offset,bytes,n);}
}
static void buffer_copy(Ctx *c,uint64_t object,uint64_t source,int assign){
    if(object==source)return;
    uint8_t bytes[BUFFER_BYTES];web_read(source,bytes,sizeof bytes);
    uint64_t base,capacity;memcpy(&base,bytes+B_BASE,8);memcpy(&capacity,bytes+B_CAP,8);
    if(bytes[B_OWNS]&&base){uint64_t copy=guest_allocate(c,capacity);if(copy)copy_bytes(copy,base,capacity);memcpy(bytes+B_BASE,&copy,8);bytes[B_OWNS]=copy?1:0;}
    else bytes[B_OWNS]=0;
    if(assign)buffer_release(c,object);
    web_write(object,bytes,sizeof bytes);
}
static int buffer_import(Ctx *c,const char *method){
    uint64_t object=c->x[0],a=c->x[1],b=c->x[2];
    uint8_t zero[BUFFER_BYTES]={0};
    if(!strcmp(method,"C1Ev")||!strcmp(method,"C2Ev")){web_write(object,zero,sizeof zero);return 1;}
    if(!strcmp(method,"C1Em")||!strcmp(method,"C2Em")){
        uint64_t base=guest_allocate(c,a);web_write(object,zero,sizeof zero);
        wr64(object+B_CAP,a);wr64(object+B_BASE,base);wr8(object+B_OWNS,base?1:0);return 1;
    }
    if(!strcmp(method,"C1EPvm")||!strcmp(method,"C2EPvm")){web_write(object,zero,sizeof zero);wr64(object+B_BASE,a);wr64(object+B_CAP,b);return 1;}
    if(!strcmp(method,"C1ERKS0_")||!strcmp(method,"C2ERKS0_")||!strcmp(method,"aSERKS0_")){buffer_copy(c,object,a,method[0]=='a');return 1;}
    if(!strcmp(method,"D1Ev")||!strcmp(method,"D2Ev")){buffer_release(c,object);return 1;}
    if(!strcmp(method,"18SetDataAndCapacityEPvm")){buffer_release(c,object);wr64(object+B_BASE,a);wr64(object+B_CAP,b);return 1;}
    if(!strcmp(method,"11SetCapacityEm")){wr64(object+B_CAP,a);return 1;}
    if(!strcmp(method,"8SetRangeEii")){wr32(object+B_OFFSET,(uint32_t)a);wr32(object+B_SIZE,(uint32_t)b);return 1;}
    if(!strcmp(method,"4SwapERS0_")){uint8_t other[BUFFER_BYTES];web_read(object,zero,sizeof zero);web_read(a,other,sizeof other);web_write(object,other,sizeof other);web_write(a,zero,sizeof zero);return 1;}
    struct {const char *name;unsigned offset,bits;} const fields[]={
        {"4BaseEv",B_BASE,64},{"4SizeEv",B_SIZE,32},{"6OffsetEv",B_OFFSET,32},{"8CapacityEv",B_CAP,64},
        {"12GetInt32DataEv",B_I32,32},{"12GetInt64DataEv",B_I64,64},{"13GetUint32DataEv",B_U32,32},
        {"9GetNvnDevEv",B_DEV,64},{"10GetNvnSyncEv",B_SYNC,64},
        {"12SetInt32DataEi",B_I32,32},{"12SetInt64DataEl",B_I64,64},{"13SetUint32DataEj",B_U32,32},
        {"9SetNvnDevEPv",B_DEV,64},{"10SetNvnSyncEPv",B_SYNC,64}};
    if(!strcmp(method,"4DataEv")){c->x[0]=rd64(object+B_BASE)+(int32_t)rd32(object+B_OFFSET);return 1;}
    for(unsigned i=0;i<sizeof fields/sizeof fields[0];i++)if(!strcmp(method,fields[i].name)){
        int set=strstr(method,"Set")!=NULL;
        if(set){if(fields[i].bits==64)wr64(object+fields[i].offset,a);else wr32(object+fields[i].offset,(uint32_t)a);}
        else c->x[0]=fields[i].bits==64?rd64(object+fields[i].offset):rd32(object+fields[i].offset);
        return 1;
    }
    return 0;
}
static char *guest_string(uint64_t address,size_t limit){
    if(!address)return NULL;
    char *text=malloc(limit+1);if(!text)return NULL;
    for(size_t i=0;i<=limit;i++){text[i]=(char)rd8(address+i);if(!text[i])return text;}
    free(text);return NULL;
}
/* These lookups and entry snapshots require movie_lock; no guest callback may
 * run while it is held. Strings live in guest allocations owned by entries. */
static Metadata *metadata_for(uint64_t object){
    if(!object)return NULL;
    uint64_t token=rd64(object);for(Metadata *m=metadata;m;m=m->next)if(m->token==token)return m;return NULL;
}
static MetadataEntry *metadata_entry(Metadata *m,const char *key){
    if(!m)return NULL;
    for(unsigned i=0;i<m->count;i++)if(!strcmp(m->entries[i].key,key))return &m->entries[i];
    return NULL;
}
static int metadata_set(Ctx *c,uint64_t object,const char *key,unsigned type,uint64_t value){
    if(strlen(key)>=sizeof ((MetadataEntry *)0)->key)return 0;
    uint64_t old=0;int stored=0;
    pthread_mutex_lock(&movie_lock);Metadata *m=metadata_for(object);MetadataEntry *entry=metadata_entry(m,key);
    if(m&&(entry||m->count<40)){
        if(!entry){entry=&m->entries[m->count++];memset(entry,0,sizeof *entry);strcpy(entry->key,key);}
        if(entry->type==MD_STRING)old=entry->value;
        entry->type=type;entry->value=value;stored=1;
    }
    pthread_mutex_unlock(&movie_lock);
    if(old)guest_release(c,old);return stored;
}
static void metadata_clear(Ctx *c,uint64_t object,int destroy){
    uint64_t strings[40];unsigned count=0;
    pthread_mutex_lock(&movie_lock);Metadata *m=metadata_for(object);
    if(m){
        for(unsigned i=0;i<m->count;i++)if(m->entries[i].type==MD_STRING&&m->entries[i].value)strings[count++]=m->entries[i].value;
        m->count=0;
        if(destroy){Metadata **link=&metadata;while(*link&&*link!=m)link=&(*link)->next;*link=m->next;free(m);}
    }
    if(destroy&&object)wr64(object,0);
    pthread_mutex_unlock(&movie_lock);
    for(unsigned i=0;i<count;i++)guest_release(c,strings[i]);
}
static void metadata_construct(uint64_t object){
    Metadata *m=calloc(1,sizeof *m);if(!m)guest_trap("movie metadata allocation failed",object);
    pthread_mutex_lock(&movie_lock);m->token=UINT64_C(0x6d64000000000000)|next_metadata++;m->next=metadata;metadata=m;wr64(object,m->token);pthread_mutex_unlock(&movie_lock);
}
static void metadata_copy(Ctx *c,uint64_t object,uint64_t source,int assign){
    if(object==source)return;
    MetadataEntry entries[40]={0};char *strings[40]={0};unsigned count=0;int failed=0;
    pthread_mutex_lock(&movie_lock);Metadata *src=metadata_for(source),*dst=assign?metadata_for(object):NULL;
    if(assign&&src&&src==dst){pthread_mutex_unlock(&movie_lock);return;}
    if(src){
        count=src->count;memcpy(entries,src->entries,count*sizeof *entries);
        for(unsigned i=0;i<count;i++)if(entries[i].type==MD_STRING){strings[i]=guest_string(entries[i].value,4096);if(!strings[i])failed=1;entries[i].value=0;}
    }
    pthread_mutex_unlock(&movie_lock);
    if(!failed)for(unsigned i=0;i<count;i++)if(strings[i]){
        entries[i].value=guest_allocate(c,strlen(strings[i])+1);
        if(!entries[i].value){failed=1;break;}
        web_write(entries[i].value,strings[i],strlen(strings[i])+1);
    }
    for(unsigned i=0;i<count;i++)free(strings[i]);
    if(failed){for(unsigned i=0;i<count;i++)if(entries[i].type==MD_STRING&&entries[i].value)guest_release(c,entries[i].value);guest_trap("movie metadata string copy failed",source);}
    if(assign)metadata_clear(c,object,0);else metadata_construct(object);
    pthread_mutex_lock(&movie_lock);dst=metadata_for(object);
    if(dst){dst->count=count;memcpy(dst->entries,entries,count*sizeof *entries);}
    pthread_mutex_unlock(&movie_lock);
    if(!dst){for(unsigned i=0;i<count;i++)if(entries[i].type==MD_STRING&&entries[i].value)guest_release(c,entries[i].value);guest_trap("movie metadata lifetime changed",object);}
}
static int metadata_import(Ctx *c,const char *method){
    uint64_t object=c->x[0],a=c->x[1],b=c->x[2];
    if(!strcmp(method,"C1Ev")||!strcmp(method,"C2Ev")){metadata_construct(object);return 1;}
    if(!strcmp(method,"C1ERKS0_")||!strcmp(method,"C2ERKS0_")||!strcmp(method,"aSERKS0_")){metadata_copy(c,object,a,method[0]=='a');return 1;}
    if(!strcmp(method,"D1Ev")||!strcmp(method,"D2Ev")||!strcmp(method,"5ClearEv")){metadata_clear(c,object,method[0]=='D');return 1;}
    if(!strcmp(method,"12CountEntriesEv")){pthread_mutex_lock(&movie_lock);Metadata *m=metadata_for(object);c->x[0]=m?m->count:0;pthread_mutex_unlock(&movie_lock);return 1;}
    unsigned type=0;int find=0,contains=!strcmp(method,"8ContainsEPKc");
    if(!strcmp(method,"8SetInt32EPKci"))type=MD_I32;
    if(!strcmp(method,"8SetInt64EPKcl"))type=MD_I64;
    if(!strcmp(method,"8SetFloatEPKcf"))type=MD_FLOAT;
    if(!strcmp(method,"10SetPointerEPKcPv"))type=MD_POINTER;
    if(!strcmp(method,"9SetStringEPKcS2_"))type=MD_STRING;
    if(!strcmp(method,"9FindInt32EPKcPi")){type=MD_I32;find=1;}
    if(!strcmp(method,"9FindInt64EPKcPl")){type=MD_I64;find=1;}
    if(!strcmp(method,"11FindPointerEPKcPPv")){type=MD_POINTER;find=1;}
    if(!strcmp(method,"10FindStringEPKcPS2_")){type=MD_STRING;find=1;}
    if(!type&&!contains)return 0;
    char *key=guest_string(a,39);
    if(!key){if(find||contains)c->x[0]=0;return 1;}
    if(find||contains){
        pthread_mutex_lock(&movie_lock);MetadataEntry *entry=metadata_entry(metadata_for(object),key);int ok=entry!=NULL;
        if(ok&&!contains){
            ok=entry->type==type||((type==MD_I32||type==MD_I64)&&(entry->type==MD_I32||entry->type==MD_I64));
            if(ok&&b){uint64_t value=entry->value;if(type==MD_I64&&entry->type==MD_I32)value=(uint64_t)(int64_t)(int32_t)value;if(type==MD_I32)wr32(b,(uint32_t)value);else wr64(b,value);}
        }
        c->x[0]=(uint64_t)ok;pthread_mutex_unlock(&movie_lock);
    }else{
        uint64_t value=type==MD_FLOAT?c->v[0].w[0]:type==MD_I32?(uint32_t)b:b;
        if(type==MD_STRING){
            char *text=b?guest_string(b,4096):calloc(1,1);if(!text){free(key);guest_trap("invalid movie metadata string",b);}
            value=guest_allocate(c,strlen(text)+1);
            if(!value){free(text);free(key);guest_trap("movie metadata string allocation failed",object);}
            web_write(value,text,strlen(text)+1);free(text);
        }
        if(!metadata_set(c,object,key,type,value)&&type==MD_STRING)guest_release(c,value);
    }
    free(key);return 1;
}
uint64_t web_movie_guest_alloc(Ctx *c,uint64_t size){return guest_allocate(c,size);}
void web_movie_guest_free(Ctx *c,uint64_t address){guest_release(c,address);}
void web_movie_metadata_clear(Ctx *c,uint64_t object){metadata_clear(c,object,0);}
int web_movie_metadata_set(Ctx *c,uint64_t object,const char *key,unsigned type,uint64_t value){
    if(type<MD_I32||type>MD_POINTER)return 0;return metadata_set(c,object,key,type,value);
}
int web_movie_metadata_string(Ctx *c,uint64_t object,const char *key,const char *value){
    if(!value||strlen(value)>4096||strlen(key)>39)return 0;
    uint64_t address=guest_allocate(c,strlen(value)+1);if(!address)return 0;
    web_write(address,value,strlen(value)+1);
    if(!metadata_set(c,object,key,MD_STRING,address)){guest_release(c,address);return 0;}return 1;
}
int web_movie_metadata_get(uint64_t object,const char *key,unsigned type,uint64_t *value){
    pthread_mutex_lock(&movie_lock);MetadataEntry *e=metadata_entry(metadata_for(object),key);int ok=e&&(type==e->type||((type==MD_I32||type==MD_I64)&&(e->type==MD_I32||e->type==MD_I64)));
    if(ok){*value=e->value;if(type==MD_I64&&e->type==MD_I32)*value=(uint64_t)(int64_t)(int32_t)*value;if(type==MD_I32)*value=(uint32_t)*value;}
    pthread_mutex_unlock(&movie_lock);return ok;
}
int web_movie_import(Ctx *c,const char *name){
    if(!strcmp(name,"_ZN5movie12SetAllocatorEPFPvmmS0_EPFvS0_S0_EPFS0_S0_mS0_ES0_")){
        pthread_mutex_lock(&movie_lock);allocator=(Allocator){c->x[0],c->x[1],c->x[3]};pthread_mutex_unlock(&movie_lock);return 1;
    }
    const char *normal="_ZN5movie6Buffer",*constant="_ZNK5movie6Buffer";
    if(!strncmp(name,normal,strlen(normal)))return buffer_import(c,name+strlen(normal));
    if(!strncmp(name,constant,strlen(constant)))return buffer_import(c,name+strlen(constant));
    normal="_ZN5movie9MediaData";constant="_ZNK5movie9MediaData";
    if(!strncmp(name,normal,strlen(normal)))return metadata_import(c,name+strlen(normal));
    if(!strncmp(name,constant,strlen(constant)))return metadata_import(c,name+strlen(constant));
    return 0;
}
