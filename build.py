#!/usr/bin/env python3
"""Guided, cross-platform CMake build and local installation for JustGraphs."""

import argparse
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent


def positive_integer(value):
    number = int(value)
    if number < 1:
        raise argparse.ArgumentTypeError("must be at least 1")
    return number


def ask(prompt, default):
    return input("{} [{}]: ".format(prompt, default)).strip() or default


def confirm(prompt):
    return input(prompt + " [y/N]: ").strip().lower() in ("y", "yes")


def read_cache(directory):
    cache = directory / "CMakeCache.txt"
    if not cache.is_file():
        return {}
    result = {}
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        match = re.match(r"([^/#][^:]*):[^=]+=(.*)", line)
        if match:
            result[match.group(1)] = match.group(2)
    return result


def discover_qt():
    for name in ("qmake6", "qmake"):
        executable = shutil.which(name)
        if not executable:
            continue
        try:
            version = subprocess.check_output(
                [executable, "-query", "QT_VERSION"], text=True,
                stderr=subprocess.DEVNULL, timeout=10,
            ).strip()
            parts = tuple(int(part) for part in version.split(".")[:2])
            if parts < (6, 9):
                continue
            prefix = subprocess.check_output(
                [executable, "-query", "QT_INSTALL_PREFIX"], text=True,
                stderr=subprocess.DEVNULL, timeout=10,
            ).strip()
            if prefix and Path(prefix).is_dir():
                return Path(prefix).resolve()
        except (OSError, subprocess.SubprocessError, ValueError):
            continue
    return None


def prerequisites(system, dry_run):
    cmake = shutil.which("cmake")
    if not cmake:
        if dry_run:
            return "cmake"
        raise RuntimeError("CMake is missing. Install CMake 3.21+ and add it to PATH.")
    version_text = subprocess.check_output([cmake, "--version"], text=True, timeout=10)
    match = re.search(r"cmake version (\d+)\.(\d+)\.(\d+)", version_text)
    if not match or tuple(map(int, match.groups())) < (3, 21, 0):
        raise RuntimeError("CMake 3.21 or newer is required.")
    print("CMake: " + match.group(0).removeprefix("cmake version "))
    hints = {
        "Darwin": "Xcode Command Line Tools (xcode-select --install)",
        "Windows": "Visual Studio Build Tools with Desktop development with C++, or a matching MinGW kit",
        "Linux": "a C++20 compiler (GCC or Clang) and a native build tool (Ninja or Make)",
    }
    print("Compiler required: " + hints[system])
    print("Qt required: Qt 6.9+ desktop kit, Core/Gui/Widgets and deployment tools")
    print("CMake will validate the compiler and Qt kit during configuration.")
    return cmake


def display(command, system):
    return subprocess.list2cmdline(command) if system == "Windows" else shlex.join(command)


def run(command, system, dry_run, stage):
    print("\n{}\n> {}".format(stage, display(command, system)), flush=True)
    if not dry_run:
        subprocess.run(command, cwd=ROOT, check=True)


