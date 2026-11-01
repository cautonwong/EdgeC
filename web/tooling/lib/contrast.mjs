// Design Token 对比度检查（v1.6 §9.2 / REQ-UX-001）
//
// 读 tokens.css 的 `:root` 与 `[data-theme='dark']`，解析 var() 链，然后按
// token-pairs.json 里声明的语义配对逐对算 WCAG 对比度。改色不改成对关系，
// 构建就红——这是唯一能防住"主色换成好看但看不清"的办法。
import { readFileSync } from 'node:fs';
import { webPath } from './util.mjs';

const TOKENS_CSS = 'core/src/ui/tokens.css';
const PAIRS_JSON = 'core/src/ui/token-pairs.json';

export function parseTokenBlocks(css) {
  const blocks = {};
  const blockRe = /(^|\n)([^{}]+)\{([^}]*)\}/g;
  for (const match of css.matchAll(blockRe)) {
    const selector = match[2].trim();
    const body = match[3];
    const vars = {};
    for (const decl of body.matchAll(/(--[a-z0-9-]+)\s*:\s*([^;]+);/g)) {
      vars[decl[1]] = decl[2].trim();
    }
    if (!Object.keys(vars).length) continue;
    if (selector.includes(':root')) blocks.light = { ...(blocks.light ?? {}), ...vars };
    else if (selector.includes("data-theme='dark'") || selector.includes('data-theme="dark"')) {
      blocks.dark = { ...(blocks.dark ?? {}), ...vars };
    }
  }
  return blocks;
}

function resolveVar(name, scope, seen = new Set()) {
  if (seen.has(name)) return null;
  seen.add(name);
  const raw = scope[name];
  if (!raw) return null;
  const varRef = raw.match(/^var\(\s*(--[a-z0-9-]+)\s*(?:,\s*([^)]+))?\)$/);
  if (varRef) {
    const resolved = resolveVar(varRef[1], scope, seen);
    if (resolved) return resolved;
    if (varRef[2]) return varRef[2].trim();
    return null;
  }
  return raw;
}

function toRgb(color) {
  const hex = color.trim().replace('#', '');
  if (!/^[0-9a-fA-F]{6}$/.test(hex)) return null;
  return [0, 2, 4].map((i) => parseInt(hex.slice(i, i + 2), 16));
}

function luminance([r, g, b]) {
  const channel = (value) => {
    const v = value / 255;
    return v <= 0.03928 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4;
  };
  return 0.2126 * channel(r) + 0.7152 * channel(g) + 0.0722 * channel(b);
}

export function contrastRatio(fg, bg) {
  const a = luminance(fg);
  const b = luminance(bg);
  const [light, dark] = a > b ? [a, b] : [b, a];
  return (light + 0.05) / (dark + 0.05);
}

export function checkContrast() {
  const css = readFileSync(webPath(TOKENS_CSS), 'utf8');
  const pairs = JSON.parse(readFileSync(webPath(PAIRS_JSON), 'utf8')).pairs;
  const blocks = parseTokenBlocks(css);
  const results = [];
  const violations = [];

  for (const pair of pairs) {
    const themeBlock = blocks[pair.theme];
    if (!themeBlock) {
      violations.push(`缺少主题块 ${pair.theme}`);
      continue;
    }
    // 深色主题只覆盖语义层，基础色板仍在 `:root` 里 —— 解析时要回落到根作用域
    const scope = { ...(blocks.light ?? {}), ...themeBlock };
    const fgRaw = resolveVar(pair.fg, scope);
    const bgRaw = resolveVar(pair.bg, scope);
    const fg = fgRaw && toRgb(fgRaw);
    const bg = bgRaw && toRgb(bgRaw);
    if (!fg || !bg) {
      violations.push(`${pair.theme}: 无法解析 ${pair.fg} / ${pair.bg}（${fgRaw} / ${bgRaw}）`);
      continue;
    }
    const ratio = contrastRatio(fg, bg);
    const row = { theme: pair.theme, fg: pair.fg, bg: pair.bg, ratio: Number(ratio.toFixed(2)), min: pair.min, usage: pair.usage };
    results.push(row);
    if (ratio + 1e-9 < pair.min) {
      violations.push(`${pair.theme}: ${pair.fg} on ${pair.bg} = ${row.ratio}:1 < ${pair.min}:1（${pair.usage}）`);
    }
  }
  return { results, violations };
}
