// 体积预算（v1.6 §6.8）：首屏 entry gzip ≤ 300KB，单页面 chunk ≤ 100KB。
//
// vendor chunk 不适用 100KB 的"单页面"预算，但也不能随便大：超预算的 vendor 必须写进
// `size-budget.json` 的 vendorAllowlist，写明原因与后续任务。没有登记的，一律按普通
// chunk 判定失败 —— 这样"暂时做不到"是显式的债，不是悄悄放宽的门槛。
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import path from 'node:path';
import { gzipSync } from 'node:zlib';
import { webPath } from './util.mjs';

const BUDGET_FILE = 'tooling/size-budget.json';

function loadBudget() {
  const file = webPath(BUDGET_FILE);
  if (!existsSync(file)) return { entryBudgetKb: 300, chunkBudgetKb: 100, vendorAllowlist: {} };
  return JSON.parse(readFileSync(file, 'utf8'));
}

export function checkSize(productId) {
  const dir = webPath('products', productId, 'dist', 'assets');
  if (!existsSync(dir)) {
    return { skipped: true, reason: `未找到 products/${productId}/dist/assets，先构建再检查体积` };
  }
  const budget = loadBudget();
  const entryBudget = budget.entryBudgetKb * 1024;
  const chunkBudget = budget.chunkBudgetKb * 1024;

  const entryFiles = entryChunks(productId);
  const files = readdirSync(dir).filter((file) => file.endsWith('.js'));
  const rows = files.map((file) => {
    const raw = readFileSync(path.join(dir, file));
    const gzip = gzipSync(raw, { level: 9 }).length;
    const kind = entryFiles.has(file) ? 'entry' : vendorKey(file, budget.vendorAllowlist) ? 'vendor' : 'chunk';
    return { file, raw: raw.length, gzip, kind, vendor: vendorKey(file, budget.vendorAllowlist) };
  });

  const violations = [];
  for (const row of rows) {
    if (row.kind === 'entry') {
      if (row.gzip > entryBudget) {
        violations.push(`${row.file}: entry gzip ${kb(row.gzip)} 超过 ${kb(entryBudget)} 预算`);
      }
      continue;
    }
    if (row.kind === 'vendor') {
      const rule = budget.vendorAllowlist[row.vendor];
      if (row.gzip > rule.maxKb * 1024) {
        violations.push(`${row.file}: vendor gzip ${kb(row.gzip)} 超过登记上限 ${rule.maxKb}KB（${rule.task}）`);
      }
      continue;
    }
    if (row.gzip > chunkBudget) {
      violations.push(`${row.file}: chunk gzip ${kb(row.gzip)} 超过 ${kb(chunkBudget)} 预算`);
    }
  }
  return { rows: rows.sort((a, b) => b.gzip - a.gzip), violations, budget };
}

/** entry 以 dist/index.html 实际引用的脚本为准，而不是靠文件名猜（猜错会把 vendor 当首屏）。 */
function entryChunks(productId) {
  const indexFile = webPath('products', productId, 'dist', 'index.html');
  if (!existsSync(indexFile)) return new Set();
  const html = readFileSync(indexFile, 'utf8');
  const out = new Set();
  for (const match of html.matchAll(/(?:src|href)="[^"]*assets\/([^"]+\.js)"/g)) out.add(match[1]);
  return out;
}

function vendorKey(file, allowlist) {
  for (const key of Object.keys(allowlist ?? {})) {
    if (file.startsWith(`${key}-`)) return key;
  }
  return null;
}

function kb(bytes) {
  return `${(bytes / 1024).toFixed(1)}KB`;
}
