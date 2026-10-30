"""Create a deterministic, fictional industrial construction XER for manual acceptance."""
from collections import Counter
from datetime import date
import hashlib
import json
from pathlib import Path
import argparse

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--tasks', type=int, choices=(1000, 10000), default=10000)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
facilities = args.tasks // 1000
output = args.output.resolve()
output.mkdir(exist_ok=False)
lines = ['ERMHDR\t20.12\t2026-10-30\tProject\tJOBS synthetic fixture\tJOBS\tdbxDatabaseNoName\tProject Management\tKRW']

def table(name, columns, rows):
    lines.append('%T\t' + name)
    lines.append('%F\t' + '\t'.join(columns))
    for row in rows:
        assert len(row) == len(columns)
        values = [str(value) for value in row]
        assert all('\t' not in value and '\n' not in value for value in values)
        lines.append('%R\t' + '\t'.join(values))

def calendar(saturday=False, finish=18):
    days = []
    for day in range(1, 8):
        periods = '' if day == 1 or (day == 7 and not saturday) else (
            '(0||0(s|09:00|f|12:00)())(0||1(s|13:00|f|' + str(finish) + ':00)())')
        days.append('(0||' + str(day) + '()(' + periods + '))')
    shutdowns = [date(2026, 12, 31), date(2027, 1, 1)]
    exceptions = ''.join('(0||' + str(index) + '(d|' + str((value-date(1899, 12, 30)).days) + ')())'
                         for index, value in enumerate(shutdowns))
    return '(0||CalendarData()((0||DaysOfWeek()(' + ''.join(days) + '))(0||Exceptions()(' + exceptions + '))))'

table('PROJECT', ['proj_id', 'proj_short_name', 'clndr_id', 'plan_start_date'],
      [[100, f'산업단지 조성 · 합성 테스트 {args.tasks:,}작업', 1, '2026-11-02 09:00']])
table('SCHEDOPTIONS', ['schedoptions_id', 'proj_id', 'sched_calendar_on_relationship_lag'],
      [[1, 100, 'rcal_Successor']])
table('CALENDAR', ['clndr_id', 'clndr_name', 'proj_id', 'base_clndr_id', 'clndr_data'], [
    [1, '설계·관리 주5일 8시간', 100, '', calendar()],
    [2, '현장 주6일 8시간', 100, '', calendar(True)],
    [3, '시운전 주5일 7시간', 100, '', calendar(finish=17)],
])
phases = [
    ('토목·기초', ['시공 계획', '측량·기준점', '가설·진입로', '토공·굴착', '지반 보강', '기초 배근', '기초 타설', '양생·점검', '기초 검측', '기초 인계']),
    ('골조', ['골조 착수 검토', '자재 반입', '철근 가공', '거푸집 설치', '골조 타설', '탈형 점검', '층간 골조', '철골·접합부', '골조 검측', '골조 인계']),
    ('기계·전기', ['MEP 착수 검토', '장비 반입', '배관 주간선', '덕트 설치', '전력·통신 간선', '패널 설치', '분기 배관', '기기 결선', '단독 기능 점검', 'MEP 인계']),
    ('마감', ['마감 착수 검토', '방수·바탕', '칸막이', '천장', '바닥', '도장·보수', '창호·문', '외부 마감', '완료 검측', '마감 인계']),
    ('시운전·준공', ['시운전 계획', '사전 점검', '전원·설비 투입', '단독 시운전', '통합 시운전', '성능 확인', '보완 조치', '사용자 교육', '준공 검사', '준공 인계']),
]
wbs = [[10, 100, 'Y', f'합성 산업단지 {args.tasks:,}작업', '', 0]]
tasks, links, assignments = [], [], []
for group in range(20 * facilities):
    zone, facility = divmod(group, facilities)
    area = 100 + zone
    if facility == 0:
        wbs.append([area, 100, 'N', f'{zone+1:02}구역', 10, zone+1])
    parent = area
    if facilities > 1:
        parent = 1000 + group
        wbs.append([parent, 100, 'N', f'{facility+1:02}공구', area, facility+1])
    for phase, (discipline, steps) in enumerate(phases):
        package = 10000 + group*5 + phase
        wbs.append([package, 100, 'N', discipline, parent, phase+1])
        for step, name in enumerate(steps):
            index = group*50 + phase*10 + step
            task = 20000 + index
            cal = 2 if phase < 2 else 3 if phase == 4 else 1
            hours = 0 if step == 9 else 8 if step == 5 else 24 if step == 4 else (8, 16, 24, 16, 24, 8, 32, 16, 8)[step] + 4*(zone % 3)
            tasks.append([task, 100, package, cal, f'Z{zone+1:02}-P{phase+1}-{step+1:02}',
                          f'{zone+1:02}구역 {facility+1:02}공구 · {discipline} · {name}',
                          'TT_FinMile' if step == 9 else 'TT_Task', 'DT_FixedDrtn', hours, '', '', '', ''])
            def edge(predecessor, kind='PR_FS', lag=0):
                links.append([30000+len(links), task, predecessor, 100, 100, kind, lag])
            if step:
                edge(task-1, 'PR_SS' if step in (3, 7) else 'PR_FF' if step == 5 else 'PR_FS',
                     8 if step == 3 else 4 if step == 7 else 0)
            elif phase:
                edge(task-1)
            if step == 9:
                edge(task-3)  # merge testing and remedial branch into handover
            if phase == 0 and step == 0:
                if facility:
                    edge(task-50+2, 'PR_SS', 8)
                elif zone:
                    edge(task-facilities*50+2, 'PR_SS', 8)
            if step != 9:
                assignments.append([40000+len(assignments), task, 100, 500+zone*2+(phase % 2), '1.00', ''])
                if step in (0, 5, 8):
                    assignments.append([40000+len(assignments), task, 100, 600+phase*2+(zone % 2), '0.25', ''])
