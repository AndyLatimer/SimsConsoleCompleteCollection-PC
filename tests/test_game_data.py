"""No retail bytes: exercise import/region safeguards with synthetic XBE/XISO fixtures."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import game_data as gd


def synthetic_xbe():
    data = bytearray(1024)
    data[:4] = b"XBEH"
    fields = {0x104: 0x10000, 0x108: 1024, 0x10C: 0x1000, 0x110: 0x178,
              0x118: 0x10200, 0x120: 0x10300,
              0x128: 0x10180 ^ 0xA8FC57AB, 0x158: 0x5B6D40B6,
              0x160: 1, 0x164: 0x10380}
    for offset, value in fields.items():
        struct.pack_into("<I", data, offset, value)
    struct.pack_into("<III", data, 0x200, 0xB0, 0, 0x4541002F)
    title = "Synthetic test".encode("utf-16-le")
    data[0x20C:0x20C + len(title)] = title
    struct.pack_into("<I", data, 0x2A0, 1)
    data[0x380:0x388] = b"XAPILIB\0"
    struct.pack_into("<4H", data, 0x388, 1, 0, 5233, 0)
    return bytes(data)


def synthetic_iso(path, files):
    sector = 2048
    image = bytearray(sector * (35 + len(files)))
    descriptor = 32 * sector
    image[descriptor:descriptor + 20] = b"MICROSOFT*XBOX*MEDIA"
    image[descriptor + 0x7EC:descriptor + 0x800] = b"MICROSOFT*XBOX*MEDIA"
    table = bytearray()
    for index, (name, data) in enumerate(files.items()):
        encoded = name.encode("ascii")
        length = (14 + len(encoded) + 3) // 4 * 4
        next_offset = (len(table) + length) // 4 if index + 1 < len(files) else 0
        entry = struct.pack("<HHIIBB", 0, next_offset, 35 + index, len(data), 0, len(encoded)) + encoded
        table += entry + bytes(length - len(entry))
        image[(35 + index) * sector:(35 + index) * sector + len(data)] = data
    struct.pack_into("<IIQ", image, descriptor + 20, 34, len(table), 0)
    image[34 * sector:34 * sector + len(table)] = table
    path.write_bytes(image)


class GameDataTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.source = self.root / "original"
        self.source.mkdir()
        self.files = {"default.xbe": synthetic_xbe(), "datasets.arc": b"synthetic resources"}
        for name, data in self.files.items():
            (self.source / name).write_bytes(data)
        self.patch = mock.patch.dict(gd.ORIGINAL, {
            "xbe_sha1": hashlib.sha1(self.files["default.xbe"]).hexdigest(),
            "required_files": {k: len(v) for k, v in self.files.items()}})
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def test_real_parser_and_case_insensitive_names(self):
        (self.source / "datasets.arc").rename(self.source / "DATASETS.ARC")
        report = gd.inspect(self.source, True)
        self.assertTrue(report["data_ready"])
        self.assertEqual(report["xbe"]["xdk"], 5233)
        self.assertEqual(report["xbe"]["title_id"], "0x4541002F")
        self.assertEqual(report["sha256"]["datasets.arc"], hashlib.sha256(self.files["datasets.arc"]).hexdigest())

    def test_missing_and_wrong_size_resources_are_rejected(self):
        (self.source / "datasets.arc").unlink()
        self.assertEqual(gd.inspect(self.source)["missing"], ["datasets.arc"])
        with self.assertRaises(gd.DataError):
            gd.import_data(self.source, self.root / "assets")
        (self.source / "datasets.arc").write_bytes(b"short")
        self.assertIn("datasets.arc", gd.inspect(self.source)["size_mismatches"])
        self.assertFalse((self.root / "assets").exists())

    def test_unknown_xbe_can_be_inspected_but_never_imported(self):
        data = bytearray(self.files["default.xbe"])
        data[-1] = 1  # different regional/revised executable, same valid structure
        (self.source / "default.xbe").write_bytes(data)
        self.assertFalse(gd.inspect(self.source)["xbe"]["supported"])
        with self.assertRaisesRegex(gd.DataError, "Unsupported XBE"):
            gd.import_data(self.source, self.root / "assets")
        self.assertFalse((self.root / "assets").exists())

    def test_import_preserves_source_and_refuses_existing_destination(self):
        target = self.root / "assets"
        self.assertTrue(gd.import_data(self.source, target)["data_ready"])
        for name, data in self.files.items():
            self.assertEqual((target / name).read_bytes(), data)
            self.assertEqual((self.source / name).read_bytes(), data)
        (target / "datasets.arc").write_bytes(b"preserve me")
        with self.assertRaises(gd.DataError):
            gd.import_data(self.source, target)
        self.assertEqual((target / "datasets.arc").read_bytes(), b"preserve me")

    def test_iso_import_and_build_preparation(self):
        assets = self.root / "assets (USA)"
        assets.mkdir()
        iso = assets / "The Sims (USA).ISO"
        synthetic_iso(iso, self.files)
        before = iso.read_bytes()
        self.assertTrue(gd.prepare(assets)["data_ready"])
        self.assertEqual((assets / "default.xbe").read_bytes(), self.files["default.xbe"])
        self.assertEqual(iso.read_bytes(), before)
        self.assertFalse((assets / "datasets.arc").exists())
        imported = self.root / "imported"
        self.assertTrue(gd.import_data(iso, imported)["data_ready"])
        self.assertEqual((imported / "datasets.arc").read_bytes(), self.files["datasets.arc"])

    def test_prepare_does_not_overwrite_a_different_existing_xbe(self):
        assets = self.root / "assets"
        assets.mkdir()
        synthetic_iso(assets / "game.iso", self.files)
        (assets / "default.xbe").write_bytes(b"keep this file")
        with self.assertRaisesRegex(gd.DataError, "preserved"):
            gd.prepare(assets)
        self.assertEqual((assets / "default.xbe").read_bytes(), b"keep this file")

    def test_multiple_images_require_explicit_selection(self):
        assets = self.root / "assets"
        assets.mkdir()
        for name in ("USA.iso", "PAL.iso"):
            synthetic_iso(assets / name, self.files)
        with self.assertRaisesRegex(gd.DataError, "Several ISOs"):
            gd.prepare(assets)

    def test_unsafe_and_duplicate_resource_paths(self):
        for name in ("../escape", "C:/escape", "folder/../../escape", "file:stream", "file."):
            with self.subTest(name=name), self.assertRaises(gd.DataError):
                gd._inventory([(name, 1)])
        with self.assertRaises(gd.DataError):
            gd._inventory([("default.xbe", 1), ("DEFAULT.XBE", 1)])

    def test_failed_copy_leaves_no_visible_installation(self):
        target = self.root / "assets"
        with mock.patch.object(gd.shutil, "copyfile", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                gd.import_data(self.source, target)
        self.assertFalse(target.exists())
        self.assertEqual(list(self.root.glob(".sims1-import-*")), [])

    def test_import_cannot_create_staging_inside_original_data(self):
        with self.assertRaisesRegex(gd.DataError, "outside"):
            gd.import_data(self.source, self.source / "assets")
        self.assertFalse((self.source / "assets").exists())
        self.assertEqual({p.name for p in self.source.iterdir()}, set(self.files))

    def test_check_requires_executable_and_reports_missing_data_as_json(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            code = gd.main(["check", str(self.source), "--executable", str(self.root / "missing.exe")])
        self.assertEqual(code, 2)
        report = json.loads(output.getvalue())
        self.assertTrue(report["data_ready"])
        self.assertFalse(report["can_start"])
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            code = gd.main(["check", str(self.root / "absent")])
        self.assertEqual(code, 2)
        self.assertFalse(json.loads(output.getvalue())["can_start"])


@unittest.skipUnless(all(shutil.which(t) for t in ("cmake", "ninja", "clang", "clang++")),
                     "Run tools/env.bat first to exercise the CMake guards")
class CMakeGuards(unittest.TestCase):
    def configure(self, sdk=True, xbe=None):
        with tempfile.TemporaryDirectory(prefix="sims1-cmake-") as tmp:
            root = Path(tmp)
            shutil.copyfile(gd.ROOT / "CMakeLists.txt", root / "CMakeLists.txt")
            if sdk:
                path = root / "sdk/cmake/XbeRecomp.cmake"
                path.parent.mkdir(parents=True)
                # Guard must fire before this file is included or any fetch starts.
                path.write_text('message(FATAL_ERROR "UNEXPECTED_SDK_INCLUDE")\n')
            if xbe is not None:
                (root / "assets").mkdir()
                (root / "assets/default.xbe").write_bytes(xbe)
            proc = subprocess.run(["cmake", "-S", str(root), "-B", str(root / "out"),
                                   "-G", "Ninja", "-DCMAKE_C_COMPILER=clang",
                                   "-DCMAKE_CXX_COMPILER=clang++"],
                                  capture_output=True, text=True, timeout=60)
            output = proc.stdout + proc.stderr
            self.assertNotEqual(proc.returncode, 0, output)
            self.assertNotIn("UNEXPECTED_SDK_INCLUDE", output)
            return output

    def test_missing_sdk_fails_without_fetching_another_revision(self):
        self.assertIn("git submodule update --init --recursive", self.configure(sdk=False))

    def test_missing_xbe_fails_before_loading_runtime(self):
        self.assertIn("Game data missing", self.configure())

    def test_non_usa_hash_cannot_reach_usa_codegen(self):
        self.assertIn("Unsupported default.xbe SHA-1", self.configure(xbe=synthetic_xbe()))


if __name__ == "__main__":
    unittest.main()
