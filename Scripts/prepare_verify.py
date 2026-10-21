#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""現在のライブラリを埋め込んだ提出コードを生成し、入力例をローカルで照合する。"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "Verify/current"
FAST_IO = "Verify/current/common/fast_io.hpp"
FAST_IO_INCLUDE = '#include "' + FAST_IO + '"'


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def preserve_submission_annotations() -> None:
    """提出コードへ追記されたURLを生成元へ保存する。判定結果は変更しない。"""
    for path in sorted((BASE / "groups").glob("*.json")):
        group = json.loads(path.read_text())
        changed = False
        for entry in group["entries"]:
            source = BASE / "submissions" / (entry["id"] + ".cpp")
            if not source.is_file():
                continue
            urls = re.findall(r"^// Submission: (https?://\S+)\s*$", source.read_text(), re.M)
            for url in urls:
                if url not in entry.get("submission_urls", []):
                    entry.setdefault("submission_urls", []).append(url)
                    changed = True
        if changed:
            path.write_text(json.dumps(group, ensure_ascii=False, indent=2) + "\n")


def expand_acl(source: str, seen: set[str] | None = None) -> str:
    """ACLの必要なヘッダーだけを展開する。提出環境のインストールに依存しない。"""
    if seen is None:
        seen = set()
    def replace(match: re.Match[str]) -> str:
        name = match[1].removesuffix(".hpp")
        if name in seen:
            return ""
        seen.add(name)
        path = BASE / "vendor" / (name + ".hpp")
        return "// ACL: " + name + "\n" + expand_acl(path.read_text(), seen)
    return re.sub(r'^#include [<"](atcoder/[a-z_]+)(?:\.hpp)?[>"]\s*$', replace, source, flags=re.M)


def collect() -> list[dict]:
    entries = []
    for path in sorted((BASE / "groups").glob("*.json")):
        for item in json.loads(path.read_text())["entries"]:
            entry = dict(item)
            entry["group"] = path.stem
            entries.append(entry)
    ids = [e["id"] for e in entries]
    if len(set(ids)) != len(ids):
        raise ValueError("Verify IDが重複しています")
    for entry in entries:
        url = entry.get("problem_url")
        if url and urlsplit(url).hostname not in {"judge.yosupo.jp", "atcoder.jp"}:
            raise ValueError("Verify先はLibrary CheckerまたはAtCoderを指定してください: " + entry["id"])
    directories = json.loads((ROOT / "Scripts/library_layout.json").read_text())
    libraries = {path.relative_to(ROOT).as_posix() for name in directories
                 for path in (ROOT / name).rglob("*.cpp")}
    covered = {name for entry in entries for name in entry["libraries"]}
    if libraries != covered:
        raise ValueError(f"本体と検証対象が一致しません: 未割当={libraries-covered}, 不明={covered-libraries}")
    return entries


def render(entry: dict, *, skip_runtime_checks: bool | None = None) -> tuple[bytes, dict[str, str]]:
    if skip_runtime_checks is None:
        skip_runtime_checks = bool(entry.get("problem_url"))
    prefix = "#define SKIP_RUNTIME_CHECKS\n" if skip_runtime_checks else ""
    local_label = ("ローカル入力例回帰" if entry.get("verification_kind") == "local_regression"
                   else "ローカル独立検査")
    header = prefix + "// Verify: " + (entry.get("problem_url") or local_label) + "\n"
    header += "".join("// Submission: " + url + "\n" for url in entry.get("submission_urls", []))
    pieces = [header]
    hashes = {}
    for name in entry["libraries"]:
        data = (ROOT / name).read_bytes()
        hashes[name] = digest(data)
        pieces.append("// Library: " + name + "\n" + data.decode() + "\n")
    driver = (ROOT / entry["driver"]).read_text()
    uses_fast_io = FAST_IO_INCLUDE in driver.splitlines()
    if entry.get("problem_url") and not uses_fast_io:
        raise ValueError("オンライン提出には共通高速入出力を指定してください: " + entry["id"])
    if uses_fast_io:
        if driver.splitlines().count(FAST_IO_INCLUDE) != 1:
            raise ValueError("共通高速入出力のincludeが重複しています: " + entry["id"])
        driver = driver.replace(FAST_IO_INCLUDE, "// Verify I/O: " + FAST_IO + "\n" +
                                (ROOT / FAST_IO).read_text())
    pieces.append(driver)
    text = expand_acl("\n".join(pieces))
    if re.search(r'^#\s*include\s*"', text, re.M) or re.search(r'^#\s*include\s*<atcoder/', text, re.M):
        raise ValueError("展開されていない依存があります: " + entry["id"])
    data = text.encode()
    if len(data) > 512 * 1024:
        raise ValueError("提出コードが512 KiBを超えています: " + entry["id"])
    return data, hashes


