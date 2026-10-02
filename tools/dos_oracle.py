"""Instrument a COPY of PAC-GAL and dump its real DOS video memory.
Only development verification uses DOSBox. Original file stays unchanged.
"""
from pathlib import Path
import struct,subprocess,os,tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE=5662; HEADER=5632; CS_ORIGIN=30

def capture(moves=(), seed=1982, setup=(), cells=(), stop=None, initialized=False, checkpoint=None,replay=False):
 b=bytearray((ROOT/'original/pac-gal/Pac-gal.05.1982.exe').read_bytes())
 patches=[]
 def patch(pos,data):
  b[BASE+pos:BASE+pos+len(data)]=data;patches.append((BASE+pos,BASE+pos+len(data)))
 # Skip speed input and opening animation, preserving BASIC initialization.
 patch(0x19,b'\xe9'+struct.pack('<h',0x47-0x1c))
 if checkpoint is None:patch(0x6d,b'\xe9'+struct.pack('<h',0x22e-0x70))
 active=bool(moves or setup or cells or initialized or checkpoint)
 pos=0x1fcf if active else 0x1e1e
 code=bytearray()
 def word(addr,value):code.extend(b'\xc7\x06'+struct.pack('<HH',addr,value&65535))
 if active:
  if replay:
   patch(0x19,b'\xe9'+struct.pack('<h',0x17e-0x1c))
   patch(0x17e,bytes.fromhex('c706200a0000')+b'\xe9'+struct.pack('<h',0x47-(0x17e+9)))
   # Feed y through the original assignment/comparison/restart branches.
   patch(0x2deb,bytes.fromhex('bbc80b909090'))
   # The clock is fixed for deterministic RNG; skip only the 15-second wait.
   patch(0x2d75,b'\xe9'+struct.pack('<h',0x2d9e-0x2d78))
   code.extend(bytes.fromhex('833e200a00')+b'\x75\x03\xe9\x00\x00'+bytes.fromhex('ff06200a'))
   # Invert jump: JE skips the JMP to continue first-run moves.
   code[5]=0x74
  patch(0x1c1,b'\xb8'+struct.pack('<H',seed&65535)+b'\xc3') # fixed BASIC RANDOMIZE seed: 1982
  # Replace BASIC RETURNs with a near return to our test driver.
  for line in (ROOT/'docs/disassembly.txt').read_text().splitlines():
   a=int(line[:8],16)
   if 0x20e1<=a<0x2d35 and 'call 0x2ec:0x11ae' in line:
    patch(a,b'\xc3\x90\x90\x90\x90')
  if stop is not None:patch(stop,b'\xc3')
  # Isolate gameplay from PLAY's compiled X-variable dependency in this
  # direct-call harness. Expanded eat notes are tested separately by audio_oracle.
  patch(0x268f,b'\x90'*5)
  if os.getenv('PACGAL_STOP'):patch(int(os.environ['PACGAL_STOP'],16),b'\xc3')
  if setup or cells:
   init=bytearray()
   for addr,value in setup:init.extend(b'\xc7\x06'+struct.pack('<HH',addr,value&65535))
   if cells:
    init.extend(bytes.fromhex('06 b800b8 8ec0'))
    for row,col,ch,attr in cells:init.extend(b'\x26\xc7\x06'+struct.pack('<HH',(row*80+col)*2,ch|(attr<<8)))
    init.extend(b'\x07')
   init.extend(b'\xc3');assert len(init)<0x22e-0x83
   patch(0x83,init)
   code.extend(b'\xe8'+struct.pack('<h',0x83-(pos+len(code)+3)))
  if os.getenv('PACGAL_MEASURE') and checkpoint is None:code.extend(bytes.fromhex('b400cd1a8916300a'))
  for dy,dx,count in moves:
   word(0x9ae,dy);word(0x9ba,dx);word(0xa14,count)
   loop=pos+len(code)
   code.extend(b'\xe8'+struct.pack('<h',0x20e1-(pos+len(code)+3)))
   if os.getenv('PACGAL_GHOSTS'):
    code.extend(b'\xe8'+struct.pack('<h',0x2875-(pos+len(code)+3)))
   code.extend(b'\xff\x0e\x14\x0a')
   code.extend(b'\x75'+struct.pack('b',loop-(pos+len(code)+2)))
 if os.getenv('PACGAL_MEASURE'):code.extend(bytes.fromhex('b400cd1a8916320a'))
 if checkpoint is not None and os.getenv('PACGAL_MEASURE'):
  patch(0x19,b'\xe9'+struct.pack('<h',0x2e73-0x1c))
  patch(0x2e73,bytes.fromhex('b400cd1a8916300a')+b'\xe9'+struct.pack('<h',0x47-(0x2e73+11)))
 if checkpoint is not None:
  assert not moves and not setup and not cells
  if isinstance(checkpoint,tuple):
   address,column=checkpoint;dest=pos;helper=0x1e1e
   original=bytes(b[BASE+address:BASE+address+4])
   h=bytearray(b'\x81\x3e\xc4\x09'+struct.pack('<H',column))
   h.extend(b'\x0f\x84'+struct.pack('<h',dest-(helper+10)))
   # 8086 has no near JE; use JNE over a near JMP instead.
   h=bytearray(b'\x81\x3e\xc4\x09'+struct.pack('<H',column)+b'\x75\x03\xe9'+struct.pack('<h',dest-(helper+11)))
   h.extend(original);h.extend(b'\xe9'+struct.pack('<h',address+4-(helper+len(h)+3)))
   patch(helper,h);patch(address,b'\xe9'+struct.pack('<h',helper-address-3)+b'\x90')
  else:patch(checkpoint,b'\xe9'+struct.pack('<h',pos-checkpoint-3))
 if replay:
  struct.pack_into('<h',code,8,len(code)-10)
 # Create output in DOS CWD, write B800:0000 then relevant DS state.
 # Save DS for state dump; preserve file handle across the screen write.
 code.extend(bytes.fromhex('1e 0e1f ba0000 31c9 b43c cd21 89c3 b800b8 8ed8 31d2 b9a00f b440 cd21 1f ba000a b91000 b440 cd21 ba2200 b90c00 b440 cd21 ba4c09 b97800 b440 cd21 b8004c cd21'))
 if os.getenv('PACGAL_MEASURE'):code[-5:-5]=bytes.fromhex('ba300ab90400b440cd21')
 # Filename address is the CS-relative offset of the following bytes.
 filename_offset=CS_ORIGIN+pos+len(code)
 # Find the filename-pointer operand after push DS / push CS / pop DS.
 j=code.index(bytes.fromhex('1e0e1fba'))+4
 struct.pack_into('<H',code,j,filename_offset)
 code.extend(b'SCREEN.BIN\x00')
 if active:assert pos+len(code)<0x20e1
 patch(pos,code)
 # Remove relocation entries touching our inserted code.
 rel=[]
 for i in range(struct.unpack_from('<H',b,6)[0]):
  off,seg=struct.unpack_from('<HH',b,28+i*4);a=HEADER+seg*16+off
  if not any(lo<=a<hi for lo,hi in patches):rel.append((off,seg))
 struct.pack_into('<H',b,6,len(rel))
 for i,r in enumerate(rel):struct.pack_into('<HH',b,28+i*4,*r)
 with tempfile.TemporaryDirectory(prefix='pacgal-oracle-') as tmp:
  Path(tmp,'PGAL.EXE').write_bytes(b)
  dump=bytearray.fromhex('0e1f ba0000 31c9 b43c cd21 89c3 b800b8 8ed8 31d2 b9a00f b440 cd21 b8004c cd21')
  struct.pack_into('<H',dump,3,0x100+len(dump));dump+=b'ERROR.BIN\0';Path(tmp,'DUMP.COM').write_bytes(dump)
  env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
  subprocess.run(['dosbox','-c','cycles '+os.getenv('PACGAL_ORACLE_CYCLES','3000'),'-c',f'mount c {tmp}','-c','c:','-c','PGAL.EXE','-c','DUMP.COM','-c','exit'],env=env,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=30)
  if not Path(tmp,'SCREEN.BIN').exists():
   v=Path(tmp,'ERROR.BIN').read_bytes()
   raise RuntimeError(bytes(v[::2]).decode('cp437').strip('\0 '))
  return Path(tmp,'SCREEN.BIN').read_bytes()
if __name__=='__main__':
 import sys
 data=capture([(0,-1,16),(-1,0,16),(0,1,16),(1,0,16)] if '--moves' in sys.argv else ())
 out=ROOT/'docs/original-moves.bin' if '--moves' in sys.argv else ROOT/'docs/original-screen.bin'
 out.write_bytes(data);print(f'Captured {len(data)} bytes: {out}')
