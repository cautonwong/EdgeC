import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router';
import AdminLayout from '../layouts/AdminLayout.vue';

// 懒加载通用页面组件
const Dashboard = () => import('../views/Dashboard.vue');
// ... (之前已写) ...

export const coreRoutes: RouteRecordRaw[] = [
  {
    path: '/',
    component: AdminLayout,
    children: [
      { path: '', redirect: 'dashboard' },
      { path: 'dashboard', name: 'Dashboard', component: Dashboard, meta: { title: 'Dashboard', icon: '📊' } },
      { path: 'devices', name: 'Devices', component: () => import('../views/devices/DeviceList.vue'), meta: { title: 'Devices', icon: '🔌' } },
      { path: 'data', name: 'Data', component: () => import('../views/data/DataCurves.vue'), meta: { title: 'Analysis', icon: '📈' } },
      { path: 'pipelines', name: 'Pipelines', component: () => import('../views/pipelines/PipelineList.vue'), meta: { title: 'Pipelines', icon: '⚙️' } },
      { path: 'scheduling', name: 'Scheduling', component: () => import('../views/system/Scheduling.vue'), meta: { title: 'Scheduling', icon: '🕒' } },
      { path: 'storage', name: 'Storage', component: () => import('../views/system/Storage.vue'), meta: { title: 'Storage', icon: '💾' } },
      { path: 'system', name: 'System', component: () => import('../views/system/SystemConfig.vue'), meta: { title: 'Config', icon: '🛠️' } },
    ]
  }
];

export function createEdgeRouter(customRoutes: RouteRecordRaw[] = []) {
  return createRouter({
    history: createWebHashHistory(),
    routes: [...coreRoutes, ...customRoutes]
  });
}
