#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a local TC002/FlyThings application; never contacts a clock.

Downloads are pinned by SHA-256 and HTTPS verified. No IDE installation or
administrator privileges are needed. Python is used on the build PC only.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import urllib.parse
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "tooling"
LOCK = ROOT / "cmake" / "tc002-dependencies.json"


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fetch(spec, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if sha256(destination) != spec["sha256"]:
            raise RuntimeError(f"Cached hash mismatch: {destination.name}; remove that cache file and retry")
        return
    if urllib.parse.urlparse(spec["url"]).scheme != "https":
        raise RuntimeError("Dependency downloads require HTTPS")
    partial = destination.with_suffix(destination.suffix + ".part")
    request = urllib.request.Request(spec["url"], headers={"User-Agent": "Owlanzi-TC002-build/0.1"})
    with urllib.request.urlopen(request, timeout=90) as response:
        if urllib.parse.urlparse(response.url).scheme != "https":
            raise RuntimeError("Dependency download redirected away from HTTPS")
        with partial.open("wb") as output:
            shutil.copyfileobj(response, output)
    if sha256(partial) != spec["sha256"]:
        partial.unlink()
        raise RuntimeError(f"Downloaded hash mismatch: {destination.name}; dependency lock needs review")
    partial.replace(destination)


def extract(archive, destination):
    destination.mkdir(parents=True, exist_ok=True)
    base = destination.resolve()
    with zipfile.ZipFile(archive) as source:
        for info in source.infolist():
            target = (destination / info.filename).resolve()
            if not target.is_relative_to(base) or ":" in info.filename or stat.S_ISLNK(info.external_attr >> 16):
                raise RuntimeError(f"Unsafe dependency archive entry: {info.filename}")
        source.extractall(destination)


def prepare(lock):
    fetch(lock["toolchain"], CACHE / "z21.zip")
    extract(CACHE / "z21.zip", CACHE / "z21")
    fetch(lock["ca_bundle"], CACHE / "cacert.pem")

    def package(spec):
        archive = CACHE / "sdk" / f'{spec["name"]}-{spec["version"]}.zip'
        fetch(spec, archive)
        extract(archive, CACHE / "sdk" / spec["directory"])

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(package, lock["packages"]))
    print(f'Prepared {len(lock["packages"])} verified SDK packages and the Z21 compiler.')


def executable(name, argument=None):
    if argument:
        path = Path(argument)
        if not path.is_file(): raise RuntimeError(f"Executable not found: {path}")
        return str(path.resolve())
    found = shutil.which(name)
    if found: return found
    # Visual Studio Build Tools already includes CMake and Ninja on many PCs.
    base = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio"
    suffix = "CMake/CMake/bin/cmake.exe" if name == "cmake" else "CMake/Ninja/ninja.exe"
    candidates = sorted(base.glob("*/*/Common7/IDE/CommonExtensions/Microsoft/" + suffix))
    if candidates: return str(candidates[-1])
    raise RuntimeError(f"{name} not found. Install CMake/Ninja or pass --{name} with its executable path.")


def run(arguments):
    subprocess.run([str(value) for value in arguments], cwd=ROOT, check=True)