def prepare(check: bool) -> list[dict]:
    if not check:
        preserve_submission_annotations()
    entries = collect()
    journal_path = BASE / "judgements.json"
    journal = {item["id"]: item for item in json.loads(journal_path.read_text())["entries"]} if journal_path.exists() else {}
    (BASE / "submissions").mkdir(exist_ok=True)
    for entry in entries:
        data, hashes = render(entry)
        output = BASE / "submissions" / (entry["id"] + ".cpp")
        if check:
            if not output.is_file() or output.read_bytes() != data:
                raise ValueError("提出コードが本体と同期していません: " + str(output))
        else:
            output.write_bytes(data)
        record = journal.get(entry["id"])
        status = "未提出"
        if record:
            status = record["status"] if record["submission_sha256"] == digest(data) else "要再提出（ソース変更）"
            entry["judge_record"] = record
        entry.update(source_hashes=hashes, submission_sha256=digest(data),
                     submission=str(output.relative_to(ROOT)), bytes=len(data),
                     judge_status=status, acl_external_dependency=False,
                     skip_runtime_checks=bool(entry.get("problem_url")))
        driver_data = (ROOT / entry["driver"]).read_bytes()
        uses_fast_io = FAST_IO_INCLUDE in driver_data.decode().splitlines()
        entry.update(driver_sha256=digest(driver_data),
                     io_mode="shared_fast_io" if uses_fast_io else "local_driver",
                     io_source_hashes={FAST_IO: digest((ROOT / FAST_IO).read_bytes())}
                     if uses_fast_io else {})
    manifest = {"format": 1, "language": "C++23", "entries": entries}
    encoded = (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode()
    path = BASE / "manifest.json"
    if check:
        if path.read_bytes() != encoded:
            raise ValueError("manifestが本体と同期していません")
    else:
        path.write_bytes(encoded)
    return entries


def matches(entry: dict, sample: dict, output: str) -> bool:
    mode = entry.get("sample_checker", "tokens")
    actual, expected = output.split(), sample["output"].split()
    if mode == "float":
        return len(actual) == len(expected) and all(
            math.isclose(float(a), float(b), rel_tol=1e-7, abs_tol=1e-7)
            for a, b in zip(actual, expected))
    if mode == "shortest_path":
        it = iter(map(int, sample["input"].split()))
        n, m, start, goal = next(it), next(it), next(it), next(it)
        edges = {(u, v): w for u, v, w in zip(it, it, it)}
        if actual == ["-1"]:
            return expected == ["-1"]
        a = list(map(int, actual))
        if len(a) < 2 or len(a) != 2 + 2 * a[1] or a[0] != int(expected[0]):
            return False
        seen, at, weight = {start}, start, 0
        for u, v in zip(a[2::2], a[3::2]):
            if u != at or (u, v) not in edges or v in seen:
                return False
            seen.add(v); at = v; weight += edges[u, v]
        return at == goal and weight == a[0]
    if mode == "lis":
        inp = list(map(int, sample["input"].split()))
        values = inp[1:]
        out = list(map(int, actual))
        if not out or out[0] != len(out)-1 or out[0] != int(expected[0]):
            return False
        indices = out[1:]
        return all(0 <= i < inp[0] for i in indices) and all(
            i < j and values[i] < values[j] for i,j in zip(indices,indices[1:]))
    if mode == "topological":
        inp = list(map(int, sample["input"].split()))
        n, m = inp[:2]
        order = list(map(int, actual))
        if sorted(order) != list(range(n)):
            return False
        position = {v:i for i,v in enumerate(order)}
        return all(position[u] < position[v] for u,v in zip(inp[2::2],inp[3::2]))
    if mode == "closest_pair":
        inp = list(map(int, sample["input"].split()))
        count, pos = inp[0], 1
        if len(actual) != 2 * count:
            return False
        for test in range(count):
            n = inp[pos]; pos += 1
            points = list(zip(inp[pos:pos + 2*n:2], inp[pos + 1:pos + 2*n:2])); pos += 2*n
            i, j = map(int, actual[2*test:2*test+2])
            if not (0 <= i < n and 0 <= j < n and i != j):
                return False
            dist = lambda p, q: (p[0]-q[0])**2 + (p[1]-q[1])**2
            if dist(points[i], points[j]) != min(dist(points[u], points[v]) for u in range(n) for v in range(u)):
                return False
        return True
    if mode == "angle_sort":
        from collections import Counter
        from functools import cmp_to_key
        inp = list(map(int, sample["input"].split()))
        points = list(zip(inp[1::2], inp[2::2]))
        values = list(map(int, actual))
        if len(values) != 2 * inp[0]:
            return False
        result = list(zip(values[::2], values[1::2]))
        if Counter(points) != Counter(result):
            return False
        def half(p):
            return -1 if p[1] < 0 else (0 if p[1] == 0 and p[0] >= 0 else 1)
        def compare(p, q):
            if half(p) != half(q):
                return half(p) - half(q)
            cross = p[0]*q[1]-p[1]*q[0]
            return -1 if cross > 0 else (1 if cross < 0 else 0)
        ordered = sorted(points, key=cmp_to_key(compare))
        return all(compare(a,b) == 0 for a,b in zip(result,ordered))
    return actual == expected


def run_tests(entries: list[dict], compiler: str, fallback: bool, output: Path) -> None:
    results = []
    selected = entries
    with tempfile.TemporaryDirectory(prefix="atcoder_verify_") as tmp:
        for entry in selected:
            for skip in (False, True):
                binary = Path(tmp) / (entry["id"] + ("_skip" if skip else "_checked"))
                tested_source, _ = render(entry, skip_runtime_checks=skip)
                source_path = ROOT / entry["submission"]
                if digest(tested_source) != entry["submission_sha256"]:
                    source_path = binary.with_suffix(".cpp")
                    source_path.write_bytes(tested_source)
                command = [compiler, "-std=c++23", "-O2", "-Wall", "-Wextra"]
                if "clang" in Path(compiler).name and Path("/usr/lib/gcc/x86_64-linux-gnu/15").is_dir():
                    command += ["--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/15"]
                if fallback:
                    command += ["-U__SIZEOF_INT128__", "-DREINSIRK_FORCE_STANDARD_ARITHMETIC",
                                "-DREINSIRK_LINE_CONTAINER_FORCE_PORTABLE"]
                command += [str(source_path), "-o", str(binary)]
                built = subprocess.run(command, capture_output=True, text=True, timeout=180)
                result = dict(id=entry["id"], compiler=compiler, skip_runtime_checks=skip,
                              standard_fallback=fallback, submission_sha256=entry["submission_sha256"],
                              tested_source_sha256=digest(tested_source),
                              command=command[:-2], compile_ok=built.returncode == 0,
                              diagnostics=built.stderr, samples=[])
                if built.returncode == 0:
                    for sample in entry["samples"]:
                        try:
                            run = subprocess.run([str(binary)], input=sample["input"], text=True,
                                                 capture_output=True, timeout=60)
                            passed = run.returncode == 0 and matches(entry, sample, run.stdout)
                            result["samples"].append(dict(passed=passed, returncode=run.returncode,
                                                          stdout=run.stdout, stderr=run.stderr))
                        except (subprocess.TimeoutExpired, ValueError, IndexError) as error:
                            result["samples"].append(dict(passed=False, error=str(error)))
                result["passed"] = result["compile_ok"] and bool(result["samples"]) and all(s["passed"] for s in result["samples"])
                results.append(result)
                print(entry["id"], "skip" if skip else "checked", "OK" if result["passed"] else "FAIL", flush=True)
                output.write_text(json.dumps(dict(results=results), ensure_ascii=False, indent=2) + "\n")
    if not all(r["passed"] for r in results):
        raise SystemExit("失敗した検査があります: " + str(output))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--test", action="store_true")
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--fallback", action="store_true")
    parser.add_argument("--id", action="append", help="検査対象ID。省略時は全件")
    parser.add_argument("--results", type=Path, default=BASE / "local_results.json")
    options = parser.parse_args()
    entries = prepare(options.check)
    print(f"{len(entries)}件の提出・補助コードを" + ("照合" if options.check else "生成") + "しました。")
    if options.test:
        if options.id:
            ids = set(options.id)
            unknown = ids - {e["id"] for e in entries}
            if unknown:
                raise ValueError("存在しないID: " + str(unknown))
            entries = [e for e in entries if e["id"] in ids]
        run_tests(entries, options.compiler, options.fallback, options.results)


if __name__ == "__main__":
    main()
