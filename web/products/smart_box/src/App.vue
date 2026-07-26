<template>
  <div class="min-h-screen bg-slate-950 text-slate-100 flex flex-col p-6 font-sans antialiased selection:bg-emerald-500 selection:text-slate-900">
    
    <!-- 头部大字标题栏与全屏导航 -->
    <header class="flex flex-col lg:flex-row justify-between items-start lg:items-center border-b border-slate-800 pb-4 mb-6 gap-4">
      <div>
        <h1 class="text-3xl font-extrabold tracking-tight text-emerald-400">智能健康监护与大屏康复中枢 (SmartBox)</h1>
        <p class="text-sm text-slate-500 font-mono mt-1">NODE: SMARTBOX-X001 // SYSTEM STATUS: ONLINE // 4K HDMI OUT</p>
      </div>

      <!-- 选项卡与功能切换导航 -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="bg-slate-900 border border-slate-800 rounded-xl p-1 flex gap-1 font-mono text-sm">
          <button 
            @click="activeTab = 'dashboard'" 
            :class="activeTab === 'dashboard' ? 'bg-emerald-500 text-slate-950 font-black' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-lg transition"
          >
            📊 监护大屏
          </button>
          <button 
            @click="activeTab = 'game'" 
            :class="activeTab === 'game' ? 'bg-emerald-500 text-slate-950 font-black' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-lg transition"
          >
            🎮 50Hz 游戏数据桥
          </button>
          <button 
            @click="activeTab = 'feedback'" 
            :class="activeTab === 'feedback' ? 'bg-emerald-500 text-slate-950 font-black' : 'text-slate-400 hover:text-slate-200'"
            class="px-4 py-2 rounded-lg transition"
          >
            🌟 正向激励与双人PK
          </button>
        </div>

        <button @click="togglePrivacyMode" :class="[privacyMode ? 'bg-red-600 hover:bg-red-700' : 'bg-slate-800 hover:bg-slate-700']" class="px-4 py-2 rounded-xl font-bold text-sm tracking-wider transition uppercase border border-slate-700">
          {{ privacyMode ? '🔒 隐私保护中 (已硬件断电)' : '🔓 开启隐私沙盒' }}
        </button>
      </div>
    </header>

    <!-- Tab 1: 主监护面板 (Dashboard View) -->
    <div v-show="activeTab === 'dashboard'" class="space-y-6">
      <!-- 三大核心健康大卡片 (极高对比度) -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-6 mb-6">
        <div class="bg-slate-900 border-2 border-slate-800 hover:border-emerald-500/50 rounded-xl p-6 transition duration-200">
          <div class="text-sm font-bold text-slate-400 font-mono tracking-widest uppercase">🏃 步态运动分析 (Cadence)</div>
          <div class="text-5xl font-mono font-extrabold text-slate-100 mt-4">112 <span class="text-lg text-slate-400">步/分</span></div>
          <p class="text-xs text-emerald-400 font-mono mt-3">状态：步态高度对称 (非对称度 4.2%)</p>
        </div>
        <div class="bg-slate-900 border-2 border-slate-800 hover:border-cyan-500/50 rounded-xl p-6 transition duration-200">
          <div class="text-sm font-bold text-slate-400 font-mono tracking-widest uppercase">❤️ 物理实时心率 (BLE)</div>
          <div class="text-5xl font-mono font-extrabold text-cyan-400 mt-4">76 <span class="text-lg text-slate-400">BPM</span></div>
          <p class="text-xs text-slate-500 font-mono mt-3">手环信号：极佳 // 心率波动稳定</p>
        </div>
        <div class="bg-slate-900 border-2 border-slate-800 hover:border-amber-500/50 rounded-xl p-6 transition duration-200">
          <div class="text-sm font-bold text-slate-400 font-mono tracking-widest uppercase">💰 本日康复金币 (Coins)</div>
          <div class="text-5xl font-mono font-extrabold text-amber-400 mt-4">{{ goldCoins }} <span class="text-lg text-slate-400">金币</span></div>
          <button @click="redeemTraffic" class="text-xs font-bold bg-amber-500 hover:bg-amber-600 text-slate-950 px-3 py-1 rounded mt-3 transition font-mono uppercase">
            兑换 10MB 外网流量 (扣减 100 金币)
          </button>
        </div>
      </div>

      <!-- 下部左侧：100Hz 示波器波形，右侧：AI 情感机器人对话卡片 -->
      <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div class="lg:col-span-2 bg-slate-900 border border-slate-800 rounded-xl p-6 flex flex-col">
          <div class="flex justify-between items-center mb-4">
            <h2 class="text-lg font-extrabold text-emerald-400 font-mono uppercase">实时健康步态波形监视器 (100Hz Scope)</h2>
            <button @click="triggerCast" class="bg-emerald-500 hover:bg-emerald-600 text-slate-950 font-bold px-4 py-1.5 rounded-lg text-xs font-mono uppercase tracking-wider transition">
              📺 一键投屏到大屏电视
            </button>
          </div>
          <canvas id="waveCanvas" class="w-full h-64 bg-slate-950 border border-slate-800/80 rounded-lg"></canvas>
        </div>

        <div class="bg-slate-900 border border-slate-800 rounded-xl p-6 flex flex-col justify-between">
          <div>
            <h2 class="text-lg font-extrabold text-emerald-400 font-mono uppercase mb-4">💬 AI 虚拟康复伴侣</h2>
            <div class="bg-slate-950 border border-slate-800 rounded-lg p-4 h-48 overflow-y-auto mb-4 text-sm font-mono flex flex-col gap-2">
              <div v-for="(chat, idx) in chatLogs" :key="idx" :class="[chat.role === 'user' ? 'text-cyan-400' : 'text-slate-300']">
                [{{ chat.time }}] {{ chat.role === 'user' ? '您' : 'AI' }}: {{ chat.text }}
              </div>
            </div>
          </div>
          <div class="flex gap-2">
            <input v-model="inputText" @keyup.enter="sendMessage" class="flex-1 bg-slate-950 border border-slate-800 rounded-lg px-4 py-2 text-sm text-slate-100 focus:outline-none focus:border-emerald-500" placeholder="老人家，想聊点什么？比如“我吃药了”" />
            <button @click="sendMessage" class="bg-emerald-500 hover:bg-emerald-600 text-slate-950 font-bold px-4 py-2 rounded-lg text-sm transition">
              发送
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Tab 2: 50Hz 实时游戏数据桥组件 (Game Bridge View) -->
    <div v-show="activeTab === 'game'">
      <GameBridge />
    </div>

    <!-- Tab 3: 正向激励与双人竞技组件 (Positive Feedback View) -->
    <div v-show="activeTab === 'feedback'">
      <PositiveFeedback />
    </div>

  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import GameBridge from './components/GameBridge.vue';
