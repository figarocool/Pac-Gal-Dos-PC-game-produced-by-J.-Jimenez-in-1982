"""Compare native C to the instrumented historical executable, including all 25 rows."""
from pathlib import Path
import os,struct,subprocess,tempfile,json
from dos_oracle import capture,ROOT
FIELDS=[2,3,6,7,4,1,5]
def run_case(name,seed=1982,ghosts=False,moves=((0,-1,1),),edits=(),stop=None,effect=False,replay=False):
 setup=[];cells=[];commands=[]
 for op,vals in edits:
  commands.append(op+' '+' '.join(map(str,vals)))
  if op=='actor':
   i,field,value=vals;setup.append((0x94c+2*(FIELDS[field]*6+i+1),value))
  elif op=='cell':cells.append(vals)
  else:setup.append((0xa00 if op=='lives' else 0xa02,vals[0]))
 os.environ['PACGAL_GHOSTS']='1' if ghosts else ''
 dos=capture(moves,seed,setup,cells,stop,True,replay=replay)
 expected=dos[:4004]+b''.join(dos[4028+2*(j*6+i):4030+2*(j*6+i)] for j in range(8) for i in range(1,6))+dos[4024:4027]
 with tempfile.TemporaryDirectory(prefix='pacgal-test-') as t:
  inputfile=Path(t,'input');output=Path(t,'output')
  if effect:commands.append('effect')
  inputfile.write_text(f'{seed} {int(ghosts)}\n'+'\n'.join(commands)+''.join(f'\nmove {dy} {dx} {n}' for dy,dx,n in moves)+('\nreplay' if replay else ''))
  subprocess.run(['/tmp/pg-probe',str(inputfile),str(output)],check=True)
  actual=output.read_bytes()
 if effect:expected=expected[:4000];actual=actual[:4000]
 if expected!=actual:
  diff=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
  Path('/tmp/pacgal-failed-dos.bin').write_bytes(dos)
  print(name,'FAIL',[(i,actual[i],expected[i]) for i in diff[:18]],flush=True)
  return False
 print(name,'OK',flush=True);return True

def cases():
 for seed in [1,2,7,42,1982,65535]:
  for n in [1,4,12,16]:
   yield dict(name=f'seed-{seed}-steps-{n*4}',seed=seed,ghosts=True,moves=((0,-1,n),(-1,0,n),(0,1,n),(1,0,n)))
 yield dict(name='stationary',moves=((0,0,1),))
 yield dict(name='wall',edits=(('actor',(0,0,2)),('actor',(0,1,1))),moves=((-1,0,1),))
 for col,dx in [(2,-1),(37,1)]:
  yield dict(name=f'tunnel-{col}',edits=(('actor',(0,0,12)),('actor',(0,1,col))),moves=((0,dx,1),))
 for lives in [1,2,3]:
  yield dict(name=f'power-{lives}',edits=(('lives',(lives,)),('actor',(0,0,6)),('actor',(0,1,2))),moves=((0,-1,1),))
 for lives in [2,3]:
  for edible in [False,True]:
   for under in [32,249]:
    edits=(('lives',(lives,)),('actor',(1,0,19)),('actor',(1,1,18)),('actor',(1,4,under)),('actor',(1,5,26 if edible else 7)),('cell',(18,36,3,138 if edible else 7)))
    yield dict(name=f'collision-{lives}-{edible}-{under}',edits=edits)
 for edible,stop in [(False,0x24b6),(True,0x270d)]:
  yield dict(name=f'animation-{edible}',edits=(('actor',(1,0,19)),('actor',(1,1,18)),('actor',(1,5,26 if edible else 7)),('cell',(18,36,3,138 if edible else 7))),stop=stop,effect=True)
 # The player makes a stationary move, then a ghost runs into it.
 for edible in [False,True]:
  for lives in [1,2,3]:
   edits=(('lives',(lives,)),('actor',(1,0,19)),('actor',(1,1,18)),('actor',(1,2,0)),('actor',(1,3,1)),('actor',(1,5,26 if edible else 7)),('cell',(18,36,3,138 if edible else 7)))
   yield dict(name=f'ghost-collision-{edible}-{lives}',ghosts=True,moves=((0,0,1),),edits=edits,stop=0x2ddd if lives==1 and not edible else None)
 yield dict(name='last-life',edits=(('lives',(1,)),('actor',(1,0,19)),('actor',(1,1,18)),('cell',(18,36,3,7))),stop=0x2ddd)
 yield dict(name='replay',edits=(('dots',(1,)),),replay=True)
 yield dict(name='victory',edits=(('dots',(1,)),),stop=0x2d65)
if __name__=='__main__':
 subprocess.run(['cc','-Isrc','-std=c11','-Wall','-Wextra','tests/probe.c','src/game.c','-o','/tmp/pg-probe','-lm'],cwd=ROOT,check=True)
 subprocess.run(['cc','-Isrc','tests/presentation.c','src/presentation.c','-o','/tmp/pg-presentation'],cwd=ROOT,check=True)
 report=Path(ROOT,'docs/deep-test.json')
 game_only="--game-only" in __import__("sys").argv
 result=[r for r in json.loads(report.read_text()) if r['case'].startswith('intro-frame-')] if game_only and report.exists() else []
 original=capture(checkpoint=0x1e1e)[:4000]
 built=subprocess.check_output(['/tmp/pg-presentation','build'])
 result.append(dict(case='maze-construction',passed=original==built))
 print('Maze construction', 'OK' if original==built else 'FAIL',flush=True)
 for frame in ([] if "--game-only" in __import__("sys").argv else range(1,121)):
  checkpoint=(0xb0,61-frame) if frame<=60 else (0x156,frame-60)
  dos=capture(checkpoint=checkpoint)[:4000]
  native=subprocess.check_output(['/tmp/pg-presentation',str(frame)])
  ok=dos==native;result.append(dict(case=f'intro-frame-{frame}',passed=ok))
  if not ok:print('INTRO FAIL',frame,flush=True)
  elif frame%20==0:print('Intro frames',frame,'OK',flush=True)
 for case in cases():
  try:ok=run_case(**case)
  except Exception as e:print(case['name'],'ERROR',str(e),flush=True);ok=False
  result.append(dict(case=case['name'],passed=ok))
 Path(ROOT,'docs/deep-test.json').write_text(json.dumps(result,indent=2)+'\n')
 print(sum(r['passed'] for r in result),'/',len(result),'passed')
 raise SystemExit(not all(r['passed'] for r in result))
