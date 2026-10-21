#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""共通部品から単独貼付用の幾何ライブラリを生成・照合する。"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

# 依存は生成時に展開する。配布される C++ ファイルに自作 include は残さない。
PARTS: dict[str, tuple[str, ...]] = {
    "epsilon": (),
    "point": ("epsilon",),
    "point_functions": ("point",),
    "linear_types": ("point",),
    "on_segment": ("point_functions", "linear_types"),
    "segment_predicates": ("on_segment",),
    "linear_results": ("point",),
    "linear_functions": ("segment_predicates", "linear_results"),
    "location": (),
    "polygon": ("on_segment", "location"),
    "circle_types": ("point",),
    "circle_functions": ("on_segment", "location", "circle_types"),
}

TARGETS: dict[str, tuple[str, ...]] = {
    "Geometry/geometory.cpp": ("linear_functions", "polygon", "circle_functions"),
    "Geometry/Point.cpp": ("point_functions",),
    "Geometry/Line_Segment.cpp": ("linear_functions",),
    "Geometry/Polygon.cpp": ("polygon",),
    "Geometry/Circle.cpp": ("circle_functions",),
}


def dependency_order(roots: tuple[str, ...]) -> list[str]:
    order: list[str] = []
    complete: set[str] = set()
    active: set[str] = set()

    def visit(part: str) -> None:
        if part in complete:
            return
        if part in active:
            raise ValueError(f"依存が循環しています: {part}")
        active.add(part)
        for dependency in PARTS[part]:
            visit(dependency)
        active.remove(part)
        complete.add(part)
        order.append(part)

    for root in roots:
        visit(root)
    return order


def load_sources(directory: Path) -> tuple[str, dict[str, str]]:
    headers = (directory / "includes.txt").read_text(encoding="utf-8")
    if not headers.endswith("\n") or any(
        not re.fullmatch(r"#include <[a-z0-9_]+>", line)
        for line in headers.splitlines()
    ):
        raise ValueError("includes.txt は標準ヘッダーの include だけを記載します")
    pieces: dict[str, str] = {}
    guards: set[str] = set()
    for name in PARTS:
        piece = (directory / f"{name}.inc").read_text(encoding="utf-8")
        opening = re.match(r"#ifndef (REINSIRK_GEO_[A-Z_]+)\n#define \1\nnamespace geo \{\n", piece)
        if opening is None or not piece.endswith("\n}\n#endif\n"):
            raise ValueError(f"部品のガード形式が不正です: {name}")
        if opening[1] in guards:
            raise ValueError(f"部品間でガードが重複しています: {opening[1]}")
        if re.search(r"^#include\s", piece, re.MULTILINE):
            raise ValueError(f"部品内の include は禁止です: {name}")
        guards.add(opening[1])
        pieces[name] = piece
    return headers, pieces


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    base = Path(__file__).resolve().parent
    parser.add_argument("--components-dir", type=Path, default=base.parent / "Geometry" / "components", help="権威ソースのディレクトリ")
    parser.add_argument("--output-dir", type=Path, default=base.parent, help="geometory.cpp と Geometry/ の出力先")
    parser.add_argument("--check", action="store_true", help="書き込まず、全生成物の一致を検証する")
    options = parser.parse_args()
    try:
        headers, pieces = load_sources(options.components_dir)
        failures = []
        for relative, roots in TARGETS.items():
            content = "// SPDX-License-Identifier: CC0-1.0\n" + headers + "\n"
            content += "\n".join(pieces[name] for name in dependency_order(roots))
            target = options.output_dir / relative
            if options.check:
                if not target.is_file() or target.read_bytes() != content.encode("utf-8"):
                    failures.append(str(target))
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(content, encoding="utf-8", newline="\n")
        if failures:
            print("生成物が権威ソースと一致しません:\n" + "\n".join(failures), file=sys.stderr)
            return 1
        print(f"幾何ライブラリ{len(TARGETS)}ファイル: " + ("同期確認済み" if options.check else "生成済み"))
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f"幾何ライブラリの生成に失敗しました: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
