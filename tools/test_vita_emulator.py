"""Run the VPK in an isolated Vita3K profile; exercise controls and capture audio/video."""
from pathlib import Path
import os,signal,subprocess,time,zipfile,re,json
from PIL import ImageGrab
root=Path(__file__).resolve().parents[1];base=Path('/tmp/pacgal-emutest');exe='/tmp/pacgal-vita3k/squashfs-root/usr/bin/Vita3K'
# Close only previous emulators using our private temporary test profile.
for entry in Path('/proc').iterdir():
 if not entry.name.isdigit():continue
 try:
  cmd=(entry/'cmdline').read_bytes().split(b'\0');env=(entry/'environ').read_bytes()
  if cmd and cmd[0].decode()==exe and b'XDG_CONFIG_HOME=/tmp/pacgal-emutest/config\0' in env:os.kill(int(entry.name),signal.SIGTERM)
 except (OSError,UnicodeDecodeError):pass
(base/'fs/ux0/data').mkdir(parents=True,exist_ok=True)
profile_file=base/'fs/ux0/data/pacgal-performance.txt'
if profile_file.exists():profile_file.unlink()
with zipfile.ZipFile(root/'dist/PAC-GAL-PSVita.vpk') as z:z.extractall(base/'fs/ux0/app/PACG19820')
module=None;record=None
try:
 module=subprocess.check_output(['pactl','load-module','module-null-sink','sink_name=pacgaltest','sink_properties=device.description=PacGalTest'],text=True).strip()
 record=subprocess.Popen(['parec','--device=pacgaltest.monitor','--file-format=wav','--rate=48000','--channels=2',str(base/'audio.wav')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
except (subprocess.CalledProcessError,FileNotFoundError) as error:print('Audio capture unavailable:',error,flush=True)
log=open(base/'fixed.log','w');env=dict(os.environ,PULSE_SINK='pacgaltest')
p=subprocess.Popen([str(root/'tools/run_vita3k.sh'),'-r','PACG19820','--app-args','--seed, 1982, --frames, 1200, --timing-report, --performance-report'],stdout=log,stderr=subprocess.STDOUT,env=env)
report={'emulator':'Vita3K','package_version':'01.03','screenshots':[],'audio_capture':bool(record)}
def window():
 data=subprocess.run(['xdotool','search','--onlyvisible','--pid',str(p.pid)],capture_output=True,text=True).stdout.split()
 for wid in reversed(data):
  name=subprocess.check_output(['xdotool','getwindowname',wid],text=True).strip()
  if 'Vita3K' in name or 'PAC-GAL' in name:return wid
 return None
def capture(wid,name):
 geometry=subprocess.check_output(['xdotool','getwindowgeometry','--shell',wid],text=True)
 fields=dict(line.split('=',1) for line in geometry.splitlines() if '=' in line)
 x,y,w,h=[int(fields[k]) for k in ['X','Y','WIDTH','HEIGHT']]
 ImageGrab.grab(bbox=(x,y,x+w,y+h)).save(base/name)
 report['screenshots'].append(name);print('Captured:',name,'window:',w,h,flush=True)
def key(wid,k):
 subprocess.run(['xdotool','windowactivate','--sync',wid],check=True)
 subprocess.run(['xdotool','keydown',k],check=True)
 time.sleep(.15)
 subprocess.run(['xdotool','keyup',k],check=True)
 time.sleep(.1)
try:
 deadline=time.monotonic()+20;wid=None
 while time.monotonic()<deadline and p.poll() is None:
  wid=window()
  if wid:break
  time.sleep(.2)
 if not wid:raise RuntimeError('No emulator game window')
 subprocess.run(['xdotool','windowmove','--sync',wid,'50','50'],check=False)
 time.sleep(2);capture(wid,'menu.png');key(wid,'x');print('Started game',flush=True)
 time.sleep(6);capture(wid,'game-wide.png');key(wid,'Left')
 time.sleep(2);key(wid,'Return');time.sleep(.4);capture(wid,'pause.png');key(wid,'Return')
 key(wid,'v');time.sleep(.4);capture(wid,'game-4x3.png');key(wid,'v')
 for k in ['Up','Right','Down','Left']:key(wid,k);time.sleep(.5)
 # End this bounded test while the game runs; no other desktop application is targeted.
 deadline=time.monotonic()+25
 while time.monotonic()<deadline and not (base/'fs/ux0/data/pacgal-performance.txt').exists():time.sleep(.25)
 report['controlled_launch']=True
except Exception as error:report['error']=str(error);print('Test issue:',error,flush=True)
finally:
 if p.poll() is None:p.terminate()
 try:p.wait(timeout=5)
 except subprocess.TimeoutExpired:p.kill();p.wait()
 if record:
  record.send_signal(signal.SIGINT)
  try:record.wait(timeout=3)
  except subprocess.TimeoutExpired:record.terminate()
 if module:subprocess.run(['pactl','unload-module',module],check=False)
 log.close()
 data=(base/'fixed.log').read_text(errors='replace')
 if (base/'fs/ux0/data/pacgal-performance.txt').exists():data+='\n'+(base/'fs/ux0/data/pacgal-performance.txt').read_text()
 report['performance']=re.findall(r'Performance:[^\r\n]+',data)
 report['timing']=re.findall(r'Timing:[^\r\n]+',data)
 report['gameplay_verified']=any(int(n)>0 for n in re.findall(r'Timing: (\d+) moves',data))
 report['game_loaded']='Main executable pacgal_vita' in data
 report['return_code']=p.returncode
 (root/'docs/vita-emulator-test.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2),flush=True)
