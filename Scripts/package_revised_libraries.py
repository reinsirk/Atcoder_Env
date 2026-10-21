#!/usr/bin/env python3
"""整備済みの本体・テンプレートと使用例から、同期した配布ZIPを作る。"""

import argparse
from pathlib import Path
import subprocess
import sys
from zipfile import ZIP_DEFLATED, ZipFile


LIBRARIES = {
    "bellman_ford_example.cpp": "Graph/Bellman-Ford.cpp",
    "spfa_example.cpp": "Graph/Bellman-Ford_SFPA.cpp",
    "binary_trie_example.cpp": "DataStructure/Binary_Trie.cpp",
    "compressed_binary_trie_example.cpp": "DataStructure/Compressed_Binary_Trie.cpp",
    "binomial_prime_example.cpp": "Math/Combination_AtCoderMod.cpp",
    "binomial_example.cpp": "Math/Combination_Mod.cpp",
    "binomial_atcoder_example.cpp": "Math/Combination_Mod.cpp",
    "dijkstra_example.cpp": "Graph/Dijkstra.cpp",
    "primality_example.cpp": "Math/prime_judge(Miller-Rabin).cpp",
    "dynamic_fenwick_example.cpp": "DataStructure/dynamic-binary-indexed-tree.cpp",
    "dynamic_fenwick_2d_example.cpp": "DataStructure/dynamic-binary-indexed-tree-2d.cpp",
    "dynamic_li_chao_example.cpp": "DataStructure/dynamic-li-chao-tree.cpp",
    "dynamic_union_find_example.cpp": "DataStructure/dynamic-union-find.cpp",
    "hash_map_example.cpp": "DataStructure/Hash_map.cpp",
    "erasable_priority_queue_example.cpp": "DataStructure/erasable-priority-queue.cpp",
    "eratosthenes_example.cpp": "Math/Eratosthenes.cpp",
    "floyd_warshall_example.cpp": "Graph/Floyd-Warshall.cpp",
    "rolling_hash_example.cpp": "String/RollingHash.cpp",
    "project_selection_example.cpp": "Graph/Project_Selection.cpp",
    "zeta_mobius_example.cpp": "Math/Zeta_Mebius.cpp",
    "zeta_set_example.cpp": "Math/Zeta/Set_Transform.cpp",
    "zeta_bitwise_example.cpp": "Math/Zeta/Bitwise_Convolution.cpp",
    "zeta_sieve_example.cpp": "Math/Zeta/Sieve.cpp",
    "zeta_integer_example.cpp": "Math/Zeta/Divisor_Multiple_Transform.cpp",
    "zeta_convolution_example.cpp": "Math/Zeta/Gcd_Lcm_Convolution.cpp",
    "zeta_sparse_example.cpp": "Math/Zeta/Sparse_Transform.cpp",
    "combination_integer_example.cpp": "Math/Combination/Binomial_Mod.cpp",
    "combination_modint_example.cpp": "Math/Combination/ModInt_Binomial.cpp",
    "combination_prime_example.cpp": "Math/Combination/Prime_Binomial.cpp",
    "radix_sort_example.cpp": "Algorithm/Radix_sort.cpp",
    "lca_example.cpp": "Graph/LCA.cpp",
    "nearest_two_points_example.cpp": "Geometry/Nearest_Two_points.cpp",
    "heap_example.cpp": "DataStructure/heap.cpp",
    "implicit_treap_example.cpp": "DataStructure/implicit_treap.cpp",
    "geometry_example.cpp": "Geometry/geometory.cpp",
    "rollback_weighted_union_find_example.cpp": "DataStructure/rollback-union-findw.cpp",
    "matrix_transform_example.cpp": "Algorithm/rotate.cpp",
    "angle_sort_example.cpp": "Geometry/偏角ソート.cpp",
    "node_example.cpp": "DataStructure/Node.cpp",
    "trie_example.cpp": "String/Trie.cpp",
    "hash_set_example.cpp": "DataStructure/Hash_set.cpp",
    "line_container_example.cpp": "DataStructure/line-container.cpp",
    "line_container_2d_example.cpp": "DataStructure/line-container-2d.cpp",
    "union_find_example.cpp": "DataStructure/union_find.cpp",
    "union_find_w_example.cpp": "DataStructure/union_find_w.cpp",
    "rollback_union_find_example.cpp": "DataStructure/rollback-union-find.cpp",
    "persistent_union_find_example.cpp": "DataStructure/persistent-union-find.cpp",
    "lis_example.cpp": "Algorithm/LIS.cpp",
    "topological_example.cpp": "Graph/Topological.cpp",
    "topological_lexicographical_example.cpp": "Graph/Topological_sort.cpp",
    "next_combination_example.cpp": "Algorithm/next_combination.cpp",
}


def package(output: Path) -> None:
    root = Path(__file__).resolve().parents[1]
    for generator in ("generate_geometry.py", "generate_zeta.py", "generate_combination.py"):
        subprocess.run([sys.executable, str(root / "Scripts" / generator), "--check"], check=True)
    subprocess.run([sys.executable, str(root / "Scripts" / "prepare_verify.py"), "--check"], check=True)
    entries = {}
    for library in dict.fromkeys((*LIBRARIES.values(), "Internal/Nyaan_hash_map_min.cpp")):
        entries[library] = (root / library).read_bytes()
    for template in ("temp.cpp", "temp2.cpp"):
        entries[template] = (root / template).read_bytes()
    for example, library in LIBRARIES.items():
        name = "Examples/" + example
        source = (root / name).read_text(encoding="utf-8")
        include = f'#include "../{library}"'
        if source.count(include) != 1:
            raise ValueError(f"ライブラリのincludeを特定できません: {name}")
        entries[name] = source.encode("utf-8")
        standalone = source.replace(include, entries[library].decode("utf-8"))
        entries["StandaloneExamples/" + example] = standalone.encode("utf-8")
    for name in ("Readme.md", "AGENTS.md", "Scripts/package_revised_libraries.py"):
        entries[name] = (root / name).read_bytes()
    for generator in ("generate_geometry.py", "generate_zeta.py", "generate_combination.py"):
        entries["Scripts/" + generator] = (root / "Scripts" / generator).read_bytes()
    for name in ("Scripts/prepare_verify.py", "Scripts/library_layout.json"):
        entries[name] = (root / name).read_bytes()
    for directory in ("Graph", "DataStructure", "Math", "String", "Geometry", "Algorithm", "Internal", "Document"):
        for source_path in sorted((root / directory).rglob("*")):
            if source_path.is_file() and source_path.suffix in (".cpp", ".md", ".inc", ".txt"):
                entries[source_path.relative_to(root).as_posix()] = source_path.read_bytes()
    for source_path in sorted((root / "Verify" / "current").rglob("*")):
        if source_path.is_file():
            entries[source_path.relative_to(root).as_posix()] = source_path.read_bytes()
    license_path = root / "LICENSE"
    if license_path.is_file():
        entries["LICENSE"] = license_path.read_bytes()
    for license_path in sorted((root / "LICENSES").glob("*.txt")):
        entries[license_path.relative_to(root).as_posix()] = license_path.read_bytes()
    for audit_path in sorted((root / "Audit").iterdir()):
        if audit_path.is_file() and audit_path.suffix in (".json", ".md"):
            entries[audit_path.relative_to(root).as_posix()] = audit_path.read_bytes()
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
        for name, content in entries.items():
            archive.writestr(name, content)
    print(f"{len(entries)}ファイルを格納: {output.resolve()}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="出力ZIPのパス")
    args = parser.parse_args()
    package(args.output)


if __name__ == "__main__":
    main()
