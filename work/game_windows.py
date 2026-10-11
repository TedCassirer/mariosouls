import ctypes as c
from ctypes import wintypes as w
import sys
u=c.WinDLL('user32');target=int(sys.argv[1]);CB=c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
def enum(h,l):
 p=w.DWORD();u.GetWindowThreadProcessId(h,c.byref(p))
 if p.value==target and u.IsWindowVisible(h):
  title=c.create_unicode_buffer(512);u.GetWindowTextW(h,title,512);cl=c.create_unicode_buffer(128);u.GetClassNameW(h,cl,128)
  print(hex(h),repr(title.value),cl.value)
 return True
u.EnumWindows(CB(enum),0)
