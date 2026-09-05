<template>
  <div class="min-h-screen bg-slate-950 text-slate-100 flex flex-col p-6 font-sans antialiased selection:bg-emerald-500 selection:text-slate-900">
    
    <!-- Top Header & Navigation -->
    <header class="flex flex-col lg:flex-row justify-between items-start lg:items-center border-b border-slate-800 pb-4 mb-6 gap-4">
      <div>
        <div class="flex items-center gap-2">
          <span class="inline-block w-3 h-3 rounded-full bg-emerald-400 animate-pulse"></span>
          <h1 class="text-2xl lg:text-3xl font-black tracking-tight text-white font-mono">
            SMARTBOX <span class="text-emerald-400 font-sans">REHAB & HEALTH HUB</span>
          </h1>
        </div>
        <p class="text-xs text-slate-500 font-mono mt-1">NODE: SMARTBOX-X001 // SYSTEM STATUS: ONLINE // 4K HDMI OUT</p>
      </div>

      <!-- Navigation Tabs & Privacy Toggle -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="bg-slate-900 border border-slate-800 rounded-2xl p-1 flex gap-1 font-mono text-xs">
          <button 
            @click="activeTab = 'dashboard'" 
            :class="activeTab === 'dashboard' ? 'bg-emerald-500 text-slate-950 font-black shadow-md' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-xl transition"
          >
            📊 监护大屏
          </button>
          <button 
            @click="activeTab = 'game'" 
            :class="activeTab === 'game' ? 'bg-emerald-500 text-slate-950 font-black shadow-md' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-xl transition"
          >
            🎮 50Hz 游戏数据桥
          </button>
          <button 
            @click="activeTab = 'feedback'" 
            :class="activeTab === 'feedback' ? 'bg-emerald-500 text-slate-950 font-black shadow-md' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-xl transition"
          >
            🌟 正向激励与双人PK
          </button>
        </div>

        <!-- Privacy High-Risk Toggle Button (Hooks into Double Authorization Modal) -->
        <button 
          @click="requestPrivacyToggle" 
          :class="[
            'px-4 py-2 rounded-2xl font-mono font-bold text-xs tracking-wider transition uppercase border flex items-center gap-2',
            privacyMode 
              ? 'bg-rose-950/80 border-rose-500 text-rose-300 shadow-[0_0_15px_rgba(239,68,68,0.4)]' 
              : 'bg-slate-900 hover:bg-slate-800 border-slate-700 text-slate-300'
          ]"
        >
          <span class="w-2 h-2 rounded-full" :class="privacyMode ? 'bg-rose-500 animate-ping' : 'bg-slate-500'"></span>
          <span>{{ privacyMode ? '🔒 隐私硬件已物理断电' : '🔓 开启隐私沙盒 (需双人复核)' }}</span>
        </button>
      </div>
    </header>

    <!-- Tab 1: Dashboard View -->
    <div v-show="activeTab === 'dashboard'" class="space-y-6">
      <!-- Top Health KPI Cards -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div class="bg-slate-900/90 border-2 border-slate-800 hover:border-emerald-500/50 rounded-3xl p-6 transition duration-200 backdrop-blur">
          <div class="text-xs font-bold text-slate-400 font-mono tracking-widest uppercase">🏃 步态运动分析 (Cadence)</div>
          <div class="text-4xl lg:text-5xl font-mono font-black text-slate-100 mt-4">112 <span class="text-sm text-slate-400">步/分</span></div>
          <p class="text-xs text-emerald-400 font-mono mt-3">状态：步态高度对称 (非对称度 4.2%)</p>
        </div>

        <div class="bg-slate-900/90 border-2 border-slate-800 hover:border-sky-500/50 rounded-3xl p-6 transition duration-200 backdrop-blur">
          <div class="text-xs font-bold text-slate-400 font-mono tracking-widest uppercase">❤️ 物理实时心率 (BLE)</div>
          <div class="text-4xl lg:text-5xl font-mono font-black text-sky-400 mt-4">76 <span class="text-sm text-slate-400">BPM</span></div>
          <p class="text-xs text-slate-500 font-mono mt-3">手环信号：极佳 // 心率波动稳定</p>
        </div>

        <div class="bg-slate-900/90 border-2 border-slate-800 hover:border-amber-500/50 rounded-3xl p-6 transition duration-200 backdrop-blur">
          <div class="text-xs font-bold text-slate-400 font-mono tracking-widest uppercase">💰 本日康复金币 (Coins)</div>
          <div class="text-4xl lg:text-5xl font-mono font-black text-amber-400 mt-4">{{ goldCoins }} <span class="text-sm text-slate-400">金币</span></div>
          <button @click="redeemTraffic" class="text-xs font-bold bg-amber-500 hover:bg-amber-600 text-slate-950 px-3 py-1.5 rounded-xl mt-3 transition font-mono uppercase shadow">
            兑换 10MB 外网流量 (扣减 100 金币)
          </button>
        </div>
      </div>

      <!-- Main 100Hz Scope & AI Companion Section -->
      <div class="grid grid-cols-1 lg:grid-cols-3 gap-6 items-start">
        <!-- 100Hz Hardware Oscilloscope Component -->
        <div class="lg:col-span-2">
          <WaveformScope100Hz />
        </div>

        <!-- AI Virtual Rehab Companion Chat Card -->
        <div class="bg-slate-900 border-2 border-slate-800 rounded-3xl p-6 flex flex-col justify-between h-[450px]">
          <div>
            <div class="flex items-center gap-2 mb-4">
              <span class="text-xl">🤖</span>
              <h2 class="text-lg font-extrabold text-emerald-400 font-mono uppercase">AI 虚拟康复伴侣</h2>
            </div>
            <div class="bg-slate-950 border border-slate-800 rounded-2xl p-4 h-64 overflow-y-auto mb-4 text-xs font-mono flex flex-col gap-2.5">
              <div v-for="(chat, idx) in chatLogs" :key="idx" :class="[chat.role === 'user' ? 'text-sky-400 text-right' : 'text-slate-300']">
                <span class="text-slate-600 text-[10px]">[{{ chat.time }}]</span> {{ chat.role === 'user' ? '您' : '小康' }}: {{ chat.text }}
              </div>
            </div>
          </div>

          <div class="flex gap-2">
            <input 
              v-model="inputText" 
              @keyup.enter="sendMessage" 
              class="flex-1 bg-slate-950 border border-slate-800 rounded-xl px-4 py-2.5 text-xs text-slate-100 focus:outline-none focus:border-emerald-500" 
              placeholder="老人家，想聊点什么？比如“我吃药了”" 
            />
            <button @click="sendMessage" class="bg-emerald-500 hover:bg-emerald-600 text-slate-950 font-bold px-4 py-2.5 rounded-xl text-xs font-mono transition">
              发送
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Tab 2: 50Hz Game Bridge View -->
    <div v-show="activeTab === 'game'">
      <GameBridge />
    </div>

    <!-- Tab 3: Positive Feedback View -->
    <div v-show="activeTab === 'feedback'">
      <PositiveFeedback />
    </div>

    <!-- Double Authorization Modal for High-Risk Physical Actions -->
    <DoubleAuthModal 
      :is-open="isAuthModalOpen"
      title="高危动作授权：硬件断电隐私沙盒 (Hardware Cutoff)"
      action-description="将切断 SmartBox 摄像头与麦克风物理继电器供电。此动作需由主操作员与监护主管双人复核输入 PIN 凭证授权。"
      @confirm="onAuthConfirmed"
      @cancel="isAuthModalOpen = false"
    />

  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import GameBridge from './components/GameBridge.vue';
