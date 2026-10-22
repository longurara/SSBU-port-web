"""Diagnose actual cached SPIR-V -> WGSL for TRIANGLE pipelines only.

PointSize is preserved as an unused varying, not silently claimed to work for
point primitives. A runtime pipeline must reject point topology until expansion
exists. Push constants are moved to an explicit uniform binding (group 3/0).
All derived shader bytes stay under ignored build/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def instructions(words):
    cursor=5
    while cursor<len(words):
        count,opcode=words[cursor]>>16,words[cursor]&65535
        if not count or cursor+count>len(words):raise ValueError('Malformed SPIR-V')
        yield opcode,list(words[cursor+1:cursor+count])
        cursor+=count


def lower(data):
    words=list(struct.unpack('<%dI'%(len(data)//4),data))
    if words[0]!=0x07230203:raise ValueError('Expected SPIR-V')
    ops=list(instructions(words))
    pointers={args[0]:args[1] for op,args in ops if op==32}
    outputs={args[1] for op,args in ops if op==59 and pointers.get(args[0])==3}
    used={args[2] for op,args in ops if op==71 and args[0] in outputs and args[1]==30}
    for op,args in ops:
        if op==72 and args[2]==30:used.add(args[3])
    free=next((slot for slot in range(16) if slot not in used),None)
    patches=[];push=[]
    for i,(op,args) in enumerate(ops):
        if op==72 and args[2:]==[11,1]:
            if free is None:raise ValueError('No diagnostic varying slot for PointSize')
            ops[i]=(op,[args[0],args[1],30,free]);patches.append(dict(kind='point-size-varying',location=free))
        if op==32 and args[1]==9:ops[i]=(op,[args[0],2,args[2]])
        if op==59 and args[2]==9:
            ops[i]=(op,[args[0],args[1],2,*args[3:]]);push.append(args[1])
    if len(push)>1:raise ValueError('Multiple push-constant blocks need binding allocation')
    if push:
        annotations=[(71,[push[0],34,3]),(71,[push[0],33,0])]
        first_type=next(i for i,(op,_) in enumerate(ops) if 19<=op<=39)
        ops[first_type:first_type]=annotations
        patches.append(dict(kind='push-constant-uniform',group=3,binding=0))
    # WGSL's floating-point optimisations do not preserve x != x as an isnan
    # implementation. Classify the original IEEE bits with unsigned operations.
    # OpIsNan/OpIsInf have scalar-bool or bool-vector result types.
    types={args[0]:(op,args[1:]) for op,args in ops if 19<=op<=39}
    declarations=[]
    def allocate():
        result=words[3];words[3]+=1;return result
    unsigned=next((key for key,value in types.items() if value==(21,[32,0])),None)
    if unsigned is None:
        unsigned=allocate();declarations.append((21,[unsigned,32,0]))
    uint_types={1:unsigned};constants={}
    def uint_type(width):
        if width not in uint_types:
            found=next((key for key,value in types.items() if value==(23,[unsigned,width])),None)
            if found is None:
                found=allocate();declarations.append((23,[found,unsigned,width]))
            uint_types[width]=found
        return uint_types[width]
    def constant(value,width):
        key=(value,width)
        if key not in constants:
            if width==1:
                result=allocate();declarations.append((43,[unsigned,result,value]))
            else:
                scalar=constant(value,1);kind=uint_type(width)
                result=allocate();declarations.append((44,[kind,result,*([scalar]*width)]))
            constants[key]=result
        return constants[key]
    rewritten=[];relational_count=0
    for op,args in ops:
        if op not in (156,157):rewritten.append((op,args));continue
        result_type,result,operand=args
        bool_op,bool_args=types[result_type]
        width=bool_args[1] if bool_op==23 else 1
        # Reject f16/f64 rather than interpreting their bits as f32. The source
        # operand's type is recorded by every instruction producing a value.
        value_type=next((a[0] for o,a in ops if o not in (71,72) and len(a)>=2 and a[1]==operand and a[0] in types),None)
        float_op,float_args=types.get(value_type,(None,[]))
        scalar_type=float_args[0] if float_op==23 else value_type
        if types.get(scalar_type)!=(22,[32]):raise ValueError('Only f32 classification is supported')
        integer=uint_type(width);bits=allocate();absolute=allocate()
        mask=constant(0x7fffffff,width);infinity=constant(0x7f800000,width)
        rewritten.extend([(124,[integer,bits,operand]),(199,[integer,absolute,bits,mask]),
                          (172 if op==156 else 170,[result_type,result,absolute,infinity])])
        relational_count+=1
    if relational_count:
        first_function=next(i for i,(op,_) in enumerate(rewritten) if op==54)
        rewritten[first_function:first_function]=declarations
        patches.append(dict(kind='ieee-f32-classification',operations=relational_count))
    ops=rewritten
    result=words[:5]
    for op,args in ops:result += [((len(args)+1)<<16)|op,*args]
    return struct.pack('<%dI'%len(result),*result),patches


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--count',type=int,default=16)
    parser.add_argument('--topology',choices=['triangles'],required=True)
    parser.add_argument('--compiler',choices=['naga','web'],default='naga')
    args=parser.parse_args()
    tool=ROOT/('deps/naga-cli/bin/naga.exe' if args.compiler=='naga' else 'deps/web-shader-build/release/ssbu-web-shader.exe')
    version=subprocess.check_output([str(tool),'--version'],text=True).strip()
    if version!=('29.0.4' if args.compiler=='naga' else 'ssbu-web-shader 0.1.0 (naga 29.0.4)'):raise ValueError('Expected pinned naga 29.0.4')
    out=ROOT/'build/web-shaders';out.mkdir(exist_ok=True)
    records=[]
    for source in sorted((ROOT/'build/native/cache/vk/spv').glob('*.spv'))[:args.count]:
        data=source.read_bytes();converted,patches=lower(data)
        lowered=out/(source.stem+'.spv');wgsl=out/(source.stem+'.wgsl')
        lowered.write_bytes(converted)
        run=subprocess.run([str(tool),str(lowered),str(wgsl)],capture_output=True,text=True)
        records.append(dict(hash=source.stem,sha256=hashlib.sha256(data).hexdigest(),ok=run.returncode==0,
            changes=patches,diagnostic=(run.stdout+run.stderr)[-1600:]))
    report=dict(tool=version,topology=args.topology,scope='Triangle shader conversion only; no points, GPU pipeline or rendering proof',
                converted=sum(item['ok'] for item in records),total=len(records),shaders=records)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
