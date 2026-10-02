"""Measure the original BASIC delay at fixed DOSBox cycle budgets.
Cycle settings are reference emulation profiles, not exact historical MHz.
"""
from pathlib import Path
import os,struct,subprocess,tempfile,json
ROOT=Path(__file__).resolve().parents[1]
BASE=5662;HEADER=5632

def measure(cycles,speed=6000,repeats=4):
 b=bytearray((ROOT/'original/pac-gal/Pac-gal.05.1982.exe').read_bytes());patches=[]
 def patch(pos,data):
  b[BASE+pos:BASE+pos+len(data)]=data;patches.append((BASE+pos,BASE+pos+len(data)))
 patch(0x19,b'\xe9'+struct.pack('<h',0x47-0x1c))
 patch(0x6d,b'\xe9'+struct.pack('<h',0x22e-0x70))
 patch(0x2ea1,b'\xc3\x90\x90\x90\x90')
 pos=0x1e1e;code=bytearray()
 def word(addr,value):code.extend(b'\xc7\x06'+struct.pack('<HH',addr,value))
 word(0x932,speed);word(0x9c6,repeats)
 code.extend(bytes.fromhex('31c0 cd1a 890e180a 89161a0a'))
 loop=pos+len(code)
 code.extend(b'\xe8'+struct.pack('<h',0x2e73-(pos+len(code)+3)))
 code.extend(bytes.fromhex('ff0ec609'))
 code.extend(b'\x75'+struct.pack('b',loop-(pos+len(code)+2)))
 code.extend(bytes.fromhex('31c0 cd1a 890e1c0a 89161e0a'))
 # Dump before/after BIOS timer values; all pointers relative to original DS.
 filename=0xa20
 word(filename,ord('T')|ord('I')<<8);word(filename+2,ord('M')|ord('E')<<8)
 word(filename+4,ord('.')|ord('B')<<8);word(filename+6,ord('I')|ord('N')<<8);word(filename+8,0)
 code.extend(bytes.fromhex('ba200a 31c9 b43c cd21 89c3 ba180a b90800 b440 cd21 b8004c cd21'))
 patch(pos,code)
 rel=[]
 for i in range(struct.unpack_from('<H',b,6)[0]):
  off,seg=struct.unpack_from('<HH',b,28+i*4);a=HEADER+seg*16+off
  if not any(lo<=a<hi for lo,hi in patches):rel.append((off,seg))
 struct.pack_into('<H',b,6,len(rel))
 for i,r in enumerate(rel):struct.pack_into('<HH',b,28+i*4,*r)
 with tempfile.TemporaryDirectory(prefix='pacgal-timing-') as tmp:
  Path(tmp,'BENCH.EXE').write_bytes(b)
  config=Path(tmp,'bench.conf');config.write_text(f'[cpu]\ncore=normal\ncycles={cycles}\n')
  env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
  subprocess.run(['dosbox','-conf',str(config),'-c',f'mount c {tmp}','-c','c:','-c','BENCH.EXE','-c','exit'],env=env,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=40)
  a,bb,c,d=struct.unpack('<4H',Path(tmp,'TIME.BIN').read_bytes())
  ticks=((c<<16)|d)-((a<<16)|bb)
  return dict(dosbox_cycles=cycles,original_speed=speed,repeats=repeats,bios_ticks=ticks,delay_ms_per_step=round(ticks*1000/18.2065/repeats,1))
if __name__=='__main__':
 results=[]
 for cycles in (300,1000,3000):
  result=measure(cycles);results.append(result);print(json.dumps(result),flush=True)
 (ROOT/'docs/timing-benchmark.json').write_text(json.dumps(results,indent=2)+'\n')
