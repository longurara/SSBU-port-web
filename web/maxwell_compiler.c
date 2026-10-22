/* Portable Maxwell -> Vulkan GLSL -> SPIR-V frontend. Outputs are temporary
 * compiler buffers, never pointers into the game's guest address space. */
#include "mx_translate.h"
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MxOut translated;
static uint32_t *spirv;
static size_t spirv_bytes;
static char diagnostic[4096], metadata[1024];
static int initialised;
static uint32_t word(const uint8_t *data,size_t offset) { uint32_t value;memcpy(&value,data+offset,4);return value; }
const char *mx_error(void) { return diagnostic; }
const char *mx_glsl(void) { return translated.glsl ? translated.glsl : ""; }
const char *mx_info(void) { return metadata; }
const uint32_t *mx_spirv(void) { return spirv; }
size_t mx_spirv_size(void) { return spirv_bytes; }

static int compile_glsl(int stage,const char *source) {
    if(!initialised) {initialised=glslang_initialize_process();if(!initialised){snprintf(diagnostic,sizeof diagnostic,"Cannot initialise glslang");return 0;}}
    glslang_input_t input={0};
    input.language=GLSLANG_SOURCE_GLSL;
    input.stage=stage==MX_VERTEX ? GLSLANG_STAGE_VERTEX : stage==MX_FRAGMENT ? GLSLANG_STAGE_FRAGMENT : GLSLANG_STAGE_COMPUTE;
    input.client=GLSLANG_CLIENT_VULKAN;
    // Vulkan 1.0 preserves GLSL discard as OpKill; no demote-helper extension
    // is introduced by targeting the newer SPIR-V 1.6 native backend.
    input.client_version=GLSLANG_TARGET_VULKAN_1_0;
    input.target_language=GLSLANG_TARGET_SPV;
    input.target_language_version=GLSLANG_TARGET_SPV_1_0;
    input.code=source;input.default_version=460;input.default_profile=GLSLANG_NO_PROFILE;
    input.messages=GLSLANG_MSG_DEFAULT_BIT|GLSLANG_MSG_SPV_RULES_BIT|GLSLANG_MSG_VULKAN_RULES_BIT;
    input.resource=glslang_default_resource();
    glslang_shader_t *shader=glslang_shader_create(&input);
    glslang_program_t *program=NULL;int ok=0;
    if(!shader){snprintf(diagnostic,sizeof diagnostic,"Cannot create GLSL compiler");return 0;}
    if(!glslang_shader_preprocess(shader,&input) || !glslang_shader_parse(shader,&input)) {
        snprintf(diagnostic,sizeof diagnostic,"%s%s",glslang_shader_get_info_log(shader),glslang_shader_get_info_debug_log(shader));goto done;
    }
    program=glslang_program_create();
    if(!program){snprintf(diagnostic,sizeof diagnostic,"Cannot create GLSL program");goto done;}
    glslang_program_add_shader(program,shader);
    if(!glslang_program_link(program,input.messages)) {
        snprintf(diagnostic,sizeof diagnostic,"%s%s",glslang_program_get_info_log(program),glslang_program_get_info_debug_log(program));goto done;
    }
    glslang_program_SPIRV_generate(program,input.stage);
    size_t count=glslang_program_SPIRV_get_size(program);
    if(count && count<4*1024*1024) {
        spirv=malloc(count*4);
        if(spirv){glslang_program_SPIRV_get(program,spirv);spirv_bytes=count*4;ok=1;}
    }
    if(!ok)snprintf(diagnostic,sizeof diagnostic,"%s",glslang_program_SPIRV_get_messages(program));
done:
    if(program)glslang_program_delete(program);
    glslang_shader_delete(shader);return ok;
}

int mx_compile(const uint8_t *control,size_t control_bytes,const uint8_t *data,size_t data_bytes,
               uint32_t int_attrs,uint32_t sint_attrs,uint32_t int_tex,uint32_t sint_tex,uint32_t force_out) {
    free(translated.glsl);memset(&translated,0,sizeof translated);
    free(spirv);spirv=NULL;spirv_bytes=0;diagnostic[0]=0;metadata[0]=0;
    if(!control || !data || control_bytes<0x720 || control_bytes>2*1024*1024 || data_bytes>8*1024*1024) {
        snprintf(diagnostic,sizeof diagnostic,"Invalid NVN stage input sizes");return 0;
    }
    uint32_t length=word(control,0x6f8),constant_size=word(control,0x6fc),constant_offset=word(control,0x700);
    int compute=word(control,0x714)==5;
    size_t prefix=compute ? 0 : 0x30;
    if(length<0x20 || length>4*1024*1024 || prefix>data_bytes || length>data_bytes-prefix ||
       (constant_size && (constant_offset>data_bytes || constant_size>data_bytes-constant_offset))) {
        snprintf(diagnostic,sizeof diagnostic,"NVN code or constants exceed supplied storage");return 0;
    }
    MxIn input={0};
    if(compute) {
        if(control_bytes<0x75c){snprintf(diagnostic,sizeof diagnostic,"Truncated compute control");return 0;}
        input.stage=MX_COMPUTE;memcpy(input.local_size,control+0x748,12);
        input.smem_bytes=word(control,0x754);input.lmem_bytes=word(control,0x758);
        if(input.smem_bytes>1024*1024 || input.lmem_bytes>1024*1024 ||
           input.local_size[0]>1536 || input.local_size[1]>1536 || input.local_size[2]>1536) {
            snprintf(diagnostic,sizeof diagnostic,"Compute resource declaration exceeds compiler limits");return 0;
        }
    } else {
        if(length<0x50){snprintf(diagnostic,sizeof diagnostic,"Truncated Maxwell program header");return 0;}
        unsigned type=(word(data,prefix)>>10)&15;
        input.stage=type==1 ? MX_VERTEX : type==5 ? MX_FRAGMENT : -1;
        if(input.stage<0){snprintf(diagnostic,sizeof diagnostic,"Unsupported Maxwell stage %u",type);return 0;}
    }
    input.code=data+prefix;input.code_len=(int)length;
    input.cdata=constant_size ? data+constant_offset : NULL;input.cdata_size=constant_size;
    input.target=1;unsigned base=input.stage==MX_COMPUTE ? 0 : 40u*(unsigned)input.stage;
    input.binding_base=base;input.ssbo_base=base+20;input.wmask_binding=base+38;
    input.tex_base=base;input.image_base=base+32;input.track_writes=1;
    input.int_attrs=int_attrs;input.sint_attrs=sint_attrs;input.int_tex=int_tex;input.sint_tex=sint_tex;input.force_out=force_out;
    if(!mx_translate(&input,&translated)) {
        snprintf(diagnostic,sizeof diagnostic,"%s: %s",translated.op,translated.why);return 0;
    }
    snprintf(metadata,sizeof metadata,"{\"stage\":%d,\"instructions\":%d,\"blocks\":%d,\"cbufs\":%u,\"ssbos\":%u,\"ssbosWritten\":%u,\"textures\":%u,\"images\":%u,\"imagesRead\":%u,\"imagesWritten\":%u,\"inputAttributes\":%u,\"outputAttributes\":%u,\"samplers\":%d}",
        input.stage,translated.insns,translated.blocks,translated.cbufs,translated.ssbos,translated.ssbo_written,translated.tex_units,translated.images,translated.images_read,translated.images_written,translated.in_attr,translated.out_attr,translated.nsmp);
    return compile_glsl(input.stage,translated.glsl);
}
