#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""共通部品から単独貼付用の二項係数ライブラリを生成・照合する。"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


PARTS: dict[str, tuple[str, ...]] = {
    "options": (),
    "binomial_mod": ("options",),
    "modint_binomial": ("binomial_mod",),
    "prime_binomial": (),
}
TARGETS: dict[str, tuple[str, tuple[str, ...]]] = {
    "Math/Combination_Mod.cpp": ("mod", ("modint_binomial",)),
    "Math/Combination_AtCoderMod.cpp": ("prime", ("prime_binomial",)),
    "Math/Combination/Binomial_Mod.cpp": ("mod", ("binomial_mod",)),
    "Math/Combination/ModInt_Binomial.cpp": ("mod", ("modint_binomial",)),
    "Math/Combination/Prime_Binomial.cpp": ("prime", ("prime_binomial",)),
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


def load_sources(directory: Path) -> tuple[dict[str, str], dict[str, str]]:
    headers: dict[str, str] = {}
    for kind in ("mod", "prime"):
        content = (directory / f"includes_{kind}.txt").read_text(encoding="utf-8")
        if not content.endswith("\n") or any(
            not re.fullmatch(r"#include <[a-z0-9_]+>", line)
            for line in content.splitlines()
        ):
            raise ValueError(f"includes_{kind}.txt は標準ヘッダーの include だけを記載します")
        headers[kind] = content
    pieces: dict[str, str] = {}
    guards: set[str] = set()
    for name in (*PARTS, "primality32"):
        content = (directory / f"{name}.inc").read_text(encoding="utf-8")
        opening = re.search(r"^#ifndef (REINSIRK_[A-Z0-9_]+)\n#define \1\n", content, re.MULTILINE)
        if opening is None or not content.endswith("\n#endif\n"):
            raise ValueError(f"部品のガード形式が不正です: {name}")
        if opening[1] in guards:
            raise ValueError(f"部品間でガードが重複しています: {opening[1]}")
        if re.search(r"^#include\s", content, re.MULTILINE):
            raise ValueError(f"部品内の include は禁止です: {name}")
        guards.add(opening[1])
        pieces[name] = content
    return headers, pieces


def render(kind: str, roots: tuple[str, ...], headers: dict[str, str], pieces: dict[str, str]) -> str:
    condition = "#ifndef REINSIRK_PRIMALITY32_DEFINED"
    if kind == "prime":
        condition = "#if !defined(SKIP_RUNTIME_CHECKS) && !defined(REINSIRK_PRIMALITY32_DEFINED)"
    prototype = (
        condition + "\n"
        "// 内部の素数判定。定義はファイル末尾。\n"
        "namespace reinsirk::detail {\n"
        "inline bool is_prime_u32(std::uint32_t n) noexcept;\n"
        "} // namespace reinsirk::detail\n"
        "#endif\n"
    )
    definition = pieces["primality32"]
    if kind == "prime":
        definition = definition.replace("#ifndef REINSIRK_PRIMALITY32_DEFINED", condition, 1)
    content = "// SPDX-License-Identifier: CC0-1.0\n\n" + headers[kind] + "\n" + prototype + "\n"
    content += "\n".join(pieces[name] for name in dependency_order(roots))
    return content + "\n" + definition


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    base = Path(__file__).resolve().parent
    parser.add_argument("--components-dir", type=Path, default=base.parent / "Math" / "Combination" / "components")
    parser.add_argument("--output-dir", type=Path, default=base.parent)
    parser.add_argument("--check", action="store_true", help="書き込まず全生成物の一致を検証する")
    options = parser.parse_args()
    try:
        headers, pieces = load_sources(options.components_dir)
        failures = []
        for relative, (kind, roots) in TARGETS.items():
            content = render(kind, roots, headers, pieces)
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
        print(f"二項係数ライブラリ{len(TARGETS)}ファイル: " + ("同期確認済み" if options.check else "生成済み"))
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f"二項係数ライブラリの生成に失敗しました: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
