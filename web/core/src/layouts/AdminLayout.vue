<template>
  <div class="admin-layout">
    <!-- Sidebar -->
    <aside class="sidebar">
      <div class="logo">EdgeOS</div>
      <nav>
        <router-link v-for="item in menuItems" :key="item.path" :to="item.path" class="nav-item">
          <span class="icon">{{ item.meta?.icon }}</span>
          <span class="title">{{ item.meta?.title }}</span>
        </router-link>
      </nav>
    </aside>

    <!-- Main Content -->
    <main class="content">
      <header class="top-bar">
        <div class="product-info">{{ contractStore.contract?.product || 'EdgeOS' }}</div>
        <div class="system-status">
          <span class="uptime">Uptime: {{ uptime }}s</span>
          <span class="indicator" :class="{ online: isOnline }"></span>
        </div>
      </header>
      
      <div class="page-body">
        <router-view></router-view>
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
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

onMounted(() => {
  fetchStatus();
  timer = setInterval(fetchStatus, 5000);
});

onUnmounted(() => clearInterval(timer));
</script>

<style scoped>
.admin-layout { display: flex; height: 100vh; width: 100vw; overflow: hidden; background: #f0f2f5; }
.sidebar { width: 240px; background: #001529; color: white; display: flex; flex-direction: column; }
.logo { height: 64px; line-height: 64px; text-align: center; font-size: 20px; font-weight: bold; background: #002140; }
.nav-item { padding: 16px 24px; color: rgba(255,255,255,0.65); text-decoration: none; display: flex; align-items: center; }
.nav-item:hover { color: white; background: #1890ff; }
.nav-item.router-link-active { color: white; background: #1890ff; }

.content { flex: 1; display: flex; flex-direction: column; overflow: hidden; }
.top-bar { height: 64px; background: white; padding: 0 24px; display: flex; justify-content: space-between; align-items: center; box-shadow: 0 1px 4px rgba(0,21,41,0.08); }
.page-body { flex: 1; padding: 24px; overflow-y: auto; }

.indicator { width: 10px; height: 10px; border-radius: 50%; background: #ccc; display: inline-block; margin-left: 8px; }
.indicator.online { background: #52c41a; box-shadow: 0 0 8px #52c41a; }
</style>
