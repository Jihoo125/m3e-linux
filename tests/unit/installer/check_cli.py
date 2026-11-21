import json
import os
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[3]
cli = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'tests/unit/installer/.clean-build/matinstall-core'
example = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else root / 'installer/matinstall-core/examples/answer.json'
answer = json.loads(example.read_text())

validated = subprocess.run([str(cli), 'validate', str(example)], text=True, capture_output=True)
assert validated.returncode == 0, validated
v = json.loads(validated.stdout)
assert v['valid'] and v['disk_size_validated'] and v['partitions'] == 2
plan_result = subprocess.run([str(cli), 'plan', str(example)], text=True, capture_output=True)
assert plan_result.returncode == 0, plan_result
plan = json.loads(plan_result.stdout)
assert plan['dry_run'] is True and plan['disk_probed'] is False
assert len(plan['partitions']) == 2 and plan['partitions'][0]['role'] == 'efi'
assert plan['partitions'][1]['role'] == 'root'
assert any('mkfs.ext4' in step['command'] for step in plan['commands'])
assert any('grub-install' in step['command'] for step in plan['commands'])
assert 'replace-this-in-your-private-answer' not in plan_result.stdout
assert any(step['command'].endswith('chpasswd  # stdin: <redacted>') for step in plan['commands'])
assert not any(step.get('stdin') not in (None, '<redacted>') for step in plan['commands'])
probe = subprocess.run([str(cli), 'probe'], text=True, capture_output=True)
assert probe.returncode == 0, probe
assert isinstance(json.loads(probe.stdout)['blockdevices'], list)
run = subprocess.run([str(cli), 'run', str(example)], text=True, capture_output=True)
assert run.returncode != 0 and not run.stdout, run
run_with_flag = subprocess.run([str(cli), 'run', str(example), '--i-understand-this-erases-data'], text=True, capture_output=True)
assert run_with_flag.returncode != 0 and not run_with_flag.stdout, run_with_flag

answer['disk'] = '/dev/matinstall-nonexistent-test-disk'
answer['source'] = '/nonexistent/matinstall-test.squashfs'
answer['disk_size_bytes'] = 128849018880
answer['destructive'] = False
tmp = pathlib.Path(os.environ.get('TMPDIR', '/tmp')) / 'matinstall-cli-safety-answer.json'
tmp.write_text(json.dumps(answer))
try:
    subprocess.run([str(cli), 'plan', str(tmp)], check=True, text=True, capture_output=True)
    missing_flag = subprocess.run([str(cli), 'run', str(tmp)], text=True, capture_output=True)
    assert missing_flag.returncode != 0
    answer['destructive'] = True
    tmp.write_text(json.dumps(answer))
    destructive_plan = subprocess.run([str(cli), 'plan', str(tmp)], text=True, capture_output=True)
    assert destructive_plan.returncode == 0 and json.loads(destructive_plan.stdout)['dry_run']
    missing_flag = subprocess.run([str(cli), 'run', str(tmp)], text=True, capture_output=True)
    assert missing_flag.returncode != 0 and not missing_flag.stdout
    both = subprocess.run([str(cli), 'run', str(tmp), '--i-understand-this-erases-data'], text=True, capture_output=True)
    assert both.returncode != 0 and not both.stdout
finally:
    tmp.unlink(missing_ok=True)

print('validate: 2 partitions; real lsblk probe parsed')
print('plan (destructive false/true): dry_run=true, password absent, commands present')
print('run: both missing confirmations refused; nonexistent target/source refused safely')
