#pragma once
using FeedbackSetState=DWORD(WINAPI*)(DWORD,XINPUT_VIBRATION*);
static FeedbackSetState feedbackOriginalRumble=nullptr;
static SRWLOCK feedbackRumbleLock=SRWLOCK_INIT;
static XINPUT_VIBRATION feedbackNativeMotors[4]={},feedbackMotors[4]={};
static DWORD feedbackMotorUntil[4]={};
static HANDLE feedbackMotorTimer=nullptr;
static DWORD WINAPI feedbackSetState(DWORD slot,XINPUT_VIBRATION* value){
 if(slot>=4||!value)return feedbackOriginalRumble(slot,value);
 AcquireSRWLockExclusive(&feedbackRumbleLock);feedbackNativeMotors[slot]=*value;
 bool active=SCFeedback::pending(feedbackMotorUntil[slot],GetTickCount());
 XINPUT_VIBRATION mixed={SCFeedback::motor(value->wLeftMotorSpeed,feedbackMotors[slot].wLeftMotorSpeed,active),SCFeedback::motor(value->wRightMotorSpeed,feedbackMotors[slot].wRightMotorSpeed,active)};
 DWORD result=feedbackOriginalRumble(slot,&mixed);ReleaseSRWLockExclusive(&feedbackRumbleLock);return result;
}
// Expire even if rendering stalls or the window is minimized. Native vibration
// is remembered separately, so an impact cannot erase incoming-hit feedback.
static void CALLBACK feedbackRumbleTick(void*,BOOLEAN){
 DWORD fg=0;GetWindowThreadProcessId(GetForegroundWindow(),&fg);DWORD now=GetTickCount();
 AcquireSRWLockExclusive(&feedbackRumbleLock);
 for(DWORD i=0;i<4;i++)if(feedbackMotorUntil[i]&&(!SCFeedback::pending(feedbackMotorUntil[i],now)||fg!=GetCurrentProcessId())){
  feedbackMotorUntil[i]=0;XINPUT_VIBRATION value=fg==GetCurrentProcessId()?feedbackNativeMotors[i]:XINPUT_VIBRATION{};feedbackOriginalRumble(i,&value);
 }
 ReleaseSRWLockExclusive(&feedbackRumbleLock);
}
static void feedbackRumbleStop(){
 if(!feedbackOriginalRumble)return;AcquireSRWLockExclusive(&feedbackRumbleLock);
 for(DWORD i=0;i<4;i++)if(feedbackMotorUntil[i]){feedbackMotorUntil[i]=0;feedbackOriginalRumble(i,&feedbackNativeMotors[i]);}
 ReleaseSRWLockExclusive(&feedbackRumbleLock);
}
static void feedbackRumble(int slot,const SCFeedback::Envelope&e,float scale){
 if(slot<0||slot>=4||!feedbackOriginalRumble||!feedbackMotorTimer)return;
 AcquireSRWLockExclusive(&feedbackRumbleLock);feedbackMotorUntil[slot]=GetTickCount()+e.rumbleMs;
 feedbackMotors[slot]={WORD((16000+18000*e.strength)*scale),WORD((27000+7000*e.strength)*scale)};
 XINPUT_VIBRATION mixed={std::max(feedbackNativeMotors[slot].wLeftMotorSpeed,feedbackMotors[slot].wLeftMotorSpeed),std::max(feedbackNativeMotors[slot].wRightMotorSpeed,feedbackMotors[slot].wRightMotorSpeed)};
 DWORD result=feedbackOriginalRumble(slot,&mixed);ReleaseSRWLockExclusive(&feedbackRumbleLock);
 logf("Impact rumble slot=%d result=%lu duration_ms=%u",slot,result,e.rumbleMs);
}
static void feedbackInstallRumble(HMODULE module){
 void* target=module?(void*)GetProcAddress(module,"XInputSetState"):nullptr;if(!target)return;
 auto status=MH_CreateHook(target,(void*)feedbackSetState,(void**)&feedbackOriginalRumble);
 if(status==MH_OK)status=MH_EnableHook(target);
 if(status==MH_OK)CreateTimerQueueTimer(&feedbackMotorTimer,nullptr,feedbackRumbleTick,nullptr,10,10,WT_EXECUTEDEFAULT);
 logf("impact rumble hook status=%d expiry_timer=%d",status,feedbackMotorTimer!=nullptr);
}
