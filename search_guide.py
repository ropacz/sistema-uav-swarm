#!/usr/bin/env python3
"""
search_guide.py — busca BM25 no UserGuide.txt do OMNeT++

Uso:
    python3 search_guide.py "communication range visualization"
    python3 search_guide.py -n 5 "ini file parameters"
    python3 search_guide.py --list-sections
"""

import re
import sys
import math
import argparse
from pathlib import Path
from collections import Counter

GUIDE_PATH = Path(__file__).parent / "UserGuide.txt"
CHUNK_LINES = 50
OVERLAP     = 10
K1          = 1.5
B           = 0.75
TOP_N       = 3

# ── Chunking ────────────────────────────────────────────────────────────────

SECTION_RE = re.compile(r"^\d+(\.\d+)*\s{2,}\S")

def load_chunks(path: Path):
    """Divide o guia em seções (por cabeçalhos) com fallback de janela deslizante."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    chunks = []
    section_starts = [i for i, ln in enumerate(lines) if SECTION_RE.match(ln)]

    if len(section_starts) >= 5:
        # chunking por seção
        for idx, start in enumerate(section_starts):
            end = section_starts[idx + 1] if idx + 1 < len(section_starts) else len(lines)
            block = "\n".join(lines[start:end]).strip()
            if block:
                chunks.append((start + 1, lines[start].strip(), block))
    else:
        # fallback: janela deslizante
        i = 0
        while i < len(lines):
            block = "\n".join(lines[i : i + CHUNK_LINES]).strip()
            header = lines[i].strip()
            chunks.append((i + 1, header, block))
            i += CHUNK_LINES - OVERLAP

    return chunks

# ── BM25 ────────────────────────────────────────────────────────────────────

def tokenize(text: str) -> list[str]:
    return re.findall(r"[a-zA-Z]{2,}", text.lower())

def bm25(query_tokens: list[str], docs_tokens: list[list[str]]) -> list[float]:
    N    = len(docs_tokens)
    avgdl = sum(len(d) for d in docs_tokens) / N if N else 1

    idf: dict[str, float] = {}
    for term in set(query_tokens):
        df = sum(1 for doc in docs_tokens if term in set(doc))
        idf[term] = math.log((N - df + 0.5) / (df + 0.5) + 1)

    scores = []
    for doc in docs_tokens:
        tf  = Counter(doc)
        dl  = len(doc)
        s   = 0.0
        for term in query_tokens:
            f = tf.get(term, 0)
            s += idf.get(term, 0) * (f * (K1 + 1)) / (f + K1 * (1 - B + B * dl / avgdl))
        scores.append(s)
    return scores

# ── CLI ─────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="Busca BM25 no UserGuide.txt do OMNeT++")
    ap.add_argument("query", nargs="*", help="termos de busca")
    ap.add_argument("-n", "--top",  type=int, default=TOP_N, help="número de resultados (default 3)")
    ap.add_argument("--guide",      default=str(GUIDE_PATH),  help="caminho do guia")
    ap.add_argument("--list-sections", action="store_true",   help="lista todas as seções")
    args = ap.parse_args()

    guide = Path(args.guide)
    if not guide.exists():
        sys.exit(f"[ERRO] Guia não encontrado: {guide}")

    chunks = load_chunks(guide)

    if args.list_sections:
        for line_no, header, _ in chunks:
            print(f"  linha {line_no:5d}  {header[:80]}")
        return

    if not args.query:
        ap.print_help()
        return

    query        = " ".join(args.query)
    query_tokens = tokenize(query)
    if not query_tokens:
        sys.exit("[ERRO] Query vazia.")

    docs_tokens = [tokenize(block) for _, _, block in chunks]
    scores      = bm25(query_tokens, docs_tokens)
    ranked      = sorted(zip(scores, chunks), reverse=True)[: args.top]

    sep = "─" * 72
    print(f"\nQuery: {query!r}\n{sep}")
    for rank, (score, (line_no, header, block)) in enumerate(ranked, 1):
        if score < 0.01:
            print(f"\n[#{rank}] Nenhum resultado relevante encontrado.")
            break
        print(f"\n[#{rank}  score={score:.2f}  linha={line_no}]  {header}")
        print(sep)
        # imprime até 30 linhas do bloco para não poluir o contexto
        block_lines = block.splitlines()
        for ln in block_lines[:30]:
            print(ln)
        if len(block_lines) > 30:
            print(f"  … ({len(block_lines) - 30} linhas omitidas — veja linha {line_no} no guia)")
        print(sep)


if __name__ == "__main__":
    main()