import PositiveFeedback from './components/PositiveFeedback.vue';

const activeTab = ref<'dashboard' | 'game' | 'feedback'>('dashboard');
const privacyMode = ref(false);
const goldCoins = ref(150);
const inputText = ref('');
const chatLogs = ref([
	{ time: '11:00:05', role: 'ai', text: '您好，我是您的智能健康伴侣小康。今天有什么我可以帮您的吗？' }
]);

const togglePrivacyMode = () => {
	privacyMode.value = !privacyMode.value;
	fetch('/api/v1/privacy/toggle', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ active: privacyMode.value ? 1 : 0 })
	});
};

const redeemTraffic = () => {
	if (goldCoins.value >= 100) {
		goldCoins.value -= 100;
		fetch('/api/v1/monetization/redeem', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ client_ip: '192.168.1.100', client_mac: 'AA:BB:CC:DD:EE:FF', coins: 100 })
		});
		chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: '兑换成功！已为您手机开通 10MB 宽带上网额度。' });
	}
};

const triggerCast = () => {
	fetch('/api/v1/ui/cast', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ target_ip: '192.168.1.50', url: window.location.href })
	});
	chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: '已向客厅电视发起 DLNA 投屏请求。' });
};

const sendMessage = () => {
	if (!inputText.value.trim()) return;
	const text = inputText.value;
	chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'user', text });
	inputText.value = '';

	// Mock AI 聊天伴侣智能用药应答
	let reply = '老人家，今天也是元气满满的一天哦。记得适当运动。';
	if (text.includes('药') || text.includes('服药')) {
		reply = '温馨提醒：用药时间到啦，请备好温开水，按时服用阿司匹林。';
	} else if (text.includes('故事')) {
		reply = '那我就给您讲一个喜鹊登梅的故事... 祝您天天开心！';
	}

	setTimeout(() => {
		chatLogs.value.push({ time: new Date().toLocaleTimeString(), role: 'ai', text: reply });
	}, 500);
};

onMounted(() => {
	const canvas = document.getElementById('waveCanvas') as HTMLCanvasElement;
	if (canvas) {
		const ctx = canvas.getContext('2d');
		if (ctx) {
			let x = 0;
			const draw = () => {
				ctx.fillStyle = 'rgba(11, 15, 30, 0.05)';
				ctx.fillRect(0, 0, canvas.width, canvas.height);
				
				ctx.strokeStyle = '#10b981';
				ctx.lineWidth = 3;
				ctx.beginPath();
				ctx.moveTo(x, canvas.height / 2 + Math.sin(x * 0.15) * 40 + (Math.random() - 0.5) * 8);
				x = (x + 2) % canvas.width;
				ctx.lineTo(x, canvas.height / 2 + Math.sin(x * 0.15) * 40 + (Math.random() - 0.5) * 8);
				ctx.stroke();
				requestAnimationFrame(draw);
			};
			draw();
		}
	}
});
</script>
