#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""部品内の公開API説明を、配布ソースの冒頭へ一括配置する。"""
from __future__ import annotations

from pathlib import PurePosixPath
import re


_BLOCK = re.compile(
    r"^[ \t]*// SIRKIT_API_BEGIN[ \t]*\n(.*?)"
    r"^[ \t]*// SIRKIT_API_END[ \t]*(?:\n|$)", re.MULTILINE | re.DOTALL
)
_DOCUMENT_BASE = "https://github.com/reinsirk/sirkit-cpp-library/blob/main/"


def validate_document_path(document: str) -> PurePosixPath:
    """コメント一行とURLのパスとして扱えるDocument名だけを受け付ける。"""
    path = PurePosixPath(document)
    if (any(character.isspace() or ord(character) < 32 or ord(character) == 127 for character in document)
            or "\\" in document or "?" in document or "#" in document
            or path.is_absolute() or ".." in path.parts
            or path.parts[:1] != ("Document",) or path.suffix != ".md"):
        raise ValueError(f"ドキュメントのパスが不正です: {document}")
    return path


def render_api_overview(content: str, documents: tuple[str, ...]) -> str:
    """説明のマーカーを展開し、出典・ライセンスの直後にAPI一覧を置く。"""
    blocks = list(_BLOCK.finditer(content))
    if not blocks:
        raise ValueError("公開APIの説明ブロックがありません")
    for marker in ("BEGIN", "END"):
        if len(re.findall(r"^[ \t]*// SIRKIT_API_" + marker + r"[ \t]*$", content, re.MULTILINE)) != len(blocks):
            raise ValueError("API説明のマーカーが対応していません")
    comments: list[str] = []
    for block in blocks:
        lines = block[1].splitlines()
        if any(line.strip() and not line.lstrip().startswith("//") for line in lines):
            raise ValueError("API説明には行コメントと空行だけを記載します")
        comments.extend(line.lstrip() if line.strip() else "//" for line in lines)
        comments.append("//")
    body = _BLOCK.sub("", content)
    if "SIRKIT_API_BEGIN" in body or "SIRKIT_API_END" in body:
        raise ValueError("API説明のマーカーが対応していません")
    while comments and comments[-1] == "//":
        comments.pop()
    links: list[str] = []
    for document in dict.fromkeys(documents):
        path = validate_document_path(document)
        links.append("// ドキュメント: " + _DOCUMENT_BASE + path.as_posix())
    if not links:
        raise ValueError("ドキュメントへの案内がありません")
    lines = body.splitlines(keepends=True)
    index = 0
    while index < len(lines) and (not lines[index].strip() or lines[index].lstrip().startswith("//")):
        index += 1
    prefix = "".join(lines[:index]).rstrip()
    rest = "".join(lines[index:]).lstrip("\n")
    overview = "\n".join([*links, "//", "// API一覧", *comments])
    return (prefix + "\n" if prefix else "") + overview + "\n\n" + rest
