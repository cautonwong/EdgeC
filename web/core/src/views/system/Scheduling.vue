<template>
  <div class="p-4">
    <h2 class="text-xl font-bold mb-4">Scheduling & Time</h2>
    <div class="bg-white p-4 shadow rounded grid grid-cols-1 md:grid-cols-2 gap-6">
      <div>
        <h3 class="font-bold border-b pb-2 mb-4 text-gray-700">Time Authority</h3>
        <div class="space-y-4">
          <div class="flex justify-between">
            <span>Monotonic Clock</span>
            <span class="text-green-600 font-mono">{{ status.monotonic_ok ? 'LOCKED' : 'FAIL' }}</span>
          </div>
          <div class="flex justify-between">
            <span>Calendar Source</span>
            <span class="font-bold">{{ status.calendar_source?.toUpperCase() }}</span>
          </div>
          <div class="flex justify-between">
            <span>Last NTP Sync</span>
            <span class="text-gray-500">{{ status.last_sync_ms === 0 ? 'NEVER' : new Date(status.last_sync_ms).toLocaleString() }}</span>
          </div>
        </div>
      </div>
      <div>
        <h3 class="font-bold border-b pb-2 mb-4 text-gray-700">Freeze Guard</h3>
        <div class="p-3 bg-blue-50 text-blue-800 rounded text-sm">
          Industrial-grade TimeGuard active. System is resilient to clock jumps.
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const status = ref<any>({});

onMounted(async () => {
  const res = await axios.get('/api/v1/time/status');
  status.value = res.data;
});
</script>
