#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "../../work/minhook/include/MinHook.h"
static void logf(const char*,...){}
#include "../hosts/ds1ptde/action_input.h"
static HRESULT WINAPI sample(IDirectInputDevice8W*,DWORD,LPVOID){return S_OK;}
int main(){
 const int keys[]={'W','S','A','D',VK_SPACE,VK_CONTROL,'H','U',VK_TAB,VK_SHIFT,'Q','E'};
 for(int i=0;i<12;i++)actionKeys[i]=MapVirtualKeyA(keys[i],MAPVK_VK_TO_VSC);
 actionOriginalDeviceState=sample;
 XINPUT_GAMEPAD original={};original.wButtons=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y|XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_DPAD_UP;original.sThumbLX=1234;original.sThumbLY=15000;original.sThumbRX=23456;original.sThumbRY=-4321;original.bLeftTrigger=128;
 auto p=original;actionFilterPad(p);assert(!memcmp(&p,&original,sizeof(p)));
 actionRoute=LONG(ActionRoute::Character);actionFilterPad(p);assert(p.sThumbLX==0&&p.sThumbLY==0&&p.sThumbRX==23456&&p.sThumbRY==-4321);assert(p.wButtons==(XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_DPAD_UP));assert(!p.bLeftTrigger);
 actionQueue(1,GetTickCount(),true);p={};actionFilterPad(p);assert(p.wButtons==XINPUT_GAMEPAD_A);assert(actionPulse(false)==0);actionCancelPulse();p={};actionFilterPad(p);assert(p.wButtons==0);
 actionQueue(2,GetTickCount(),false);p={};actionFilterPad(p);assert(p.wButtons==0);BYTE k[256]={};actionDeviceState(nullptr,256,k);assert(k[actionKeys[11]]==0x80);Sleep(170);memset(k,0,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[actionKeys[11]]==0);
 actionRoute=LONG(ActionRoute::Ladder);p=original;actionFilterPad(p);assert(p.sThumbLX==0&&p.sThumbLY==15000);memset(k,0x80,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[actionKeys[0]]==0x80&&k[actionKeys[1]]==0x80&&k[actionKeys[2]]==0&&k[actionKeys[4]]==0);
 actionRoute=LONG(ActionRoute::Menu);p=original;actionFilterPad(p);assert(p.sThumbLY==original.sThumbLY&&p.wButtons==(original.wButtons&~XINPUT_GAMEPAD_Y));
 actionRoute=LONG(ActionRoute::Locked);DIMOUSESTATE2 mouse={};mouse.rgbButtons[0]=mouse.rgbButtons[1]=0x80;mouse.lX=150;actionDeviceState(nullptr,sizeof(mouse),&mouse);assert(mouse.lX==150&&mouse.rgbButtons[0]==0&&mouse.rgbButtons[1]==0);
 puts("PASS: native movement suppression, camera, ladder/menu routes, source-specific action pulses, expiry/cancellation, mouse filtering");
}
