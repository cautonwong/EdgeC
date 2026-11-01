/**
 * 路由由**静态 Registry**生成（§6.2）：启动时只做过滤，不注册新路由。
 *
 * 守卫只负责体验：未登录 → 登录页；必须改密 → 只放行改密页；无权限/能力缺失 →
 * 明确原因的拒绝页。任何一条都不替代服务端鉴权。
 */
import { createRouter, createWebHashHistory, type RouteComponent, type RouteRecordRaw } from 'vue-router';
import type { PageDef, Registry } from '../sdk/types';
import AppShell from './layouts/AppShell.vue';
import ChangePasswordView from './views/ChangePasswordView.vue';
import DeniedView from './views/DeniedView.vue';
import LoginView from './views/LoginView.vue';
import NotFoundView from './views/NotFoundView.vue';
import { useCapabilityStore } from './stores/capability';
import { useRegistryStore } from './stores/registry';
import { useSessionStore } from './stores/session';

export type PageResolver = (page: PageDef) => RouteComponent | (() => Promise<RouteComponent>);

export function createAppRouter(registry: Registry, resolvePage: PageResolver) {
  const pageRoutes: RouteRecordRaw[] = registry.pages.map((page) => ({
    path: page.path,
    name: page.id,
    component: resolvePage(page) as RouteComponent,
    meta: { pageId: page.id, title: page.title },
  }));

  const router = createRouter({
    history: createWebHashHistory(),
    routes: [
      { path: '/login', name: 'login', component: LoginView, meta: { public: true } },
      { path: '/change-password', name: 'change-password', component: ChangePasswordView, meta: { passwordResetOnly: true } },
      {
        path: '/',
        component: AppShell,
        children: [
          { path: '', name: 'home', redirect: () => firstVisiblePath() },
          ...pageRoutes,
          { path: '/denied/:pageId', name: 'denied', component: DeniedView },
          { path: '/:pathMatch(.*)*', name: 'not-found', component: NotFoundView },
        ],
      },
    ],
  });

  router.beforeEach(async (to) => {
    const session = useSessionStore();
    const registryStore = useRegistryStore();
    const capability = useCapabilityStore();

    if (to.meta.public) return true;

    if (!session.isAuthenticated) {
      return { name: 'login', query: to.fullPath === '/' ? {} : { redirect: to.fullPath } };
    }

    // 登录后（含切换账号、刷新页面）必须先把能力快照取回来：没有它，任何需要能力的
    // 页面都只能按"不支持"处理，菜单会莫名其妙地少一半。
    if (capability.unknown && !capability.loading) {
      await capability.load();
    }

    // 强制改密期间只允许待在改密页（§6.1）
    if (session.mustChangePassword && !to.meta.passwordResetOnly) {
      return { name: 'change-password' };
    }

    const pageId = to.meta.pageId as string | undefined;
    if (pageId) {
      const page = registryStore.registry?.pages.find((candidate) => candidate.id === pageId);
      if (!page) return { name: 'not-found' };
      if (registryStore.stateOf(page) !== 'visible') {
        return { name: 'denied', params: { pageId: page.id } };
      }
    }
    return true;
  });

  return router;
}

function firstVisiblePath(): string {
  const registryStore = useRegistryStore();
  const first = registryStore.menu[0]?.items[0];
  return first?.path ?? '/denied/none';
}
