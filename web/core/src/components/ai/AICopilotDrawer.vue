<template>
  <div>
    <!-- Backdrop -->
    <transition name="fade">
      <div 
        v-if="isOpen" 
        class="fixed inset-0 bg-slate-950/40 backdrop-blur-sm z-50 transition-opacity"
        @click="closeDrawer"
      ></div>
    </transition>

    <!-- Slide-over Panel -->
    <transition name="slide-right">
      <div 
        v-if="isOpen" 
        class="fixed top-0 right-0 bottom-0 z-50 bg-slate-900 border-l border-slate-800 shadow-2xl flex flex-col font-sans text-slate-200 transition-all duration-300 ease-in-out"
        :style="{ width: isFullScreen ? '100vw' : `${drawerWidthPx}px` }"
      >
        <!-- Top Navigation Header -->
        <div class="h-16 px-5 border-b border-slate-800 flex items-center justify-between bg-slate-950/60 backdrop-blur-md flex-shrink-0 select-none">
          <div class="flex items-center gap-3">
            <div class="w-8 h-8 rounded-xl bg-gradient-to-tr from-blue-600 to-indigo-500 flex items-center justify-center shadow-lg shadow-blue-500/20 ring-1 ring-white/20">
              <span class="text-white font-black text-sm">⚡</span>
            </div>
            <div>
              <div class="flex items-center gap-2">
                <span class="font-bold text-sm text-white tracking-tight">EdgeOS AI 工业伴侣</span>
                <span 
                  class="px-1.5 py-0.5 rounded-full text-[10px] font-mono font-bold flex items-center gap-1 border"
                  :class="isMcpConnected ? 'bg-emerald-500/10 text-emerald-400 border-emerald-500/30' : 'bg-amber-500/10 text-amber-400 border-amber-500/30'"
                  :title="isMcpConnected ? 'MCP WebSocket Connected (Port 8089)' : 'Using Standalone Fallback / Offline Engine'"
                >
                  <span class="w-1.5 h-1.5 rounded-full" :class="isMcpConnected ? 'bg-emerald-400 animate-pulse' : 'bg-amber-400'"></span>
                  {{ isMcpConnected ? 'MCP LIVE' : 'MCP OFFLINE' }}
                </span>
              </div>
              <div class="text-[10px] text-slate-400 font-mono">ReAct Industrial Co-Pilot</div>
            </div>
          </div>

          <!-- Actions -->
          <div class="flex items-center gap-1.5">
            <!-- Settings Button -->
            <button 
              @click="isSettingsOpen = !isSettingsOpen"
              class="p-2 rounded-lg hover:bg-slate-800 text-slate-400 hover:text-white transition-colors"
              title="Provider & Model Settings"
            >
              ⚙️
            </button>

            <!-- Fullscreen Toggle -->
            <button 
              @click="isFullScreen = !isFullScreen"
              class="p-2 rounded-lg hover:bg-slate-800 text-slate-400 hover:text-white transition-colors"
              title="Toggle Fullscreen"
            >
              {{ isFullScreen ? '🗗' : '🗖' }}
            </button>

            <!-- Clear History -->
            <button 
              @click="clearHistory"
              class="p-2 rounded-lg hover:bg-slate-800 text-slate-400 hover:text-white transition-colors"
              title="Clear Conversation"
            >
              🗑️
            </button>

            <!-- Close Button -->
            <button 
              @click="closeDrawer"
              class="p-2 rounded-lg hover:bg-slate-800 text-slate-400 hover:text-white transition-colors"
              title="Close Drawer (Esc)"
            >
              ✕
            </button>
          </div>
        </div>

        <!-- Settings Drawer Overlay -->
        <div v-if="isSettingsOpen" class="p-4 bg-slate-950 border-b border-slate-800 space-y-3 text-xs flex-shrink-0 animate-fadeIn">
          <div class="font-bold text-slate-200 uppercase tracking-widest text-[11px] flex justify-between">
            <span>LLM Backend & MCP Settings</span>
            <button @click="isSettingsOpen = false" class="text-slate-400 hover:text-white">✕</button>
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div>
              <label class="block text-slate-400 text-[10px] mb-1">Provider</label>
              <select v-model="config.provider" class="w-full bg-slate-900 border border-slate-800 rounded-lg p-2 text-slate-200">
                <option value="local_edge">Local Edge Embedded (Zero Dependency)</option>
                <option value="ollama">Ollama (Local LLM: 11434)</option>
                <option value="deepseek">DeepSeek API (V3/R1)</option>
                <option value="claude">Anthropic Claude (3.5 Sonnet)</option>
                <option value="openai">OpenAI (GPT-4o)</option>
              </select>
            </div>
            <div>
              <label class="block text-slate-400 text-[10px] mb-1">Model Name</label>
              <input v-model="config.modelName" class="w-full bg-slate-900 border border-slate-800 rounded-lg p-2 text-slate-200 font-mono" />
            </div>
          </div>

          <div class="grid grid-cols-2 gap-3">
            <div>
              <label class="block text-slate-400 text-[10px] mb-1">Endpoint URL</label>
              <input v-model="config.endpoint" class="w-full bg-slate-900 border border-slate-800 rounded-lg p-2 text-slate-200 font-mono" />
            </div>
            <div>
              <label class="block text-slate-400 text-[10px] mb-1">API Key (Optional)</label>
              <input v-model="config.apiKey" type="password" placeholder="sk-..." class="w-full bg-slate-900 border border-slate-800 rounded-lg p-2 text-slate-200 font-mono" />
            </div>
          </div>

          <div>
            <label class="block text-slate-400 text-[10px] mb-1">MCP Server WebSocket URL</label>
            <input v-model="config.mcpServerWsUrl" class="w-full bg-slate-900 border border-slate-800 rounded-lg p-2 text-slate-200 font-mono text-[11px]" />
          </div>
        </div>

        <!-- Context Pill Strip -->
        <div v-if="activeContext" class="px-5 py-2.5 bg-slate-950/40 border-b border-slate-800/80 flex items-center justify-between text-xs flex-shrink-0">
          <div class="flex items-center gap-2 overflow-hidden">
            <span class="text-blue-400 text-sm">🎯</span>
            <div class="truncate">
              <span class="text-slate-400">Context:</span>
              <span class="font-bold text-slate-200 ml-1">{{ activeContext.pageTitle || 'Monitoring View' }}</span>
              <span v-if="activeContext.selectedTagName" class="ml-1 text-emerald-400 font-mono font-bold">
                [{{ activeContext.selectedTagName }} = {{ activeContext.currentValue }} {{ activeContext.unit }}]
              </span>
            </div>
          </div>
          <button @click="activeContext = null" class="text-slate-500 hover:text-slate-300 text-[11px] underline flex-shrink-0 ml-2">
            Detach
          </button>
        </div>

        <!-- Chat History Scroll Container -->
        <div ref="chatContainer" class="flex-1 overflow-y-auto p-5 space-y-4">
          <div 
            v-for="msg in chatMessages" 
            :key="msg.id" 
            class="flex flex-col"
            :class="msg.role === 'user' ? 'items-end' : 'items-start'"
          >
            <!-- User Message -->
            <div 
              v-if="msg.role === 'user'" 
              class="max-w-[85%] rounded-2xl rounded-tr-sm bg-blue-600 text-white px-4 py-3 text-xs leading-relaxed shadow-lg shadow-blue-600/10"
            >
              {{ msg.content }}
            </div>

            <!-- Assistant Message -->
            <div 
              v-else 
              class="max-w-[95%] rounded-2xl rounded-tl-sm bg-slate-800/90 border border-slate-700/60 text-slate-200 px-4 py-3.5 text-xs leading-relaxed shadow-lg shadow-black/20 w-full"
            >
              <!-- Context Tag Attached to Message -->
              <div v-if="msg.context?.selectedTagName" class="mb-2.5 pb-2 border-b border-slate-700/60 flex items-center gap-2 text-[10px] text-slate-400 font-mono">
                <span class="text-blue-400">⚡ Target:</span>
                <span class="font-bold text-slate-300">{{ msg.context.selectedTagName }}</span>
                <span class="text-emerald-400">({{ msg.context.currentValue }} {{ msg.context.unit }})</span>
              </div>

              <!-- MCP Tool Calls Stream List -->
              <div v-if="msg.toolCalls && msg.toolCalls.length > 0" class="mb-3 space-y-1.5">
                <div class="text-[10px] font-bold text-slate-400 uppercase tracking-widest">Physical ReAct Actions:</div>
                <McpToolCallCard 
                  v-for="tool in msg.toolCalls" 
                  :key="tool.id" 
                  :tool="tool" 
                />
              </div>

              <!-- Message Markdown Content -->
              <div class="prose prose-invert prose-xs max-w-none space-y-2 whitespace-pre-wrap leading-relaxed text-slate-200" v-html="renderMarkdown(msg.content)"></div>

              <!-- Footer Latency -->
              <div v-if="msg.metrics?.latencyMs" class="mt-3 pt-2 border-t border-slate-700/40 text-[10px] text-slate-500 font-mono flex items-center justify-between">
                <span>⏱️ Reasoning Latency: {{ msg.metrics.latencyMs }} ms</span>
                <span>{{ formatTimestamp(msg.timestamp) }}</span>
              </div>
            </div>
          </div>

          <!-- Thinking Spinner -->
          <div v-if="isThinking" class="flex items-center gap-3 p-4 rounded-2xl bg-slate-800/50 border border-slate-800 text-xs text-slate-400 animate-pulse">
            <svg class="animate-spin h-4 w-4 text-blue-400" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
              <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
              <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
            </svg>
            <span>AI 正在执行物理测点诊断与 MCP 联动分析...</span>
          </div>
        </div>

        <!-- Quick Prompts Strip -->
        <div class="px-5 py-2 bg-slate-950/50 border-t border-slate-800/80 flex items-center gap-2 overflow-x-auto text-[11px] flex-shrink-0 no-scrollbar">
          <button 
            v-for="macro in quickPrompts" 
            :key="macro.label"
            @click="sendQuickPrompt(macro.prompt)"
            class="px-2.5 py-1 rounded-lg bg-slate-800/80 hover:bg-slate-700 text-slate-300 hover:text-white border border-slate-700 whitespace-nowrap transition-colors flex-shrink-0 cursor-pointer"
          >
            {{ macro.label }}
          </button>
        </div>

        <!-- Input Box -->
        <div class="p-4 bg-slate-950 border-t border-slate-800 flex-shrink-0">
          <div class="relative flex items-end gap-2 bg-slate-900 border border-slate-800 rounded-2xl p-2 focus-within:border-blue-500 focus-within:ring-1 focus-within:ring-blue-500 transition-all">
            <textarea
              ref="inputField"
              v-model="inputPrompt"
              @keydown.enter.exact.prevent="handleSubmit"
              @keydown.esc="closeDrawer"
              rows="2"
              placeholder="输入工业诊断问题或指令 (Enter 发送, Shift+Enter 换行)..."
              class="w-full bg-transparent border-0 resize-none text-xs text-slate-100 placeholder-slate-500 focus:outline-none px-2 py-1 leading-relaxed"
            ></textarea>
            <button
              @click="handleSubmit"
              :disabled="isThinking || !inputPrompt.trim()"
              class="p-2.5 rounded-xl bg-blue-600 hover:bg-blue-500 disabled:bg-slate-800 disabled:text-slate-600 text-white transition-colors cursor-pointer flex-shrink-0"
              title="Send Message"
            >
              <svg class="w-4 h-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 19l9 2-9-18-9 18 9-2zm0 0v-8" />
              </svg>
            </button>
          </div>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick, onMounted, onUnmounted } from 'vue';
