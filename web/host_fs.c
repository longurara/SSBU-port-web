/* Read-only extracted RomFS backed by the owner's WORKERFS/NodeFS mount. POSIX
 * I/O runs through Emscripten's main-runtime-thread proxy; offsets stay 64-bit. */
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include "runtime.h"
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define NOT_FOUND 0x202u
#define NOT_MOUNTED 0x35f202u
#define DENIED 0x320002u
#define IO_ERROR 0x3e802u
typedef struct { int fd, used, writable; uint32_t generation; } GuestFile;
typedef struct { int used, mode; char path[1024]; DIR *dir; } GuestDir;
typedef struct { int used, kind, fd, eof, err, ungot; int64_t pos, size; } StdioStream;
static GuestFile files[256];
static GuestDir dirs[64];
static StdioStream std_streams[3] = {
    {1, 1, -1, 0, 0, -1, 0, 0},
    {1, 2, -1, 0, 0, -1, 0, 0},
    {1, 3, -1, 0, 0, -1, 0, 0},
};
static StdioStream file_streams[64];
static pthread_mutex_t fs_lock = PTHREAD_MUTEX_INITIALIZER;
static char mount_name[16];
static char save_mount[16];
static char cache_mount[16];
static int mounted;
static atomic_int account_user_open;

static void string_at(uint64_t address, char *out, size_t size) {
    for (size_t i=0;i<size;i++) { out[i]=(char)rd8(address+i); if (!out[i]) return; }
    guest_trap("file path too long",address);
}
static uint32_t resolve_path_ex(uint64_t address, char out[1024], int *writable) {
    char guest[800]; string_at(address,guest,sizeof guest);
    const char *colon=strchr(guest,':');
    if (!colon) return NOT_MOUNTED;
    size_t mlen=(size_t)(colon-guest);
    const char *root=NULL;
    if (mounted && mlen==strlen(mount_name) && !strncmp(guest,mount_name,mlen)) { root="/romfs"; if (writable) *writable=0; }
    else if (save_mount[0] && mlen==strlen(save_mount) && !strncmp(guest,save_mount,mlen)) { root="/userdata/save"; if (writable) *writable=1; }
    else if (cache_mount[0] && mlen==strlen(cache_mount) && !strncmp(guest,cache_mount,mlen)) { root="/userdata/cache"; if (writable) *writable=1; }
    else return NOT_MOUNTED;
    const char *relative=colon+1; while (*relative=='/') relative++;
    if (strchr(relative,'\\') || strchr(relative,':')) return NOT_FOUND;
    const char *part=relative;
    while (*part) {
        const char *end=strchr(part,'/'); size_t length=end?(size_t)(end-part):strlen(part);
        if ((length==1 && part[0]=='.') || (length==2 && part[0]=='.' && part[1]=='.')) return NOT_FOUND;
        if (!end) break; part=end+1;
    }
    if (*relative) snprintf(out,1024,"%s/%s",root,relative);
    else snprintf(out,1024,"%s",root);
    return 0;
}
static uint32_t resolve_path(uint64_t address, char out[1024]) {
    return resolve_path_ex(address, out, NULL);
}
static uint32_t buckets(uint32_t count) {
    if (count<3) return 3; if (count<19) return count|1;
    while (!(count%2)||!(count%3)||!(count%5)||!(count%7)||!(count%11)||!(count%13)||!(count%17)) count++;
    return count;
}
static int count_tree(const char *path,uint32_t *directories,uint32_t *file_count,uint64_t *dir_bytes,uint64_t *file_bytes) {
    DIR *directory=opendir(path); if (!directory) return 0;
    struct dirent *entry;
    while ((entry=readdir(directory))) {
        if (!strcmp(entry->d_name,".")||!strcmp(entry->d_name,"..")) continue;
        char child[1024]; if (snprintf(child,sizeof child,"%s/%s",path,entry->d_name)>=(int)sizeof child) {closedir(directory);return 0;}
        struct stat status; if (stat(child,&status)) {closedir(directory);return 0;}
        size_t name_size=(strlen(entry->d_name)+3)&~(size_t)3;
        if (S_ISDIR(status.st_mode)) {(*directories)++;*dir_bytes+=24+name_size;if(!count_tree(child,directories,file_count,dir_bytes,file_bytes)){closedir(directory);return 0;}}
        else if (S_ISREG(status.st_mode)) {(*file_count)++;*file_bytes+=32+name_size;}
    }
    closedir(directory); return 1;
}
static GuestFile *file_for(uint64_t handle) {
    unsigned index=(unsigned)handle;
    if (!index||index>256||!files[index-1].used||files[index-1].generation!=(uint32_t)(handle>>32)) return NULL;
    return &files[index-1];
}
static uint64_t read_file(uint64_t handle,uint64_t offset,uint64_t buffer,uint64_t size,int *error) {
    GuestFile *file=file_for(handle); *error=0;
    if(!file || offset>INT64_MAX || size>INT64_MAX-offset){*error=1;return 0;}
    static unsigned char staging[65536]; uint64_t total=0;
    while(total<size) {
        size_t chunk=size-total<sizeof staging?(size_t)(size-total):sizeof staging;
        ssize_t count=pread(file->fd,staging,chunk,(off_t)(offset+total));
        if(count<0){*error=1;break;} if(!count)break;
        web_write(buffer+total,staging,(size_t)count);total+=(uint64_t)count;
    }
    return total;
}
int web_fs_import(Ctx *c,const char *name) {
    if(!strcmp(name,"_ZN2nn4nifm10InitializeEv")||!strcmp(name,"_ZN2nn4nifm18IsNetworkAvailableEv")||
       !strcmp(name,"_ZN2nn4nifm22IsNetworkRequestOnHoldEv")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn4nifm20SubmitNetworkRequestEv")||!strcmp(name,"_ZN2nn4nifm27SubmitNetworkRequestAndWaitEv")||
       !strcmp(name,"_ZN2nn4nifm20CancelNetworkRequestEv"))return 1;
    if(!strcmp(name,"_ZN2nn7account10InitializeEv")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7account12ListAllUsersEPiPNS0_3UidEi")) {
        wr32(c->x[0],1);if((int)c->x[2]>0){wr64(c->x[1],1);wr64(c->x[1]+8,0);}
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account22TryOpenPreselectedUserEPNS0_10UserHandleE")) {
        wr64(c->x[0],1);wr64(c->x[0]+8,0);wr64(c->x[0]+16,0);atomic_store(&account_user_open,1);c->x[0]=1;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account8OpenUserEPNS0_10UserHandleERKNS0_3UidE")) {
        wr64(c->x[0],rd64(c->x[1]));wr64(c->x[0]+8,rd64(c->x[1]+8));wr64(c->x[0]+16,1);atomic_store(&account_user_open,1);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account9CloseUserERKNS0_10UserHandleE")) {
        atomic_store(&account_user_open,0);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account9GetUserIdEPNS0_3UidERKNS0_10UserHandleE")) {
        wr64(c->x[0],rd64(c->x[1]));wr64(c->x[0]+8,rd64(c->x[1]+8));c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account13GetLastOpenedUserEPNS0_3UidE")||
       !strcmp(name,"_ZN2nn7account16ShowUserSelectorEPNS0_3UidERKNS0_18UserSelectionSettingsE")||
       !strcmp(name,"_ZN2nn7account16ShowUserSelectorEPNS0_3UidERKNS0_21UserSelectionSettingsE")) {
        if(c->x[0]){wr64(c->x[0],1);wr64(c->x[0]+8,0);}c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account15ShowUserCreatorEv")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7account11GetNicknameEPNS0_8NicknameERKNS0_3UidE")) {
        static const char nickname[]="Player";
        for(unsigned i=0;i<33;i++)wr8(c->x[0]+i,0);
        web_write(c->x[0],nickname,sizeof nickname-1);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account16GetUserExistenceEPbRKNS0_3UidE")){wr8(c->x[0],1);c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7account12AsyncContext7HasDoneEPb")){if(c->x[1])wr8(c->x[1],1);c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7account32IsNetworkServiceAccountAvailableEPbRKNS0_10UserHandleE")){wr8(c->x[0],0);c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7account36EnsureNetworkServiceAccountAvailableERKNS0_10UserHandleE")||
       !strcmp(name,"_ZN2nn7account44EnsureNetworkServiceAccountIdTokenCacheAsyncEPNS0_12AsyncContextERKNS0_10UserHandleE")||
       !strcmp(name,"_ZN2nn7account26GetNetworkServiceAccountIdEPNS0_23NetworkServiceAccountIdERKNS0_10UserHandleE")||
       !strcmp(name,"_ZN2nn7account29LoadCachedNintendoAccountInfoEPNS0_25CachedNintendoAccountInfoERKNS0_10UserHandleEPvm")||
       !strcmp(name,"_ZN2nn7account12AsyncContext9GetResultEv")){c->x[0]=0x1907cULL;return 1;}
    if(!strcmp(name,"_ZN2nn7account37LoadNetworkServiceAccountIdTokenCacheEPmPcmmRKNS0_10UserHandleE")||
       !strcmp(name,"_ZN2nn7account37LoadNetworkServiceAccountIdTokenCacheEPmPcmRKNS0_10UserHandleE")) {
        if(c->x[0])wr64(c->x[0],0);c->x[0]=0x1907cULL;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account12AsyncContext6CancelEv")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn7accounteqERKNS0_3UidES3_")) {
        c->x[0]=(rd64(c->x[0])==rd64(c->x[1])&&rd64(c->x[0]+8)==rd64(c->x[1]+8))?1:0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7accountneERKNS0_3UidES3_")) {
        c->x[0]=(rd64(c->x[0])!=rd64(c->x[1])||rd64(c->x[0]+8)!=rd64(c->x[1]+8))?1:0;return 1;
    }
    if(!strcmp(name,"_ZN2nn7account25CachedNintendoAccountInfoC1Ev")||
       !strcmp(name,"_ZN2nn7account12AsyncContextC1Ev")||!strcmp(name,"_ZN2nn7account12AsyncContextD1Ev")||
       !strcmp(name,"_ZN2nn7account12AsyncContextaSEOS1_")||!strcmp(name,"_ZN2nn7account12AsyncContextC1EOS1_"))return 1;
    if(!strcmp(name,"fopen")||!strcmp(name,"freopen")||!strcmp(name,"fclose")||!strcmp(name,"fread")||
       !strcmp(name,"getc")||!strcmp(name,"fgetc")||!strcmp(name,"ungetc")||!strcmp(name,"feof")||!strcmp(name,"ferror")) {
        pthread_mutex_lock(&fs_lock);
        uint64_t handle = (!strcmp(name,"freopen") ? c->x[2] : !strcmp(name,"fread") ? c->x[3] : !strcmp(name,"ungetc") ? c->x[1] : c->x[0]);
        StdioStream *s = NULL;
        if (handle == 0x7700000010ULL) s = &std_streams[0];
        else if (handle == 0x7700000020ULL) s = &std_streams[1];
        else if (handle == 0x7700000030ULL) s = &std_streams[2];
        else if (handle >= 0x7700000100ULL && handle < 0x7700000100ULL + 64ULL * 16ULL && ((handle - 0x7700000100ULL) & 15ULL) == 0) {
            unsigned idx = (unsigned)((handle - 0x7700000100ULL) / 16ULL);
            if (file_streams[idx].used) s = &file_streams[idx];
        }
        if (!strcmp(name,"fopen") || !strcmp(name,"freopen")) {
            unsigned slot = 64;
            if (!strcmp(name,"fopen")) {
                for (unsigned i = 0; i < 64; i++) if (!file_streams[i].used) { slot = i; s = &file_streams[i]; break; }
            }
            if (!s) { pthread_mutex_unlock(&fs_lock); c->x[0] = 0; return 1; }
            if (!strcmp(name,"freopen") && !c->x[0]) {
                s->pos = 0; s->eof = s->err = 0; s->ungot = -1;
                pthread_mutex_unlock(&fs_lock); c->x[0] = handle; return 1;
            }
            char mode[16] = {0}, path[1024];
            if (c->x[1]) string_at(c->x[1], mode, sizeof mode);
            int ok = c->x[0] && mode[0] == 'r' && !strchr(mode, '+') && !resolve_path(c->x[0], path);
            if (!strcmp(name,"freopen") && s->kind == 0 && s->fd >= 0) { close(s->fd); s->fd = -1; }
            int fd = ok ? open(path, O_RDONLY) : -1;
            struct stat st;
            if (fd >= 0 && fstat(fd, &st)) { close(fd); fd = -1; }
            if (fd < 0) {
                if (s->kind == 0) s->used = 0;
                pthread_mutex_unlock(&fs_lock); c->x[0] = 0; return 1;
            }
            s->used = 1; s->kind = 0; s->fd = fd; s->pos = 0; s->size = (int64_t)st.st_size;
            s->eof = s->err = 0; s->ungot = -1;
            pthread_mutex_unlock(&fs_lock);
            c->x[0] = !strcmp(name,"fopen") ? (0x7700000100ULL + (uint64_t)slot * 16ULL) : handle;
            return 1;
        }
        if (!strcmp(name,"fclose")) {
            if (!s) { pthread_mutex_unlock(&fs_lock); c->x[0] = (uint64_t)-1LL; return 1; }
            if (s->kind == 0) { if (s->fd >= 0) close(s->fd); s->fd = -1; s->used = 0; }
            pthread_mutex_unlock(&fs_lock); c->x[0] = 0; return 1;
        }
        if (!strcmp(name,"fread") || !strcmp(name,"getc") || !strcmp(name,"fgetc")) {
            int is_getc = strcmp(name,"fread") != 0;
            uint64_t dst = is_getc ? 0 : c->x[0], size = is_getc ? 1 : c->x[1], count = is_getc ? 1 : c->x[2];
            size_t n = (size_t)(size * count), done = 0;
            uint8_t ch = 0;
            if (!s || !size || !count) { pthread_mutex_unlock(&fs_lock); c->x[0] = is_getc ? 0xffffffffULL : 0; return 1; }
            if (s->ungot >= 0) {
                ch = (uint8_t)s->ungot; s->ungot = -1;
                if (!is_getc) wr8(dst, ch);
                done = 1;
            }
            if (s->kind != 0 || s->fd < 0) {
                if (done < n) s->eof = 1;
            } else if (done < n) {
                int64_t want = (int64_t)(n - done);
                if (want > s->size - s->pos) want = s->size - s->pos;
                while (want > 0) {
                    static unsigned char staging[65536];
                    size_t chunk = (size_t)want < sizeof staging ? (size_t)want : sizeof staging;
                    ssize_t got = pread(s->fd, staging, chunk, (off_t)s->pos);
                    if (got < 0) { s->err = 1; break; }
                    if (got == 0) break;
                    if (is_getc) ch = staging[0];
                    else web_write(dst + done, staging, (size_t)got);
                    s->pos += (int64_t)got; done += (size_t)got; want -= (int64_t)got;
                }
                if (done < n && !s->err) s->eof = 1;
            }
            pthread_mutex_unlock(&fs_lock);
            c->x[0] = is_getc ? (done == 1 ? (uint64_t)ch : 0xffffffffULL) : (done / size);
            return 1;
        }
        if (!strcmp(name,"ungetc")) {
            if (!s || (int32_t)c->x[0] == -1 || s->ungot >= 0) { pthread_mutex_unlock(&fs_lock); c->x[0] = 0xffffffffULL; return 1; }
            s->ungot = (int)(uint8_t)c->x[0]; s->eof = 0;
            pthread_mutex_unlock(&fs_lock); c->x[0] = (uint8_t)c->x[0]; return 1;
        }
        if (!strcmp(name,"feof")) { uint64_t r = s ? (uint64_t)s->eof : 0; pthread_mutex_unlock(&fs_lock); c->x[0] = r; return 1; }
        if (!strcmp(name,"ferror")) { uint64_t r = s ? (uint64_t)s->err : 0; pthread_mutex_unlock(&fs_lock); c->x[0] = r; return 1; }
        pthread_mutex_unlock(&fs_lock);
    }
    if(strncmp(name,"_ZN2nn2fs",sizeof("_ZN2nn2fs")-1))return 0;
    if(!strcmp(name,"_ZN2nn2fs12SetAllocatorEPFPvmEPFvS1_mE")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn2fs22QueryMountRomCacheSizeEPm")) {
        uint32_t directories=1,count=0;uint64_t dirs_sz=24,entries=0;
        pthread_mutex_lock(&fs_lock);int ok=count_tree("/romfs",&directories,&count,&dirs_sz,&entries);pthread_mutex_unlock(&fs_lock);
        if(ok)wr64(c->x[0],4ULL*buckets(directories)+dirs_sz+4ULL*buckets(count)+entries);
        c->x[0]=ok?0:NOT_FOUND;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs8MountRomEPKcPvm")) {
        char requested[16];string_at(c->x[0],requested,sizeof requested);
        pthread_mutex_lock(&fs_lock);struct stat status;uint32_t result=0;
        if(mounted)result=0x7802;else if(stat("/romfs",&status)||!S_ISDIR(status.st_mode))result=NOT_FOUND;
        else {strcpy(mount_name,requested);mounted=1;}
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs18IsSaveDataExistingEPbRKNS_7account3UidE")) {
        struct stat status;
        pthread_mutex_lock(&fs_lock);int exists=(stat("/userdata/save",&status)==0&&S_ISDIR(status.st_mode));pthread_mutex_unlock(&fs_lock);
        wr8(c->x[0],(uint8_t)exists);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs18IsSaveDataExistingERKNS_7account3UidE")) {
        struct stat status;
        pthread_mutex_lock(&fs_lock);int exists=(stat("/userdata/save",&status)==0&&S_ISDIR(status.st_mode));pthread_mutex_unlock(&fs_lock);
        c->x[0]=(uint64_t)exists;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs14EnsureSaveDataERKNS_7account3UidEll")||
       !strcmp(name,"_ZN2nn2fs14EnsureSaveDataERKNS_7account3UidE")) {
        pthread_mutex_lock(&fs_lock);mkdir("/userdata",0777);mkdir("/userdata/save",0777);pthread_mutex_unlock(&fs_lock);
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs13MountSaveDataEPKcRKNS_7account3UidE")) {
        pthread_mutex_lock(&fs_lock);mkdir("/userdata",0777);mkdir("/userdata/save",0777);
        string_at(c->x[0],save_mount,sizeof save_mount);pthread_mutex_unlock(&fs_lock);
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs17MountCacheStorageEPKc")) {
        pthread_mutex_lock(&fs_lock);mkdir("/userdata",0777);mkdir("/userdata/cache",0777);
        string_at(c->x[0],cache_mount,sizeof cache_mount);pthread_mutex_unlock(&fs_lock);
        c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs7UnmountEPKc")) {
        char requested[16];string_at(c->x[0],requested,sizeof requested);
        pthread_mutex_lock(&fs_lock);
        if(mounted&&!strcmp(requested,mount_name)){mounted=0;mount_name[0]=0;}
        else if(save_mount[0]&&!strcmp(requested,save_mount))save_mount[0]=0;
        else if(cache_mount[0]&&!strcmp(requested,cache_mount))cache_mount[0]=0;
        pthread_mutex_unlock(&fs_lock);c->x[0]=0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs6CommitEPKc")){c->x[0]=0;return 1;}
    if(!strcmp(name,"_ZN2nn2fs10CreateFileEPKcl")) {
        char path[1024];int writable=0;pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path_ex(c->x[0],path,&writable);
        if(!result&&!writable)result=DENIED;
        if(!result){
            int fd=open(path,O_CREAT|O_TRUNC|O_RDWR,0666);
            if(fd<0)result=IO_ERROR;
            else{if((int64_t)c->x[1]>0)ftruncate(fd,(off_t)c->x[1]);close(fd);}
        }
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs10DeleteFileEPKc")) {
        char path[1024];int writable=0;pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path_ex(c->x[0],path,&writable);
        if(!result&&!writable)result=DENIED;
        if(!result&&unlink(path))result=NOT_FOUND;
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs15CreateDirectoryEPKc")) {
        char path[1024];int writable=0;pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path_ex(c->x[0],path,&writable);
        if(!result&&!writable)result=DENIED;
        if(!result)mkdir(path,0777);
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs15DeleteDirectoryEPKc")) {
        char path[1024];int writable=0;pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path_ex(c->x[0],path,&writable);
        if(!result&&!writable)result=DENIED;
        if(!result)rmdir(path);
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci")) {
        char path[1024];int writable=0;pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path_ex(c->x[1],path,&writable);
        if(!result && (c->x[2]&2) && !writable)result=DENIED;
        int descriptor=-1;if(!result){descriptor=open(path,(c->x[2]&2)?O_RDWR:O_RDONLY);if(descriptor<0)result=NOT_FOUND;}
        if(!result){unsigned i;for(i=0;i<256&&files[i].used;i++){}if(i==256){close(descriptor);result=IO_ERROR;}else{files[i].fd=descriptor;files[i].used=1;files[i].writable=writable;files[i].generation++;wr64(c->x[0],((uint64_t)files[i].generation<<32)|(i+1));}}
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs9CloseFileENS0_10FileHandleE")) {
        pthread_mutex_lock(&fs_lock);GuestFile *file=file_for(c->x[0]);if(file){close(file->fd);file->used=0;}pthread_mutex_unlock(&fs_lock);return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs11GetFileSizeEPlNS0_10FileHandleE")) {
        pthread_mutex_lock(&fs_lock);GuestFile *file=file_for(c->x[1]);struct stat status;
        int ok=file&&!fstat(file->fd,&status);if(ok)wr64(c->x[0],(uint64_t)status.st_size);pthread_mutex_unlock(&fs_lock);c->x[0]=ok?0:IO_ERROR;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm")) {
        pthread_mutex_lock(&fs_lock);int error;uint64_t got=read_file(c->x[0],c->x[1],c->x[2],c->x[3],&error);pthread_mutex_unlock(&fs_lock);
        c->x[0]=error?IO_ERROR:got==c->x[3]?0:0x177a02;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs8ReadFileEPmNS0_10FileHandleElPvm")) {
        pthread_mutex_lock(&fs_lock);int error;uint64_t got=read_file(c->x[1],c->x[2],c->x[3],c->x[4],&error);pthread_mutex_unlock(&fs_lock);
        wr64(c->x[0],got);c->x[0]=error?IO_ERROR:0;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs9WriteFileENS0_10FileHandleElPKvmRKNS0_11WriteOptionE")) {
        pthread_mutex_lock(&fs_lock);GuestFile *file=file_for(c->x[0]);
        uint32_t result=(file&&file->writable)?0:DENIED;
        if(!result){
            static unsigned char staging[65536];uint64_t offset=c->x[1],buffer=c->x[2],size=c->x[3],total=0;
            while(total<size){
                size_t chunk=size-total<sizeof staging?(size_t)(size-total):sizeof staging;
                web_read(buffer+total,staging,chunk);
                ssize_t w=pwrite(file->fd,staging,chunk,(off_t)(offset+total));
                if(w<=(ssize_t)0){result=IO_ERROR;break;}
                total+=(uint64_t)w;
            }
        }
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs12GetEntryTypeEPNS0_18DirectoryEntryTypeEPKc")) {
        char path[1024];pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path(c->x[1],path);struct stat status;
        if(!result&&stat(path,&status))result=NOT_FOUND;if(!result)wr32(c->x[0],S_ISDIR(status.st_mode)?0:1);
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs13OpenDirectoryEPNS0_15DirectoryHandleEPKci")) {
        char path[1024];pthread_mutex_lock(&fs_lock);uint32_t result=resolve_path(c->x[1],path);
        DIR *d=result?NULL:opendir(path);if(!result&&!d)result=NOT_FOUND;
        if(!result){
            unsigned i;for(i=0;i<64&&dirs[i].used;i++);
            if(i==64){closedir(d);result=IO_ERROR;}
            else{dirs[i].used=1;dirs[i].mode=(int)c->x[2];strcpy(dirs[i].path,path);dirs[i].dir=d;wr64(c->x[0],i+1);}
        }
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs22GetDirectoryEntryCountEPlNS0_15DirectoryHandleE")) {
        pthread_mutex_lock(&fs_lock);unsigned idx=(unsigned)c->x[1];
        GuestDir *gd=(idx>=1&&idx<=64&&dirs[idx-1].used)?&dirs[idx-1]:NULL;
        uint32_t result=gd?0:IO_ERROR;int64_t count=0;
        if(gd){
            DIR *d=opendir(gd->path);
            if(d){
                struct dirent *e;
                while((e=readdir(d))){
                    if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
                    char child[1024];if(snprintf(child,sizeof child,"%s/%s",gd->path,e->d_name)>=(int)sizeof child)continue;
                    struct stat st;if(stat(child,&st))continue;
                    int is_dir=S_ISDIR(st.st_mode);
                    if((is_dir&&(gd->mode&1))||(!is_dir&&(gd->mode&2)))count++;
                }
                closedir(d);
            }
            wr64(c->x[0],(uint64_t)count);
        }
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs13ReadDirectoryEPlPNS0_14DirectoryEntryENS0_15DirectoryHandleEl")) {
        pthread_mutex_lock(&fs_lock);unsigned idx=(unsigned)c->x[2];
        GuestDir *gd=(idx>=1&&idx<=64&&dirs[idx-1].used)?&dirs[idx-1]:NULL;
        uint32_t result=gd?0:IO_ERROR;int64_t read_count=0,max_entries=(int64_t)c->x[3];uint64_t out_entries=c->x[1];
        if(gd&&gd->dir){
            struct dirent *e;
            while(read_count<max_entries&&(e=readdir(gd->dir))){
                if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
                char child[1024];if(snprintf(child,sizeof child,"%s/%s",gd->path,e->d_name)>=(int)sizeof child)continue;
                struct stat st;if(stat(child,&st))continue;
                int is_dir=S_ISDIR(st.st_mode);
                if(!((is_dir&&(gd->mode&1))||(!is_dir&&(gd->mode&2))))continue;
                uint64_t ent=out_entries+(uint64_t)read_count*0x310ULL;
                for(unsigned k=0;k<0x310;k+=8)wr64(ent+k,0);
                size_t nlen=strlen(e->d_name);if(nlen>0x300)nlen=0x300;
                web_write(ent,e->d_name,nlen);
                wr8(ent+0x304,is_dir?0:1);
                wr64(ent+0x308,is_dir?0:(uint64_t)st.st_size);
                read_count++;
            }
            wr64(c->x[0],(uint64_t)read_count);
        }
        pthread_mutex_unlock(&fs_lock);c->x[0]=result;return 1;
    }
    if(!strcmp(name,"_ZN2nn2fs14CloseDirectoryENS0_15DirectoryHandleE")) {
        pthread_mutex_lock(&fs_lock);unsigned idx=(unsigned)c->x[0];
        if(idx>=1&&idx<=64&&dirs[idx-1].used){if(dirs[idx-1].dir)closedir(dirs[idx-1].dir);dirs[idx-1].used=0;}
        pthread_mutex_unlock(&fs_lock);c->x[0]=0;return 1;
    }
    return 0;
}