def install_launcher(prefix, system):
    if system == "Darwin":
        app = prefix / "JustGraphs.app"
        if not (app / "Contents/MacOS/JustGraphs-launcher").is_file():
            raise RuntimeError("Installation did not produce the macOS app launcher.")
        return ["open", str(app)], prefix
    binary_dir = prefix / "bin"
    executable = binary_dir / ("JustGraphs.exe" if system == "Windows" else "JustGraphs")
    if not executable.is_file():
        raise RuntimeError("Installed executable is missing: " + str(executable))
    if system == "Windows":
        launcher = prefix / "Start-JustGraphs.cmd"
        launcher.write_bytes(
            b'@echo off\r\nsetlocal\r\npushd "%~dp0bin" || exit /b 1\r\n'
            b'"JustGraphs.exe" %*\r\nset "justgraphs_exit_code=%errorlevel%"\r\n'
            b'popd\r\nexit /b %justgraphs_exit_code%\r\n'
        )
    else:
        launcher = prefix / "start-justgraphs"
        launcher.write_text(
            '#!/bin/sh\n'
            'justgraphs_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
            'cd "$justgraphs_dir/bin" || exit 1\n'
            'exec ./JustGraphs "$@"\n', encoding="utf-8",
        )
        launcher.chmod(launcher.stat().st_mode | 0o111)
    print("Launcher: " + str(launcher))
    return [str(executable)], binary_dir


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--yes", "-y", action="store_true", help="use defaults without prompts")
    parser.add_argument("--config", choices=("Debug", "Release"), help="build mode (default: Release)")
    parser.add_argument("--prefix", type=Path, help="installation folder (default: dist/<OS>/<mode>)")
    parser.add_argument("--build-dir", type=Path, help="build folder (default: build/setup/<OS>/<mode>)")
    parser.add_argument("--qt-prefix", type=Path, help="Qt desktop kit folder containing lib/cmake/Qt6")
    parser.add_argument("--generator", help="CMake generator; existing build folders retain theirs")
    parser.add_argument("--jobs", type=positive_integer, default=min(os.cpu_count() or 2, 8))
    parser.add_argument("--build-only", action="store_true", help="build without installing")
    parser.add_argument("--test", action="store_true", help="run all registered tests before installing or finishing")
    parser.add_argument("--run", action="store_true", help="launch the installed app")
    parser.add_argument("--dry-run", action="store_true", help="show commands without building or writing files")
    args = parser.parse_args()
    system = platform.system()
    if system not in ("Darwin", "Windows", "Linux"):
        parser.error("supported platforms: macOS, Windows, Linux")
    if args.run and args.build_only:
        parser.error("--run requires installation; remove --build-only")
    interactive = not (args.yes or args.dry_run)
    if interactive and not sys.stdin.isatty():
        parser.error("non-interactive input requires --yes")

    print("JustGraphs build and install\nPlatform: " + system)
    cmake = prerequisites(system, args.dry_run)
    config = args.config or "Release"
    if interactive and not args.config:
        while True:
            choice = ask("Build mode: Release or Debug", config).lower()
            if choice in ("release", "debug"):
                config = choice.capitalize()
                break
            print("Enter Release or Debug.")
    prefix = args.prefix or ROOT / "dist" / system.lower() / config.lower()
    if interactive and not args.prefix and not args.build_only:
        prefix = Path(ask("Install folder", str(prefix)))
    prefix = prefix.expanduser().resolve()
    build_dir = (args.build_dir or ROOT / "build/setup" / system.lower() / config.lower()).expanduser().resolve()
    if build_dir == ROOT or prefix == ROOT:
        raise RuntimeError("Choose build and installation folders different from the source root.")
    cache = read_cache(build_dir)
    cached_source = cache.get("CMAKE_HOME_DIRECTORY")
    if cached_source and Path(cached_source).resolve() != ROOT:
        raise RuntimeError("Build folder belongs to another project; choose a different --build-dir.")
    cached_generator = cache.get("CMAKE_GENERATOR")
    if args.generator and cached_generator and args.generator != cached_generator:
        raise RuntimeError("Build folder uses {}. Choose another --build-dir to change generators.".format(cached_generator))
    generator = args.generator or cached_generator or os.environ.get("CMAKE_GENERATOR")
    if not generator and system != "Windows" and shutil.which("ninja"):
        generator = "Ninja"
    qt_prefix = args.qt_prefix
    if qt_prefix:
        qt_prefix = qt_prefix.expanduser().resolve()
        if not (qt_prefix / "lib/cmake/Qt6/Qt6Config.cmake").is_file():
            raise RuntimeError("--qt-prefix must point to a Qt desktop kit containing lib/cmake/Qt6/Qt6Config.cmake.")
        cached_qt = cache.get("Qt6_DIR")
        if cached_qt and not cached_qt.endswith("-NOTFOUND"):
            if Path(cached_qt).resolve() != (qt_prefix / "lib/cmake/Qt6").resolve():
                raise RuntimeError("Build folder uses another Qt kit. Choose a different --build-dir to change kits.")
    elif not cache.get("Qt6_DIR") and not os.environ.get("CMAKE_PREFIX_PATH"):
        qt_prefix = discover_qt()

    print("\nMode: {}\nBuild: {}\nInstall: {}\nGenerator: {}\nQt: {}".format(
        config, build_dir, prefix, generator or "CMake default", qt_prefix or "CMake discovery/cache",
    ))
    if interactive and not confirm("Proceed with " + ("build" if args.build_only else "build and installation") + "?"):
        print("Cancelled.")
        return 0
    configure = [cmake, "-S", str(ROOT), "-B", str(build_dir),
                 "-DCMAKE_BUILD_TYPE=" + config, "-DCMAKE_INSTALL_PREFIX=" + str(prefix),
                 "-DCMAKE_INSTALL_BINDIR=bin", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]
    if generator:
        configure += ["-G", generator]
    if qt_prefix:
        configure += ["-DCMAKE_PREFIX_PATH=" + str(qt_prefix),
                      "-DQt6_DIR=" + str(qt_prefix / "lib/cmake/Qt6")]
    if args.test:
        configure += ["-DBUILD_TESTING=ON"]
    run(configure, system, args.dry_run, "[1/3] Configure")
    run([cmake, "--build", str(build_dir), "--config", config, "--parallel", str(args.jobs)],
        system, args.dry_run, "[2/3] Build")
    if args.test:
        companion = Path(cmake).with_name("ctest.exe" if system == "Windows" else "ctest")
        ctest = str(companion) if companion.is_file() else shutil.which("ctest")
        if not ctest and not args.dry_run:
            raise RuntimeError("CTest is missing. Install the CMake tools before using --test.")
        run([ctest or "ctest", "--test-dir", str(build_dir), "-C", config,
             "--output-on-failure"], system, args.dry_run, "Verify registered tests")
    if args.build_only:
        print("\nBuild complete." if not args.dry_run else "\nDry run complete.")
        return 0
    run([cmake, "--install", str(build_dir), "--config", config, "--prefix", str(prefix)],
        system, args.dry_run, "[3/3] Install app, assets, and Qt runtime")
    if args.dry_run:
        print("\nDry run complete; no files changed.")
        return 0
    launch_command, launch_cwd = install_launcher(prefix, system)
    print("\nInstalled: " + str(prefix))
    if args.run or (interactive and confirm("Launch JustGraphs now?")):
        subprocess.Popen(launch_command, cwd=launch_cwd)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nCancelled.", file=sys.stderr)
        sys.exit(130)
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print("\nInstallation failed: " + str(error), file=sys.stderr)
        print("Check prerequisites and Qt kit selection; rerun to resume the build.", file=sys.stderr)
        sys.exit(1)