import { useAICopilot } from '../../services/AIProviderService';
import McpToolCallCard from './McpToolCallCard.vue';

const {
  isCopilotOpen: isOpen,
  chatMessages,
  activeContext,
  isThinking,
  sendUserMessage,
  clearHistory,
  config,
  isMcpConnected,
} = useAICopilot();

const drawerWidthPx = ref(520);
const isFullScreen = ref(false);
const isSettingsOpen = ref(false);
const inputPrompt = ref('');
const chatContainer = ref<HTMLElement | null>(null);
const inputField = ref<HTMLTextAreaElement | null>(null);

const quickPrompts = [
  { label: '⚡ 全站异常扫描', prompt: '请对全站测点执行异常巡检扫描 (datahub_scan_abnormal)，并报告异常测点与原因。' },
  { label: '🔍 储能策略诊断', prompt: '请诊断当前储能系统的充放电与防逆流控制策略是否符合工况要求。' },
  { label: '📡 南向通道健康度', prompt: '请列出当前所有南向通信通道并评估报文误码率与通信质量。' },
  { label: '🚨 SOE 事故反演', prompt: '请查询最近 5 条 SOE 毫秒级故障告警事件并提供排障建议。' },
  { label: '📊 采样波形分析', prompt: '请读取 U_A 相瞬态录波数据并评估谐波畸变情况。' },
];

