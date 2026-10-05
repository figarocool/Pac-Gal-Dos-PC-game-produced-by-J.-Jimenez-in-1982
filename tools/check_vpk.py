"""Validate Vita package metadata and LiveArea PNG encoding, not just sizes."""
from pathlib import Path
import hashlib,io,json,struct,sys,zipfile,zlib
import xml.etree.ElementTree as ET
from PIL import Image
root=Path(__file__).resolve().parents[1]
package=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dist/PAC-GAL-PSVita.vpk'
expected={'sce_sys/icon0.png':(128,128),'sce_sys/livearea/contents/bg.png':(840,500),'sce_sys/livearea/contents/startup.png':(280,158)}
with zipfile.ZipFile(package) as z:
 assert z.testzip() is None,'ZIP CRC error'
 assert z.read('eboot.bin')[:4]==b'SCE\x00','Invalid SELF header'
 data=z.read('sce_sys/param.sfo');magic,_,keys,values,count=struct.unpack_from('<5I',data)
 assert magic==0x46535000,'Invalid SFO'
 metadata={}
 for i in range(count):
  off,fmt,size,_,voff=struct.unpack_from('<HHIII',data,20+i*16)
  key=data[keys+off:].split(b'\0')[0].decode();raw=data[values+voff:values+voff+size]
  metadata[key]=raw.rstrip(b'\0').decode() if fmt==0x204 else raw.hex()
 assert metadata['TITLE_ID']=='PACG19820'
 assert metadata['APP_VER']=='01.17'
 pngs={}
 for name,size in expected.items():
  data=z.read(name);assert data[:8]==b'\x89PNG\r\n\x1a\n'
  w,h,depth,color,compression,filtering,interlace=struct.unpack('>IIBBBBB',data[16:29])
  assert (w,h)==size and depth==8 and color==3 and interlace==0,(name,'PNG must be 8-bit indexed, not RGBA')
  chunks=[];pos=8;palette=0
  while pos<len(data):
   n=struct.unpack_from('>I',data,pos)[0];chunk=data[pos+4:pos+8];payload=data[pos+8:pos+8+n]
   crc=struct.unpack_from('>I',data,pos+8+n)[0]
   assert zlib.crc32(chunk+payload)&0xffffffff==crc,'PNG CRC error'
   chunks.append(chunk.decode())
   if chunk==b'PLTE':palette=len(payload)//3;assert len(payload)%3==0
   pos+=12+n
  assert chunks[0]=='IHDR' and chunks[-1]=='IEND' and 'IDAT' in chunks
  assert 1<=palette<=256 and 'tRNS' not in chunks,(name,'Unexpected transparency')
  image=Image.open(io.BytesIO(data));image.verify()
  pngs[name]=dict(width=w,height=h,depth=depth,color_type=color,palette_colors=palette,chunks=chunks)
 xml=ET.fromstring(z.read('sce_sys/livearea/contents/template.xml'))
 for e in xml.iter():
  if e.tag in ['image','startup-image']:
   assert 'sce_sys/livearea/contents/'+e.text.strip() in z.namelist(),e.text
 for name in ['LICENSE','NOTICE']:assert z.read(name)
report=dict(vpk=str(package.relative_to(root)),bytes=package.stat().st_size,sha256=hashlib.sha256(package.read_bytes()).hexdigest(),title_id=metadata['TITLE_ID'],version=metadata['APP_VER'],zip_crc='passed',self_header='passed',images=pngs,livearea_xml='passed',hardware_test=False)
(root/'dist/PAC-GAL-PSVita-build.json').write_text(json.dumps(report,indent=2)+'\n')
print('VPK validated:',report['version'],report['bytes'],'bytes; all LiveArea PNGs are 8-bit indexed with no alpha.')
