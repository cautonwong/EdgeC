#!/usr/bin/env python3
"""
Code Review Graph (CRG) Server & CLI Engine for EdgeOS / EdgeC
依据 docs/modular_2.md 规范实现：
提供与官方 code-review-graph 兼容的 CLI 与 MCP Server 协议：
- `code-review-graph build`: 扫描代码库构建全库 AST 符号表、调用拓扑 (Call Hierarchy) 与单测映射
- `code-review-graph serve`: 提供标准 MCP stdio 服务，暴露 `get_review_context_tool`
"""

import os
import sys
import re
import json
import sqlite3
import asyncio
from typing import List, Dict, Set, Any, Optional

try:
    from mcp.server.mcpserver import MCPServer
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
            print(f"🏗️ [CRG] 正在扫描全库源码并构建 SQLite 拓扑图谱: {self.db_path} ...", file=sys.stderr)
        
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
            if verbose:
                cur.execute("SELECT COUNT(*) FROM files")
                total_files = cur.fetchone()[0]
                cur.execute("SELECT COUNT(*) FROM symbols")
                total_symbols = cur.fetchone()[0]
                print(f"✅ [CRG] 构建完成: 索引 {total_files} 个文件, {total_symbols} 个符号定义。", file=sys.stderr)

    def query_review_context(self, changed_files: List[str]) -> Dict[str, Any]:
        """查询代码审查上下文：爆炸半径、调用链与受波及单测"""
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

                base_name = os.path.basename(cf)
                cur.execute("""
                    SELECT f.path FROM includes i
                    JOIN files f ON i.source_file_id = f.id
                    WHERE i.included_path LIKE ? AND f.is_test = 1
                """, (f"%{base_name}%",))
                for t_row in cur.fetchall():
                    impacted_tests.add(t_row["path"])

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

    def query_symbol_definitions(self, symbol_name: str) -> List[Dict[str, Any]]:
        """根据符号名查询定义信息"""
        definitions = []
        with self._get_conn() as conn:
            cur = conn.cursor()
            cur.execute("""
                SELECT s.name, s.kind, s.line, f.path, f.module
                FROM symbols s
                JOIN files f ON s.file_id = f.id
                WHERE s.name = ? OR s.name LIKE ?
                LIMIT 20
            """, (symbol_name, f"%{symbol_name}%"))
            for r in cur.fetchall():
                definitions.append({
                    "symbol": r["name"],
                    "kind": r["kind"],
                    "file": r["path"],
                    "line": r["line"],
                    "module": r["module"]
                })
        return definitions

    def query_symbol_callers(self, symbol_name: str) -> List[Dict[str, Any]]:
        """根据符号名查询跨文件调用方"""
        callers = []
        with self._get_conn() as conn:
            cur = conn.cursor()
            cur.execute("""
                SELECT r.symbol_name, f.path as caller_file, r.line, f.module
                FROM symbol_references r
                JOIN files f ON r.caller_file_id = f.id
                WHERE r.symbol_name = ?
                LIMIT 20
            """, (symbol_name,))
            for r in cur.fetchall():
                callers.append({
                    "symbol": r["symbol_name"],
                    "caller_file": r["caller_file"],
                    "line": r["line"],
                    "module": r["module"]
                })
        return callers

    def query_file_dependencies(self, file_path: str) -> Dict[str, Any]:
        """查询特定文件的依赖头文件及被引用关系"""
        includes = []
        depended_by = []
        with self._get_conn() as conn:
            cur = conn.cursor()
            cur.execute("""
                SELECT i.included_path
                FROM includes i
                JOIN files f ON i.source_file_id = f.id
                WHERE f.path = ?
            """, (file_path,))
            for r in cur.fetchall():
                includes.append(r["included_path"])

            base_name = os.path.basename(file_path)
            cur.execute("""
                SELECT f.path
                FROM includes i
                JOIN files f ON i.source_file_id = f.id
                WHERE i.included_path LIKE ?
            """, (f"%{base_name}%",))
            for r in cur.fetchall():
                depended_by.append(r["path"])

        return {
            "file": file_path,
            "direct_includes": includes,
            "included_by_files": depended_by
        }


crg_instance = CodeReviewGraph()

if HAS_MCP:
    server = MCPServer("CodeReviewGraph")

    @server.tool(name="get_review_context_tool", description="【CRG MCP】查询全库 AST 依赖图谱、调用链关系与变更爆炸半径 (Blast Radius)")
    def get_review_context_tool(changed_files: List[str]) -> str:
        res = crg_instance.query_review_context(changed_files)
        return json.dumps(res, indent=2, ensure_ascii=False)

    @server.tool(name="get_symbol_definitions", description="【CRG MCP】根据函数或结构体符号名称查询全库定义位置与模块归属")
    def get_symbol_definitions(symbol_name: str) -> str:
        res = crg_instance.query_symbol_definitions(symbol_name)
        return json.dumps(res, indent=2, ensure_ascii=False)

    @server.tool(name="get_symbol_callers", description="【CRG MCP】根据符号名查询全库跨模块调用方层级 (Call Hierarchy)")
    def get_symbol_callers(symbol_name: str) -> str:
        res = crg_instance.query_symbol_callers(symbol_name)
        return json.dumps(res, indent=2, ensure_ascii=False)

    @server.tool(name="get_file_dependencies", description="【CRG MCP】查询指定文件的直接 #include 头文件及全库反向包含依赖")
    def get_file_dependencies(file_path: str) -> str:
        res = crg_instance.query_file_dependencies(file_path)
        return json.dumps(res, indent=2, ensure_ascii=False)

    @server.tool(name="get_affected_tests", description="【CRG MCP】查询代码变更直接或间接影响的全库 CTest / GTest 单元测试集合")
    def get_affected_tests(changed_files: List[str]) -> str:
        res = crg_instance.query_review_context(changed_files)
        return json.dumps({"impacted_tests": res.get("impacted_tests", [])}, indent=2, ensure_ascii=False)

    @server.tool(name="get_blast_radius_summary", description="【CRG MCP】评估当前改动对全局系统的综合风险分值 (Risk Score) 与受波及产品线")
    def get_blast_radius_summary(changed_files: List[str]) -> str:
        res = crg_instance.query_review_context(changed_files)
        summary = {
            "risk_score": res.get("risk_score", "LOW"),
            "impacted_modules": res.get("impacted_modules", []),
            "impacted_products": res.get("impacted_products", [])
        }
        return json.dumps(summary, indent=2, ensure_ascii=False)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "build":
        crg = CodeReviewGraph()
        crg.build(verbose=True)
    elif len(sys.argv) > 1 and sys.argv[1] == "serve":
        if HAS_MCP:
            asyncio.run(server.run_stdio_async())
        else:
            print("MCPServer not installed", file=sys.stderr)
    else:
        crg = CodeReviewGraph()
        crg.build(verbose=True)
        sample = ["modules/edge_infra/npu/npu_ascend_acl.c"]
        ctx = crg.query_review_context(sample)
        print("Sample Review Context:", json.dumps(ctx, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
