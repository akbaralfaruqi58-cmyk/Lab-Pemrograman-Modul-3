#!/usr/bin/env python3
"""Fail the build when the domain layer depends on I/O or outer layers.

This is a small standard-library-only guard for Lab 3.2. It deliberately
checks imports/includes and direct I/O calls instead of searching comments.
"""
from __future__ import annotations

import ast
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP_DOMAIN = ROOT / "cpp" / "include" / "pinjamlab" / "domain"
PY_DOMAIN = ROOT / "py" / "pinjamlab" / "domain"

violations: list[str] = []


def strip_cpp_comments(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    source = re.sub(r"//.*", "", source)
    return source


def check_cpp() -> None:
    forbidden_include = re.compile(
        r'^\s*#\s*include\s*[<"](?:fstream|iostream|sstream|filesystem|cstdio|'
        r'pinjamlab/(?:io|service|app)/[^>"]*)[>"]',
        re.MULTILINE,
    )
    forbidden_calls = re.compile(
        r"\b(?:std::(?:cout|cerr|cin|ifstream|ofstream|fstream|filesystem)|"
        r"(?:printf|fprintf|fopen|freopen))\b"
    )
    for path in sorted(CPP_DOMAIN.rglob("*.hpp")):
        text = strip_cpp_comments(path.read_text(encoding="utf-8"))
        for match in forbidden_include.finditer(text):
            violations.append(
                f"C++ domain include: {path.relative_to(ROOT)}: {match.group(0).strip()}"
            )
        for match in forbidden_calls.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            violations.append(
                f"C++ domain I/O call: {path.relative_to(ROOT)}:{line}: {match.group(0)}"
            )


def check_python() -> None:
    forbidden_top_level = {"csv", "sys", "io", "pathlib", "os"}
    forbidden_calls = {"open", "print", "input", "__import__"}

    for path in sorted(PY_DOMAIN.rglob("*.py")):
        source = path.read_text(encoding="utf-8")
        try:
            tree = ast.parse(source, filename=str(path))
        except SyntaxError as exc:
            violations.append(f"Python domain syntax error: {path.relative_to(ROOT)}: {exc}")
            continue

        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    root_name = alias.name.split(".", 1)[0]
                    if root_name in forbidden_top_level or alias.name.startswith(
                        ("pinjamlab.io", "pinjamlab.service", "pinjamlab.app")
                    ):
                        violations.append(
                            f"Python domain import: {path.relative_to(ROOT)}:{node.lineno}: import {alias.name}"
                        )

            elif isinstance(node, ast.ImportFrom):
                module = node.module or ""
                first = module.split(".", 1)[0] if module else ""
                imported_names = {alias.name for alias in node.names}
                if (
                    first in {"io", "service", "app"} and node.level > 0
                ) or module.startswith(("pinjamlab.io", "pinjamlab.service", "pinjamlab.app")):
                    violations.append(
                        f"Python domain import: {path.relative_to(ROOT)}:{node.lineno}: from {'.' * node.level}{module} import ..."
                    )
                if node.level >= 1 and not module and imported_names.intersection({"io", "service", "app"}):
                    violations.append(
                        f"Python domain import: {path.relative_to(ROOT)}:{node.lineno}: from . import {', '.join(sorted(imported_names))}"
                    )
                if first in forbidden_top_level and node.level == 0:
                    violations.append(
                        f"Python domain import: {path.relative_to(ROOT)}:{node.lineno}: from {module} import ..."
                    )

            elif isinstance(node, ast.Call):
                function_name = None
                if isinstance(node.func, ast.Name):
                    function_name = node.func.id
                elif isinstance(node.func, ast.Attribute):
                    function_name = node.func.attr
                if function_name in forbidden_calls or function_name in {"read_text", "write_text", "read_bytes", "write_bytes"}:
                    violations.append(
                        f"Python domain I/O call: {path.relative_to(ROOT)}:{node.lineno}: {function_name}()"
                    )


def main() -> int:
    if not CPP_DOMAIN.is_dir() or not PY_DOMAIN.is_dir():
        print("FAIL: expected C++ and Python domain directories are missing.", file=sys.stderr)
        return 2

    check_cpp()
    check_python()

    if violations:
        print("FAIL: dependency rule violations found:", file=sys.stderr)
        for violation in violations:
            print(f"  - {violation}", file=sys.stderr)
        return 1

    print("PASS: domain/ depends only on domain and standard non-I/O types.")
    print("PASS: no imports/includes from io, service, or app; no direct file/console I/O.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