def package(build, lock):
    device = build / "device"
    run([CACHE / "z21/bin/arm-pc-linux-gnueabihf-strip.exe", "--strip-unneeded", device / "lib/libzkgui.so"])
    cfg = {"baud": "115200", "defBrightness": -1, "languageCode": "en_US",
           "languagePath": "/tmp/owlanzi-tc002-app/tr/", "resPath": "/tmp/owlanzi-tc002-app/ui/",
           "rotateScreen": 0, "rotateTouch": 0, "screensaverTimeOut": -1,
           "startupLibPath": "/tmp/owlanzi-tc002-app/lib/libzkgui.so",
           "startupTouchCalib": False, "touchDev": "/dev/input/event1", "uart": "ttyS1", "zkdebug": False}
    (device / "EasyUI.cfg").write_text(json.dumps(cfg, indent=2) + "\n", encoding="utf-8")
    # SDK .so files are link stubs for stock system libraries. Never distribute
    # or install them in place of the clock's real system libraries.
    needed_output = subprocess.check_output([
        str(CACHE / "z21/bin/arm-pc-linux-gnueabihf-readelf.exe"), "-d",
        str(device / "lib/libzkgui.so")], text=True)
    needed = re.findall(r"\(NEEDED\).*?\[([^]]+)\]", needed_output)
    version_output = subprocess.check_output([
        str(CACHE / "z21/bin/arm-pc-linux-gnueabihf-readelf.exe"), "--version-info",
        str(device / "lib/libzkgui.so")], text=True)
    symbol_versions = sorted(set(re.findall(r"Name: ((?:GLIBC|GLIBCXX|CXXABI|GCC)_[\d.]+)", version_output)))
    names = ["EasyUI.cfg", "lib/libzkgui.so", "ui/main.ftu", "ui/cacert.pem"]
    manifest = {"schema": 1, "target": "tc002", "platform": "Z21", "kind": "flythings-debug-app",
                "version": re.search(r"^OWLANZI_APP_VERSION:STRING=(.+)$", (build/"CMakeCache.txt").read_text(), re.M).group(1), "upstream_commit": lock["ulanzi_revision"],
                "hardware_verified": False, "persistent_image": False,
                "files": {name: {"sha256": sha256(device / name), "size": (device / name).stat().st_size} for name in names},
                "needed_sonames": needed, "required_symbol_versions": symbol_versions,
                "dependency_lock_sha256": sha256(LOCK)}
    (device / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    archive = build / ("owlanzi-tc002-app-"+manifest["version"]+"-local.zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
        for name in sorted(names + ["manifest.json"]):
            info = zipfile.ZipInfo(name, date_time=(2026, 9, 9, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            output.writestr(info, (device / name).read_bytes())
    print(f"Local test package: {archive}")
    print("ARM build verified. Hardware behavior and persistent installation still require the real TC002.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true", help="Fetch and verify dependencies only")
    parser.add_argument("--offline", action="store_true", help="Use existing extracted dependencies without downloading")
    parser.add_argument("--cmake")
    parser.add_argument("--ninja")
    parser.add_argument("--build-dir", default="build/tc002")
    parser.add_argument("--app-version", default="0.2.4")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--device-abi-dir", type=Path, help="Link against libstdc++ read from the local test clock")
    args = parser.parse_args()
    if os.name != "nt":
        raise RuntimeError("This verified build uses the Windows-hosted Z21 compiler. See docs/TC002_PLATFORM.md.")
    if not re.fullmatch(r"[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}",args.app_version): raise ValueError("Invalid app version")
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    if not args.offline: prepare(lock)
    if args.prepare: return
    cmake, ninja = executable("cmake", args.cmake), executable("ninja", args.ninja)
    build = (ROOT / args.build_dir).resolve()
    if not build.is_relative_to(ROOT): raise RuntimeError("Build directory must stay inside this repository")
    run([cmake, "-S", ROOT, "-B", build, "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={ninja}",
         f"-DCMAKE_TOOLCHAIN_FILE={ROOT / 'cmake/tc002-windows.cmake'}", "-DOWLANZI_BUILD_TC002=ON",
         "-DOWLANZI_BUILD_TESTS=OFF", "-DCMAKE_BUILD_TYPE=Release", f"-DOWLANZI_APP_VERSION={args.app_version}", f"-DPython3_EXECUTABLE={sys.executable}",
         f"-DTC002_DEVICE_ABI_DIR={args.device_abi_dir.resolve().as_posix() if args.device_abi_dir else ''}"])
    run([cmake, "--build", build, "--target", "zkgui", "ota-switch", "--parallel", max(1, min(args.jobs, 32))])
    package(build, lock)


if __name__ == "__main__":
    try: main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"TC002 build failed: {error}", file=sys.stderr)
        raise SystemExit(1)
