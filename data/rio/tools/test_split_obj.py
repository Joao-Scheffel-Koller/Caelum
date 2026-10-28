"""Small regression for deterministic splitting and recursive size limits."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import json


class SplitObjTest(unittest.TestCase):
    def test_reproducible_recursive_export(self):
        scratch=Path(__file__).resolve().parents[3]/'build'
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as temp:
            root=Path(temp)
            text=['mtllib test.mtl','vn 0 1 0','vt 0 0','vt 1 0','vt 0 1']
            for z in (100,600,1100,1600):
                for x in (100,600,1100,1600):
                    text.extend([f'v {x} 0 {z}',f'v {x+1} 0 {z}',f'v {x} 0 {z+1}'])
            for i in range(16):
                text.append('usemtl '+('blue' if i%2 else 'white'))
                text.append(f'f {3*i+1}/1/1 {3*i+2}/2/1 {3*i+3}/3/1' if i<15 else 'f -3/1/-1 -2/2/-1 -1/3/-1')
            (root/'test.obj').write_text('\n'.join(text)+'\n')
            (root/'test.mtl').write_text('newmtl white\nKd 1 1 1\nnewmtl blue\nKd 0 0 1\n')
            outputs=[]
            for run in ('first','second'):
                out=root/run
                subprocess.run([sys.executable,str(Path(__file__).with_name('split_obj.py')),str(root/'test.obj'),str(out),'--max-bytes','1024'],check=True,capture_output=True)
                outputs.append({str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.is_file()})
                data=json.loads((out/'manifest.json').read_text())
                self.assertEqual(sum(c['faces'] for c in data['chunks']),16)
                self.assertGreater(len(data['chunks']),1)
                self.assertTrue(all(c['bytes']<=1024 for c in data['chunks']))
            self.assertEqual(*outputs)


if __name__=='__main__':unittest.main()
