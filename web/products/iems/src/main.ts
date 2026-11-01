/**
 * iEMS 产品入口 = 组合根。
 *
 * 这里**只做装配**：把构建期生成的 Registry 交给 Core Shell，再把自己 pages 目录的
 * 静态 glob 交出去。没有运行期注册、没有从服务端拉路由、没有插件加载。
 */
import { createProductApp } from '@edgeos/web-core';
import { parseRegistry } from '@edgeos/web-core/sdk';
import registryJson from './generated/registry.json';
import './style.css';

const registry = parseRegistry(registryJson, 'iems');
const pages = import.meta.glob('../pages/**/*.vue');

createProductApp({ registry, pages }).catch((err) => {
  // 启动失败必须显式暴露，不能白屏让人猜
  const root = document.getElementById('app');
  if (root) {
    root.innerHTML = `<pre style="padding:24px;color:#c62828;white-space:pre-wrap">WebUI 启动失败：${String(err?.message ?? err)}</pre>`;
  }
  console.error('[webui] bootstrap failed', err);
});
