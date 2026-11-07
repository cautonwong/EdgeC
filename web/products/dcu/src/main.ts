/**
 * DCU 产品入口（组合根）。与 iEMS 同构：只有 Registry + pages glob 的差别。
 */
import { createProductApp } from '@edgeos/web-core/app';
import { parseRegistry } from '@edgeos/web-core/sdk';
import registryJson from './generated/registry.json';
import './style.css';

const registry = parseRegistry(registryJson, 'dcu');
const pages = import.meta.glob('../pages/**/*.vue');

createProductApp({ registry, pages }).catch((err) => {
  const root = document.getElementById('app');
  if (root) {
    root.innerHTML = `<pre style="padding:24px;color:#c62828;white-space:pre-wrap">WebUI 启动失败：${String(err?.message ?? err)}</pre>`;
  }
  console.error('[webui] bootstrap failed', err);
});
