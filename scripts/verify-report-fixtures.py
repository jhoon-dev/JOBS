"""Read exports from jobs_report_tests independently; never rewrite them.

Optional development/CI dependencies: openpyxl==3.1.5, pypdf==6.10.0.
Usage: python scripts/verify-report-fixtures.py build
"""
import re
import sys
import zipfile
from pathlib import Path

from openpyxl import load_workbook
from pypdf import PdfReader


def verify(root: Path):
    for name in ("reports-smoke.xlsx", "reports-resources.xlsx"):
        with zipfile.ZipFile(root / name) as archive:
            assert archive.testzip() is None, "ZIP checksum failure"
            assert not any("externalLink" in p or "vbaProject" in p for p in archive.namelist())
        book = load_workbook(root / name, data_only=False)
        assert "출처와 설정" in book.sheetnames
        for sheet in book:
            assert sheet.freeze_panes == "A7"
            assert sheet.auto_filter.ref.startswith("A6:")
            assert not any(cell.data_type == "f" for row in sheet for cell in row)
        if name == "reports-smoke.xlsx":
            assert len(book.sheetnames) == 14
            costs = book["작업별 원가"]
            assert costs["G7"].value == 6 and costs["G7"].data_type == "n"
            assert costs["G8"].value == 0 and costs["G8"].data_type == "n"
            task = book["계획 일정"]["B7"]
            assert task.value.startswith("=HYPERLINK") and task.data_type == "s"
            assert book["실적과 예측"]["C7"].value == "미정"
            assert book["기간별 원가"]["G7"].value == 6
            assert book["기간별 원가"]["G8"].value == "미정"
            metadata = dict(book["출처와 설정"].iter_rows(min_row=7, values_only=True))
            assert metadata["보고서 ID"] == "report-contract-fixture"
            assert metadata["원본 상태"] == "미저장 메모리 revision"
        else:
            assert "작업별 원가" not in book.sheetnames and "자원 등록과 가용량" in book.sheetnames
        print(f"PASS: {name}: {len(book.sheetnames)} sheets, ZIP/OOXML/types/unknown/provenance")

    for name in ("reports-smoke.pdf", "reports-long-portrait.pdf", "reports-a3.pdf"):
        reader = PdfReader(root / name, strict=True)
        compact = re.sub(r"\s+", "", "\n".join(p.extract_text() or "" for p in reader.pages))
        assert reader.pages and all("JOBS" in (p.extract_text() or "") for p in reader.pages)
        if name == "reports-long-portrait.pdf":
            numbers = [int(n) for n in re.findall(r"Segment_(\d+)", compact)]
            assert len(numbers) == 700 and set(numbers) == set(range(700)), "long row lost/duplicated content"
            assert "END_SENTINEL_739" in compact and len(reader.pages) >= 3
            assert abs(float(reader.pages[0].mediabox.width) - 595) < 3
            assert abs(float(reader.pages[0].mediabox.height) - 842) < 3
        else:
            assert "report-contract-fixture" in compact and "6.00" in compact
            width = 1191 if name == "reports-a3.pdf" else 842
            assert abs(float(reader.pages[0].mediabox.width) - width) < 3
        print(f"PASS: {name}: {len(reader.pages)} pages, size/text/provenance/continuation")


if __name__ == "__main__":
    verify(Path(sys.argv[1] if len(sys.argv) > 1 else "build"))
