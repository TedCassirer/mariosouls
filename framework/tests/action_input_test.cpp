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
static bool testInputAvailable=true;
static unsigned testGuardBinding=0x40000;
template<class T> static bool get(uintptr_t address,T& value){
 const unsigned char managerCode[]={0xa1,0x58,0x84,0x37,0x01,0x8b,0x40,0x08,0xc3};
 const unsigned char configCode[]={0x8b,0x4b,0x2c,0x85,0xc9,0x0f,0x84,0xc2,0x01,0x00,0x00};
 if(address==0xf72480&&sizeof(T)==sizeof(managerCode)){memcpy(&value,managerCode,sizeof(T));return true;}
 if(address==0xf7268e&&sizeof(T)==sizeof(configCode)){memcpy(&value,configCode,sizeof(T));return true;}
 if(!testInputAvailable||sizeof(T)!=4)return false;
 uint32_t scalar=0;
 switch(address){
  case 0x1378458:scalar=0x100000;break;
  case 0x10002c:scalar=0x110000;break;
  case 0x110000:scalar=0x116bb5c;break;
  case 0x110050:scalar=testGuardBinding;break;
  case 0x110048:scalar='H';break;
  case 0x110054:scalar=VK_TAB;break;
  default:return false;
 }
 memcpy(&value,&scalar,sizeof(scalar));return true;
}
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
 actionBlockHeldA=1;p=original;actionFilterPad(p);assert(!(p.wButtons&XINPUT_GAMEPAD_A));
 actionBlockHeldA=0;p=original;actionFilterPad(p);assert(p.wButtons&XINPUT_GAMEPAD_A);
 actionRoute=LONG(ActionRoute::Locked);DIMOUSESTATE2 mouse={};mouse.rgbButtons[0]=mouse.rgbButtons[1]=0x80;mouse.lX=150;actionDeviceState(nullptr,sizeof(mouse),&mouse);assert(mouse.lX==150&&mouse.rgbButtons[0]==0&&mouse.rgbButtons[1]==0);
 puts("PASS: native movement suppression, camera, ladder/menu routes, source-specific action pulses, expiry/cancellation, mouse filtering");
 actionCancelPulse();actionRoute=LONG(ActionRoute::Character);equipmentGuardInput=1;
 p={};p.wButtons=XINPUT_GAMEPAD_RIGHT_SHOULDER|XINPUT_GAMEPAD_B;actionFilterPad(p);assert(p.wButtons==XINPUT_GAMEPAD_LEFT_SHOULDER);
 // Active saved options bind guard to Shift, despite an INI Alt binding.
 testGuardBinding=0x40000;memset(k,0,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0x80&&k[DIK_LMENU]==0);
 equipmentGuardInput=0;actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0);
 equipmentGuardInput=1;actionRoute=LONG(ActionRoute::Locked);p={};actionFilterPad(p);assert(!p.wButtons);memset(k,0x80,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0);
 puts("PASS: only guard reaches native controller; configured keyboard binding synthesized and released; interaction route cannot leak guard");
 unsigned decoded[7]={},count=0;
 const unsigned expected[]={DIK_LCONTROL,DIK_RCONTROL,DIK_LSHIFT,DIK_RSHIFT,DIK_LMENU,DIK_RMENU};
 for(unsigned i=0;i<6;i++){assert(actionDecodeGuardBinding(0x10000u<<i,decoded,count));assert(count==1&&decoded[0]==expected[i]);}
 assert(actionDecodeGuardBinding(0x40000|'H',decoded,count)&&count==2&&decoded[0]==DIK_H&&decoded[1]==DIK_LSHIFT);
 assert(!actionDecodeGuardBinding(0,decoded,count));assert(!actionDecodeGuardBinding(0x400000,decoded,count));assert(!actionDecodeGuardBinding(0x100,decoded,count));
 actionRoute=LONG(ActionRoute::Character);memset(k,0,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0x80);
 testGuardBinding=0x200000;actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0&&k[DIK_RMENU]==0x80);
 testInputAvailable=false;actionDeviceState(nullptr,256,k);assert(k[DIK_RMENU]==0&&!actionKeyboardGuardReady);
 testInputAvailable=true;testGuardBinding=0x40000;actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0x80&&actionKeyboardGuardReady);
 testGuardBinding=0x400000;actionDeviceState(nullptr,256,k);assert(k[DIK_LSHIFT]==0&&!actionKeyboardGuardReady);
 puts("PASS: all six native modifiers, combined binding, runtime option changes, missing config, unsupported binding fail closed");
 testGuardBinding=0x40000;equipmentGuardInput=0;actionRoute=LONG(ActionRoute::Locked);
 actionQueue(3,GetTickCount(),true);p={};actionFilterPad(p);assert(p.bLeftTrigger==255&&!p.bRightTrigger&&!p.wButtons);
 memset(k,0,sizeof(k));actionDeviceState(nullptr,256,k);assert(!k[DIK_TAB]);
 actionQueue(4,GetTickCount(),true);p={};actionFilterPad(p);assert(p.wButtons==XINPUT_GAMEPAD_RIGHT_SHOULDER&&!p.bLeftTrigger);
 actionQueue(3,GetTickCount(),false);memset(k,0,sizeof(k));actionDeviceState(nullptr,256,k);assert(k[DIK_TAB]==0x80&&!k[DIK_H]&&!k[DIK_E]);
 actionQueue(4,GetTickCount(),false);actionDeviceState(nullptr,256,k);assert(k[DIK_H]==0x80&&!k[DIK_TAB]&&!k[DIK_E]);
 actionCancelPulse();actionDeviceState(nullptr,256,k);assert(!k[DIK_H]&&!k[DIK_TAB]);
 puts("PASS: source-specific native parry/light attack pulses, no item/guard leakage and complete release");
}
