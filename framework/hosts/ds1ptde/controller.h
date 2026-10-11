#pragma once
#include <windows.h>
#include <xinput.h>
#include <math.h>
#include <algorithm>

namespace SCController {
struct Input {
    float x=0, y=0;
    bool jump=false, attack=false, crouch=false, guard=false;
};

// Framework axes: positive X right, positive Y forward.
inline Input map(const XINPUT_GAMEPAD& pad) {
    Input result;
    const float x=pad.sThumbLX, y=pad.sThumbLY;
    const float length=sqrtf(x*x+y*y);
    constexpr float deadzone=XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    if(length>deadzone) {
        const float speed=std::min(1.f,(length-deadzone)/(32767.f-deadzone));
        result.x=x/length*speed;
        result.y=y/length*speed;
    }
    result.jump=(pad.wButtons&XINPUT_GAMEPAD_A)!=0;
    result.attack=(pad.wButtons&(XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_RIGHT_SHOULDER))!=0;
    result.guard=(pad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER)!=0;
    result.crouch=pad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    return result;
}

inline void filterNative(XINPUT_GAMEPAD& pad) {
    // Jump must not also grab a ladder, and the character's movement must not drive the host.
    pad.sThumbLX=pad.sThumbLY=0;
    pad.bLeftTrigger=0;
    pad.wButtons&=~(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_Y);
    constexpr WORD combo=XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB;
    if((pad.wButtons&combo)==combo)pad.wButtons&=~combo;
}

struct Sample {
    bool connected=false;
    int slot=-1;
    XINPUT_GAMEPAD pad={};
};

class Reader {
    using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
    GetState getState=nullptr;
    bool initialized=false, scanned=false;
    int slot=-1;
    DWORD scanTime=0;
public:
    void* function()const{return (void*)getState;}
    void setFunction(void* function){getState=(GetState)function;}
    Sample poll(DWORD now) {
        if(!initialized) {
            initialized=true;
            // Prefer the same XInput generation as PTDE. No extra redistributable needed.
            for(const char* name:{"xinput1_3.dll","xinput1_4.dll","xinput9_1_0.dll"}) {
                char path[MAX_PATH]={};
                const UINT n=GetSystemDirectoryA(path,MAX_PATH);
                if(!n||n>=MAX_PATH-24)continue;
                strcat(path,"\\");strcat(path,name);
                HMODULE module=LoadLibraryA(path);
                if(!module)continue;
                getState=(GetState)GetProcAddress(module,"XInputGetState");
                if(getState)break;
                FreeLibrary(module);
            }
        }
        if(!getState)return {};
        XINPUT_STATE state={};
        if(slot>=0) {
            if(getState(slot,&state)==ERROR_SUCCESS)return {true,slot,state.Gamepad};
            slot=-1;
        }
        // Disconnected XInput slots can be slow: scan at most once per second.
        if(scanned&&DWORD(now-scanTime)<1000)return {};
        scanned=true;scanTime=now;
        for(DWORD i=0;i<XUSER_MAX_COUNT;i++) {
            state={};
            if(getState(i,&state)==ERROR_SUCCESS) {
                slot=int(i);
                return {true,slot,state.Gamepad};
            }
        }
        return {};
    }
};

class Toggle {
    bool mustRelease=true, holding=false, fired=false;
    DWORD began=0;
public:
    bool update(const Sample& sample,bool focus,DWORD now) {
        constexpr WORD combo=XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB;
        if(!focus||!sample.connected) {
            mustRelease=true;holding=fired=false;
            return false;
        }
        const WORD buttons=sample.pad.wButtons&combo;
        if(mustRelease) {
            if(!buttons)mustRelease=false;
            return false;
        }
        if(buttons!=combo) {holding=fired=false;return false;}
        if(!holding) {holding=true;began=now;}
        if(!fired&&DWORD(now-began)>=600) {fired=true;return true;}
        return false;
    }
};
}
