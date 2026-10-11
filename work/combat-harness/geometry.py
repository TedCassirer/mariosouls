"""Read-only validation against the same private static collision used by Mario."""
from pathlib import Path
import numpy as np,struct
from host import GAME,StateError
class Geometry:
 def __init__(self):
  data=(GAME/'MarioSouls/collision/m18_01_00_00.bin').read_bytes()
  if data[:4]!=b'MSC1':raise StateError('Unexpected collision format')
  n=struct.unpack_from('<I',data,4)[0];self.t=np.frombuffer(data[8:],dtype='<f4').reshape(n,3,3).astype(float)
 def floor(self,xyz):
  p=np.array(xyz);t=self.t[:,::-1];e1=t[:,1]-t[:,0];e2=t[:,2]-t[:,0];normal=np.cross(e1,e2);mask=normal[:,1]>1e-5
  t=t[mask];normal=normal[mask];a=t[:,0];b=t[:,1];d=t[:,2];den=(b[:,2]-d[:,2])*(a[:,0]-d[:,0])+(d[:,0]-b[:,0])*(a[:,2]-d[:,2])
  u=((b[:,2]-d[:,2])*(p[0]-d[:,0])+(d[:,0]-b[:,0])*(p[2]-d[:,2]))/den
  v=((d[:,2]-a[:,2])*(p[0]-d[:,0])+(a[:,0]-d[:,0])*(p[2]-d[:,2]))/den
  y=a[:,1]-(normal[:,0]*(p[0]-a[:,0])+normal[:,2]*(p[2]-a[:,2]))/normal[:,1]
  valid=(u>=-1e-5)&(v>=-1e-5)&(u+v<=1.00001)&(y<=p[1]+.5)&(y>=p[1]-1)
  if not valid.any():raise StateError('No nearby static floor at requested fixture position')
  return float(y[valid].max())
 def blocked(self,start,end):
  a=np.array(start);d=np.array(end)-a;t=self.t;e1=t[:,1]-t[:,0];e2=t[:,2]-t[:,0];h=np.cross(d,e2);det=np.einsum('ij,ij->i',e1,h);mask=np.abs(det)>1e-7
  e1=e1[mask];e2=e2[mask];h=h[mask];det=det[mask];s=a-t[mask,0]
  u=np.einsum('ij,ij->i',s,h)/det;q=np.cross(s,e1);v=np.einsum('j,ij->i',d,q)/det;f=np.einsum('ij,ij->i',e2,q)/det
  return bool(((u>=0)&(v>=0)&(u+v<=1)&(f>.02)&(f<.98)).any())
if __name__=='__main__':
 g=Geometry()
 for p in [[-14.35,185.35,-22.2],[-15.8,185.35,-23.074],[-14.35,185.35,-20.2],[-7.76,208.5,-2.5]]:
  try:print(p,g.floor(p),g.blocked([p[0],p[1]+.7,p[2]],[-14.668,186.013,-23.074]))
  except StateError as e:print(p,str(e))
