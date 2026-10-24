#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""公開API一覧とDocumentリンクが各ソースの冒頭にあることを確認する。"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

from source_api_docs import validate_document_path


FOLDERS = ("Algorithm", "DataStructure", "Geometry", "Graph", "Internal", "Math", "String")
DOCUMENT_PREFIX = "https://github.com/reinsirk/sirkit-cpp-library/blob/main/"


def check(root: Path) -> tuple[int, int]:
    sources = sorted(file for folder in FOLDERS for file in (root / folder).rglob("*.cpp"))
    sources.extend(root / name for name in ("temp.cpp", "temp2.cpp"))
    legacy = root / "Bellman-Ford_SFPA.cpp"
    if legacy.is_file():
        sources.append(legacy)
    failures: list[str] = []
    links = 0
    for file in sources:
        text = file.read_text(encoding="utf-8")
        lines = text.splitlines()
        first_code = next((i for i, line in enumerate(lines)
                           if line.strip() and not line.lstrip().startswith("//")), len(lines))
        headings = [i for i, line in enumerate(lines) if line == "// API一覧"]
        name = file.relative_to(root).as_posix()
        if len(headings) != 1 or headings[0] >= first_code:
            failures.append(f"{name}: API一覧をincludeより前の一か所に置いてください")
        if "SIRKIT_API_BEGIN" in text or "SIRKIT_API_END" in text:
            failures.append(f"{name}: 生成用のAPIマーカーが残っています")
        documents = [line.removeprefix("// ドキュメント:").removeprefix(" ")
                     for line in lines[:first_code] if line.startswith("// ドキュメント:")]
        if any(line.startswith("// ドキュメント:") for line in lines[first_code:]):
            failures.append(f"{name}: Documentリンクは冒頭の一覧へまとめてください")
        if not documents:
            failures.append(f"{name}: 先頭にDocumentへのリンクがありません")
        for url in documents:
            links += 1
            relative = url.removeprefix(DOCUMENT_PREFIX)
            try:
                path = validate_document_path(relative)
            except ValueError:
                failures.append(f"{name}: Documentリンクを解決できません: {url}")
                continue
            if not url.startswith(DOCUMENT_PREFIX) or not (root / path).is_file():
                failures.append(f"{name}: Documentリンクを解決できません: {url}")
    if failures:
        raise ValueError("\n".join(failures))
    return len(sources), links


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="書き込まず配置とリンクを確認する")
    options = parser.parse_args()
    del options
    try:
        count, links = check(Path(__file__).resolve().parents[1])
        print(f"{count}ソースのAPI配置・{links}件のDocumentリンク: 確認済み")
        return 0
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
