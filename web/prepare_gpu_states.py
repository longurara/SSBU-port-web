"""Reuse the native frontend's portable small-state implementation in Wasm.

The selected functions contain no renderer calls. Guest pointer memcpy/memset
operations become checked guest-memory operations, including cross-page states.
This keeps one state encoding for the native frontend and the web executor.
"""
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
PREFIXES=('nvnColorState','nvnBlendState','nvnChannelMaskState','nvnPolygonState',
          'nvnDepthStencilState','nvnVertexAttribState','nvnVertexStreamState','nvnMultisampleState')


def matching(text,start,opening='(',closing=')'):
    depth=0;quote=None;i=start
    while i<len(text):
        char=text[i]
        if quote:
            if char=='\\':i+=2;continue
            if char==quote:quote=None
        elif char in ('"',"'"):quote=char
        elif text.startswith('//',i):
            end=text.find('\n',i);i=len(text) if end<0 else end;continue
        elif text.startswith('/*',i):
            end=text.find('*/',i+2)
            if end<0:raise ValueError('Unterminated C comment')
            i=end+2;continue
        elif char==opening:depth+=1
        elif char==closing:
            depth-=1
            if depth==0:return i+1
        i+=1
    raise ValueError('Unterminated C delimiter')


def arguments(text):
    result=[];start=0;i=0
    while i<len(text):
        if text[i]=='(':i=matching(text,i);continue
        if text[i]==',':result.append(text[start:i].strip());start=i+1
        i+=1
    return [*result,text[start:].strip()]


def guest_expression(value):
    if value.startswith('P(') and matching(value,1)==len(value):return value[2:-1]
    return None


def memory_operations(body):
    cursor=0
    while match:=re.search(r'\b(memcpy|memset)\s*\(',body[cursor:]):
        start=cursor+match.start();opening=cursor+match.end()-1;end=matching(body,opening)
        args=arguments(body[opening+1:end-1])
        if len(args)!=3:raise ValueError('Unexpected memory call shape')
        destination=guest_expression(args[0]);source=guest_expression(args[1]);replacement=None
        if match[1]=='memcpy':
            if destination is not None and source is not None:raise ValueError('Guest-to-guest state copy needs explicit staging')
            if destination is not None:replacement=f'web_write({destination}, {args[1]}, {args[2]})'
            if source is not None:replacement=f'web_read({source}, {args[0]}, {args[2]})'
        elif destination is not None:replacement=f'state_fill({destination}, {args[1]}, {args[2]})'
        if replacement:body=body[:start]+replacement+body[end:];cursor=start+len(replacement)
        else:cursor=end
    if re.search(r'\bP\s*\(',body):raise ValueError('Unconverted guest pointer in state function')
    return body


def prepare():
    source=(ROOT/'runtime/nvn_gl.c').read_text(encoding='utf-8')
    bindings={name:function for name,function in re.findall(r'\{\s*"(nvn\w+)"\s*,\s*(\w+)\s*\}',source)
              if name.startswith(PREFIXES) and name!='nvnMultisampleStateGetSampleLocationsGrid'}
    definitions={match[1]:match for match in re.finditer(r'static\s+void\s+(\w+)\s*\([^)]*\)\s*\{',source)}
    functions={};pending=list(bindings.values())
    while pending:
        name=pending.pop()
        if name in functions:continue
        definition=definitions[name];opening=definition.end()-1;end=matching(source,opening,'{','}')
        body=source[definition.start():end]
        for called in re.findall(r'\b(\w+)\s*\(',body):
            if called in definitions and called!=name and called not in functions:pending.append(called)
        functions[name]=memory_operations(body)
    header='''/* Generated from the native frontend's small-state functions. */
#include "runtime.h"
#include <string.h>
static void state_fill(uint64_t address, int value, size_t size) {
    unsigned char bytes[64];
    if(size>sizeof bytes)guest_trap("small-state fill exceeds limit",address);
    memset(bytes,value,size);web_write(address,bytes,size);
}
#define RET(value) (c->x[0]=(uint64_t)(value))
'''+''.join(f'#define A{i} (c->x[{i}])\n' for i in range(8))
    declarations='\n'.join(body[:body.index('{')].rstrip()+';' for body in functions.values())
    table='\n'.join(f'    {{"{name}",{function}}},' for name,function in sorted(bindings.items()))
    content=header+'\n'+declarations+'\n\n'+'\n'.join(functions.values())+'''
int web_gpu_states_import(Ctx *c,const char *name) {
    static const struct {const char *name;void (*call)(Ctx *);} entries[]={
'''+table+'''
    };
    for(unsigned i=0;i<sizeof entries/sizeof *entries;i++)if(!strcmp(name,entries[i].name)){entries[i].call(c);return 1;}
    return 0;
}
'''
    output=ROOT/'build/gpu-states';output.mkdir(exist_ok=True,parents=True)
    path=output/'gpu_states.c';path.write_text(content,encoding='utf-8',newline='\n')
    (output/'manifest.json').write_text(json.dumps(dict(functions=len(functions),entryPoints=len(bindings),sourceSha256=hashlib.sha256(source.encode()).hexdigest()),indent=2)+'\n')
    return path


if __name__=='__main__':print(prepare())
