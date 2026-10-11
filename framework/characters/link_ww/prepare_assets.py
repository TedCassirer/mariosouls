"""Privately convert an owned GZLE01 disc to the framework's neutral mesh pack.

Requires local DolphinTool 2609a, SuperBMD 1.3.11, gclib (MIT), numpy and Pillow.
No game data or decompiled game code is distributed by this recipe.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT/'work/tools/gclib'), str(ROOT/'work/python-deps')]
from gclib.rarc import RARC
from gclib.j3d import J3D

NS = {'c': 'http://www.collada.org/2005/11/COLLADASchema'}
CLIPS = ['wait', 'waits', 'walk', 'dash', 'walks', 'dashs', 'rollf',
         'cuta', 'cutl', 'cutr', 'cutturn', 'jmpst', 'mjmp', 'jmped',
         'rest', 'koshikake', 'crouch', 'ladderltor', 'ladderrtol',
         'bindrinkpre', 'bindrinking', 'bindrinkafter', 'dooropenalink', 'talka', 'damf']


def xyz(v):
    return np.array([v.x, v.y, v.z], dtype=float)


def transform(s, r, t):
    x,y,z = r; cx,cy,cz = np.cos(r); sx,sy,sz = np.sin(r)
    m = np.eye(4)
    m[:3,:3] = np.array([[cy*cz, sx*sy*cz-cx*sz, cx*sy*cz+sx*sz],
                         [cy*sz, sx*sy*sz+cx*cz, cx*sy*sz-sx*cz],
                         [-sy, sx*cy, cx*cy]]) @ np.diag(s)
    m[:3,3] = t
    return m


def skeleton(model):
    parents = [-1]*len(model.jnt1.joints)
    stack = [-1]; current = -1
    for node in model.inf1.flat_hierarchy:
        typ = int(node.type)
        if typ == 0x10:
            parents[node.index] = stack[-1]; current = node.index
        elif typ == 1: stack.append(current)
        elif typ == 2: stack.pop(); current = stack[-1]
    world=[]
    for i,j in enumerate(model.jnt1.joints):
        rot = (xyz(j.rotation)+32768)%65536-32768
        local=transform(xyz(j.scale), rot*math.pi/32768, xyz(j.translation))
        world.append(world[parents[i]]@local if parents[i]>=0 else local)
    return parents, np.array(world)


def bck_frames(path, parents, inverse_bind):
    data=path.read_bytes(); start=data.index(b'ANK1'); data=data[start:]
    u16=lambda o:struct.unpack_from('>H',data,o)[0]
    u32=lambda o:struct.unpack_from('>I',data,o)[0]
    duration=u16(0xA); count=u16(0xC)
    if count!=len(parents) or duration>600: raise ValueError('Unexpected animation skeleton/duration: '+path.name)
    tables=[np.frombuffer(data,dtype='>f4',count=u16(0xE),offset=u32(0x18)),
            np.frombuffer(data,dtype='>i2',count=u16(0x10),offset=u32(0x1C)),
            np.frombuffer(data,dtype='>f4',count=u16(0x12),offset=u32(0x20))]
    def track(j,axis,kind,f):
        off=u32(0x14)+j*54+axis*18+kind*6
        n,index,tangent=struct.unpack_from('>3H',data,off); table=tables[kind]
        if not n: raise ValueError('Empty animation track')
        if n==1: value=float(table[index])
        else:
            stride=3 if tangent==0 else 4
            keys=table[index:index+n*stride].astype(float).reshape(n,stride)
            if f<=keys[0,0]: value=keys[0,1]
            elif f>=keys[-1,0]: value=keys[-1,1]
            else:
                k=np.searchsorted(keys[:,0],f,side='right')-1; a,b=keys[k:k+2]
                dt=b[0]-a[0]; q=(f-a[0])/dt
                value=(2*q**3-3*q*q+1)*a[1]+(q**3-2*q*q+q)*dt*a[-1]+(-2*q**3+3*q*q)*b[1]+(q**3-q*q)*dt*b[2]
        return value*(2**data[9]*math.pi/32768) if kind==1 else value
    frames=[]
    root_origin=np.array([track(0,k,2,0) for k in range(3)])
    for f in range(duration+1):
        world=[]
        for j,parent in enumerate(parents):
            s,r,t=[np.array([track(j,k,kind,f) for k in range(3)]) for kind in range(3)]
            # In-place clips: world motion is owned by the controller and host.
            if j==0: t[[0,2]]=root_origin[[0,2]]
            local=transform(s,r,t); world.append(world[parent]@local if parent>=0 else local)
        frames.append((np.array(world)@inverse_bind)[:,:3,:])
    return np.array(frames,dtype='<f4'), data[8]==2


def dae_meshes(path):
    root=ET.parse(path).getroot()
    def sources(node):
        result={}
        for s in node.findall('c:source',NS):
            f=s.find('c:float_array',NS); names=s.find('c:Name_array',NS)
            if f is not None:
                stride=int(s.find('c:technique_common/c:accessor',NS).get('stride'))
                result[s.get('id')]=np.fromstring(f.text,sep=' ').reshape(-1,stride)
            elif names is not None: result[s.get('id')]=names.text.split()
        return result
    skins={}; inverse={}
    geometry_ids={g.get('id') for g in root.findall('c:library_geometries/c:geometry',NS)}
    for controller in root.findall('c:library_controllers/c:controller',NS):
        skin=controller.find('c:skin',NS)
        # SuperBMD appends duplicate controllers with rounded, transposed binds.
        # Retain the first Assimp controller for each actual geometry.
        if skin.get('source')[1:] not in geometry_ids or skin.get('source')[1:] in skins:continue
        src=sources(skin)
        inputs={i.get('semantic'):src[i.get('source')[1:]] for i in skin.findall('c:joints/c:input',NS)}
        names=inputs['JOINT']
        for name,mat in zip(names,inputs['INV_BIND_MATRIX']): inverse[name]=mat.reshape(4,4).T
        vw=skin.find('c:vertex_weights',NS)
        ins={i.get('semantic'):(int(i.get('offset')),src[i.get('source')[1:]]) for i in vw.findall('c:input',NS)}
        count=np.fromstring(vw.find('c:vcount',NS).text,sep=' ',dtype=int)
        v=np.fromstring(vw.find('c:v',NS).text,sep=' ',dtype=int).reshape(-1,max(i[0] for i in ins.values())+1)
        weights=[];offset=0
        for n in count:
            pairs=[(names[row[ins['JOINT'][0]]],float(ins['WEIGHT'][1][row[ins['WEIGHT'][0]]][0])) for row in v[offset:offset+n]]
            weights.append(sorted(pairs,key=lambda x:-x[1])); offset+=n
        skins[skin.get('source')[1:]]=weights
    meshes=[]
    for index,g in enumerate(root.findall('c:library_geometries/c:geometry',NS)):
        mesh=g.find('c:mesh',NS);src=sources(mesh)
        for v in mesh.findall('c:vertices',NS): src[v.get('id')]=src[v.find('c:input',NS).get('source')[1:]]
        p=mesh.find('c:triangles',NS)
        if p is None: p=mesh.find('c:polylist',NS)
        ins={i.get('semantic'):(int(i.get('offset')),src[i.get('source')[1:]]) for i in p.findall('c:input',NS) if i.get('set','0')=='0'}
        indices=np.fromstring(p.find('c:p',NS).text,sep=' ',dtype=int).reshape(-1,max(x[0] for x in ins.values())+1)
        vc=p.find('c:vcount',NS)
        if vc is not None and any(np.fromstring(vc.text,sep=' ',dtype=int)!=3): raise ValueError('Expected triangulated SuperBMD output')
        vertices=[]
        for row in indices:
            def get(key,default): return ins[key][1][row[ins[key][0]]] if key in ins else np.array(default)
            pos=get('VERTEX',[0,0,0]);normal=get('NORMAL',[0,0,1]);uv=get('TEXCOORD',[0,0])
            weights=skins.get(g.get('id'),[])
            vertices.append((pos,normal,uv,weights[row[ins['VERTEX'][0]]] if weights else []))
        meshes.append(vertices)
    return meshes,inverse


class Atlas:
    def __init__(self):
        self.image=Image.new('RGBA',(1024,1024)); self.rects={};self.x=2;self.y=2;self.row=0
    def add(self,key,image):
        if key in self.rects:return self.rects[key]
        w,h=image.size
        if self.x+w+2>1024:self.x=2;self.y+=self.row+4;self.row=0
        if self.y+h+2>1024:raise ValueError('Atlas full')
        self.image.paste(image,(self.x,self.y))
        # Extrude borders so atlas filtering cannot sample adjacent materials.
        self.image.paste(image.crop((0,0,1,h)).resize((2,h)),(self.x-2,self.y))
        self.image.paste(image.crop((w-1,0,w,h)).resize((2,h)),(self.x+w,self.y))
        self.image.paste(image.crop((0,0,w,1)).resize((w,2)),(self.x,self.y-2))
        self.image.paste(image.crop((0,h-1,w,h)).resize((w,2)),(self.x,self.y+h))
        rect=(self.x,self.y,w,h);self.rects[key]=rect;self.x+=w+4;self.row=max(self.row,h)
        return rect


def prepare(disc,output,dolphin,superbmd):
    output=output.resolve(); output.parent.mkdir(parents=True,exist_ok=True)
    lab=output.parent/'source'; lab.mkdir(exist_ok=True)
    header=subprocess.run([str(dolphin),'header','-i',str(disc)],capture_output=True,text=True,check=True).stdout
    if 'GZLE01' not in header:raise ValueError('This recipe supports the USA GZLE01 disc only')
    for arcname in ['Link','LkAnm']:
        subprocess.run([str(dolphin),'extract','-i',str(disc),'-o',str(lab),'-s',f'res/Object/{arcname}.arc'],capture_output=True,check=True)
        arc=RARC(lab/'files/res/Object'/f'{arcname}.arc'); dest=lab/arcname;dest.mkdir(exist_ok=True)
        for f in arc.file_entries:
            if not f.is_dir:
                if Path(f.name).name!=f.name:raise ValueError('Archive path escape')
                f.decompress_data_if_necessary();(dest/f.name).write_bytes(f.data.getvalue())
    models={};geometries={}; inverses={};atlas=Atlas();packed=[]
    for name in ['cl','hands','swa','swgripa','sha','bottleemp']:
        model=J3D(lab/'Link'/f'{name}.bdl'); models[name]=model
        folder=lab/'converted'/name;folder.mkdir(parents=True,exist_ok=True);dae=folder/f'{name}.dae'
        subprocess.run([str(superbmd),'-i',str(lab/'Link'/f'{name}.bdl'),'-o',str(dae)],capture_output=True,check=True)
        geometries[name],inverses[name]=dae_meshes(dae)
    body=models['cl'];names=body.jnt1.joint_names;parents,bind=skeleton(body)
    # Use the exporter's exact bind matrices to avoid angle quantization drift.
    for i,name in enumerate(names):
        if name in inverses['cl']:bind[i]=np.linalg.inv(inverses['cl'][name])
    inverse_bind=np.linalg.inv(bind)
    def material_for_shape(model):
        stack=[-1];cur=-1;result={}
        for node in model.inf1.flat_hierarchy:
            kind=int(node.type)
            if kind==0x11:cur=node.index
            elif kind==0x12:result[node.index]=cur
            elif kind==1:stack.append(cur)
            elif kind==2:stack.pop();cur=stack[-1]
        return result
    def add_model(name,group,only=None,parent=None,local=None,hand=False):
        model=models[name];materials=material_for_shape(model);folder=lab/'converted'/name
        for shape,vertices in enumerate(geometries[name]):
            mat_index=materials[shape];mat=model.mat3.materials[mat_index];matname=model.mat3.mat_names[mat_index]
            if only is not None and shape not in only:continue
            if name=='cl' and ('dam' in matname):continue
            part=(2 if shape==7 else 4 if shape==8 else 1) if name=='cl' else group
            texname=model.tex1.texture_names[mat.textures[0]]
            texture=Image.open(folder/(texname+'.png')).convert('RGBA')
            if name=='cl' and matname in ['eyeL','eyeR']:
                mask=np.array(texture);iris=np.array(Image.open(folder/'hitomi.png').convert('RGBA').resize(texture.size))
                # The second TEV layer shifts pupil coordinates by -0.05 U.
                iris=iris[:,np.clip(np.arange(iris.shape[1])-round(.05*iris.shape[1]),0,iris.shape[1]-1)]
                mask[:,:,:3]=(mask[:,:,:3].astype(float)*iris[:,:,:3]/255).astype(np.uint8);texture=Image.fromarray(mask)
            rect=atlas.add(name+'/'+texname+('/eye' if matname in ['eyeL','eyeR'] else ''),texture)
            attach=bind[parent]@(local if local is not None else np.eye(4)) if parent is not None else None
            for pos,normal,uv,weights in vertices:
                if attach is not None:
                    mat=attach
                    if hand:
                        if len(weights)!=1:raise ValueError('Hand must be rigid')
                        mat=attach@inverses[name][weights[0][0]]
                    pos=(mat@np.append(pos,1))[:3];normal=mat[:3,:3]@normal;weights=[(names[parent],1.)]
                if not weights:raise ValueError('Unbound vertex')
                if len(weights)>4:raise ValueError('More than four skin influences')
                bones=[names.index(b) for b,w in weights];ws=[w for b,w in weights];total=sum(ws)
                bones += [0]*(4-len(bones));ws=[w/total for w in ws]+[0.]*(4-len(ws))
                x,y,w,h=rect
                # Assimp DAE coordinates have V flipped relative to PNG pixels.
                u,v=uv[:2];u=float(np.clip(u,0,1));v=float(np.clip(1-v,0,1))
                uv2=[(x+.5+u*(w-1))/1024,(y+.5+v*(h-1))/1024]
                packed.append(struct.pack('<11f4H4fI',*pos,*normal,*uv2,1,1,1,*bones,*ws,part))
    add_model('cl',1)
    add_model('hands',8,only=[2],parent=8,hand=True)
    add_model('hands',16,only=[7],parent=12,hand=True)
    add_model('swa',32,parent=8);add_model('swgripa',32,parent=8)
    add_model('sha',64,parent=12)
    # Separate sheathed props are activated during traversal and interaction poses.
    add_model('swgripa',128,parent=13,local=transform([1]*3,[0,0,-0x1C71*math.pi/32768],[-11.25,4.5,.45]))
    add_model('sha',128,parent=13,local=transform([1]*3,[0,0,-math.pi/2],[15.5,4.75,0]))
    add_model('bottleemp',256,parent=8)
    add_model('hands',512,only=[5],parent=8,hand=True)
    clips=[]
    for name in CLIPS:
        frames,loop=bck_frames(lab/'LkAnm'/(name+'.bck'),parents,inverse_bind)
        clips.append((name,frames,loop))
    if len(packed)%3 or len(packed)//3>16384:raise ValueError('Geometry exceeds SDK bounds')
    data=bytearray(struct.pack('<8s5I',b'SCMESH01',1024,1024,len(packed),len(names),len(clips)))
    data.extend(atlas.image.tobytes());data.extend(b''.join(packed))
    for name,frames,loop in clips:
        data.extend(struct.pack('<32s2I',name.encode(),len(frames),int(loop)));data.extend(frames.tobytes())
    output.write_bytes(data);atlas.image.save(output.with_suffix('.atlas.png'))
    receipt={'game_id':'GZLE01','format':'SCMESH01','vertices':len(packed),'triangles':len(packed)//3,'bones':names,
             'clips':{name:{'frames':len(f),'loop':loop} for name,f,loop in clips},'pack_sha256':hashlib.sha256(data).hexdigest(),
             'disc_sha256':hashlib.file_digest(disc.open('rb'),'sha256').hexdigest(),'source':'User supplied disc; private asset pack, do not distribute'}
    output.with_suffix('.json').write_text(json.dumps(receipt,indent=2))
    print(json.dumps({'pack':str(output),'triangles':len(packed)//3,'clips':len(clips),'bytes':len(data)}))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--dolphin',type=Path,default=ROOT/'work/tools/dolphin-2609a-x64/Dolphin-x64/DolphinTool.exe')
    p.add_argument('--superbmd',type=Path,default=ROOT/'work/tools/SuperBMD-1/SuperBMD.exe')
    a=p.parse_args();prepare(a.input.resolve(),a.output,a.dolphin.resolve(),a.superbmd.resolve())
