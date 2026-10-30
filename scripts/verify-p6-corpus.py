"""Compare unmodified public P6 fixtures with JOBS and the pinned independent MPXJ reader."""
import argparse
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path('build'))
parser.add_argument('--corpus', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
source, output = args.corpus.resolve(), args.output.resolve()
output.mkdir(parents=True, exist_ok=False)
manifest = json.loads((source/'manifest.json').read_text(encoding='utf-8-sig'))
name = 'jobs_primavera_corpus_probe' + ('.exe' if sys.platform == 'win32' else '')
binary = next(p for p in (args.build_dir/name, args.build_dir/'Release'/name) if p.is_file()).resolve()
run = subprocess.run([str(binary), str(source), str(output/'exports')], capture_output=True, timeout=120)
(output/'jobs.json').write_bytes(run.stdout)
(output/'jobs.log').write_bytes(run.stderr)
if run.returncode:
    raise SystemExit(f'JOBS corpus probe exited {run.returncode}; inspect jobs.json/jobs.log and Qt runtime PATH')
jobs = {r['file']: r for r in json.loads(run.stdout)['files']}

import mpxj
import jpype
import importlib.metadata
assert importlib.metadata.version('mpxj') == '16.10.0'
assert importlib.metadata.version('JPype1') == '1.6.0'
jpype.startJVM()
from org.mpxj.reader import UniversalProjectReader
from org.mpxj import TimeUnit
from java.nio.charset import Charset
from java.time import DayOfWeek


def raw_tables(data, encoding):
    tables = {}
    rows = None
    fields = []
    for line in data.decode(encoding).splitlines():
        cells = line.split('\t')
        if cells[0] == '%T':
            rows = tables[cells[1]] = []
        elif cells[0] == '%F':
            fields = cells[1:]
        elif cells[0] == '%R':
            rows.append(dict(zip(fields, cells[1:]+['']*max(0, len(fields)-len(cells)+1))))
    return tables


def read_model(path, encoding):
    proxy = UniversalProjectReader().getProjectReaderProxy(str(path))
    if proxy is None:
        raise ValueError('No independent P6 reader')
    try:
        reader = proxy.getProjectReader()
        reader.setIgnoreErrors(False)
        reader.setCharset(Charset.forName(encoding))
        model = proxy.read()
        if list(model.getIgnoredErrors()):
            raise ValueError('Independent reader ignored errors')
        return model
    finally:
        proxy.close()


matrix = []
failed = False
try:
    for entry in manifest['files']:
        path = source/entry['file']
        data = path.read_bytes()
        assert hashlib.sha256(data).hexdigest() == entry['sha256']
        tables = raw_tables(data, entry['encoding'])
        native = jobs[path.name]
        row = {'file': path.name, 'sha256': entry['sha256'], 'source_url': entry['url'],
               'jobs_status': native['status'], 'jobs_error': native.get('error_code'),
               'raw_counts': {t: len(tables.get(t, [])) for t in ('TASK', 'TASKPRED', 'CALENDAR', 'RSRC', 'ROLES', 'TASKRSRC')},
               'p6_execution_equivalence': 'unverified', 'differences': []}
        try:
            model = read_model(path, entry['encoding'])
            leaves = {str(t.getUniqueID()): t for t in model.getTasks() if not t.getSummary()}
            row['mpxj_status'] = 'read'
            row['mpxj_counts'] = {'tasks': len(leaves), 'relationships': sum(len(list(t.getPredecessors())) for t in leaves.values()),
                                  'calendars': len(list(model.getCalendars())),
                                  'resources': sum(not r.getRole() for r in model.getResources()),
                                  'roles': sum(bool(r.getRole()) for r in model.getResources()),
                                  'assignments': len(list(model.getResourceAssignments()))}
            assert set(leaves) == {r['task_id'] for r in tables.get('TASK', [])}, 'independent task identity differs'
            assert row['mpxj_counts']['relationships'] == len(tables.get('TASKPRED', [])), 'independent relationships differ'
            assert row['mpxj_counts']['assignments'] == len(tables.get('TASKRSRC', [])), 'independent assignment count differs'
            assert row['mpxj_counts']['resources'] == len(tables.get('RSRC', [])), 'independent resource count differs'
            assert {str(r.getUniqueID()) for r in model.getResources() if not r.getRole()} == {r['rsrc_id'] for r in tables.get('RSRC', [])}, 'independent resource identity differs'
            independent_edges = sorted((str(r.getPredecessorTask().getUniqueID()),
                                        str(r.getSuccessorTask().getUniqueID()), str(r.getType()),
                                        round(float(r.getLag().convertUnits(TimeUnit.MINUTES, model.getProjectProperties()).getDuration()), 4))
                                       for t in leaves.values() for r in t.getPredecessors())
            source_edges = sorted((r['pred_task_id'], r['task_id'], r['pred_type'].removeprefix('PR_'),
                                   round(float(Decimal(r.get('lag_hr_cnt') or '0')*60), 4))
                                  for r in tables.get('TASKPRED', []))
            assert independent_edges == source_edges, 'independent relationship types/lags differ'
            row['relationship_types_checked'] = sorted({r[2] for r in source_edges})
            if native['status'] == 'imported':
                by_id = {t['id']: t for t in native['tasks']}
                for task in tables['TASK']:
                    target = by_id['xer-task-'+task['task_id']]
                    independent = leaves[task['task_id']]
                    assert target['name'] == str(independent.getName()), 'task name differs'
                    planned = Decimal(task.get('target_drtn_hr_cnt') or '0')*60
                    if task['task_type'] in ('TT_Mile', 'TT_FinMile'):
                        planned = 0
                    assert abs(Decimal(target['duration_minutes'])-planned) <= Decimal('.00003'), 'planned duration differs'
                    duration = independent.getDuration()
                    if duration is not None:
                        current = float(duration.convertUnits(TimeUnit.MINUTES, model.getProjectProperties()).getDuration())
                        if abs(current-target['duration_minutes']) > .01:
                            row['differences'].append({'task_id': task['task_id'], 'field': 'current_vs_planned_duration',
                                                       'mpxj_current_minutes': current, 'jobs_planned_minutes': target['duration_minutes']})
                source_calendars = {str(c.getUniqueID()): c for c in model.getCalendars()}
                for calendar in native['calendars']:
                    independent = source_calendars[calendar['id'].removeprefix('xer-cal-')]
                    for day in range(1, 8):
                        hours = independent.getCalendarHours(DayOfWeek.of(day))
                        actual = []
                        if hours is not None:
                            for period in hours:
                                start = period.getStart().toSecondOfDay()//60
                                actual.append([start, start+period.getDurationAsMilliseconds()//60000])
                        expected = calendar['periods'] if day in calendar['weekdays'] else []
                        assert actual == expected, f'effective calendar hours differ: {calendar["id"]} day {day}'
                exported = output/'exports'/path.name
                exported_model = read_model(exported, 'UTF-8')
                exported_tasks = [t for t in exported_model.getTasks() if not t.getSummary()]
                assert len(exported_tasks) == len(native['tasks']), 'independent exported tasks differ'
                assert sorted(str(t.getName()) for t in exported_tasks) == sorted(t['name'] for t in native['tasks']), 'exported names differ'
                independent_minutes = sorted(round(float(t.getDuration().convertUnits(TimeUnit.MINUTES, exported_model.getProjectProperties()).getDuration()), 4) for t in exported_tasks)
                assert independent_minutes == sorted(t['duration_minutes'] for t in native['tasks']), 'exported planned durations differ'
                assert native['jobs_round_trip_dates_equal'], 'JOBS supported projection dates differ'
                row['supported_projection'] = 'passed'
                row['milestones_checked'] = sum(t['duration_minutes'] == 0 for t in native['tasks'])
                row['jobs_finish'] = native['finish']
            else:
                row['differences'].append({'field': 'native_projection', 'reason': native['reason']})
        except Exception as error:
            row['mpxj_status'] = 'error'
            row['independent_error'] = str(error)
            failed = True
        matrix.append(row)
finally:
    jpype.shutdownJVM()
result = {'status': 'failed' if failed else 'passed', 'mpxj': '16.10.0', 'jpype': '1.6.0',
          'scope': 'Unmodified public P6 files; independent parsing and supported JOBS projection. No Oracle execution equivalence.',
          'files': matrix}
(output/'matrix.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print(json.dumps([{k: r.get(k) for k in ('file', 'jobs_status', 'jobs_error', 'mpxj_status', 'supported_projection', 'independent_error')} for r in matrix], ensure_ascii=False, indent=2))
raise SystemExit(1 if failed else 0)
