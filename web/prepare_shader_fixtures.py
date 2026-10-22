"""Stage local diagnostic shader captures; all game-derived bytes stay in build/."""
import argparse
import base64
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--count',type=int,default=16)
    parser.add_argument('--spread',action='store_true',help='sample across the entire captured run')
    args=parser.parse_args()
    if not 1<=args.count<=256:parser.error('count must be 1..256')
    files=sorted((ROOT/'build/maxwell-capture').glob('*.mxd'))
    if args.spread and args.count>1 and len(files)>args.count:
        files=[files[index*(len(files)-1)//(args.count-1)] for index in range(args.count)]
    else:files=files[:args.count]
    if not files:parser.error('No local MXD1 shader captures')
    fixtures=[]
    for path in files:
        data=path.read_bytes()
        fixtures.append(dict(name=path.name,sha256=hashlib.sha256(data).hexdigest(),bytes=base64.b64encode(data).decode()))
    output=ROOT/'build/web-poc/maxwell-fixtures.json'
    output.write_text(json.dumps(dict(scope='Local diagnostic shader captures; compilation only, not gameplay',fixtures=fixtures))+'\n')
    print(f'Staged {len(fixtures)} captured shaders under ignored build/web-poc')


if __name__=='__main__':main()
