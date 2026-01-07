<template>
  <span :class="[loading ? 'opacity-50 animate-pulse' : '']">
    {{ formattedValue }}
    <small v-if="showUnit && meta?.unit" class="ml-0.5 opacity-60">{{ meta.unit }}</small>
  </span>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import axios from 'axios';
// @ts-ignore
import { TagID } from '@generated/tags_type';
import { useContractStore } from '../services/ContractLoader';

const props = defineProps<{
  tagId: TagID;
  showUnit?: boolean;
  refreshMs?: number;
}>();

const contractStore = useContractStore();
const value = ref<number | string | null>(null);
const loading = ref(true);
let timer: any;

const meta = computed(() => contractStore.getTag(props.tagId));

const formattedValue = computed(() => {
  if (value.value === null) return '--';
  if (typeof value.value === 'number') {
    const precision = meta.value?.attributes?.ui?.precision ?? 2;
    return value.value.toFixed(precision);
  }
  return value.value;
});

const fetchData = async () => {
  try {
    const id = props.tagId;
    const res = await axios.get(`/api/v1/tags/${id}`);
    value.value = res.data.value;
    loading.value = false;
  } catch (e) {
    console.error(`Failed to fetch tag ${props.tagId}`, e);
  }
};

onMounted(() => {
  fetchData();
  if (props.refreshMs) {
    timer = setInterval(fetchData, props.refreshMs);
  }
});

onUnmounted(() => {
  if (timer) clearInterval(timer);
});
</script>
