/**
 * 产品入口的装配（组合根）：一个产品 = Registry + 自己的页面 map + Core Shell。
 *
 * 顺序照 §6.1 走：system/info（匿名）→ 登录态判定 → auth/me → capabilities →
 * 生成菜单/路由 → 挂载。任何一步失败都不"猜默认值"，而是把状态交给 UI 显示。
 */
import '../ui/tokens.css';
import { createApp, type App as VueApp } from 'vue';
import { createPinia } from 'pinia';
import type { Router } from 'vue-router';
import type { PageDef, Registry } from '../sdk/types';
import App from './App.vue';
import { createAppRouter, type PageResolver } from './router';
import { useCapabilityStore } from './stores/capability';
import { useRegistryStore } from './stores/registry';
import { useSessionStore } from './stores/session';

/** Core 自带页面：构建期由 vite 静态收集，运行期按键查表，不存在"按字符串加载任意模块"。 */
const coreModules = import.meta.glob('../modules/**/*.vue');

export interface ProductAppOptions {
  registry: Registry;
  /** 产品页面的 glob map，键形如 `./pages/MetersView.vue` */
  pages: Record<string, () => Promise<unknown>>;
  mountSelector?: string;
}

export interface ProductAppHandle {
  app: VueApp;
  router: Router;
}

export async function createProductApp(options: ProductAppOptions): Promise<ProductAppHandle> {
  const app = createApp(App);
  const pinia = createPinia();
  app.use(pinia);

  const registryStore = useRegistryStore(pinia);
  registryStore.setRegistry(options.registry);

  const session = useSessionStore(pinia);
  await session.bootstrap();

  const capability = useCapabilityStore(pinia);
  if (session.isAuthenticated) {
    await capability.load();
  }

  // glob 的键形如 `../modules/Dashboard.vue` / `../pages/MetersView.vue`，统一去掉前缀，
  // 只保留 Registry 里写的相对路径（`modules/...`、`pages/...`）作为查表键。
  const normalize = (key: string) => key.replace(/^\.\.?\//, '');
  // Core 的 component 是相对 `core/src/modules/` 写的（如 `events/EventStreamHub.vue`），
  // 所以查表键要去掉 glob 带出来的 `modules/` 前缀。
  const coreMap = new Map(
    Object.entries(coreModules).map(([key, loader]) => [normalize(key).replace(/^modules\//, ''), loader]),
  );
  const packMap = new Map(Object.entries(options.pages).map(([key, loader]) => [normalize(key), loader]));

  const resolvePage: PageResolver = (page: PageDef) => {
    const loader = page.source === 'core' ? coreMap.get(page.component) : packMap.get(page.component);
    if (!loader) throw new Error(`Registry 指向的页面不存在: ${page.source}/${page.component}`);
    return loader as () => Promise<unknown> as never;
  };

  const router = createAppRouter(options.registry, resolvePage);
  app.use(router);
  app.mount(options.mountSelector ?? '#app');

  return { app, router };
}
