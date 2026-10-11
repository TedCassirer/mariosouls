"""Exercise restoration in temporary directories, never the real game folder."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import checkpoint as c


class CheckpointTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.root=Path(self.tmp.name)
        self.game=self.root/'fake-game';self.game.mkdir()
        (self.game/'DARKSOULS.exe').write_bytes(b'fixture executable')
        self.archive=self.root/'runtime.zip'
        self.contents={'dinput8.dll':b'checkpoint library','MarioSouls/settings.ini':b'checkpoint settings'}
        with zipfile.ZipFile(self.archive,'w') as z:
            for name,data in self.contents.items():z.writestr(name,data)
        self.manifest={'archive':'runtime.zip','archive_sha256':c.digest(self.archive),
            'game_directory':str(self.game),'game_exe_sha256':c.digest(self.game/'DARKSOULS.exe'),
            'files':{name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in self.contents.items()}}
        self.root_patch=patch.object(c,'ROOT',self.root);self.root_patch.start()
        self.process_patch=patch.object(c,'game_running',return_value=False);self.process_patch.start()

    def tearDown(self):
        self.process_patch.stop();self.root_patch.stop();self.tmp.cleanup()

    def test_verify_rejects_corruption(self):
        c.verify(self.manifest)
        self.archive.write_bytes(self.archive.read_bytes()+b'changed')
        with self.assertRaises(ValueError):c.verify(self.manifest)

    def test_paths_cannot_escape(self):
        for name in ['../outside','/absolute','C:/outside','..\\outside']:
            with self.assertRaises(ValueError):c.safe_path(self.game,name)

    def test_restore_backs_up_runtime_and_preserves_saves(self):
        (self.game/'dinput8.dll').write_bytes(b'newer runtime')
        (self.game/'character.sl2').write_bytes(b'untouched save')
        c.restore(self.manifest,c.verify(self.manifest))
        for name,data in self.contents.items():self.assertEqual((self.game/name).read_bytes(),data)
        self.assertEqual((self.game/'character.sl2').read_bytes(),b'untouched save')
        backup=next((self.root/'work/checkpoints').glob('before-restore-*'))
        self.assertEqual((backup/'dinput8.dll').read_bytes(),b'newer runtime')

    def test_running_game_blocks_restore(self):
        with patch.object(c,'game_running',return_value=True):
            with self.assertRaises(RuntimeError):c.restore(self.manifest,self.archive)
        self.assertFalse((self.game/'dinput8.dll').exists())

    def test_partial_write_is_rolled_back(self):
        (self.game/'dinput8.dll').write_bytes(b'original runtime')
        real_copy=c.shutil.copy2
        def fail_once(source,dest,*a,**kw):
            if Path(dest)==self.game/'MarioSouls/settings.ini':raise OSError('simulated write failure')
            return real_copy(source,dest,*a,**kw)
        with patch.object(c.shutil,'copy2',side_effect=fail_once):
            with self.assertRaises(OSError):c.restore(self.manifest,self.archive)
        self.assertEqual((self.game/'dinput8.dll').read_bytes(),b'original runtime')
        self.assertFalse((self.game/'MarioSouls/settings.ini').exists())


if __name__=='__main__':unittest.main()
