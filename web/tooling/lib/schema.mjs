// 配置 Schema（v1.6 §6.5 / §6.4）
//
// 为什么要有这一层：配置页的风险等级、写权限、确认窗口、字段校验，如果分散在
// 前端表单、后端 handler 和文档三处，必然漂移。这里把 schema 当**唯一事实来源**：
//   - 构建期：校验字段/风险等级/权限引用，算摘要写进 Registry（前端据此渲染表单）；
//   - 运行期：服务端读同一份文件做校验、按 riskClass 决定要不要确认窗口。
// 前端不做"权威校验"，只做提前提示；服务端校验结果才是结论。
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { PERMISSION_RE, Problems, digestOf, webPath } from './util.mjs';

export const RISK_CLASSES = ['R0', 'R1', 'R2', 'R3', 'R4', 'R5'];
const FORMATS = ['ipv4', 'cidr', 'hostname', 'rfc3339', 'int'];
const TYPES = ['string', 'integer', 'boolean', 'array'];

export function loadSchemas(catalog) {
  const dir = webPath('schemas');
  const problems = new Problems('schemas/');
  const schemas = [];

  if (!existsSync(dir)) {
    return { schemas, problems };
  }

  for (const file of readdirSync(dir).filter((name) => name.endsWith('.json')).sort()) {
    const full = path.join(dir, file);
    let doc;
    try {
      doc = JSON.parse(readFileSync(full, 'utf8'));
    } catch (err) {
      problems.add(`不是合法 JSON: ${err.message}`, file);
      continue;
    }
    const where = file;

    if (doc.schemaVersion !== 1) problems.add('schemaVersion 必须为 1', where);
    const expectedId = file.replace(/\.json$/, '');
    if (doc.id !== expectedId) problems.add(`id 必须等于文件名（${expectedId}）`, where);
    if (!doc.title) problems.add('缺少 title', where);
    if (!/^\d+\.\d+\.\d+$/.test(String(doc.version))) problems.add('version 必须是 x.y.z', where);
    if (!RISK_CLASSES.includes(doc.riskClass)) problems.add(`riskClass 必须是 ${RISK_CLASSES.join('/')}`, where);
    if (!PERMISSION_RE.test(String(doc.writePermission ?? ''))) problems.add('writePermission 不是精确权限 ID', where);
    else if (!catalog.byId.has(doc.writePermission)) problems.add(`writePermission 未在权限目录声明: ${doc.writePermission}`, where);
    if (doc.readPermission && !catalog.byId.has(doc.readPermission)) {
      problems.add(`readPermission 未在权限目录声明: ${doc.readPermission}`, where);
    }
    // 风险等级决定确认窗口：R2/R3/R4 必须有窗口，R0/R1 不应有（否则每次改都弹倒计时）
    const window = doc.confirmWindowSeconds;
    if (!Number.isInteger(window) || window < 0) problems.add('confirmWindowSeconds 必须是非负整数', where);
    if (['R2', 'R3', 'R4'].includes(doc.riskClass) && !(window > 0)) {
      problems.add(`${doc.riskClass} 必须有确认窗口（confirmWindowSeconds > 0）`, where);
    }
    if (['R0', 'R1'].includes(doc.riskClass) && window > 0) {
      problems.add(`${doc.riskClass} 不应有确认窗口`, where);
    }

    if (doc.type !== 'object') problems.add('顶层 type 必须是 object', where);
    const properties = doc.properties ?? {};
    if (Object.keys(properties).length === 0) problems.add('至少声明一个字段', where);
    for (const [name, field] of Object.entries(properties)) {
      if (!TYPES.includes(field?.type)) problems.add(`字段 ${name} 的 type 必须是 ${TYPES.join('/')}`, where);
      if (!field?.title) problems.add(`字段 ${name} 缺少 title`, where);
      if (field?.format && !FORMATS.includes(field.format)) {
        problems.add(`字段 ${name} 的 format 未实现: ${field.format}`, where);
      }
      if (field?.type === 'array') {
        if (field.items?.type !== 'string') problems.add(`字段 ${name} 的 items.type 目前只支持 string`, where);
        if (field.items?.format && !FORMATS.includes(field.items.format)) {
          problems.add(`字段 ${name}.items 的 format 未实现: ${field.items.format}`, where);
        }
      }
      if (field?.enum && !Array.isArray(field.enum)) problems.add(`字段 ${name} 的 enum 必须是数组`, where);
    }
    for (const name of doc.required ?? []) {
      if (!properties[name]) problems.add(`required 引用了未声明字段: ${name}`, where);
    }

    // 整份 schema 进 Registry：前端据此渲染表单，服务端读同一份文件校验，
    // 不存在"前端一份字段定义、后端另一份"的漂移空间。
    schemas.push({ ...doc, digest: digestOf(doc) });
  }

  const ids = new Set();
  for (const schema of schemas) {
    if (ids.has(schema.id)) problems.add(`schema id 重复: ${schema.id}`);
    ids.add(schema.id);
  }

  return { schemas, problems };
}