table('PROJWBS', ['wbs_id', 'proj_id', 'proj_node_flag', 'wbs_name', 'parent_wbs_id', 'seq_num'], wbs)
table('TASK', ['task_id', 'proj_id', 'wbs_id', 'clndr_id', 'task_code', 'task_name', 'task_type', 'duration_type',
               'target_drtn_hr_cnt', 'cstr_type', 'cstr_date', 'cstr_type2', 'cstr_date2'], tasks)
table('TASKPRED', ['task_pred_id', 'task_id', 'pred_task_id', 'proj_id', 'pred_proj_id', 'pred_type', 'lag_hr_cnt'], links)
resources = [[500+zone*2+team, f'{zone+1:02}동 현장 {"A" if team == 0 else "B"}팀', 'RT_Labor', 2]
             for zone in range(20) for team in range(2)]
resources += [[600+phase*2+team, f'{name} 공통지원 {team+1}팀', 'RT_Labor', 1]
              for phase, (name, _) in enumerate(phases) for team in range(2)]
table('RSRC', ['rsrc_id', 'rsrc_name', 'rsrc_type', 'clndr_id'], resources)
table('RSRCRATE', ['rsrc_rate_id', 'rsrc_id', 'start_date', 'max_qty_per_hr'],
      [[700+i, row[0], '2026-11-02 00:00', '2.00'] for i, row in enumerate(resources)])
table('TASKRSRC', ['taskrsrc_id', 'task_id', 'proj_id', 'rsrc_id', 'target_qty_per_hr', 'curv_id'], assignments)
lines.append('%E')
payload = ('\n'.join(lines)+'\n').encode('utf-8')
path = output / f'industrial-campus-{args.tasks}.xer'
path.write_bytes(payload)
assert len(tasks) == args.tasks and all(int(row[2]) < int(row[1]) for row in links)
summary = {'synthetic': True, 'source': 'deterministic JOBS demo generator; no customer data',
           'tasks': len(tasks), 'wbs_nodes_excluding_project_root': len(wbs)-1,
           'milestones': sum(row[6] == 'TT_FinMile' for row in tasks),
           'relationships': len(links), 'relationship_types': dict(Counter(row[5] for row in links)),
           'calendars': 3, 'resources': len(resources), 'assignments': len(assignments),
           'start': '2026-11-02', 'bytes': len(payload), 'sha256': hashlib.sha256(payload).hexdigest()}
(output / 'fixture.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
(output / 'README.txt').write_text('실제 고객 프로젝트가 아닌 합성 테스트 데이터입니다.\n'
    f'20개 구역 × {facilities}개 공구 × 5개 공종 × 10개 작업. FS/SS/FF 관계, 달력, 계획정지일, 자원 배정 포함.\n'
    '기간 기반 일정 검사이며 P6 재계산 일치 및 자원 가용량 충족을 보증하지 않습니다.\n', encoding='utf-8')
print(json.dumps({'file': str(path), **summary}, ensure_ascii=False, indent=2))
