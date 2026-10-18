<template>
  <div class="app-container">
    <!-- Quick Status Bar Demonstration -->
    <div class="fixed top-4 right-8 z-[100] flex gap-4 pointer-events-none">
      <div class="bg-slate-900/80 backdrop-blur px-4 py-2 rounded-2xl border border-white/10 text-white text-xs font-mono flex items-center gap-2">
        <span class="opacity-50">GRID:</span>
        <TagValue :tag-id="TagID.GRID_POWER" :refresh-ms="2000" show-unit />
      </div>
      <div class="bg-blue-600/80 backdrop-blur px-4 py-2 rounded-2xl border border-white/10 text-white text-xs font-mono flex items-center gap-2 shadow-lg">
        <span class="opacity-50">SOC:</span>
        <TagValue :tag-id="TagID.BATTERY_SOC" :refresh-ms="5000" show-unit />
      </div>
    </div>

    <router-view></router-view>
  </div>
</template>

<script setup lang="ts">

</script>

<style>
:root { --primary-color: #1890ff; }
body { margin: 0; font-family: sans-serif; background: #f8fafc; }
.app-container { min-height: 100vh; }
</style>
