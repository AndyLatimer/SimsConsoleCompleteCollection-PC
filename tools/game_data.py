#!/usr/bin/env python3
"""Inspect Xbox data, import without overwriting, and report launcher readiness.

Only Python's standard library and the repository's existing XBE/XISO readers.
Reports contain metadata, never commercial game bytes. PAL inspection is allowed;
import/build preparation requires the exact XBE used by the USA hooks.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile

from xbe_info import Xbe, XbeError
from xiso_extract import Xiso, XisoError, safe_join

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "collection/game.json").read_text(encoding="utf-8"))
ORIGINAL = MANIFEST["original_data"]
MAX_XBE = 16 * 1024 * 1024


class DataError(Exception):
    pass


def supported_xbe(digest: str) -> bool:
    return digest == ORIGINAL["xbe_sha1"] or digest in ORIGINAL.get("compatible_xbe_sha1", [])


def _inventory(paths: list[tuple[str, int]]) -> dict[str, int]:
    result = {}
    for name, size in paths:
        # All supported disc files are at the root. Reject Windows aliases, alternate
        # streams and traversal before passing a name to the upstream extractor.
        parts = name.replace("\\", "/").split("/")
        if any(p in ("", ".", "..") or ":" in p or p.endswith((" ", ".")) for p in parts):
            raise DataError(f"Unsafe resource path: {name!r}")
        key = "/".join(parts).casefold()
        if key in result:
            raise DataError(f"Duplicate resource name: {name}")
        if size < 0:
            raise DataError(f"Invalid resource size: {name}")
        result[key] = size
    return result


def _xbe_report(data: bytes) -> dict:
    if not 0 < len(data) <= MAX_XBE:
        raise DataError("default.xbe exceeds the inspection size limit or is empty")
    # Reuse the proven parser, whose interface accepts a path. This temporary file
    # is removed on both success and failure; no original bytes enter source control.
    with tempfile.TemporaryDirectory(prefix="sims1-xbe-") as tmp:
        path = Path(tmp) / "default.xbe"
        path.write_bytes(data)
        xbe = Xbe(str(path))
        return {"sha1": xbe.sha1, "size": len(data),
                "title_id": xbe.certificate["title_id_hex"],
                "title": xbe.certificate["title_name"],
                "regions": xbe.certificate["game_region_flags"],
                "xdk": getattr(xbe, "xdk_build", None), "entry_point": f"0x{xbe.entry_point:X}",
                "supported": supported_xbe(xbe.sha1)}


def _folder_files(source: Path) -> list[Path]:
    files = []
    for path in sorted(source.rglob("*")):
        if path.is_symlink() or path.is_junction():
            raise DataError(f"Linked resource is not supported: {path}")
        if path.is_file():
            files.append(path)
    return files


def inspect(source: Path, hash_resources: bool = False) -> dict:
    source = source.resolve()
    digests = {}
    if source.is_dir():
        paths = _folder_files(source)
        inventory = _inventory([(p.relative_to(source).as_posix(), p.stat().st_size) for p in paths])
        # A source tree normally has just an extracted XBE plus a complete ISO.
        # Follow the ISO if the extracted data set is incomplete, as the runtime does.
        images = [p for p in paths if p.parent == source and p.suffix.lower() == ".iso"]
        if images and not set(ORIGINAL["required_files"]).issubset(inventory):
            if len(images) != 1:
                raise DataError("Several ISOs found; inspect/import one explicit image")
            return inspect(images[0], hash_resources)
        xbe_paths = [p for p in paths if p.parent == source and p.name.lower() == "default.xbe"]
        if len(xbe_paths) != 1:
            raise DataError("Expected one default.xbe at the root of the extracted Xbox data")
        if xbe_paths[0].stat().st_size > MAX_XBE:
            raise DataError("default.xbe exceeds the inspection size limit")
        xbe = _xbe_report(xbe_paths[0].read_bytes())
        if hash_resources:
            for path in paths:
                with path.open("rb") as stream:
                    digests[path.relative_to(source).as_posix().casefold()] = hashlib.file_digest(stream, "sha256").hexdigest()
        kind = "directory"
    elif source.is_file() and source.suffix.lower() == ".iso":
        with Xiso(str(source)) as iso:
            entries = iso.files()
            inventory = _inventory([(e.path, e.size) for e in entries])
            matches = [e for e in entries if e.path.casefold() == "default.xbe"]
            if len(matches) != 1 or matches[0].size > MAX_XBE:
                raise DataError("Image has no valid root default.xbe (Xbox required)")
            xbe = _xbe_report(b"".join(iso.stream(matches[0])))
            if hash_resources:
                for entry in entries:
                    digest = hashlib.sha256()
                    for block in iso.stream(entry):
                        digest.update(block)
                    digests[entry.path.casefold()] = digest.hexdigest()
        kind = "iso"
    else:
        raise DataError(f"Xbox ISO or extracted directory not found: {source}")

    missing = [name for name in ORIGINAL["required_files"] if name not in inventory]
    mismatched = {name: {"expected": size, "actual": inventory[name]}
                  for name, size in ORIGINAL["required_files"].items()
                  if name in inventory and inventory[name] != size}
    return {"schema_version": 1, "game_id": MANIFEST["id"], "source": str(source),
            "kind": kind, "xbe": xbe, "files": inventory,
            "sha256": digests, "missing": missing, "size_mismatches": mismatched,
            "data_ready": xbe["supported"] and not missing and not mismatched,
            "integrity_note": "XBE SHA-1 verified; resource sizes checked, contents need gameplay validation"}


def require_ready(report: dict) -> None:
    if not report["xbe"]["supported"]:
        raise DataError(f"Unsupported XBE {report['xbe']['sha1']}; USA hooks require {ORIGINAL['xbe_sha1']}. PAL analysis must remain separate.")
    if not report["data_ready"]:
        raise DataError(f"Incomplete/modified USA data: missing={report['missing']}, size_mismatches={report['size_mismatches']}")


def prepare(source: Path) -> dict:
    """Validate existing assets and extract only the XBE needed for recompilation."""
    report = inspect(source)
    require_ready(report)
    target = source.resolve() / "default.xbe"
    if not source.is_dir():
        raise DataError("prepare expects the assets directory; use import for an external ISO")
    if target.exists():
        if target.stat().st_size > MAX_XBE or not supported_xbe(hashlib.sha1(target.read_bytes()).hexdigest()):
            raise DataError("Existing assets/default.xbe differs; it was preserved")
    elif report["kind"] == "iso":
        with Xiso(report["source"]) as iso:
            entry = next(e for e in iso.files() if e.path.casefold() == "default.xbe")
            data = b"".join(iso.stream(entry))
        if not supported_xbe(hashlib.sha1(data).hexdigest()):
            raise DataError("ISO changed during preparation")
        with target.open("xb") as stream:
            stream.write(data)
    return report


def import_data(source: Path, destination: Path) -> dict:
    report = inspect(source)
    require_ready(report)
    destination = destination.resolve()
    if destination.exists():
        raise DataError(f"Destination already exists; nothing overwritten: {destination}")
    if report["kind"] == "directory" and Path(report["source"]) in destination.parents:
        raise DataError("Destination must be outside the original data directory")
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Fully copy and recheck in a sibling staging folder before exposing assets/.
    with tempfile.TemporaryDirectory(prefix=".sims1-import-", dir=destination.parent) as tmp:
        staged = Path(tmp) / "assets"
        staged.mkdir()
        actual_source = Path(report["source"])
        if report["kind"] == "iso":
            with Xiso(str(actual_source)) as iso:
                entries = iso.files()
                _inventory([(e.path, e.size) for e in entries])
                for entry in entries:
                    iso.extract(entry, str(staged), False)
        else:
            for path in _folder_files(actual_source):
                target = Path(safe_join(str(staged), path.relative_to(actual_source).as_posix()))
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
        require_ready(inspect(staged))
        # rename does not replace an existing directory on Windows. Check again
        # for platforms where rename could replace a concurrent empty directory.
        if destination.exists():
            raise DataError("Destination appeared during import; preserved")
        os.rename(staged, destination)
    return inspect(destination)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("inspect", "import", "prepare", "check"):
        p = sub.add_parser(name)
        p.add_argument("source", type=Path, nargs=None if name in ("inspect", "import") else "?", default=ROOT / "assets")
        if name == "import":
            p.add_argument("--destination", type=Path, default=ROOT / "assets")
        if name == "inspect":
            p.add_argument("--hash-resources", action="store_true", help="SHA-256 for later USA/PAL resource comparison")
        if name == "check":
            p.add_argument("--executable", type=Path, default=ROOT / "out/build/release/Sims1.exe")
    args = parser.parse_args(argv)
    try:
        if args.command == "import":
            report = import_data(args.source, args.destination)
        elif args.command == "prepare":
            report = prepare(args.source)
        else:
            report = inspect(args.source, getattr(args, "hash_resources", False))
        if args.command == "check":
            report["executable_found"] = args.executable.is_file()
            report["can_start"] = report["data_ready"] and report["executable_found"]
            report["gameplay_verified"] = False
        print(json.dumps(report, ensure_ascii=True, indent=2))
        return 0 if args.command == "inspect" or report.get("can_start", report["data_ready"]) else 2
    except (DataError, XbeError, XisoError, OSError, ValueError, struct.error, IndexError, AttributeError) as exc:
        print(json.dumps({"schema_version": 1, "game_id": MANIFEST["id"], "data_ready": False,
                          "can_start": False, "error": str(exc)}, ensure_ascii=True, indent=2))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
