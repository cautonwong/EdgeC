#!/usr/bin/env node
// 构建期工具入口。CI 与本地跑同一份命令，避免"CI 绿、本地红"。
//
//   node tooling/cli.mjs validate              # 校验全部产品的构建输入
//   node tooling/cli.mjs registry --product iems
//   node tooling/cli.mjs boundaries            # 依赖边界（Core/Pack 单向）
//   node tooling/cli.mjs size --product iems    # 体积预算
//   node tooling/cli.mjs all                   # validate + boundaries + registry(all)
//   node tooling/cli.mjs catalog               # 打印权限目录摘要
import process from 'node:process';
import { checkBoundaries } from './lib/boundaries.mjs';
import { checkContrast } from './lib/contrast.mjs';
import { listProducts, loadCatalog, loadCoreManifest, loadProduct } from './lib/manifest.mjs';
import { buildRegistry } from './lib/registry.mjs';
import { loadSchemas } from './lib/schema.mjs';
import { checkSize } from './lib/size.mjs';

const argv = process.argv.slice(2);
const command = argv[0] ?? 'all';
const flag = (name) => {
  const index = argv.indexOf(`--${name}`);
  return index >= 0 ? argv[index + 1] : undefined;
};

function productArg() {
  const id = flag('product');
  if (!id) throw new Error('缺少 --product <id>');
  return id;
}

function log(message) {
  process.stdout.write(message + '\n');
}

async function run() {
  switch (command) {
    case 'validate': {
      const products = listProducts();
      loadCoreManifest();
      loadCatalog();
      for (const id of products) {
        loadProduct(id);
        log(`✓ ${id} 构建输入合法`);
      }
      log(`✓ Core 模块清单与权限目录合法（${products.length} 个产品）`);
      return;
    }
    case 'registry': {
      const ids = flag('all') !== undefined || argv.includes('--all') ? listProducts() : [productArg()];
      for (const id of ids) {
        const { registry, outFile } = buildRegistry(id);
        log(`✓ ${id}: ${registry.pages.length} 页面 / ${registry.api.length} 接口 → ${outFile}`);
        log(`  registry digest ${registry.digests.registry}`);
        if (registry.warnings.unusedPermissions.length) {
          log(`  ! 未被页面/接口/角色使用的权限 ${registry.warnings.unusedPermissions.length} 个: ${registry.warnings.unusedPermissions.join(', ')}`);
        }
        if (registry.warnings.droppedOperations.length) {
          log(`  ! 因 release-scope 关闭而未生成的接口: ${registry.warnings.droppedOperations.map((d) => d.operationId).join(', ')}`);
        }
      }
      return;
    }
    case 'boundaries': {
      const violations = await checkBoundaries();
      if (violations.length) {
        process.stderr.write(`依赖边界检查失败（${violations.length} 项）:\n${violations.map((v) => '  - ' + v).join('\n')}\n`);
        process.exitCode = 1;
        return;
      }
      log('✓ 依赖边界检查通过（Core 不依赖 Pack，Pack 只走 @edgeos/web-core）');
      return;
    }
    case 'size': {
      const id = productArg();
      const result = checkSize(id);
      if (result.skipped) {
        log(`- ${id}: ${result.reason}`);
        return;
      }
      for (const row of result.rows) {
        const tag = row.kind === 'entry' ? '  (entry)' : row.kind === 'vendor' ? `  (vendor: ${row.vendor}，已登记)` : '';
        log(`  ${row.file.padEnd(44)} ${(row.gzip / 1024).toFixed(1).padStart(7)}KB gzip${tag}`);
      }
      if (result.violations.length) {
        process.stderr.write(`体积预算超限:\n${result.violations.map((v) => '  - ' + v).join('\n')}\n`);
        process.exitCode = 1;
        return;
      }
      log(`✓ ${id} 体积在预算内`);
      return;
    }
    case 'contrast': {
      const { results, violations } = checkContrast();
      for (const row of results) {
        log(`  ${row.theme.padEnd(5)} ${row.fg.padEnd(20)} on ${row.bg.padEnd(20)} ${String(row.ratio).padStart(6)}:1 (min ${row.min})  ${row.usage}`);
      }
      if (violations.length) {
        process.stderr.write(`对比度不达标:\n${violations.map((v) => '  - ' + v).join('\n')}\n`);
        process.exitCode = 1;
        return;
      }
      log(`✓ ${results.length} 组语义配对均达标`);
      return;
    }
    case 'schemas': {
      const catalog = loadCatalog();
      const { schemas, problems } = loadSchemas(catalog);
      problems.throwIfAny();
      for (const schema of schemas) {
        const fields = Object.keys(schema.properties ?? {});
        log(`  ${schema.id.padEnd(22)} ${schema.riskClass}  ${String(schema.confirmWindowSeconds).padStart(3)}s  ${schema.writePermission.padEnd(26)} 字段: ${fields.join(', ')}`);
      }
      log(`✓ ${schemas.length} 份配置 Schema 合法`);
      return;
    }
    case 'catalog': {
      const catalog = loadCatalog();
      log(`权限目录 ${catalog.list.length} 条:`);
      for (const perm of catalog.list) {
        log(`  ${perm.id.padEnd(34)} ${perm.sensitivity ?? 'low'}  ${perm.description}`);
      }
      return;
    }
    case 'all': {
      loadCoreManifest();
      loadCatalog();
      const products = listProducts();
      for (const id of products) loadProduct(id);
      log(`✓ 构建输入合法（${products.length} 个产品）`);
      const violations = await checkBoundaries();
      if (violations.length) {
        process.stderr.write(`依赖边界检查失败（${violations.length} 项）:\n${violations.map((v) => '  - ' + v).join('\n')}\n`);
        process.exitCode = 1;
        return;
      }
      log('✓ 依赖边界检查通过');
      const contrast = checkContrast();
      if (contrast.violations.length) {
        process.stderr.write(`对比度不达标:\n${contrast.violations.map((v) => '  - ' + v).join('\n')}\n`);
        process.exitCode = 1;
        return;
      }
      log(`✓ 设计 Token 对比度达标（${contrast.results.length} 组配对）`);
      for (const id of products) {
        const { registry, outFile } = buildRegistry(id);
        log(`✓ ${id}: ${registry.pages.length} 页面 / ${registry.api.length} 接口 → ${outFile}`);
      }
      return;
    }
    default:
      throw new Error(`未知命令: ${command}`);
  }
}

run().catch((err) => {
  process.stderr.write(`${err.message}\n`);
  process.exitCode = 1;
});