function closeDrawer() {
  isOpen.value = false;
}

function sendQuickPrompt(p: string) {
  sendUserMessage(p);
}

function handleSubmit() {
  if (!inputPrompt.value.trim() || isThinking.value) return;
  const p = inputPrompt.value;
  inputPrompt.value = '';
  sendUserMessage(p);
}

function scrollToBottom() {
  nextTick(() => {
    if (chatContainer.value) {
      chatContainer.value.scrollTop = chatContainer.value.scrollHeight;
    }
  });
}

watch(chatMessages, () => {
  scrollToBottom();
}, { deep: true });

watch(isOpen, (val) => {
  if (val) {
    nextTick(() => {
      inputField.value?.focus();
      scrollToBottom();
    });
  }
});

function formatTimestamp(ts: number): string {
  const d = new Date(ts);
  return `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}:${d.getSeconds().toString().padStart(2, '0')}`;
}

function renderMarkdown(raw: string): string {
  if (!raw) return '';
  // Lightweight secure Markdown renderer
  let html = raw
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');

  // Headers
  html = html.replace(/^### (.*$)/gim, '<h3 class="text-sm font-bold text-white mt-3 mb-1">$1</h3>');
  html = html.replace(/^#### (.*$)/gim, '<h4 class="text-xs font-bold text-slate-200 mt-2 mb-1">$1</h4>');

  // Blockquotes
  html = html.replace(/^\> (.*$)/gim, '<blockquote class="p-2 rounded-lg bg-slate-900 border-l-2 border-blue-500 text-slate-300 text-[11px] my-1.5">$1</blockquote>');

  // Code inline
  html = html.replace(/`([^`]+)`/g, '<code class="px-1.5 py-0.5 rounded bg-slate-950 text-emerald-400 font-mono text-[11px] border border-slate-800">$1</code>');

  // Bold
  html = html.replace(/\*\*([^*]+)\*\*/g, '<strong class="font-bold text-slate-100">$1</strong>');

  // Lists
  html = html.replace(/^- (.*$)/gim, '<li class="ml-4 list-disc text-slate-300 text-xs">$1</li>');

  return html;
}

// Global Hotkeys: Ctrl+K / Alt+A / Esc
function handleKeyDown(e: KeyboardEvent) {
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') {
    e.preventDefault();
    isOpen.value = !isOpen.value;
  } else if (e.altKey && e.key.toLowerCase() === 'a') {
    e.preventDefault();
    isOpen.value = !isOpen.value;
  }
}

onMounted(() => {
  window.addEventListener('keydown', handleKeyDown);
});

onUnmounted(() => {
  window.removeEventListener('keydown', handleKeyDown);
});
</script>

<style scoped>
.slide-right-enter-active,
.slide-right-leave-active {
  transition: transform 0.3s cubic-bezier(0.16, 1, 0.3, 1);
}
.slide-right-enter-from,
.slide-right-leave-to {
  transform: translateX(100%);
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}
.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

.no-scrollbar::-webkit-scrollbar {
  display: none;
}
.no-scrollbar {
  -ms-overflow-style: none;
  scrollbar-width: none;
}
</style>
