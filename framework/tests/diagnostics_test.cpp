#undef NDEBUG
#include "../core/diagnostics.h"
#include "../hosts/ds1ptde/report_files.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace SCDiagnostics;
static Frame frame(uint32_t time){Frame f;f.time=time;f.actor=f.rig=f.requested=f.presented=f.guestValid=f.positionValid=true;f.map=f.cachedMap=1002;f.hp=f.maxHP=600;f.stamina=f.maxStamina=100;f.owner=1;f.generation=1;return f;}
int main(int argc,char**argv){
 if(argc>1){
  if(std::string(argv[1])=="--review"){
   Journal j;j.observe(frame(250));puts(reviewHTML(j.body("mario64","0.9.0-dev.1","test-build","test-fingerprint",true)).c_str());return 0;
  }
  if(std::string(argv[1])=="--stress"){
   Journal j;for(unsigned i=0;i<10000;i++){auto f=frame(i*250);f.attack=i%3;j.observe(f);}
   puts(j.json("mario64","v","c","s",false).c_str());return 0;
  }
  if(std::string(argv[1])=="--wrap"){
   Journal j;j.observe(frame(0xffffff00));j.observe(frame(0xfffffff0));j.observe(frame(0x00000100));
   puts(j.json("mario64","v","c","s",false).c_str());return 0;
  }
  Journal j;Frame f=frame(0);j.observe(f);f.time=250;f.hp=590;j.observe(f);f.time=500;f.hp=600;j.observe(f);
  f.time=750;f.map=1001;j.observe(f);f.time=1000;f.cachedMap=1001;f.generation=2;j.observe(f);
  f.time=1250;f.actor=f.rig=f.guestValid=false;f.owner=0;j.observe(f);
  f.time=1500;f=frame(1500);f.generation=3;j.observe(f);
  f.time=1750;f.position[0]=std::numeric_limits<float>::infinity();f.guestPosition[0]=std::numeric_limits<float>::quiet_NaN();j.observe(f);
  puts(j.json("mario64","0.9.0","abc123","fingerprint",false).c_str());return 0;
 }
 assert(identifier("mario64")=="mario64"&&identifier("C:\\Users\\name")=="unknown");
 assert(identifier(std::string(48,'a'))=="unknown"&&identifier("link_ww")=="link_ww");
 assert(jsonString("\"\n</script>&")=="\"\\\"\\u000a\\u003c/script\\u003e\\u0026\"");
 assert(html("</textarea>&\"'")=="&lt;/textarea&gt;&amp;&quot;&#39;");
 assert(number(std::numeric_limits<float>::infinity())=="null");
 Trigger key;assert(!key.update(true,true,0));assert(!key.update(true,true,1));key.update(false,true,2);
 assert(key.update(true,true,3));key.update(false,true,4);assert(!key.update(true,true,5));
 key.update(false,false,10003);assert(!key.update(true,true,10004));key.update(false,true,10005);assert(key.update(true,true,10006));
 Trigger wrap;wrap.update(false,true,0xfffffff0);assert(wrap.update(true,true,0xfffffff1));wrap.update(false,true,0xfffffff2);assert(!wrap.update(true,true,10));
 wrap.update(false,true,20000);assert(wrap.update(true,true,20001));
 Journal journal;for(unsigned i=0;i<10000;i++){auto f=frame(i*250);f.attack=i%3;journal.observe(f);}
 auto json=journal.json("mario64","v","c","s",false);assert(json.size()<512*1024);
 assert(json.find("\"time_ms\":0,")==std::string::npos&&json.find("\"time_ms\":2499750,")!=std::string::npos);
 auto review=reviewHTML("</textarea><script>alert(1)</script>");assert(review.find("<script>")==std::string::npos);
 assert(review.find("form-action https://github.com")!=std::string::npos);
 // Real Windows filesystem checks, isolated from the game and user saves.
 wchar_t temp[MAX_PATH],suffix[100];assert(GetTempPathW(MAX_PATH,temp));
 swprintf(suffix,100,L"MarioSouls-report-test-%lu-%llu-\u03bb",GetCurrentProcessId(),GetTickCount64());std::wstring base=std::wstring(temp)+suffix;
 assert(CreateDirectoryW(base.c_str(),nullptr));auto first=SCReportFiles::save(base,json,review),second=SCReportFiles::save(base,json,review);
 assert(first.ok&&second.ok&&first.directory!=second.directory);
 assert(!SCReportFiles::save(base,"",review).ok);assert(!SCReportFiles::save(base,std::string(512*1024+1,'a'),review).ok);
 assert(!SCReportFiles::save(base,json,std::string(64*1024+1,'a')).ok);
 assert(!SCReportFiles::save(base+L"\\missing",json,review).ok);
 DWORD error=0;assert(!SCReportFiles::writeNew(first.review,"overwrite",error)&&error==ERROR_FILE_EXISTS);
 for(const auto&r:{first,second}){assert(DeleteFileW((r.directory+L"\\report.json").c_str()));assert(DeleteFileW(r.review.c_str()));assert(RemoveDirectoryW(r.directory.c_str()));}
 assert(RemoveDirectoryW(base.c_str()));
 puts("PASS diagnostics: bounded history, focus/cooldown/wrap guards, finite JSON, injection escaping, Unicode report paths, collision-safe creation, failure handling; no game access");
}
