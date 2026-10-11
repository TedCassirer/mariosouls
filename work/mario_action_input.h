#pragma once
// Native actions receive bounded input; native locomotion never borrows Mario's stick.
enum class ActionRoute { Free, Mario, Locked, Ladder, Menu };
static volatile LONG actionRoute=0,actionButton=0,actionUntil=0,actionPadSource=0;
static bool actionInputReady=false;
static DWORD actionKeyboardPolls=0;
static void actionQueue(int button,DWORD now,bool fromPad=false){
 InterlockedExchange(&actionPadSource,fromPad);InterlockedExchange(&actionButton,button);InterlockedExchange(&actionUntil,LONG(now+150));
}
static int actionPulse(bool forPad){return bool(InterlockedCompareExchange(&actionPadSource,0,0))==forPad&&int32_t(DWORD(InterlockedCompareExchange(&actionUntil,0,0))-GetTickCount())>0?int(InterlockedCompareExchange(&actionButton,0,0)):0;}
static void actionCancelPulse(){InterlockedExchange(&actionUntil,0);InterlockedExchange(&actionButton,0);}
static ActionRoute actionCurrentRoute(){return ActionRoute(InterlockedCompareExchange(&actionRoute,0,0));}
static void actionFilterPad(XINPUT_GAMEPAD& p){
 auto route=actionCurrentRoute();if(route==ActionRoute::Free)return;
 if(route!=ActionRoute::Menu){
  p.sThumbLX=0;if(route!=ActionRoute::Ladder)p.sThumbLY=0;
  p.bLeftTrigger=p.bRightTrigger=0;
  p.wButtons&=~(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y|XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER);
 }else p.wButtons&=~XINPUT_GAMEPAD_Y;
 int pulse=actionPulse(true);if(pulse==1)p.wButtons|=XINPUT_GAMEPAD_A;if(pulse==2)p.wButtons|=XINPUT_GAMEPAD_X;
}
using ActionDeviceStateFn=HRESULT(WINAPI*)(IDirectInputDevice8W*,DWORD,LPVOID);
static ActionDeviceStateFn actionOriginalDeviceState=nullptr;
static unsigned actionKeys[12]={};
static HRESULT WINAPI actionDeviceState(IDirectInputDevice8W*d,DWORD size,LPVOID data){
 HRESULT result=actionOriginalDeviceState(d,size,data);
 auto route=actionCurrentRoute();if(FAILED(result)||!data||route==ActionRoute::Free)return result;
 if(size==256){
  auto keys=(BYTE*)data;++actionKeyboardPolls;
  if(route!=ActionRoute::Menu){
   for(int i=0;i<10;++i){if(route==ActionRoute::Ladder&&(i==0||i==1))continue;if(actionKeys[i]<256)keys[actionKeys[i]]=0;}
  }
  // Q/E cannot leak through ahead of the action eligibility check.
  if(actionKeys[10]<256)keys[actionKeys[10]]=0;if(route!=ActionRoute::Menu&&actionKeys[11]<256)keys[actionKeys[11]]=0;
  int pulse=actionPulse(false);if(pulse&&actionKeys[pulse==1?10:11]<256)keys[actionKeys[pulse==1?10:11]]=0x80;
 }else if((size==sizeof(DIMOUSESTATE)||size==sizeof(DIMOUSESTATE2))&&route!=ActionRoute::Menu){
  auto mouse=(DIMOUSESTATE*)data;mouse->rgbButtons[0]=mouse->rgbButtons[1]=0;
 }
 return result;
}
static void installActionInput(){
 char sys[MAX_PATH];GetSystemDirectoryA(sys,MAX_PATH);strcat(sys,"\\dinput8.dll");HMODULE dll=LoadLibraryA(sys);
 using CreateFn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
 auto create=(CreateFn)GetProcAddress(dll,"DirectInput8Create");IDirectInput8W*input=nullptr;IDirectInputDevice8W*keyboard=nullptr;
 if(!create||FAILED(create(GetModuleHandleA(nullptr),0x800,IID_IDirectInput8W,(void**)&input,nullptr)))return;
 if(SUCCEEDED(input->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr))){
  void*target=(*(void***)keyboard)[9];auto status=MH_CreateHook(target,(void*)actionDeviceState,(void**)&actionOriginalDeviceState);
  actionInputReady=status==MH_OK&&MH_EnableHook(target)==MH_OK;logf("Action keyboard input hook=%d ready=%d",status,actionInputReady);keyboard->Release();
 }
 input->Release();
 const int keys[]={'W','S','A','D',VK_SPACE,VK_CONTROL,'H','U',VK_TAB,VK_SHIFT,'Q','E'};
 for(int i=0;i<12;i++)actionKeys[i]=MapVirtualKeyA(keys[i],MAPVK_VK_TO_VSC);
}
