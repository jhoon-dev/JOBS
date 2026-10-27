"""Independent TSV checks and MPXJ 16.10.0 parsing; never a P6 compatibility declaration."""
import hashlib
import json
import sys
from decimal import Decimal
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
source = (root / "primavera-export.xer").read_bytes()
assert source.startswith(b"\xef\xbb\xbf")
tables = {}
current = None
fields = None
ended = False
for index, line in enumerate(source.decode("utf-8-sig").splitlines()):
    cells = line.split("\t")
    if index == 0:
        assert cells[0:2] == ["ERMHDR", "20.12"] and cells[-1] == "KRW"
        continue
    assert not ended
    if cells[0] == "%T":
        assert cells[1] not in tables
        current = tables[cells[1]] = []
        fields = None
    elif cells[0] == "%F":
        assert fields is None and len(set(cells[1:])) == len(cells) - 1
        fields = cells[1:]
    elif cells[0] == "%R":
        assert fields and len(cells) == len(fields) + 1
        current.append(dict(zip(fields, cells[1:])))
    else:
        assert cells == ["%E"]
        ended = True
assert ended
assert len(tables["PROJECT"]) == 1
assert len(tables["PROJWBS"]) == 3 and tables["PROJWBS"][0]["wbs_name"] == "한글 XER 계획"
tasks = tables["TASK"]
assert len(tasks) == 6
assert [t["task_name"] for t in tasks] == ["A 설계", "B 개발", "C SS", "D FF", "E SF", "완료점"]
assert [Decimal(t["target_drtn_hr_cnt"]) for t in tasks] == [8, 2, 1, 2, 1, 0]
assert all(t["status_code"] == "TK_NotStart" and "act_start_date" not in t for t in tasks)
task_ids = {t["task_id"] for t in tasks}
assert len(task_ids) == 6
assert {r["pred_type"] for r in tables["TASKPRED"]} == {"PR_FS", "PR_SS", "PR_FF", "PR_SF"}
assert [Decimal(r["lag_hr_cnt"]) for r in tables["TASKPRED"]] == [1, -1, -1, 1]
assert all(r["task_id"] in task_ids and r["pred_task_id"] in task_ids for r in tables["TASKPRED"])
assert tables["SCHEDOPTIONS"][0]["sched_calendar_on_relationship_lag"] == "rcal_Successor"
assert len(tables["CALENDAR"]) == 3
assert len(tables["RSRC"]) == 2 and tables["RSRC"][1]["rsrc_type"] == "RT_Equip"
assert [Decimal(r["max_qty_per_hr"]) for r in tables["RSRCRATE"]] == [Decimal("1.50"), 0]
assert [Decimal(r["target_qty_per_hr"]) for r in tables["TASKRSRC"]] == [Decimal("0.50"), Decimal("1.25")]
report = json.loads((root / "primavera-export.json").read_text(encoding="utf-8"))
assert report["xerSha256"] == hashlib.sha256(source).hexdigest()
assert report["compatibility"]["externalRoundTrip"] == "unverified"
assert report["compatibility"]["jobsRecomputation"] == "unverified"
assert report["outputProjectCode"] == tables["PROJECT"][0]["proj_short_name"]
print("PASS: independent TSV reader, XER tables/fields/references/values/encoding/hash/verdicts")

import jpype
import mpxj  # Registers the bundled MPXJ jars with JPype.

jpype.startJVM()
proxy = None
try:
    from org.mpxj.primavera import PrimaveraXERFileReader
    from org.mpxj.reader import UniversalProjectReader
    from org.mpxj import TimeUnit
    # MPXJ's documented BOM handling belongs to the universal reader. Its proxy
    # preserves that preprocessing while permitting strict error reporting.
    proxy = UniversalProjectReader().getProjectReaderProxy(str(root / "primavera-export.xer"))
    assert proxy is not None
    reader = proxy.getProjectReader()
    assert isinstance(reader, PrimaveraXERFileReader)
    reader.setIgnoreErrors(False)
    model = proxy.read()
    assert not list(model.getIgnoredErrors()), list(model.getIgnoredErrors())
    leaves = [task for task in model.getTasks() if not task.getSummary()]
    assert len(leaves) == 6
    assert [str(t.getName()) for t in leaves] == [t["task_name"] for t in tasks]
    assert [round(float(t.getDuration().convertUnits(TimeUnit.MINUTES, model.getProjectProperties()).getDuration())) for t in leaves] == [480, 120, 60, 120, 60, 0]
    assert sum(len(list(t.getPredecessors())) for t in leaves) == 4
    assert len(list(model.getResources())) == 2
    assert len(list(model.getResourceAssignments())) == 2
    assert len(list(model.getCalendars())) == 3
    assert any(len(list(calendar.getCalendarExceptions())) == 2 for calendar in model.getCalendars())
    assert [float(a.getUnits()) for a in model.getResourceAssignments()] == [50.0, 125.0]
    from java.time import DayOfWeek
    proxy.close()
    proxy = UniversalProjectReader().getProjectReaderProxy(str(root / "primavera-24h.xer"))
    proxy.getProjectReader().setIgnoreErrors(False)
    around = proxy.read()
    assert not list(around.getIgnoredErrors())
    calendar = next(c for c in around.getCalendars() if str(c.getName()) == "24-hour")
    assert sum(r.getDurationAsMilliseconds() for r in calendar.getCalendarHours(DayOfWeek.MONDAY)) == 86400000
    print("PASS: MPXJ 16.10.0 strict read, names/durations/relations/calendars/exceptions/resources/assignments")
except Exception as error:
    if hasattr(error, "stacktrace"):
        print(error.stacktrace(), file=sys.stderr)
    raise
finally:
    if proxy is not None:
        proxy.close()
    jpype.shutdownJVM()
