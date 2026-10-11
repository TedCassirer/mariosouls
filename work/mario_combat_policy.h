#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <unordered_set>
namespace MarioCombat {
enum Move { None=-1,Punch1,Punch2,Kick,JumpKick,Dive,SlideKick,Stomp,Pound };
inline bool pending(uint32_t until,uint32_t now){return until!=0&&int32_t(until-now)>0;}
// Mario owns falling movement, so native warp updates cannot measure the fall.
// Environmental damage uses the host HP pool once, independently of attacks.
inline int fallDamage(float drop,int maxHP,float safe=8,float lethal=20){if(drop<=safe)return 0;if(drop>=lethal)return maxHP;return int(std::ceil(maxHP*.5f*(drop-safe)/(lethal-safe)));}
inline const char* name(int m){static const char* n[]={"punch1","punch2","combo_kick","jump_kick","dive","slide_kick","stomp","ground_pound"};return m>=0&&m<8?n[m]:"none";}
struct Settings {
 float base=100,upgradeStep=10,fixedPower=0;
 int upgrade=0;bool scaling=true;
 float multiplier[8]={.65f,.65f,1.10f,.95f,1.15f,1.f,.90f,1.65f};
 int cost[8]={8,8,16,18,24,20,16,30};
 float poise[8]={5,5,10,8,10,8,5,15};
};
inline float strengthBonus(int s){s=std::clamp(s,1,99);return std::max(0,std::min(s,20)-10)*3.f+std::max(0,std::min(s,40)-20)*2.f+std::max(0,s-40)*.5f;}
inline float power(const Settings& s,int str,int dex){return std::clamp(s.fixedPower>0?s.fixedPower:s.base+s.upgradeStep*std::clamp(s.upgrade,0,15)+(s.scaling?strengthBonus(str)+strengthBonus(dex)*.5f:0),1.f,1200.f);}
inline int identify(uint32_t action,unsigned arg){
 if(action==0x00800380||action==0x00800457){if(arg<=2)return Punch1;if(arg<=5)return Punch2;if(arg==6)return Kick;if(arg==9)return SlideKick;return None;}
 if(action==0x018008ac)return JumpKick;
 if(action==0x0188088a||action==0x00880456)return Dive;
 if(action==0x018008aa||action==0x0080045a)return SlideKick;
 if(action==0x008008a9||action==0x0080023c)return Pound;
 return None;
}
struct Window {
 int move=None,age=0,serial=0;bool paid=false,exhausted=false,stompUsed=false;float attackPower=0;int charged=0;
 std::unordered_set<int> hit;
 void reset(){move=None;age=0;paid=false;exhausted=false;stompUsed=false;hit.clear();}
 bool observe(uint32_t action,unsigned arg,bool grounded){
  if(grounded)stompUsed=false;
  int next=identify(action,arg);
  // One automatic stomp per airtime, including the rebound. It is charged on
  // eligible contact, never merely for descending during a navigation jump.
  if(move==Stomp&&next==None&&!grounded){++age;return false;}
  if(next==move){++age;return false;}
  move=next;age=0;paid=false;exhausted=false;charged=0;attackPower=0;hit.clear();if(next!=None){++serial;return true;}return false;
 }
 bool beginStomp(){if(stompUsed||move!=None)return false;stompUsed=true;move=Stomp;age=0;++serial;paid=false;exhausted=false;charged=0;hit.clear();return true;}
 bool pay(int stamina,int cost,float attack){if(paid||exhausted)return false;charged=0;exhausted=stamina<cost;if(exhausted)return false;paid=true;charged=cost;attackPower=attack;return true;}
 bool active(uint32_t action,uint32_t flags,int animFrame,float vy,int timer=0)const{
  if(!paid||exhausted)return false;
  switch(move){
   case Punch1:case Punch2:return age<18&&(flags&0x00100000);
   case Kick:return age<14&&(flags&0x00200000);
   case JumpKick:return age<18&&(flags&0x00200000);
   case Dive:case SlideKick:return age<24;
   case Stomp:return vy<0;
   case Pound:return (action==0x008008a9&&vy<0)||(action==0x0080023c&&timer<=2);
   default:return false;
  }
 }
 bool canHit(int id)const{return paid&&!exhausted&&!hit.count(id)&&hit.size()<size_t(move==Pound?3:8);}
};
}
