from pathlib import Path
import struct,math,json
import numpy as np
from scipy.optimize import least_squares
rom=Path(r'C:\Users\Ted\Downloads\Super Mario 64 (USA)\Super Mario 64 (USA).z64').read_bytes()
def anim(n,frame):
 base=0x4ec000;off,size=struct.unpack_from('>II',rom,base+8+n*8);a=base+off
 flags,scale,start,loop,end,bones,values,index,_=struct.unpack_from('>6hIII',rom,a)
 result=[]
 for i in range(63):
  count,offset=struct.unpack_from('>HH',rom,a+index+i*4)
  result.append(struct.unpack_from('>h',rom,a+values+(offset+min(frame,count-1))*2)[0])
 return np.array(result,dtype=float)
parents=[-1,0,1,2,2,4,5,6,2,8,9,10]
offsets=[[0,0,0],[0,0,0],[68,0,0],[87,0,0],[67,-10,79],[0,0,0],[65,0,0],[60,0,0],[68,-10,-79],[0,0,0],[65,0,0],[60,0,0]]
def matrix(t,a):
 sx,sy,sz=np.sin(a);cx,cy,cz=np.cos(a)
 return np.array([[cy*cz,cy*sz,-sy,0],[sx*sy*cz-cx*sz,sx*sy*sz+cx*cz,sx*cy,0],[cx*sy*cz+sx*sz,cx*sy*sz-sx*cz,cx*cy,0],[*t,1.]])
def fk(p):
 nodes=[]
 for i in range(12):
  pos=np.array(offsets[i],dtype=float)+(p[:3] if i==0 else 0)
  mat=matrix(pos,p[3+i*3:6+i*3]*math.pi/32768)
  nodes.append(mat if parents[i]<0 else mat@nodes[parents[i]])
 return nodes
base=anim(0xc3,0);nodes=fk(base)
mouth=np.array([30,95,0,1.])@nodes[3]
print('idle mouth',mouth,'hand',np.array([40,0,0,1.])@nodes[11])
# Arm IK: keep idle torso and legs, place the bottle grip under the mouth.
# A bent elbow plus wrist rotation points the neck toward the mouth.
target=mouth[:3]+np.array([-32,-22,20])
original=base[30:39]*math.pi/32768
def residual(a):
 p=base.copy();p[30:39]=a*32768/math.pi;n=fk(p)
 grip=(np.array([40,0,0,1.])@n[11])[:3]
 elbow=n[10][3,:3];wrist=n[11][3,:3]
 return np.r_[(grip-target)/35, .12*(a-original), max(0,130-elbow[1])/60]
best=least_squares(residual,original,max_nfev=3000)
pose=base.copy();pose[30:39]=best.x*32768/math.pi
nodes=fk(pose);grip=np.array([40,0,0,1.])@nodes[11]
axis=(mouth[:3]-grip[:3]);axis=axis/np.linalg.norm(axis)
local_axis=axis@nodes[11][:3,:3].T
print('drink right arm channels',np.round(pose[30:39]).astype(int).tolist())
print('grip',grip,'mouth',mouth,'bottle local axis',local_axis,'residual',np.linalg.norm(residual(best.x)[:3]))
Path('work/interaction-v2/drink-pose.json').write_text(json.dumps({'angles':np.round(pose[30:39]).astype(int).tolist(),'axis':local_axis.tolist(),'grip':grip.tolist(),'mouth':mouth.tolist()},indent=2))
