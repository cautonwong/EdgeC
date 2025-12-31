<template>
  <div class="admin-layout" :class="`theme-${contractStore.contract?.ui_config?.branding?.theme || 'default'}`">
    <!-- Sidebar -->
    <aside class="sidebar">
      <div class="logo">
        <img v-if="contractStore.contract?.ui_config?.branding?.logo" 
             :src="contractStore.contract.ui_config.branding.logo" class="h-8 w-8 inline-block mr-2" />
        {{ contractStore.contract?.ui_config?.branding?.title || 'EdgeOS' }}
      </div>
      <nav>
        <!-- 核心差异化：根据契约导航开关进行显隐控制 -->
        <router-link 
          v-for="item in menuItems" 
          :key="item.path" 
          :to="item.path" 
          v-if="contractStore.isPageVisible(item.name as string)"
          class="nav-item"
        >
          <span class="icon">{{ item.meta?.icon }}</span>
          <span class="title">{{ item.meta?.title }}</span>
        </router-link>
      </nav>
    </aside>

    <!-- Main Content -->
    <main class="content">
      <header class="top-bar">
        <div class="product-info">
          <span class="font-bold text-gray-700">{{ contractStore.contract?.product || 'EdgeOS Core' }}</span>
          <span class="ml-2 text-xs text-gray-400 font-mono">v1.0.0</span>
        </div>
        <div class="system-status">
          <span class="text-sm text-gray-500 mr-4">Uptime: {{ formatTime(uptime) }}</span>
          <div class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium" 
               :class="isOnline ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'">
            {{ isOnline ? 'CONNECTED' : 'OFFLINE' }}
          </div>
        </div>
      </header>
      
      <div class="page-body">
        <router-view></router-view>
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { coreRoutes } from '../router';
import { useContractStore } from '../services/ContractLoader';
import axios from 'axios';

const contractStore = useContractStore();
const menuItems = coreRoutes.filter(r => r.name);
const uptime = ref(0);
const isOnline = ref(false);
let timer: number;

const fetchStatus = async () => {
  try {
    const res = await axios.get('/api/v1/system/health');
    uptime.value = Math.round(res.data.uptime);
    isOnline.value = true;
  } catch (e) {
    isOnline.value = false;
  }
};

function formatTime(sec: number) {
  const h = Math.floor(sec / 3600);
  const m = Math.floor((sec % 3600) / 60);
  const s = sec % 60;
  return `${h}:${m}:${s}`;
}

onMounted(async () => {
  // 初始化阶段强制加载契约，实现菜单自愈
  await contractStore.loadContract();
  fetchStatus();
  timer = setInterval(fetchStatus, 5000);
});

onUnmounted(() => clearInterval(timer));
</script>

<style scoped>
.admin-layout { display: flex; height: 100vh; width: 100vw; overflow: hidden; background: #f8fafc; }
.sidebar { width: 260px; background: #0f172a; color: white; display: flex; flex-direction: column; }
.logo { height: 64px; line-height: 64px; text-align: center; font-size: 18px; font-weight: 800; background: #1e293b; border-bottom: 1px solid #334155; }
.nav-item { padding: 14px 24px; color: #94a3b8; text-decoration: none; display: flex; align-items: center; transition: all 0.2s; }
.nav-item:hover { color: white; background: #334155; }
.nav-item.router-link-active { color: white; background: #3b82f6; font-weight: 600; }

.content { flex: 1; display: flex; flex-direction: column; overflow: hidden; }
.top-bar { height: 64px; background: white; padding: 0 24px; display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #e2e8f0; }
.page-body { flex: 1; padding: 24px; overflow-y: auto; }
</style>