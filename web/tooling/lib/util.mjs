// 工具库：路径、YAML、规范化 JSON、摘要、文件遍历。
// 刻意只用 node 内置模块 + js-yaml（已随 web workspace 安装），构建期工具不引入
// 额外依赖，避免离线设备/CI 上再装一套包管理器。
import { createHash } from 'node:crypto';
import { readFileSync, existsSync, statSync } from 'node:fs';
import { readdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';

export const TOOLING_DIR = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
export const WEB_ROOT = path.dirname(TOOLING_DIR);

export function webPath(...parts) {
  return path.join(WEB_ROOT, ...parts);
}

export function readYaml(file) {
  if (!existsSync(file)) throw new Error(`文件不存在: ${rel(file)}`);
  const text = readFileSync(file, 'utf8');
  try {
    return yaml.load(text);
  } catch (err) {
    throw new Error(`${rel(file)} 不是合法 YAML: ${err.message}`);
  }
}

export function readJson(file) {
  return JSON.parse(readFileSync(file, 'utf8'));
}

export function rel(file) {
  return path.relative(WEB_ROOT, file).split(path.sep).join('/');
}

/** 稳定序列化：对象键排序、数组保持原序。用于生成可复现的 Registry 与摘要。 */
export function canonicalJson(value) {
  const walk = (node) => {
    if (Array.isArray(node)) return node.map(walk);
    if (node && typeof node === 'object') {
      const out = {};
      for (const key of Object.keys(node).sort()) {
        if (node[key] === undefined) continue;
        out[key] = walk(node[key]);
      }
      return out;
    }
    return node;
  };
  return JSON.stringify(walk(value));
}

export function sha256(text) {
  return createHash('sha256').update(text).digest('hex');
}

export function digestOf(value) {
  return `sha256:${sha256(canonicalJson(value))}`;
}

export async function listFiles(dir, filter = () => true) {
  if (!existsSync(dir)) return [];
  const out = [];
  const walk = async (current) => {
    for (const entry of await readdir(current, { withFileTypes: true })) {
      if (entry.name === 'node_modules' || entry.name === 'dist' || entry.name === 'generated') continue;
      const full = path.join(current, entry.name);
      if (entry.isDirectory()) await walk(full);
      else if (filter(full)) out.push(full);
    }
  };
  await walk(dir);
  return out.sort();
}

export function isDir(p) {
  return existsSync(p) && statSync(p).isDirectory();
}

/** 校验器：收集全部问题再抛，避免"改一个报一个"。 */
export class Problems {
  constructor(label) {
    this.label = label;
    this.items = [];
  }

  add(message, where = '') {
    this.items.push(where ? `${where}: ${message}` : message);
  }

  get ok() {
    return this.items.length === 0;
  }

  throwIfAny() {
    if (!this.ok) {
      const head = `${this.label} 未通过（${this.items.length} 项）:`;
      throw new Error([head, ...this.items.map((i) => `  - ${i}`)].join('\n'));
    }
  }
}

export const ID_RE = /^[a-z0-9][a-z0-9-]*(\.[a-z0-9][a-z0-9-]*)+$/;

/**
 * 动作集合。§5.2 定义了 read / write / execute / admin 四种语义，但附录 A 的权限目录
 * 里还有 `core.task:cancel`、`core.audit:export`、`system.log:export`。两处冲突时以
 * 权限目录为准（它才是运行时事实），同时把额外动作显式列出来——不接受任意动作名，
 * 新增动作必须改这里，避免 `foo:whatever` 混进目录。
 */
export const PERMISSION_ACTIONS = ['read', 'write', 'execute', 'admin', 'cancel', 'export'];
export const PERMISSION_RE = new RegExp(`^[a-z0-9][a-z0-9-]*\\.[a-z0-9][a-z0-9-]*:(${PERMISSION_ACTIONS.join('|')})$`);
export const PERMISSION_WILDCARD_RE = new RegExp(`^[a-z0-9][a-z0-9-]*\\.[a-z0-9*][a-z0-9-*]*:(\\*|${PERMISSION_ACTIONS.join('|')})$`);

export function fail(message) {
  throw new Error(message);
}
