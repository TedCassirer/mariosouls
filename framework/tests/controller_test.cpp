#undef NDEBUG
#include "../hosts/ds1ptde/controller.h"
#include <assert.h>
#include <stdio.h>
using namespace SCController;
static bool closeEnough(float a,float b){return fabsf(a-b)<.001f;}
int main(){
    XINPUT_GAMEPAD pad={};
    assert(closeEnough(map(pad).x,0)&&closeEnough(map(pad).y,0));
    pad.sThumbLX=6000;pad.sThumbLY=4000;
    assert(closeEnough(map(pad).x,0)&&closeEnough(map(pad).y,0));
    pad={};pad.sThumbLY=32767;
    assert(closeEnough(aim(pad).y,1));
    pad.sThumbRX=32767;assert(closeEnough(aim(pad).x,1)&&closeEnough(aim(pad).y,0));
    pad.sThumbRX=1000;assert(closeEnough(aim(pad).y,1));
    pad.sThumbRX=0;pad.sThumbRY=-32768;assert(closeEnough(aim(pad).y,-1));
    pad.sThumbRY=0;
    assert(closeEnough(map(pad).y,1)&&closeEnough(map(pad).x,0));
    pad.sThumbLY=-32768;
    assert(closeEnough(map(pad).y,-1));
    pad={};pad.sThumbLX=32767;
    assert(closeEnough(map(pad).x,1));
    pad.sThumbLX=-32768;
    assert(closeEnough(map(pad).x,-1));
    pad.sThumbLY=32767;auto diagonal=map(pad);
    assert(closeEnough(sqrtf(diagonal.x*diagonal.x+diagonal.y*diagonal.y),1));
    pad={};pad.sThumbLY=20000;auto walk=map(pad);
    assert(walk.y>.4f&&walk.y<.6f);
    pad.wButtons=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B;pad.bLeftTrigger=31;
    auto actions=map(pad);assert(actions.jump&&actions.attack&&actions.crouch);
    pad.wButtons=XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y;pad.bLeftTrigger=30;
    actions=map(pad);assert(!actions.jump&&!actions.attack&&!actions.crouch);
    pad.sThumbRX=12345;pad.sThumbRY=-12345;pad.bRightTrigger=200;
    pad.sThumbLX=30000;pad.bLeftTrigger=100;pad.wButtons|=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_START;
    filterNative(pad);assert(pad.sThumbLX==0&&pad.sThumbLY==0&&pad.bLeftTrigger==0);
    assert(pad.sThumbRX==12345&&pad.sThumbRY==-12345&&pad.bRightTrigger==200);
    assert(pad.wButtons==(XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_START));
    Sample s={true,0,{}};Toggle toggle;
    assert(!toggle.update(s,true,0));
    s.pad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB;
    assert(!toggle.update(s,true,100));assert(!toggle.update(s,true,699));
    assert(toggle.update(s,true,700));assert(!toggle.update(s,true,3000));
    assert(!toggle.update(s,false,3001));assert(!toggle.update(s,true,4000));
    assert(!toggle.update(s,true,5000)); // Focus return with held buttons cannot toggle.
    s.pad.wButtons=0;assert(!toggle.update(s,true,5001));
    s.pad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB;
    assert(!toggle.update(s,true,5010));assert(toggle.update(s,true,5610));
    assert(!toggle.update({},true,6000));assert(!toggle.update(s,true,7000));
    assert(!toggle.update(s,true,8000)); // Reconnect also requires a release.
    s.pad.wButtons=0;toggle.update(s,true,0xffffff00u);
    s.pad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB;
    assert(!toggle.update(s,true,0xffffff10u));assert(toggle.update(s,true,0x200u));
    Reader reader;auto live=reader.poll(GetTickCount());
    printf("PASS: analog direction, deadzone, speed, diagonal limit, buttons, toggle hold, focus, reconnect, clock wrap. Live controller connected=%d slot=%d\n",live.connected,live.slot);
}
