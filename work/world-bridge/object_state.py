def objects(h):
 root=h.u(0x137d5f8);base=h.u(root+0x20);out=[]
 for i in range(h.i(root+0x1c)):
  m=base+i*0xec;n=h.i(m+0x3c);p=h.u(m+0x40)
  if not 0<n<10000:continue
  for j in range(n):
   o=p+j*0xc0;hp=int.from_bytes(h.read(o+0x2a,2),'little',signed=True)
   if hp>=0:out.append(dict(address=hex(o),index=j,hp=hp,flags=h.read(o+0x2c,4).hex(),position=[h.f(o+off) for off in [0x6c,0x7c,0x8c]]))
 return out
