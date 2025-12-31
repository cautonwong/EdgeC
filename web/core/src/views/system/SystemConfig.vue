<template>
  <div class="p-4">
    <h2 class="text-xl font-bold mb-4">System Configuration</h2>
    
    <div class="bg-white rounded shadow overflow-hidden">
      <table class="w-full">
        <thead class="bg-gray-50 text-left">
          <tr>
            <th class="p-3">Key</th>
            <th class="p-3">Value</th>
            <th class="p-3">Action</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in params" :key="item.key" class="border-b">
            <td class="p-3 font-mono">{{ item.key }}</td>
            <td class="p-3">
              <input v-model="item.value" class="border rounded px-2 py-1 w-full" />
            </td>
            <td class="p-3">
              <button @click="saveParam(item)" class="text-blue-600 hover:underline">Save</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';

const params = ref<any[]>([]);

const fetchParams = async () => {
  try {
    const res = await axios.get('/api/v1/system/config');
    // 假设后端返回 { params: [{key: 'a', value: 1}] }
    params.value = res.data.params || [];
  } catch (e) {
    // Mock for demo
    params.value = [
      { key: "max_charge_limit", value: 3000 },
      { key: "grid_price_threshold", value: 0.8 }
    ];
  }
};

const saveParam = async (item: any) => {
  try {
    await axios.post('/api/v1/system/config', {
      key: item.key,
      value: parseFloat(item.value)
    });
    alert('Config saved successfully');
  } catch (e) {
    alert('Failed to save config');
  }
};

onMounted(fetchParams);
</script>
