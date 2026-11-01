/**
 * 配置 Schema 的前端校验（纯函数）。
 *
 * 定位：**提前提示**，不是权威。服务端用同一份 schema 再校验一次，返回 422 +
 * fieldErrors；前端把服务端的错误挂在对应字段上。两边规则保持一致，但只有服务端的
 * 结论算数（§6.5）。
 *
 * 这里不引第三方 schema 库：字段类型只有 string / integer / boolean / array 四种，
 * format 只有 ipv4 / cidr / hostname / rfc3339，几十行就够，且能在 node 里直接单测。
 */
import type { ConfigSchema, ConfigSchemaField } from './types';

export interface FieldError {
  field: string;
  message: string;
}

const IPV4_RE = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

export function checkFormat(format: string | undefined, value: string): boolean {
  if (!format) return true;
  if (format === 'ipv4') {
    const match = IPV4_RE.exec(value);
    return !!match && match.slice(1).every((part) => Number(part) <= 255);
  }
  if (format === 'cidr') {
    const [address, prefix] = value.split('/');
    if (!IPV4_RE.test(address ?? '')) return false;
    const bits = Number(prefix);
    return Number.isInteger(bits) && bits >= 0 && bits <= 32;
  }
  if (format === 'hostname') return /^[A-Za-z0-9]([A-Za-z0-9.-]{0,253})$/.test(value);
  if (format === 'rfc3339') return !Number.isNaN(Date.parse(value));
  return true;
}

export function validateField(field: ConfigSchemaField, value: unknown): string | null {
  if (value === undefined || value === null || value === '') return null;
  if (field.enum && !field.enum.includes(value as never)) {
    return `取值必须是 ${field.enum.join(' / ')}`;
  }
  switch (field.type) {
    case 'string':
      if (typeof value !== 'string') return '必须是字符串';
      return field.format && !checkFormat(field.format, value) ? `格式不符合 ${field.format}` : null;
    case 'integer': {
      const number = typeof value === 'number' ? value : Number(value);
      if (!Number.isInteger(number)) return '必须是整数';
      if (field.minimum !== undefined && number < field.minimum) return `不得小于 ${field.minimum}`;
      if (field.maximum !== undefined && number > field.maximum) return `不得大于 ${field.maximum}`;
      return null;
    }
    case 'boolean':
      return typeof value === 'boolean' ? null : '必须是布尔值';
    case 'array': {
      if (!Array.isArray(value)) return '必须是数组';
      if (field.minItems !== undefined && value.length < field.minItems) return `至少 ${field.minItems} 项`;
      if (field.maxItems !== undefined && value.length > field.maxItems) return `最多 ${field.maxItems} 项`;
      const bad = value.findIndex(
        (item) => typeof item !== 'string' || (field.items?.format ? !checkFormat(field.items.format, item) : false),
      );
      return bad >= 0 ? `第 ${bad + 1} 项格式不符合 ${field.items?.format ?? 'string'}` : null;
    }
    default:
      return null;
  }
}

export function validateValues(schema: ConfigSchema, values: Record<string, unknown>): FieldError[] {
  const errors: FieldError[] = [];
  for (const name of schema.required ?? []) {
    const value = values[name];
    if (value === undefined || value === null || value === '') errors.push({ field: name, message: '必填' });
  }
  for (const [name, value] of Object.entries(values)) {
    const field = schema.properties[name];
    if (!field) {
      errors.push({ field: name, message: '未声明的字段' });
      continue;
    }
    const message = validateField(field, value);
    if (message) errors.push({ field: name, message });
  }
  return errors;
}

/** 只在草稿与基线之间取变化，用于"差异预览"和"是否有未应用修改"。 */
export function diffValues(
  baseline: Record<string, unknown>,
  draft: Record<string, unknown>,
): { field: string; before: unknown; after: unknown }[] {
  const keys = new Set([...Object.keys(baseline), ...Object.keys(draft)]);
  const changes = [];
  for (const key of [...keys].sort()) {
    const before = baseline[key];
    const after = draft[key];
    if (JSON.stringify(before) === JSON.stringify(after)) continue;
    changes.push({ field: key, before, after });
  }
  return changes;
}

/** 表单默认值：数组给空数组、布尔给 false、其余留空字符串，避免 v-model 拿到 undefined。 */
export function emptyValues(schema: ConfigSchema): Record<string, unknown> {
  const values: Record<string, unknown> = {};
  for (const [name, field] of Object.entries(schema.properties)) {
    if (field.type === 'array') values[name] = [];
    else if (field.type === 'boolean') values[name] = false;
    else if (field.type === 'integer') values[name] = field.minimum ?? 0;
    else values[name] = '';
  }
  return values;
}
