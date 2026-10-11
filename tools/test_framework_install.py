"""Isolated installer transaction checks, with a fake game and no save access."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import framework as f
import framework_install as installer

class Installation(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);self.game=self.root/'game';self.build=self.root/'work/framework-build';self.folder=self.root/'framework/characters/fixture'
  for p in [self.game/'MarioSouls',self.build/'characters/fixture',self.folder,self.root/'checkpoints']:p.mkdir(parents=True)
  self.runtime=self.game/'SoulsCharacterFramework/characters/fixture/runtime.dll';self.runtime.parent.mkdir(parents=True);self.runtime.write_bytes(b'runtime')
  (self.game/'DARKSOULS.exe').write_bytes(b'fake-executable');(self.game/'dinput8.dll').write_bytes(b'old-host')
  (self.game/'mariosouls.ini').write_text('[MarioSouls]\nLibrary='+str(self.runtime)+'\n[Combat]\nBasePower=105\n');(self.game/'MarioSouls/install.json').write_text('{}')
  (self.game/'MarioSouls/harness.ini').write_text('[Harness]\nArmed=1\nHeartbeat=123\n')
  (self.build/'dinput8.dll').write_bytes(b'new-host');(self.build/'characters/fixture/character.dll').write_bytes(b'adapter')
  self.profile={'status':'implemented','assets':{'required':False,'runtime':'runtime.dll'}};(self.folder/'character.json').write_text(json.dumps(self.profile))
  expected={'adapter_sha256':f.sha(self.build/'characters/fixture/character.dll'),'host_sha256':f.sha(self.build/'dinput8.dll')}
  (self.build/'characters/fixture/build.json').write_text(json.dumps({**expected,'source_sha256':{}}))
  (self.build/'test-report.json').write_text(json.dumps({**expected,'status':'passed','character':'fixture','runtime_sha256':f.sha(self.runtime)}))
  (self.root/'checkpoints/mariosouls-v0.7.json').write_text(json.dumps({'game_exe_sha256':f.sha(self.game/'DARKSOULS.exe')}))
  self.patches=[patch.object(f,'ROOT',self.root),patch.object(f,'GAME',self.game),patch.object(f,'BUILD',self.build),patch.object(f,'load_profile',return_value=(self.folder,self.profile)),patch.object(installer,'require_closed')]
  for p in self.patches:p.start()
 def tearDown(self):
  for p in reversed(self.patches):p.stop()
  self.temp.cleanup()
 def call(self,check=False):installer.install('fixture',None,str(self.runtime),check)
 def test_check_does_not_mutate(self):
  before={str(p):f.sha(p) for p in self.root.rglob('*') if p.is_file()};self.call(True)
  self.assertEqual(before,{str(p):f.sha(p) for p in self.root.rglob('*') if p.is_file()})
 def test_reinstall_runtime_at_destination(self):
  self.call();self.assertEqual((self.game/'dinput8.dll').read_bytes(),b'new-host');self.assertEqual(self.runtime.read_bytes(),b'runtime')
  self.assertIn('Armed = 0',(self.game/'MarioSouls/harness.ini').read_text())
  backups=list((self.root/'work/checkpoints').glob('before-framework-*'));self.assertEqual((backups[0]/'dinput8.dll').read_bytes(),b'old-host')
 def test_process_guard(self):
  with patch.object(installer,'require_closed',side_effect=RuntimeError('Running')):
   with self.assertRaisesRegex(RuntimeError,'Running'):self.call()
  self.assertEqual((self.game/'dinput8.dll').read_bytes(),b'old-host')
 def test_untested_binary(self):
  (self.build/'dinput8.dll').write_bytes(b'different')
  with self.assertRaisesRegex(RuntimeError,'exact build'):self.call()
 def test_rollback_after_partial_copy(self):
  original=installer.shutil.copy2
  # Inject one failure after the host/adapter have been copied, then permit rollback.
  def once(src,dst,*a,**kw):
   if Path(dst)==self.runtime and not getattr(once,'failed',False):once.failed=True;raise OSError('simulated disk failure')
   return original(src,dst,*a,**kw)
  with patch.object(installer.shutil,'copy2',side_effect=once):
   with self.assertRaisesRegex(OSError,'simulated'):self.call()
  self.assertEqual((self.game/'dinput8.dll').read_bytes(),b'old-host');self.assertFalse((self.runtime.parent/'character.dll').exists());self.assertEqual(self.runtime.read_bytes(),b'runtime')

if __name__=='__main__':unittest.main()
