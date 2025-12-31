<template>
  <div class="p-4">
    <h2 class="text-xl font-bold mb-4">Storage & Persistence</h2>
    <div class="card p-4 bg-white shadow rounded mb-4">
      <div class="flex justify-between items-center mb-4">
        <div>
          <div class="text-gray-500">DB Engine</div>
          <div class="text-xl font-bold">SQLite WAL Mode</div>
        </div>
        <div class="text-right">
          <div class="text-gray-500">DB Size</div>
          <div class="text-xl font-bold">{{ stats.db_size_kb }} KB</div>
        </div>
      </div>
      <div class="border-t pt-4">
        <div class="text-sm text-gray-600">Table Statistics:</div>
        <ul class="list-disc pl-5 mt-2">
          <li>System Audit Log: 1,245 rows</li>
          <li>Minute Freeze Table: 14,400 rows</li>
          <li>Real-time Snapshot: {{ stats.table_count }} tables active</li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const stats = ref<any>({});

onMounted(async () => {
  const res = await axios.get('/api/v1/storage/status');
  stats.value = res.data;
});
</script>
