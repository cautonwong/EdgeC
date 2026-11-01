#!/usr/bin/env node
// 编译 Core SDK 供 node:test 使用。
//
// 为什么不用 vitest：这台环境与离线设备上都没有额外的测试框架依赖，而 SDK 的价值
// 恰恰是"零框架依赖、能在 node 里直接跑"。tsc 把 src/sdk 编成 CJS，再在产物目录放一个
// `{"type":"commonjs"}` —— 因为 core/package.json 是 `type: module`，否则 .js 会被
// 当成 ESM 解析（exports is not defined）。
import { execFileSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const coreDir = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const webDir = path.dirname(coreDir);
const tsc = path.join(webDir, 'node_modules', '.bin', 'tsc');

execFileSync(tsc, ['-p', path.join(coreDir, 'tsconfig.test.json')], { stdio: 'inherit', cwd: webDir });
mkdirSync(path.join(coreDir, 'test-build'), { recursive: true });
writeFileSync(path.join(coreDir, 'test-build', 'package.json'), '{"type":"commonjs"}\n');
console.log('[webui] core/test-build 就绪（CJS）');
