<template>
  <div class="rounded-xl border border-slate-700/60 bg-slate-900/90 overflow-hidden text-xs shadow-md my-2 font-mono">
    <!-- Header Bar -->
    <div 
      class="px-3 py-2 bg-slate-800/80 flex items-center justify-between cursor-pointer select-none hover:bg-slate-800 transition-colors"
      @click="isExpanded = !isExpanded"
    >
      <div class="flex items-center gap-2 overflow-hidden">
        <!-- Status Indicator -->
        <div class="flex-shrink-0">
          <svg aria-hidden="true" v-if="tool.status === 'executing'" class="animate-spin h-3.5 w-3.5 text-blue-400" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
            <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
            <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
          </svg>
          <span v-else-if="tool.status === 'success'" class="text-emerald-400 font-bold">✓</span>
          <span v-else class="text-rose-400 font-bold">✗</span>
        </div>

        <!-- Domain Badge -->
        <span 
          class="px-1.5 py-0.5 rounded text-xs font-bold"
          :class="domainColorClass"
        >
          {{ getToolDomain(tool.toolName) }}
        </span>

        <!-- Tool Name -->
        <span class="font-bold text-slate-200 truncate">{{ tool.toolName }}</span>
      </div>

      <div class="flex items-center gap-2 flex-shrink-0">
        <!-- Latency / Duration -->
        <span v-if="tool.durationMs !== undefined" class="text-xs text-slate-400 font-mono">
          {{ tool.durationMs }} ms
        </span>
        
        <!-- Chevron -->
        <svg aria-hidden="true"
          class="w-3.5 h-3.5 text-slate-400 transition-transform duration-200"
          :class="{ 'rotate-180': isExpanded }"
          fill="none" viewBox="0 0 24 24" stroke="currentColor"
        >
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
        </svg>
      </div>
    </div>

    <!-- Collapsible Body -->
    <div v-show="isExpanded" class="p-3 border-t border-slate-800 space-y-2.5 bg-slate-950/60">
      <!-- Arguments Section -->
      <div>
        <div class="flex items-center justify-between text-xs text-slate-400 font-semibold mb-1">
          <span>Parameters</span>
          <button 
            @click.stop="copyJson(tool.args)" 
            class="text-xs text-blue-400 hover:text-blue-300 underline cursor-pointer"
          >
            Copy
          </button>
        </div>
        <pre class="p-2 rounded-lg bg-slate-900 border border-slate-800 text-xs text-slate-300 overflow-x-auto max-h-32">{{ formatJson(tool.args) }}</pre>
      </div>

      <!-- Result / Error Section -->
      <div>
        <div class="flex items-center justify-between text-xs text-slate-400 font-semibold mb-1">
          <span>{{ tool.status === 'error' ? 'Error Output' : 'Physical Return' }}</span>
          <button 
            v-if="tool.result || tool.error"
            @click.stop="copyJson(tool.result || tool.error)" 
            class="text-xs text-blue-400 hover:text-blue-300 underline cursor-pointer"
          >
            Copy
          </button>
        </div>

        <pre 
          v-if="tool.status === 'error'"
          class="p-2 rounded-lg bg-rose-950/40 border border-rose-800/50 text-xs text-rose-300 overflow-x-auto"
        >{{ tool.error }}</pre>

        <pre 
          v-else-if="tool.result !== undefined"
          class="p-2 rounded-lg bg-slate-900 border border-slate-800 text-xs text-emerald-300 overflow-x-auto max-h-48"
        >{{ formatJson(tool.result) }}</pre>

        <div v-else class="text-xs text-slate-500 py-1">Executing on physical target...</div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import type { McpToolCall } from '../../services/AIProviderService';

const props = defineProps<{
  tool: McpToolCall;
}>();

const isExpanded = ref(false);

function getToolDomain(name: string): string {
  if (name.startsWith('datahub_')) return 'DataHub';
  if (name.startsWith('dcu_')) return 'Power Quality';
  if (name.startsWith('smartbox_')) return 'Transformer';
  if (name.startsWith('iems_')) return 'Energy';
  if (name.startsWith('security_')) return 'Safety';
  return 'System';
}

const domainColorClass = computed(() => {
  const domain = getToolDomain(props.tool.toolName);
  switch (domain) {
    case 'DataHub': return 'bg-blue-500/20 text-blue-300 border border-blue-500/30';
    case 'Power Quality': return 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30';
    case 'Transformer': return 'bg-amber-500/20 text-amber-300 border border-amber-500/30';
    case 'Energy': return 'bg-purple-500/20 text-purple-300 border border-purple-500/30';
    case 'Safety': return 'bg-rose-500/20 text-rose-300 border border-rose-500/30';
    default: return 'bg-slate-700 text-slate-300';
  }
});

function formatJson(val: any): string {
  try {
    return JSON.stringify(val, null, 2);
  } catch (e) {
    return String(val);
  }
}

function copyJson(val: any): void {
  try {
    navigator.clipboard.writeText(JSON.stringify(val, null, 2));
  } catch (e) {
    // Ignore clipboard errors
  }
}
</script>
