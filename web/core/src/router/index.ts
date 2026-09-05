import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router';
import AdminLayout from '../layouts/AdminLayout.vue';
import { useContractStore } from '../services/ContractLoader';

// 懒加载通用页面组件
const Dashboard = () => import('../views/Dashboard.vue');

export const coreRoutes: RouteRecordRaw[] = [
  {
    path: '/',
    component: AdminLayout,
    children: [
      { path: '', redirect: 'dashboard' },
      { path: 'dashboard', name: 'Dashboard', component: Dashboard, meta: { title: 'Dashboard', icon: '📊' } },
      { path: 'energy', name: 'Energy', component: () => import('../views/energy/EnergyFlowView.vue'), meta: { title: 'Microgrid', icon: '⚡' } },
      { path: 'devices', name: 'Devices', component: () => import('../views/devices/DeviceList.vue'), meta: { title: 'Devices', icon: '🔌' } },
      { path: 'data', name: 'Data', component: () => import('../views/data/DataCurves.vue'), meta: { title: 'Analysis', icon: '📈' } },
      { path: 'pipelines', name: 'Pipelines', component: () => import('../views/pipelines/PipelineList.vue'), meta: { title: 'Pipelines', icon: '⚙️' } },
      { path: 'scheduling', name: 'Scheduling', component: () => import('../views/system/Scheduling.vue'), meta: { title: 'Scheduling', icon: '🕒' } },
      { path: 'storage', name: 'Storage', component: () => import('../views/system/Storage.vue'), meta: { title: 'Storage', icon: '💾' } },
      { path: 'dcu/packets', name: 'DcuPackets', component: () => import('../views/dcu/PacketCapture.vue'), meta: { title: 'Packet Sniffer', icon: '📡' } },
      { path: 'dcu/power-quality', name: 'DcuPowerQuality', component: () => import('../views/dcu/PowerQuality.vue'), meta: { title: 'Power Quality', icon: '🧭' } },
      { path: 'events', name: 'Events', component: () => import('../views/events/EventStreamHub.vue'), meta: { title: 'SOE Events', icon: '🚨' } },
      { path: 'logs', name: 'Logs', component: () => import('../views/logs/LogStreamTerminal.vue'), meta: { title: 'Live Logs', icon: '📟' } },
      { path: 'metrics', name: 'Metrics', component: () => import('../views/metrics/MetricsMatrix.vue'), meta: { title: 'Metrics Matrix', icon: '🎛️' } },
      { path: 'system', name: 'System', component: () => import('../views/system/SystemConfig.vue'), meta: { title: 'Config', icon: '🛠️' } },
    ]
  }
];

export function getVisibleMenuItems() {
  try {
    const contractStore = useContractStore();
    const root = coreRoutes.find(r => r.path === '/');
    if (!root || !root.children) return [];
    
    return root.children
      .filter(r => r.name)
      .filter(r => contractStore.isPageVisible(r.name as string));
  } catch (e) {
    // If store is not yet available, return empty or all items as fallback
    console.warn('Store not ready in getVisibleMenuItems', e);
    const root = coreRoutes.find(r => r.path === '/');
    return root?.children?.filter(r => r.name) || [];
  }
}

export function createEdgeRouter(customRoutes: RouteRecordRaw[] = []) {
  return createRouter({
    history: createWebHashHistory(),
    routes: [...coreRoutes, ...customRoutes]
  });
}
