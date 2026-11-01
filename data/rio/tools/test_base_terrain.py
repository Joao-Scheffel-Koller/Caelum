"""Regression tests for bounded background generation; no game/window needed."""
from pathlib import Path
import tempfile
import unittest
from build_base_terrain import build,project


class BaseTerrainTests(unittest.TestCase):
    def test_projection_matches_export(self):
        self.assertAlmostEqual(project(-22.84895,-43.2142)[0],0)
        x,z=project(-22.9045972,-43.163205)
        self.assertAlmostEqual(x,5231.293756,places=3)
        self.assertAlmostEqual(z,6195.886840,places=3)

    def test_bounds_winding_height_and_reproducibility(self):
        scratch=Path(__file__).resolve().parents[3]/'build';scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as temporary:
            root=Path(temporary)
            self.assertTrue(root.resolve().is_relative_to(scratch.resolve()))
            source=root/'map.osm'
            source.write_text('<osm><bounds minlat="-22.9306" minlon="-43.2803" maxlat="-22.7673" maxlon="-43.1481"/></osm>')
            output=root/'base_terrain.obj'
            report=build(source,output);first=output.read_bytes()
            points=[tuple(map(float,line.split()[1:])) for line in first.decode().splitlines() if line.startswith('v ')]
            self.assertEqual(len(points),4)
            self.assertTrue(all(p[1]==-.02 for p in points))
            self.assertEqual(sum(line.startswith('f ') for line in first.decode().splitlines()),2)
            self.assertIn('usemtl GRASS_0',first.decode())
            self.assertAlmostEqual(report['origin'][0],-22.84895)
            self.assertAlmostEqual(report['origin'][1],-43.2142)
            expected=[project(-22.7673,-43.2803),project(-22.9306,-43.1481)]
            self.assertAlmostEqual(min(p[0] for p in points),expected[0][0],places=5)
            self.assertAlmostEqual(max(p[2] for p in points),expected[1][1],places=5)
            a,b,c=points[:3]
            normal_y=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2])
            self.assertGreater(normal_y,0)
            self.assertEqual(build(source,output)['obj_sha256'],report['obj_sha256'])
            self.assertEqual(first,output.read_bytes())
            source.write_text('<osm/>')
            with self.assertRaises(ValueError):build(source,output)


if __name__=='__main__':unittest.main()
