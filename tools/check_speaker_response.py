"""Independent SciPy Butterworth reference versus the actual C coefficients.
Requires NumPy/SciPy and a host C compiler; never modifies runtime sources.
"""
from pathlib import Path
import ctypes,json,subprocess,tempfile
import numpy as np
from scipy.signal import butter,sosfreqz
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='pacgal-filter-') as tmp:
 p=Path(tmp)
 (p/'probe.c').write_text('''#include "speaker.c"
void coefficients(float *out,unsigned rate) {
 Speaker s;speaker_pc_init(&s,rate,.15f);
 for(unsigned i=0;i<4;i++) {
  SpeakerFilter *f=&s.filter[i];float *a=out+i*6;
  a[0]=f->b0;a[1]=f->b1;a[2]=f->b2;a[3]=1;a[4]=f->a1;a[5]=f->a2;
 }
}
''')
 subprocess.run(['cc','-shared','-fPIC','-O2','-I'+str(root/'src'),str(p/'probe.c'),str(root/'src/audio.c'),'-lm','-o',str(p/'probe.so')],check=True)
 lib=ctypes.CDLL(str(p/'probe.so'));lib.coefficients.argtypes=[ctypes.POINTER(ctypes.c_float),ctypes.c_uint]
 report={'reference':'scipy.signal.butter(3), independently designed HP120 / LP4300','rates':[]}
 for rate in (44100,48000):
  buf=(ctypes.c_float*24)();lib.coefficients(buf,rate)
  actual=np.array(buf,dtype=np.float64).reshape(4,6)
  expected=np.vstack([butter(3,120,btype='highpass',fs=rate,output='sos'),butter(3,4300,btype='lowpass',fs=rate,output='sos')])
  freq=np.geomspace(20,rate*.49,2000)
  _,a=sosfreqz(actual,worN=freq,fs=rate);_,b=sosfreqz(expected,worN=freq,fs=rate)
  error=float(np.max(abs(a-b)));assert error<.001,(rate,error)
  _,points=sosfreqz(actual,worN=[120,1000,4300],fs=rate)
  report['rates'].append({'rate':rate,'max_complex_response_error':error,'gain_db_at_120_1000_4300_hz':(20*np.log10(abs(points))).tolist()})
 report['passed']=True
 (root/'docs/speaker-response-check.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))
