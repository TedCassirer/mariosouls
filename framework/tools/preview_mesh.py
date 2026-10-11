"""Render a private SCMESH01 pack offline to inspect extraction and skinning."""
import argparse
import struct
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

DT=np.dtype([('p','<f4',3),('n','<f4',3),('uv','<f4',2),('color','<f4',3),('bone','<u2',4),('weight','<f4',4),('group','<u4')])

def load(path):
    data=Path(path).read_bytes();magic,w,h,n,b,c=struct.unpack_from('<8s5I',data)
    assert magic==b'SCMESH01' and n%3==0
    atlas=np.frombuffer(data,np.uint8,count=w*h*4,offset=28).reshape(h,w,4)
    at=28+w*h*4;verts=np.frombuffer(data,DT,count=n,offset=at);at+=n*72;clips={}
    for _ in range(c):
        name,frames,loop=struct.unpack_from('<32s2I',data,at);at+=40
        clips[name.split(b'\0')[0].decode()]=np.frombuffer(data,'<f4',count=frames*b*12,offset=at).reshape(frames,b,3,4);at+=frames*b*48
    assert at==len(data)
    return atlas,verts,clips

def render(atlas,vertices,matrices,groups=1|8|16|32|64,angle=0,size=640):
    v=vertices[(vertices['group']&groups)!=0];p=np.zeros((len(v),3));n=p.copy()
    for i in range(4):
        m=matrices[v['bone'][:,i]];w=v['weight'][:,i,None]
        p+=(np.einsum('nij,nj->ni',m[:,:,:3],v['p'])+m[:,:,3])*w
        n+=np.einsum('nij,nj->ni',m[:,:,:3],v['n'])*w
    c,s=np.cos(angle),np.sin(angle);rot=np.array([[c,0,s],[0,1,0],[-s,0,c]])
    p=p@rot.T;n=n@rot.T
    scale=size/205.; screen=np.column_stack((size/2+p[:,0]*scale,size*.85-p[:,1]*scale,-p[:,2]))
    im=np.full((size,size,3),[38,45,52],np.uint8);depth=np.full((size,size),np.inf)
    shade=np.clip(.62+.38*(n@np.array([-.3,.5,.8])),.3,1)
    uv=v['uv']*np.array([atlas.shape[1],atlas.shape[0]])
    for i in range(0,len(v),3):
        t=screen[i:i+3];x0,y0=np.maximum(np.floor(t[:,:2].min(0)).astype(int),0);x1,y1=np.minimum(np.ceil(t[:,:2].max(0)).astype(int),size-1)
        if x1<x0 or y1<y0:continue
        den=(t[1,1]-t[2,1])*(t[0,0]-t[2,0])+(t[2,0]-t[1,0])*(t[0,1]-t[2,1])
        if abs(den)<.0001:continue
        y,x=np.mgrid[y0:y1+1,x0:x1+1];x=x+.5;y=y+.5
        a=((t[1,1]-t[2,1])*(x-t[2,0])+(t[2,0]-t[1,0])*(y-t[2,1]))/den
        b=((t[2,1]-t[0,1])*(x-t[2,0])+(t[0,0]-t[2,0])*(y-t[2,1]))/den;cc=1-a-b
        z=a*t[0,2]+b*t[1,2]+cc*t[2,2];mask=(a>=0)&(b>=0)&(cc>=0)&(z<depth[y0:y1+1,x0:x1+1])
        tex=(a[...,None]*uv[i]+b[...,None]*uv[i+1]+cc[...,None]*uv[i+2]).astype(int)
        tex[:,:,0]=np.clip(tex[:,:,0],0,atlas.shape[1]-1);tex[:,:,1]=np.clip(tex[:,:,1],0,atlas.shape[0]-1)
        rgba=atlas[tex[:,:,1],tex[:,:,0]];mask&=rgba[:,:,3]>127
        light=a*shade[i]+b*shade[i+1]+cc*shade[i+2]
        im[y0:y1+1,x0:x1+1][mask]=(rgba[:,:,:3]*light[...,None])[mask].astype(np.uint8)
        depth[y0:y1+1,x0:x1+1][mask]=z[mask]
    return Image.fromarray(im)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('pack');ap.add_argument('output');a=ap.parse_args()
    atlas,v,c=load(a.pack);sheet=Image.new('RGB',(1280,1280));draw=ImageDraw.Draw(sheet)
    for i,(name,frame,groups,angle) in enumerate([('waits',0,1|8|16|32|64,0),('cuta',8,1|8|16|32|64,.4),('dash',10,1|2|4|128,0),('bindrinking',8,1|4|128|256|512,0)]):
        sheet.paste(render(atlas,v,c[name][frame],groups,angle),(i%2*640,i//2*640));draw.text((i%2*640+15,i//2*640+15),f'{name} / frame {frame}',fill='white')
    Path(a.output).parent.mkdir(parents=True,exist_ok=True);sheet.save(a.output)
