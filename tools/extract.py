"""Recover the BASIC PRINT-built screen from the original executable.
Development tool only; the native game does not load or execute DOS code.
"""
from pathlib import Path
import re, struct, subprocess
ROOT=Path(__file__).resolve().parents[1]
b=(ROOT/'original/pac-gal/Pac-gal.05.1982.exe').read_bytes()
ds=0x83b0
regs=dict(ax=0,bx=0,cx=0,dx=0); local={}
strings={0x9d8:b' ',0x9dc:bytes([219]),0x9e0:b' '+bytes([186]),0x9e4:bytes([196]),0x9f0:b' '+bytes([249]),0x9f4:b' '+bytes([219]),0x9f8:bytes([205,205]),0x9fc:bytes([205])}
def desc(n):
    if n in strings:return strings[n]
    size,ptr=struct.unpack_from('<HH',b,ds+n)
    return b[ds+ptr:ds+ptr+size]
def val(s):
    if s in regs:return regs[s]
    if s.startswith('[bp-'):return local[s]
    return int(s,0)
rows=[bytearray(b' '*80) for _ in range(25)]; attrs=[bytearray([7]*80) for _ in range(25)]; y=x=0; color=7; writes=[]
for line in (ROOT/'docs/disassembly.txt').read_text().splitlines():
    pos=int(line[:8],16)
    if not 0x2e4<=pos<0x1e1b:continue
    inst=line[28:].strip(); op,_,args=inst.partition(' ')
    if op=='mov':
        a,z=args.split(',');
        if a in regs:regs[a]=val(z)
        elif a.startswith('[bp-'):local[a]=val(z)
        else:raise ValueError(inst)
    elif op=='xchg':
        a,z=args.split(',');regs[a],regs[z]=regs[z],regs[a]
    elif op=='xor':regs[args.split(',')[0]]=0
    elif op=='call':
        f=int(args.split(':')[1],16)
        if f==0x39a:y=regs['bx']-1
        elif f==0x3b4:x=regs['bx']-1
        elif f==0x1245:regs['bx']=bytes([regs['bx']])
        elif f in (0x1361,0x1368):regs['bx']=(bytes([regs['dx']]) if f==0x1361 else desc(regs['dx']))*regs['bx']
        elif f==0x28fa:
            s=regs['bx'];s=desc(s) if isinstance(s,int) else s
            for c in s:
                if c==13:x=0
                elif c==10:y+=1
                else:
                    if y<25:
                        rows[y][x]=c;attrs[y][x]=color;writes.append((y*80+x,c,color))
                    x+=1
                    if x==80:x=0;y+=1
        elif f==0x208b:color=(regs['bx']&15)|((regs['bx']&16)<<3)
        elif f in (0x20a5,0x216f,0x2a42,0x2a0c):pass
        else:raise ValueError(inst)
    elif op!='int3':raise ValueError(inst)
with (ROOT/'src/build.h').open('w') as f:
    f.write('/* Original PRINT operations, in execution order. */\nstatic const struct { unsigned short cell; unsigned char ch,attr; } original_draw[] = {\n')
    for cell,c,color in writes:f.write(f' {{{cell},{c},{color}}},\n')
    f.write('};\n')
# Remove bottom status line; keep the maze and its original character codes.
with (ROOT/'src/maze.h').open('w') as f:
    f.write('/* Recovered from PAC-GAL May 1982 PRINT statements. */\nstatic const unsigned char original_screen[24][80] = {\n')
    for row in rows[:24]:f.write(' {'+','.join(map(str,row))+'},\n')
    f.write('};\nstatic const unsigned char original_attr[24][80] = {\n')
    for row in attrs[:24]:f.write(' {'+','.join(map(str,row))+'},\n')
    f.write('};\n')
text='\n'.join(bytes(row).decode('cp437') for row in rows)
(ROOT/'docs/maze.txt').write_text(text+'\n')
print(text)
print('Dots:',sum(row.count(249) for row in rows),'power pellets:',sum(c==249 and a==138 for row,ar in zip(rows,attrs) for c,a in zip(row,ar)))
# DOSBox's IBM-compatible 8x8 BIOS font, embedded so no assets are required.
p=Path('/usr/bin/dosbox').read_bytes();i=p.index(bytes.fromhex('7e81a581bd99817e'))-8
font=p[i:i+256*8]
with (ROOT/'src/font.h').open('w') as f:
    f.write('/* IBM-compatible BIOS font from DOSBox, GPL-2.0-or-later. */\nstatic const unsigned char font[256][8] = {\n')
    for j in range(256):f.write(' {'+','.join(map(str,font[j*8:j*8+8]))+'},\n')
    f.write('};\n')
