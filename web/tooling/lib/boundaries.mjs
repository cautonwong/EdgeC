// 依赖边界检查（v1.6 §4.1 / §4.3 / WBUI-010）
//
// 规则不是"建议"，是构建门禁：
//   R1 core/src/sdk 是纯逻辑（无 vue/pinia/axios），否则它无法在 node 里被单测；
//   R2 core 永不 import products（Core 不得分叉）；
//   R3 products 只能通过 `@edgeos/web-core` 这个公开入口依赖 Core，不能伸手进 src/；
//   R4 products 之间互不引用；
//   R5 ui 层不依赖 app/modules；modules 层不依赖 app。
import path from 'node:path';
import { listFiles, rel, webPath } from './util.mjs';

const IMPORT_RE = /(?:from\s*|import\s*\(\s*)['"]([^'"]+)['"]/g;
const SDK_FORBIDDEN = ['vue', 'pinia', 'vue-router', 'axios', 'echarts', 'js-yaml'];
const CORE_ENTRY = '@edgeos/web-core';

export async function checkBoundaries() {
  const violations = [];
  const coreFiles = await listFiles(webPath('core/src'), (f) => /\.(ts|vue|mjs|js)$/.test(f));
  const productsRoot = webPath('products');
  const productDirs = [...new Set(await listFiles(productsRoot, (f) => /product\.yaml$/.test(f)).then((fs) => fs.map((f) => path.dirname(f))))];

  for (const file of coreFiles) {
    const zone = zoneOf(file);
    for (const spec of await importsOf(file)) {
      if (zone === 'sdk' && SDK_FORBIDDEN.includes(spec.split('/')[0])) {
        violations.push(`${rel(file)}: sdk 层不得依赖 ${spec}（纯逻辑必须可在 node 中直接测试）`);
      }
      if (spec.startsWith('@edgeos/web-core')) continue;
      const resolved = resolveRelative(file, spec);
      if (!resolved) continue;
      if (!resolved.startsWith(webPath('core'))) {
        violations.push(`${rel(file)}: core 不得引用 ${rel(resolved)}`);
        continue;
      }
      const target = zoneOf(resolved);
      if (zone === 'sdk' && ['app', 'modules', 'ui', 'legacy'].includes(target)) {
        violations.push(`${rel(file)}: sdk 层不得依赖 ${target} 层（${rel(resolved)}）`);
      }
      if (zone === 'ui' && ['app', 'modules'].includes(target)) {
        violations.push(`${rel(file)}: ui 层不得依赖 ${target} 层（${rel(resolved)}）`);
      }
      if (zone === 'modules' && target === 'app') {
        // 唯一例外：shell 暴露给页面的只读授权上下文（单向依赖 页面 → shell）
        const allowed = resolved.split(path.sep).join('/').includes('/core/src/app/composables/');
        if (!allowed) violations.push(`${rel(file)}: 页面模块只能依赖 app/composables 的只读上下文（${rel(resolved)}）`);
      }
    }
  }

  for (const dir of productDirs) {
    const files = await listFiles(path.join(dir, 'src'), (f) => /\.(ts|vue|mjs|js)$/.test(f));
    for (const file of files) {
      for (const spec of await importsOf(file)) {
        if (spec === CORE_ENTRY || spec.startsWith(CORE_ENTRY + '/')) {
          const sub = spec.slice(CORE_ENTRY.length + 1);
          if (sub && !['sdk', 'ui'].includes(sub.split('/')[0])) {
            violations.push(`${rel(file)}: Pack 只能依赖 core 的 sdk/ui 公开入口，不得依赖 ${spec}`);
          }
          continue;
        }
        const resolved = resolveRelative(file, spec);
        if (!resolved) continue;
        if (resolved.includes(`${path.sep}core${path.sep}src`)) {
          violations.push(`${rel(file)}: Pack 不得用相对路径伸手进 Core 源码（${spec}），请走 ${CORE_ENTRY}`);
          continue;
        }
        const other = productDirs.find((d) => resolved.startsWith(d) && d !== dir);
        if (other) {
          violations.push(`${rel(file)}: Pack 之间不得互相引用（${rel(other)}）`);
        }
      }
    }
  }

  return violations;
}

function zoneOf(file) {
  const normalized = file.split(path.sep).join('/');
  if (normalized.includes('/core/src/sdk/')) return 'sdk';
  if (normalized.includes('/core/src/ui/')) return 'ui';
  if (normalized.includes('/core/src/app/')) return 'app';
  if (normalized.includes('/core/src/modules/')) return 'modules';
  return 'legacy';
}

function resolveRelative(fromFile, spec) {
  if (!spec.startsWith('.')) return null;
  const base = path.dirname(fromFile);
  return path.resolve(base, spec);
}

async function importsOf(file) {
  const { readFile } = await import('node:fs/promises');
  const text = await readFile(file, 'utf8');
  const out = [];
  for (const match of text.matchAll(IMPORT_RE)) out.push(match[1]);
  return out;
}
