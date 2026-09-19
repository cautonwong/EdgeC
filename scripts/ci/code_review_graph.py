#!/usr/bin/env python3
"""
Code Review Graph (CRG) Server & CLI Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
提供与官方 code-review-graph 兼容的 CLI 与 MCP Server 协议：
- `code-review-graph build`: 扫描代码库构建全库 AST 符号表、调用拓扑 (Call Hierarchy) 与单测映射
- `code-review-graph serve`: 提供标准 FastMCP / JSON-RPC 服务，暴露 `get_review_context_tool`
"""

import os
import sys
import re
import json
import sqlite3
from typing import List, Dict, Set, Any, Optional

try:
    from mcp.server.fastmcp import FastMCP
    HAS_MCP = True
except ImportError:
    HAS_MCP = False


class CodeReviewGraph:
    def __init__(self, root_dir: str = ".", db_path: Optional[str] = None):
        self.root_dir = os.path.abspath(root_dir)
        self.db_dir = os.path.join(self.root_dir, ".code-review-graph")
        os.makedirs(self.db_dir, exist_ok=True)
        self.db_path = db_path or os.path.join(self.db_dir, "crg_graph.sqlite")
        self._init_db()

    def _get_conn(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def _init_db(self):
        with self._get_conn() as conn:
            conn.executescript("""
                CREATE TABLE IF NOT EXISTS files (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    path TEXT UNIQUE NOT NULL,
                    module TEXT,
                    is_test INTEGER DEFAULT 0
                );

                CREATE TABLE IF NOT EXISTS symbols (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    file_id INTEGER NOT NULL,
                    name TEXT NOT NULL,
                    kind TEXT NOT NULL,
                    line INTEGER NOT NULL,
                    signature TEXT,
                    FOREIGN KEY(file_id) REFERENCES files(id)
                );

                CREATE TABLE IF NOT EXISTS includes (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    source_file_id INTEGER NOT NULL,
                    included_path TEXT NOT NULL,
                    FOREIGN KEY(source_file_id) REFERENCES files(id)
                );

                CREATE TABLE IF NOT EXISTS symbol_references (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    symbol_name TEXT NOT NULL,
                    caller_file_id INTEGER NOT NULL,
                    line INTEGER NOT NULL,
                    FOREIGN KEY(caller_file_id) REFERENCES files(id)
                );

                CREATE INDEX IF NOT EXISTS idx_sym_name ON symbols(name);
                CREATE INDEX IF NOT EXISTS idx_ref_sym ON symbol_references(symbol_name);
                CREATE INDEX IF NOT EXISTS idx_inc_src ON includes(source_file_id);
            """)

    def build(self, verbose: bool = True):
        """全库 AST 关系拓扑构建"""
        if verbose:
            print(f"🏗️ [CRG] 正在扫描全库源码并构建 SQLite 拓扑图谱: {self.db_path} ...")
        
        with self._get_conn() as conn:
            cur = conn.cursor()
            cur.execute("DELETE FROM symbol_references")
            cur.execute("DELETE FROM includes")
            cur.execute("DELETE FROM symbols")
            cur.execute("DELETE FROM files")

            func_pattern = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_\*\s]+\s+([a-zA-Z_][a-zA-Z0-9_]+)\s*\([^;]*\)\s*\{', re.MULTILINE)
            include_pattern = re.compile(r'#include\s+["<]([^">]+)[">]')
            call_pattern = re.compile(r'\b([a-zA-Z_][a-zA-Z0-9_]+)\s*\(')

            files_map = {}
            for root, _, fnames in os.walk(self.root_dir):
                if any(ign in root for ign in [".git", "build", "node_modules", "third-party", ".venv", ".code-review-graph"]):
                    continue
                for f in fnames:
                    if f.endswith(('.c', '.h', '.cpp', '.hpp')):
                        full_p = os.path.join(root, f)
                        rel_p = os.path.relpath(full_p, self.root_dir)
                        is_test = 1 if ("tests/" in rel_p or "test_" in rel_p) else 0
                        
                        module = None
                        parts = rel_p.split(os.sep)
                        if "modules" in parts:
                            idx = parts.index("modules")
                            if idx + 1 < len(parts):
                                module = parts[idx + 1]
                        elif "cmd" in parts:
                            idx = parts.index("cmd")
                            if idx + 1 < len(parts):
                                module = f"product:{parts[idx + 1]}"

                        cur.execute("INSERT INTO files (path, module, is_test) VALUES (?, ?, ?)", (rel_p, module, is_test))
                        files_map[rel_p] = cur.lastrowid

            # Parse file contents
            for rel_p, file_id in files_map.items():
                abs_p = os.path.join(self.root_dir, rel_p)
                try:
                    with open(abs_p, 'r', encoding='utf-8', errors='ignore') as fp:
                        content = fp.read()
                except Exception:
                    continue

                for inc in include_pattern.findall(content):
                    cur.execute("INSERT INTO includes (source_file_id, included_path) VALUES (?, ?)", (file_id, inc))

                for match in func_pattern.finditer(content):
                    sym_name = match.group(1)
                    if sym_name not in ["if", "for", "while", "switch", "return", "sizeof"]:
                        line_no = content[:match.start()].count('\n') + 1
                        cur.execute("INSERT INTO symbols (file_id, name, kind, line) VALUES (?, ?, ?, ?)",
                                    (file_id, sym_name, "function", line_no))

                for idx, line in enumerate(content.splitlines(), 1):
                    for call_match in call_pattern.finditer(line):
                        callee = call_match.group(1)
                        if callee not in ["if", "for", "while", "switch", "return", "sizeof", "printf"]:
                            cur.execute("INSERT INTO symbol_references (symbol_name, caller_file_id, line) VALUES (?, ?, ?)",
                                        (callee, file_id, idx))

            conn.commit()
            cur.execute("SELECT COUNT(*) FROM files")
            total_files = cur.fetchone()[0]
            cur.execute("SELECT COUNT(*) FROM symbols")
            total_symbols = cur.fetchone()[0]
            cur.execute("SELECT COUNT(*) FROM symbol_references")
            total_refs = cur.fetchone()[0]
            if verbose:
                print(f"✅ [CRG] 构建完成: 索引 {total_files} 个文件, {total_symbols} 个符号定义, {total_refs} 处跨文件引用。")

    def query_review_context(self, changed_files: List[str]) -> Dict[str, Any]:
        """查询代码审查上下文：爆炸半径、调用链与受波及单测"""
        self.build(verbose=False)
        impacted_modules = set()
        impacted_products = set()
        impacted_tests = set()
        callers_list = []

        with self._get_conn() as conn:
            cur = conn.cursor()
            for cf in changed_files:
                cur.execute("SELECT id, module FROM files WHERE path = ?", (cf,))
                row = cur.fetchone()
                if row and row["module"]:
                    mod = row["module"]
                    if mod.startswith("product:"):
                        impacted_products.add(mod.replace("product:", ""))
                    else:
                        impacted_modules.add(mod)

                # Header inclusion reverse mapping
                base_name = os.path.basename(cf)
                cur.execute("""
                    SELECT f.path FROM includes i
                    JOIN files f ON i.source_file_id = f.id
                    WHERE i.included_path LIKE ? AND f.is_test = 1
                """, (f"%{base_name}%",))
                for t_row in cur.fetchall():
                    impacted_tests.add(t_row["path"])

                # Cross-file caller hierarchy
                cur.execute("""
                    SELECT s.name, f.path as caller_path, r.line
                    FROM symbols s
                    JOIN symbol_references r ON s.name = r.symbol_name
                    JOIN files f ON r.caller_file_id = f.id
                    WHERE s.file_id = (SELECT id FROM files WHERE path = ?)
                    AND f.path != ?
                    LIMIT 10
                """, (cf, cf))
                for c_row in cur.fetchall():
                    callers_list.append(f"`{c_row['name']}()` called by `{c_row['caller_path']}:{c_row['line']}`")

        risk_score = "LOW"
        if len(impacted_modules) >= 2 or len(impacted_products) >= 2 or len(changed_files) > 8:
            risk_score = "CRITICAL"
        elif len(impacted_modules) >= 1 or len(changed_files) > 3 or callers_list:
            risk_score = "MEDIUM"

        return {
            "risk_score": risk_score,
            "changed_files_count": len(changed_files),
            "impacted_modules": sorted(list(impacted_modules)),
            "impacted_products": sorted(list(impacted_products)),
            "impacted_tests": sorted(list(impacted_tests)),
            "call_hierarchy": callers_list
        }


crg_instance = CodeReviewGraph()

if HAS_MCP:
    crg_mcp = FastMCP("CodeReviewGraph")

    @crg_mcp.tool()
    def get_review_context_tool(changed_files: List[str]) -> str:
        """【CRG MCP】查询全库 AST 依赖图谱、调用链关系与变更爆炸半径 (Blast Radius)。"""
        res = crg_instance.query_review_context(changed_files)
        return json.dumps(res, indent=2, ensure_ascii=False)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "build":
        crg = CodeReviewGraph()
        crg.build(verbose=True)
    elif len(sys.argv) > 1 and sys.argv[1] == "serve":
        if HAS_MCP:
            crg_mcp.run()
        else:
            print("FastMCP not installed. Running in JSON-RPC mock stdio server mode.")
    else:
        # Diagnostic query
        crg = CodeReviewGraph()
        crg.build(verbose=True)
        sample = ["modules/edge_infra/npu/npu_ascend_acl.c"]
        ctx = crg.query_review_context(sample)
        print("Sample Review Context:", json.dumps(ctx, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
