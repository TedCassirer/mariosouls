static void fireFlowerTest(const SC_CharacterAPI*api,const char*asset,const char*runtime){
 auto*d=api->describe();std::vector<uint8_t>pixels(d->textureWidth*d->textureHeight*4);SC_Texture texture={pixels.data(),(uint32_t)pixels.size()};SC_Init init={sizeof(init),asset,runtime,logMessage};require(api->initialize(&init,&texture),"flower initialize");
 SC_Surface floor[2]={{{{-16000,0,-16000},{-16000,0,16000},{16000,0,16000}},0},{{{-16000,0,-16000},{16000,0,16000},{16000,0,-16000}},0}};api->surfaces(floor,2,0);
 int id=api->create(0,0,0,0);Buffers b(d->maxTriangles);SC_State s{};SC_Input in{};in.camLookZ=1;for(int n=0;n<3;n++)api->step(id,&in,&s,&b.mesh);
 auto blue=[&](){unsigned n=0;for(unsigned i=0;i<b.mesh.numTrianglesUsed*3;i++){auto*c=b.mesh.color+i*3;if(c[2]>.15f&&c[0]<c[2]*.12f&&c[1]<c[2]*.12f)n++;}return n;};
 require(blue()>50,"ordinary Mario has blue overalls");
 unsigned base=b.mesh.numTrianglesUsed;
 std::vector<float>originalColors(b.mesh.color,b.mesh.color+base*9);
 for(unsigned side:{1u,2u,3u}){
  std::vector<float>first;
  for(int frame=0;frame<=30;frame++){
   SC_EquipmentPose e={sizeof(e),side&1?SC_EQUIPMENT_PYROMANCY:SC_EQUIPMENT_NONE,side&2?SC_EQUIPMENT_PYROMANCY:SC_EQUIPMENT_NONE,side==2?SC_EQUIPMENT_CAST_LEFT:SC_EQUIPMENT_CAST_RIGHT,frame/30.f,0};
   require(api->equipment(id,&e),"flower equipment both hands");
   SC_Pose p={};p.kind=SC_POSE_NATIVE_COMBAT;p.phase=-1;p.position[0]=17;p.position[1]=31;p.position[2]=53;
   require(api->pose(id,&p,&s,&b.mesh),"cast presentation");require(s.attackId==-1&&!s.flags,"casting presentation emits no guest combat");
   SC_Attachments a={sizeof(a)};api->attachments(id,&a);require(a.valid==side,"flower socket corresponds to selected hand");if(side&1)rigid(a.right);if(side&2)rigid(a.left);
   require(b.mesh.numTrianglesUsed>base+150&&b.mesh.numTrianglesUsed<b.mesh.capacityTriangles,"flower geometry fits caller capacity");
   require(blue()==0,"fire outfit replaces blue overalls");
   for(unsigned i=0;i<base*3;i++){
    const float*c=originalColors.data()+i*3;
    if(c[0]>.4f&&c[0]<.5f)for(int k=0;k<3;k++)require(closeFloat(c[k],b.mesh.color[i*3+k]),"hair and shoes retain original brown material");
   }
   for(unsigned i=0;i<b.mesh.numTrianglesUsed*9;i++)require(std::isfinite(b.mesh.position[i])&&std::isfinite(b.mesh.color[i]),"finite flower geometry");
   if(frame==0)first=b.positions;if(frame==15)require(first!=b.positions,"cast visibly animates");
  }
 }
 SC_Pose p={};p.kind=SC_POSE_REST;p.phase=.5f;require(api->pose(id,&p,&s,&b.mesh),"rest with fire outfit");require(blue()==0,"interaction retains outfit without held prop");
 SC_EquipmentPose none={sizeof(none)};require(api->equipment(id,&none),"unequip flower");api->resume(id,0,0,0,0);api->step(id,&in,&s,&b.mesh);require(blue()>50&&b.mesh.numTrianglesUsed==base,"unequip restores original palette and topology");
 api->destroy(id);api->shutdown();puts("PASS Fire Flower: both hands, finite capacity, cast poses, no guest hits, persistent interaction outfit and exact unequip topology");
}
