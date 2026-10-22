"""Build an ignored local font variant with known kern/palt records for ABI tests.

The source font is a user input. This script ships no font data; its output must
stay under build/. Checksums are rebuilt, including head.checkSumAdjustment.
"""
import struct


def words(*values):
    return struct.pack('>'+'H'*len(values),*(n & 0xffff for n in values))


def glyph_id(data,code):
    n=struct.unpack_from('>H',data,4)[0]
    records=[struct.unpack_from('>4sIII',data,12+i*16) for i in range(n)]
    cmap=next(offset for tag,_,offset,_ in records if tag==b'cmap')
    entries=struct.unpack_from('>H',data,cmap+2)[0]
    candidates=[struct.unpack_from('>HHI',data,cmap+4+i*8) for i in range(entries)]
    for platform,encoding,offset in sorted(candidates,key=lambda r:r[:2]!=(3,10)):
        if platform!=3 or encoding not in (1,10):continue
        sub=cmap+offset;fmt=struct.unpack_from('>H',data,sub)[0]
        if fmt==12:
            groups=struct.unpack_from('>I',data,sub+12)[0]
            for i in range(groups):
                first,last,start=struct.unpack_from('>III',data,sub+16+i*12)
                if first<=code<=last:return start+code-first
        elif fmt==4:
            count=struct.unpack_from('>H',data,sub+6)[0]//2
            ends=sub+14;starts=ends+2*count+2;deltas=starts+2*count;ranges=deltas+2*count
            for i in range(count):
                end=struct.unpack_from('>H',data,ends+2*i)[0]
                start=struct.unpack_from('>H',data,starts+2*i)[0]
                if start<=code<=end:
                    delta=struct.unpack_from('>h',data,deltas+2*i)[0]
                    offset=struct.unpack_from('>H',data,ranges+2*i)[0]
                    if not offset:return (code+delta)&0xffff
                    glyph=struct.unpack_from('>H',data,ranges+2*i+offset+2*(code-start))[0]
                    return (glyph+delta)&0xffff if glyph else 0
    raise ValueError('Fixture source needs A and B in a Unicode cmap')


def checksum(data):
    padded=data+b'\0'*((-len(data))%4)
    return sum(struct.unpack('>'+'I'*(len(padded)//4),padded))&0xffffffff


def make_fixture(source,target):
    data=source.read_bytes();a=glyph_id(data,ord('A'));b=glyph_id(data,ord('B'))
    coverage=words(1,1,a)
    pair=words(1,20,5,0,1,12)+words(1,b,-2,-80)+coverage
    single=words(2,12,5,1,5,20)+coverage
    lookup_pair=words(2,0,1,8)+pair
    lookup_single=words(1,0,1,8)+single
    lookups=words(2,6,6+len(lookup_pair))+lookup_pair+lookup_single
    features=words(2)+b'kern'+words(14)+b'palt'+words(20)+words(0,1,0)+words(0,1,1)
    scripts=words(1)+b'latn'+words(8)+words(4,0)+words(0,0xffff,2,0,1)
    gpos=struct.pack('>I',0x10000)+words(10,10+len(scripts),10+len(scripts)+len(features))+scripts+features+lookups
    tables={}
    for i in range(struct.unpack_from('>H',data,4)[0]):
        tag,_,offset,length=struct.unpack_from('>4sIII',data,12+i*16)
        tables[tag]=data[offset:offset+length]
    tables[b'GPOS']=gpos
    head=bytearray(tables[b'head']);struct.pack_into('>I',head,8,0);tables[b'head']=bytes(head)
    n=len(tables);power=1<<(n.bit_length()-1)
    output=bytearray(data[:4]+words(n,power*16,power.bit_length()-1,n*16-power*16)+b'\0'*(n*16))
    head_at=0
    for i,(tag,body) in enumerate(sorted(tables.items())):
        offset=len(output);struct.pack_into('>4sIII',output,12+i*16,tag,checksum(body),offset,len(body))
        output.extend(body);output.extend(b'\0'*((-len(output))%4))
        if tag==b'head':head_at=offset
    struct.pack_into('>I',output,head_at+8,(0xb1b0afba-checksum(bytes(output)))&0xffffffff)
    assert checksum(bytes(output))==0xb1b0afba
    target.write_bytes(output)
