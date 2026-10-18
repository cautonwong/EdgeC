<template>
  <div class="app-container">
    <router-view></router-view>
  </div>
</template>

<script setup lang="ts">
// Initialization is handled by AdminLayout
</script>

<style>
:root { --primary-color: #1890ff; }
body { margin: 0; font-family: sans-serif; background: #f8fafc; }
.app-container { min-height: 100vh; }
</style>
