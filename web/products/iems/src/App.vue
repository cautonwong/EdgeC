<template>
  <router-view></router-view>
</template>

<script setup lang="ts">
import { onMounted } from 'vue';
import { useContractStore } from '@edgeos/web-core';

const store = useContractStore();

onMounted(() => {
  // 全局加载契约
  store.loadContract();
});
</script>

<style>
/* 全局基础样式 */
:root { --primary-color: #1890ff; }
body { margin: 0; font-family: sans-serif; }
</style>