import PositiveFeedback from './components/PositiveFeedback.vue';
import WaveformScope100Hz from './components/WaveformScope100Hz.vue';
import DoubleAuthModal from './components/DoubleAuthModal.vue';

const activeTab = ref<'dashboard' | 'game' | 'feedback'>('dashboard');
const privacyMode = ref(false);
const isAuthModalOpen = ref(false);
const goldCoins = ref(150);
const inputText = ref('');
const chatLogs = ref([
  { time: '11:00:05', role: 'ai', text: '您好，我是您的智能健康伴侣小康。今天有什么我可以帮您的吗？' }
]);

function requestPrivacyToggle() {
  if (!privacyMode.value) {
    // Entering privacy mode (cutting off hardware power) requires double authorization
    isAuthModalOpen.value = true;
  } else {
    // Exiting privacy mode can be directly restored
    privacyMode.value = false;
    fetch('/api/v1/privacy/toggle', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ active: 0 })
    }).catch(() => {});
    chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: '已恢复传感器与硬件设备供电。' });
  }
}

function onAuthConfirmed(payload: { operator: string; supervisor: string }) {
  isAuthModalOpen.value = false;
  privacyMode.value = true;
  fetch('/api/v1/privacy/toggle', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ 
      active: 1,
      operator_pin: payload.operator,
      supervisor_pin: payload.supervisor
    })
  }).catch(() => {});
  chatLogs.value.push({ 
    time: new Date().toLocaleTimeString(), 
    role: 'ai', 
    text: '【双人授权通过】已成功切断摄像头及麦克风物理硬件电源，进入隐私沙盒！' 
  });
}

function redeemTraffic() {
  if (goldCoins.value >= 100) {
    goldCoins.value -= 100;
    fetch('/api/v1/monetization/redeem', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ client_ip: '192.168.1.100', client_mac: 'AA:BB:CC:DD:EE:FF', coins: 100 })
    }).catch(() => {});
    chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: '兑换成功！已为您手机开通 10MB 宽带上网额度。' });
  }
}

function sendMessage() {
  if (!inputText.value.trim()) return;
  const text = inputText.value;
  chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'user', text });
  inputText.value = '';

  let reply = '老人家，今天也是元气满满的一天哦。记得适当运动。';
  if (text.includes('药') || text.includes('服药')) {
    reply = '温馨提醒：用药时间到啦，请备好温开水，按时服用阿司匹林。';
  } else if (text.includes('故事')) {
    reply = '那我就给您讲一个喜鹊登梅的故事... 祝您天天开心！';
  }

  setTimeout(() => {
    chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: reply });
  }, 500);
}
</script>
