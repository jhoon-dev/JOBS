"""Exercise real formatter/analyzer failure gates in an isolated temporary repo."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import unittest

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("quality", REPO / "scripts/check-cpp-quality.py")
quality = importlib.util.module_from_spec(spec)
spec.loader.exec_module(quality)
parser = argparse.ArgumentParser()
parser.add_argument("--format-tool", default="clang-format")
parser.add_argument("--tidy-tool")
options = parser.parse_args()
options.format_tool = shutil.which(options.format_tool) or str(Path(options.format_tool).resolve())


class GateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.original = quality.ROOT
        quality.ROOT = self.root
        (self.root / "src").mkdir()
        (self.root / "tests").mkdir()
        for name in (".clang-format", ".clang-tidy"):
            shutil.copy2(REPO / name, self.root / name)
        (self.root / ".clang-format-files").write_text("src/Example.cpp\n")
        (self.root / "src/Example.cpp").write_text("int example( ){return 1;}\n")
        for args in (("init", "-q"), ("config", "user.name", "Quality gate test"),
                     ("config", "user.email", "quality@example.invalid"), ("add", "."),
                     ("commit", "-qm", "initial")):
            quality.git(*args)

    def tearDown(self):
        quality.ROOT = self.original
        self.temp.cleanup()

    def test_format_failure_is_read_only_then_explicit_fix_is_idempotent(self):
        path = self.root / "src/Example.cpp"
        before = path.read_bytes()
        args = SimpleNamespace(tool=options.format_tool, base=None, fix=False)
        self.assertEqual(quality.format_check(args)["status"], "failed")
        self.assertEqual(before, path.read_bytes())
        args.fix = True
        self.assertEqual(quality.format_check(args)["status"], "success")
        args.fix = False
        self.assertEqual(quality.format_check(args)["status"], "success")

    def test_changed_committed_staged_and_new_sources_are_selected(self):
        base = quality.git("rev-parse", "HEAD").decode().strip()
        (self.root / "tests/Added.h").write_text("#pragma once\n")
        quality.git("add", ".")
        quality.git("commit", "-qm", "added header")
        (self.root / "src/Staged.cpp").write_text("int staged;\n")
        quality.git("add", ".")
        (self.root / "src/New.cpp").write_text("int fresh;\n")
        names = {p.name for p in quality.format_paths(base)}
        self.assertEqual(names, {"Example.cpp", "Added.h", "Staged.cpp", "New.cpp"})

    def test_outside_or_missing_enrollment_fails_closed(self):
        for entry in ("../outside.cpp", "src/Missing.cpp"):
            (self.root / ".clang-format-files").write_text(entry)
            with self.assertRaises(ValueError):
                quality.format_paths()

    @unittest.skipUnless(options.tidy_tool, "real clang-tidy smoke runs in Linux CI")
    def test_real_analyzer_rejects_null_dereference_and_accepts_repair(self):
        source = self.root / "src/Example.cpp"
        source.write_text("int example() { int *p = nullptr; return *p; }\n")
        build = self.root / "build"
        build.mkdir()
        database = [{"directory": str(self.root), "file": str(source),
                     "arguments": [shutil.which("clang++-18") or shutil.which("clang++"), "-std=c++20", "-c", str(source)]}]
        (build / "compile_commands.json").write_text(json.dumps(database))
        args = SimpleNamespace(tool=options.tidy_tool, build_dir=str(build), jobs=1)
        self.assertEqual(quality.tidy_check(args)["status"], "failed")
        log = (build / "cpp-quality/src_Example.cpp.log").read_text()
        self.assertIn("clang-analyzer-core.NullDereference", log)
        source.write_text("int example() { return 1; }\n")
        self.assertEqual(quality.tidy_check(args)["status"], "success")
        (self.root / "src/Unlisted.cpp").write_text("int missing;\n")
        with self.assertRaises(ValueError):
            quality.tidy_check(args)


result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GateTests))
raise SystemExit(0 if result.wasSuccessful() else 1)
