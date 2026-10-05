"""LiveArea packaging images drawn with the game's embedded BIOS glyphs."""
from pathlib import Path
import re
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
font=[list(map(int,row.split(','))) for row in re.findall(r'\{([0-9,]+)\}',(root/'src/font.h').read_text())]
def text(im,s,x,y,scale,color):
 d=ImageDraw.Draw(im)
 for c in s:
  for yy,bits in enumerate(font[ord(c)]):
   for xx in range(8):
    if bits&(128>>xx):d.rectangle((x+xx*scale,y+yy*scale,x+(xx+1)*scale-1,y+(yy+1)*scale-1),fill=color)
  x+=8*scale
def save_vita_png(im,path):
 # Vita promotion needs an 8-bit indexed PNG. These assets are opaque;
 # RGB quantization preserves their few original colors without an alpha
 # channel or tRNS chunk. RGBA/color-type 6 can fail with 0x8010113D.
 indexed=im.convert('RGB').quantize(colors=256,method=Image.Quantize.MEDIANCUT)
 indexed.info.clear()
 indexed.save(path,format='PNG',bits=8,optimize=False)
out=root/'vita/sce_sys';area=out/'livearea/contents' 
im=Image.new('RGBA',(128,128),(0,0,0,255));text(im,'PAC',28,12,3,'white');text(im,'GAL',28,90,3,'white');text(im,'\1',16,44,5,'yellow');text(im,'\3',72,44,5,'cyan');save_vita_png(im,out/'icon0.png')
im=Image.new('RGBA',(840,500),(0,0,0,255));text(im,'P A C - G A L',108,80,6,'white');text(im,'1982',356,160,4,'#aaaaaa');text(im,'\1 \3 \4 \5 \6',204,240,6,'#ffff55');save_vita_png(im,area/'bg.png')
im=Image.new('RGBA',(280,158),(0,0,0,255));text(im,'PAC-GAL',56,32,3,'white');text(im,'\1 \3 \4',60,72,4,'yellow');text(im,'1982',108,124,2,'#aaaaaa');save_vita_png(im,area/'startup.png')
