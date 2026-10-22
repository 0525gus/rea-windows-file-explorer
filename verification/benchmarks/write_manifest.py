"""Hash only caller-selected harness and existing A build inputs; never run the app."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--build-directory', required=True)
args = parser.parse_args()
task_root = Path(__file__).resolve().parent
project_root = task_root.parent.parent
build_root = Path(args.build_directory).resolve()
a_root = project_root / 'experiments/A'


def identity(path):
    path = Path(path)
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return {'path': str(path.resolve()), 'bytes': path.stat().st_size,
            'sha256': digest}


freeze = json.loads((project_root / 'verification/phase6-freeze-verification.json').read_text(encoding='utf-8'))
a_manifest_path = a_root / 'docs/evaluation/initial-freeze-manifest.json'
b_manifest_path = project_root / 'docs/evaluation/initial-freeze-manifest.json'
assert identity(a_manifest_path)['sha256'] == freeze['A_manifest_sha256']
assert identity(b_manifest_path)['sha256'] == freeze['B_manifest_sha256']
a_manifest = json.loads(a_manifest_path.read_text(encoding='utf-8'))
initial_a_source = []
for record in a_manifest['files']:
    relative = record['path'].replace('\\', '/')
    if relative == 'CMakeLists.txt' or relative.startswith(('include/', 'src/')):
        live = identity(a_root / relative)
        frozen = identity(a_root / 'artifacts/initial-freeze' / relative)
        assert live['sha256'] == frozen['sha256'] == record['sha256'], relative
        initial_a_source.append({'relative_path': relative, 'sha256': record['sha256'],
                                 'bytes': record['bytes'], 'live_equals_frozen': True})

expected_libraries = {
    'explorer_core.lib': 'b557d6802cd79c061ec3ae4f763c8e79751ac27fc17c17805a18c700903ace5c',
    'explorer_ui.lib': '83fa500d3891b8da4c2eba4464ae4032d9f7246ba657f9a9ed9fa2a21045375e',
}
libraries = []
for name, expected in expected_libraries.items():
    value = identity(a_root / 'build-windows' / name)
    assert value['sha256'] == expected
    libraries.append(value)
runtime = []
for path in (build_root / 'a-runtime-dlls.txt').read_text(encoding='utf-8').splitlines():
    if path:
        original = identity(path)
        deployed = identity(build_root / Path(path).name)
        assert deployed['sha256'] == original['sha256']
        runtime.append({'sdk': original, 'app_local': deployed})

logs = sorted((build_root / 'logs').glob('*'))
assert logs
build_log = logs[-1]
stages = {}
for stage in ('configure', 'build'):
    metrics_path = build_log / f'{stage}.metrics.json'
    value = json.loads(metrics_path.read_text(encoding='utf-8'))
    assert value['exit_code'] == 0
    stages[stage] = {'metrics': identity(metrics_path), 'actual': value,
                     'stdout': identity(build_log / f'{stage}.stdout.log'),
                     'stderr': identity(build_log / f'{stage}.stderr.log')}

manifest = {
    'schema_version': 1,
    'prepared_at_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope': 'Phase6 validation-only A shown-GUI benchmark; neither frozen implementation modified',
    'benchmark_executed': False,
    'freeze': freeze,
    'freeze_record': identity(project_root / 'verification/phase6-freeze-verification.json'),
    'A_initial_manifest': identity(a_manifest_path),
    'B_initial_manifest': identity(b_manifest_path),
    'A_existing_imported_libraries': libraries,
    'A_source_identity_reference': initial_a_source,
    'A_headers_used': 'experiments/A/artifacts/initial-freeze/include',
    'harness_files': [identity(task_root / name) for name in
                      ('CMakeLists.txt', 'a_gui_benchmark.cpp', 'write_manifest.py', 'README.md')],
    'executable': identity(build_root / 'comparison_a_gui.exe'),
    'runtime_dlls': runtime,
    'configure_cache': identity(build_root / 'CMakeCache.txt'),
    'compile_commands': identity(build_root / 'compile_commands.json'),
    'actual_build_stages': stages,
    'B_measurement_semantics_reference': identity(project_root / 'artifacts/initial-freeze-b/src/main.cpp'),
    'fixture_manifest': identity(project_root / 'test-data/fixture-v1/manifest.json'),
    'expected_names_sha256': 'ff84bde034b35b40f6a7d8ef0e85c04966d069644ac12ea9db0408591d4fa8b0',
    'readiness_limitation': 'A has no public global controller-worker idle API; require correct current and listing location, not loading, not operation-busy and exact 1200 model/proxy names. No private state inspection.',
    'identity_limitation': 'Existing static libraries are pinned by digest and linked unchanged. No reproducible source-to-library correspondence proof is asserted beyond source snapshot equality and existing A CMake build metadata.',
}
output = task_root / 'a-gui-benchmark-manifest.json'
with output.open('x', encoding='utf-8', newline='\n') as stream:
    json.dump(manifest, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
print(json.dumps({'manifest': str(output), 'sha256': identity(output)['sha256'],
                  'executable': manifest['executable'], 'benchmark_executed': False}))
