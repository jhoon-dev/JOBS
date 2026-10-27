"""Independently inspect the fixed C++ export using Python's standard XML reader."""
import hashlib
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else "build")
payload = (root / "project-xml-export.xml").read_bytes()
report = json.loads((root / "project-xml-export.json").read_text(encoding="utf-8"))
ns = {"p": "http://schemas.microsoft.com/project"}
doc = ET.fromstring(payload)
assert doc.tag == "{http://schemas.microsoft.com/project}Project"
assert doc.findtext("p:SaveVersion", namespaces=ns) == "12"
assert doc.findtext("p:Name", namespaces=ns) == "한글 계획 & 교환"
assert doc.findtext("p:StartDate", namespaces=ns) == "2026-10-26T09:00:00"
tasks = doc.findall("p:Tasks/p:Task", ns)
ids = {t.findtext("p:UID", namespaces=ns) for t in tasks}
assert len(tasks) == len(ids) == 7
leaves = [t for t in tasks if t.findtext("p:Summary", namespaces=ns) == "0"]
assert len(leaves) == 6
assert leaves[0].findtext("p:Duration", namespaces=ns) == "PT8H0M0S"
assert leaves[-1].findtext("p:Milestone", namespaces=ns) == "1"
assert leaves[-1].findtext("p:Duration", namespaces=ns) == "PT0H0M0S"
assert all(t.find("p:Start", ns) is None and t.find("p:ActualWork", ns) is None for t in tasks)
links = doc.findall("p:Tasks/p:Task/p:PredecessorLink", ns)
assert {l.findtext("p:Type", namespaces=ns) for l in links} == {"0", "1", "2", "3"}
assert [l.findtext("p:LinkLag", namespaces=ns) for l in links] == ["600", "-600", "-600", "600"]
assert all(l.findtext("p:PredecessorUID", namespaces=ns) in ids for l in links)
assert leaves[1].findtext("p:ConstraintType", namespaces=ns) == "4"
assert leaves[1].findtext("p:ConstraintDate", namespaces=ns) == "2026-10-28T10:00:00"
calendars = doc.findall("p:Calendars/p:Calendar", ns)
cal_ids = {c.findtext("p:UID", namespaces=ns) for c in calendars}
assert len(calendars) == 3
assert all(t.findtext("p:CalendarUID", namespaces=ns) in cal_ids for t in leaves)
assert all(len(c.findall("p:WeekDays/p:WeekDay[p:DayType='2']/p:WorkingTimes/p:WorkingTime", ns)) == 2 for c in calendars)
assert any(c.find("p:WeekDays/p:WeekDay[p:DayType='0']/p:TimePeriod", ns) is not None for c in calendars)
resources = doc.findall("p:Resources/p:Resource", ns)
assert len(resources) == 1
assert resources[0].findtext("p:Name", namespaces=ns) == "설계자 <1>"
assert resources[0].findtext("p:MaxUnits", namespaces=ns) == "1.50"
assignments = doc.findall("p:Assignments/p:Assignment", ns)
assert [a.findtext("p:Units", namespaces=ns) for a in assignments] == ["0.50", "1.25"]
assert all(a.findtext("p:TaskUID", namespaces=ns) in ids for a in assignments)
assert report["xmlSha256"] == hashlib.sha256(payload).hexdigest()
assert report["compatibility"]["jobsRecomputation"] == "unverified"
assert report["compatibility"]["externalRoundTrip"] == "unverified"
assert len(report["mapping"]) == 13
print("Independent Project XML reader: namespace, identities, four links, calendars, resource units and source verdicts passed")
