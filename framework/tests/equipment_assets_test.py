"""Equipment asset boundaries: coordinate fitting, normals and safe prop files."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'hosts/ds1ptde'))
from prepare_equipment import transform_vertices, write_prop, MAGIC, HEADER
import numpy as np


class EquipmentAssets(unittest.TestCase):
    def triangle(self):
        return np.array([[0, 0, 0, 0, 1, 0, 0, 0],
                         [1, 0, 0, 0, 1, 0, 1, 0],
                         [0, 0, 1, 0, 1, 0, 0, 1]], dtype='<f4')

    def test_native_sword_and_shield_face_canonical_forward(self):
        p, n = transform_vertices([[0, -1, 0]], [[0, -1, 0]], np.eye(4), 'sword')
        np.testing.assert_allclose(p, [[0, 0, 1]])
        np.testing.assert_allclose(n, [[0, 0, 1]])
        p, n = transform_vertices([[.3, .4, -.2]], [[0, 0, -1]], np.eye(4), 'shield')
        np.testing.assert_allclose(p, [[-.3, .4, .2]])
        np.testing.assert_allclose(n, [[0, 0, 1]])

    def test_scaled_bone_normals_remain_perpendicular(self):
        matrix = np.diag([1., 2., 1., 1.])
        matrix[:3, 3] = [3., 4., 5.]
        p, n = transform_vertices([[0, 0, 0], [1, 1, 0]],
                                  [[1, -1, 0], [1, -1, 0]], matrix, 'shield')
        self.assertAlmostEqual(float(np.dot(p[1] - p[0], n[0])), 0., places=6)
        self.assertAlmostEqual(float(np.linalg.norm(n[0])), 1., places=6)
        np.testing.assert_allclose(p[0], [-3, 4, -5])

    def test_binary_layout_texture_alpha_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / 'prop.scprop'
            pixels = bytes([10, 20, 30, 0, 40, 50, 60, 255])
            info = write_prop(target, self.triangle(), [[0, 1, 2]], 2, 1, pixels)
            data = target.read_bytes()
            values = HEADER.unpack_from(data)
            self.assertEqual(values[:5], (MAGIC, 3, 1, 2, 1))
            self.assertEqual(values[5:], (0, 0, 0, 1, 0, 1))
            self.assertEqual(struct.unpack_from('<3I', data, HEADER.size + 3 * 32), (0, 1, 2))
            self.assertEqual(data[-8:], pixels)
            self.assertEqual(len(data), HEADER.size + 3 * 32 + 3 * 4 + 8)
            self.assertEqual(info['triangles'], 1)
            with self.assertRaises(FileExistsError):
                write_prop(target, self.triangle(), [[0, 1, 2]], 2, 1, pixels)
            self.assertEqual(target.read_bytes(), data)

    def test_invalid_input_is_rejected_before_creating_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / 'invalid.scprop'
            bad = self.triangle(); bad[0, 0] = float('nan')
            for vertices, indices, width, height, pixels in [
                (bad, [[0, 1, 2]], 1, 1, bytes(4)),
                (self.triangle(), [[0, 1, 3]], 1, 1, bytes(4)),
                (self.triangle(), [[0, -1, 2]], 1, 1, bytes(4)),
                (self.triangle(), [[0, 1, 2]], 2049, 1, bytes(4)),
                (self.triangle(), [[0, 1, 2]], 1, 1, bytes(3)),
            ]:
                with self.assertRaises(ValueError):
                    write_prop(target, vertices, indices, width, height, pixels)
                self.assertFalse(target.exists())


if __name__ == '__main__':
    unittest.main()
