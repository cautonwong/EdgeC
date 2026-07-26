<template>
  <div class="w-full bg-slate-900 border-2 border-slate-800 rounded-2xl p-8 text-slate-100 shadow-2xl font-sans my-6">
    
    <!-- 顶栏标题与模式切换 -->
    <div class="flex flex-col xl:flex-row justify-between items-start xl:items-center pb-6 border-b border-slate-800 gap-4">
      <div>
        <div class="flex items-center gap-3">
          <span class="text-3xl">🌟</span>
          <h2 class="text-3xl font-black text-amber-400 tracking-tight">正向激励与双人竞技中心 (Positive Feedback)</h2>
        </div>
        <p class="text-base text-slate-400 font-mono mt-1">
          适老化 TTS 语音关怀 // 训练成绩动画 // 4K 双人分屏同屏竞技
        </p>
      </div>

      <!-- 模式选择器 -->
      <div class="flex items-center gap-3 bg-slate-950 p-1.5 rounded-xl border border-slate-800">
        <button 
          @click="activeMode = 'single'" 
          :class="activeMode === 'single' ? 'bg-amber-500 text-slate-950 font-black' : 'text-slate-400 hover:text-slate-200'"
          class="px-5 py-2.5 rounded-lg text-sm font-mono transition"
        >
          🏆 单人结算激励模式
        </button>
        <button 
          @click="activeMode = 'dual'" 
          :class="activeMode === 'dual' ? 'bg-indigo-500 text-white font-black' : 'text-slate-400 hover:text-slate-200'"
          class="px-5 py-2.5 rounded-lg text-sm font-mono transition"
        >
          ⚔️ 双人 4K 同屏竞技比对
        </button>
      </div>
    </div>

    <!-- 模式 1: 单人正向激励与结算评分 (Score Animation + TTS Voice Praise) -->
    <div v-if="activeMode === 'single'" class="mt-6 space-y-6">
      
      <!-- 结算动效核心大卡片 -->
      <div class="bg-slate-950 border border-slate-800 rounded-2xl p-8 relative overflow-hidden flex flex-col items-center justify-center text-center">
        
        <!-- 背景炫彩光晕 -->
        <div class="absolute inset-0 bg-gradient-to-tr from-amber-500/10 via-emerald-500/10 to-indigo-500/10 blur-3xl pointer-events-none"></div>

        <div class="relative z-10">
          <div class="inline-block bg-amber-950/80 border border-amber-800/80 text-amber-400 px-4 py-1.5 rounded-full font-mono text-xs font-bold uppercase tracking-widest mb-4">
            REHAB SESSION COMPLETED // 处方训练结算
          </div>

          <!-- 分数数字增长动效 -->
          <div class="text-7xl md:text-8xl font-black font-mono text-transparent bg-clip-text bg-gradient-to-r from-amber-400 via-emerald-400 to-cyan-400">
            {{ displayScore }} <span class="text-2xl text-slate-400 font-normal">分</span>
          </div>

          <!-- 五星好评度 -->
          <div class="flex justify-center gap-2 text-4xl my-4">
            <span v-for="star in 5" :key="star" class="transition-all duration-300 transform hover:scale-125" :class="star <= starsCount ? 'text-amber-400 drop-shadow-[0_0_12px_rgba(251,191,36,0.6)]' : 'text-slate-700'">
              ★
            </span>
          </div>

          <!-- 完成度圆环与进度 -->
          <div class="w-full max-w-md mx-auto my-4 bg-slate-900 border border-slate-800 rounded-full h-6 p-1 relative overflow-hidden shadow-inner">
            <div 
              class="h-full rounded-full bg-gradient-to-r from-emerald-500 to-amber-400 transition-all duration-1000 ease-out flex items-center justify-end pr-2 font-mono text-xs font-bold text-slate-950"
              :style="{ width: `${displayCompletion}%` }"
            >
              {{ displayCompletion }}%
            </div>
          </div>

          <p class="text-lg font-bold text-slate-200 mt-2 font-mono">
            {{ praiseMessage }}
          </p>

          <!-- TTS 播报控制按钮 -->
          <div class="flex justify-center gap-4 mt-6">
            <button 
              @click="triggerTTSPraise" 
              class="bg-amber-500 hover:bg-amber-600 text-slate-950 font-black px-6 py-3 rounded-xl text-base font-mono transition flex items-center gap-2 shadow-lg shadow-amber-500/20"
            >
              <span>🗣️ 播报适老化 TTS 语音夸奖</span>
            </button>
            <button 
              @click="triggerConfetti" 
              class="bg-slate-800 hover:bg-slate-700 text-emerald-400 border border-slate-700 font-bold px-5 py-3 rounded-xl text-base font-mono transition"
            >
              🎉 重放得分动画与特效
            </button>
          </div>
        </div>
      </div>

      <!-- 子女关怀与家属互动卡片 -->
      <div class="bg-slate-950 border border-slate-800 rounded-2xl p-6">
        <h3 class="text-xl font-extrabold text-emerald-400 font-mono mb-4 flex items-center gap-2">
          💌 家属远程关怀与爱心留言 (Family Praise)
        </h3>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div v-for="(msg, idx) in familyMessages" :key="idx" class="bg-slate-900 border border-slate-800/80 rounded-xl p-4 flex gap-4 items-start">
            <div class="text-3xl">{{ msg.avatar }}</div>
            <div class="flex-1">
              <div class="flex justify-between items-center">
                <span class="font-bold text-slate-200 text-sm font-mono">{{ msg.sender }}</span>
                <span class="text-xs text-slate-500 font-mono">{{ msg.time }}</span>
              </div>
              <p class="text-sm text-slate-300 mt-1 font-mono leading-relaxed">{{ msg.text }}</p>
              <button 
                @click="speakText(msg.sender + '留言说：' + msg.text)" 
                class="text-xs text-amber-400 hover:underline mt-2 font-mono block"
              >
                ▶ 朗读家属留言
              </button>
            </div>
          </div>
        </div>
      </div>

    </div>

    <!-- 模式 2: 双人 4K 同屏分屏竞技 (Dual-Player Split Screen) -->
    <div v-else-if="activeMode === 'dual'" class="mt-6 space-y-6">
      
      <!-- 4K 同屏分屏竞技顶栏 -->
      <div class="bg-slate-950 border border-slate-800 rounded-xl p-4 flex justify-between items-center font-mono">
        <div class="flex items-center gap-2 text-sm text-emerald-400 font-bold">
          <span>⚔️ 双人实时运动数据对比总线 (50Hz DataHub Synced)</span>
        </div>
        <div class="text-xs text-slate-400">
          裁判胜负判定规则：关节标准角度与完成度综合评定
        </div>
        <button 
          @click="startDualBattle" 
          class="bg-indigo-600 hover:bg-indigo-700 text-white font-bold text-xs px-4 py-2 rounded-lg transition"
        >
          🔄 重新匹配 / 开始PK
        </button>
      </div>

      <!-- 分屏 1 vs 1 对比布局 (4K TV 大字卡片) -->
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        
        <!-- P1 选手卡片 (红/橙主题) -->
        <div 
          class="bg-slate-950 border-2 rounded-2xl p-6 transition-all duration-300 relative overflow-hidden"
          :class="winner === 'p1' ? 'border-amber-500 shadow-xl shadow-amber-500/20' : 'border-slate-800'"
        >
          <div v-if="winner === 'p1'" class="absolute top-4 right-4 bg-amber-500 text-slate-950 font-black text-xs px-3 py-1 rounded-full font-mono uppercase tracking-widest shadow-md">
            👑 本局获胜 (WINNER)
          </div>

          <div class="flex items-center gap-4 border-b border-slate-800 pb-4 mb-4">
            <div class="w-16 h-16 rounded-2xl bg-amber-500/20 border border-amber-500/40 flex items-center justify-center text-3xl">
              👴
            </div>
            <div>
              <h4 class="text-xl font-extrabold text-amber-400 font-mono">玩家一: 张爷爷</h4>
              <p class="text-xs text-slate-400 font-mono">设备: BLE-REHAB-01 (智能康复踏板)</p>
            </div>
          </div>

          <!-- 核心指标列 -->
          <div class="grid grid-cols-2 gap-4 font-mono">
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">实时得分</span>
              <span class="text-4xl font-black text-amber-400">{{ player1.score }}</span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">动作完成度</span>
              <span class="text-4xl font-black text-emerald-400">{{ player1.completion }}%</span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">踏板步频 (Cadence)</span>
              <span class="text-2xl font-bold text-slate-200">{{ player1.cadence }} <span class="text-xs text-slate-500">RPM</span></span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">下肢拉力 (Force)</span>
              <span class="text-2xl font-bold text-slate-200">{{ player1.force }} <span class="text-xs text-slate-500">N</span></span>
            </div>
          </div>
        </div>

        <!-- P2 选手卡片 (蓝/青主题) -->
        <div 
          class="bg-slate-950 border-2 rounded-2xl p-6 transition-all duration-300 relative overflow-hidden"
          :class="winner === 'p2' ? 'border-cyan-500 shadow-xl shadow-cyan-500/20' : 'border-slate-800'"
        >
          <div v-if="winner === 'p2'" class="absolute top-4 right-4 bg-cyan-500 text-slate-950 font-black text-xs px-3 py-1 rounded-full font-mono uppercase tracking-widest shadow-md">
            👑 本局获胜 (WINNER)
          </div>

          <div class="flex items-center gap-4 border-b border-slate-800 pb-4 mb-4">
            <div class="w-16 h-16 rounded-2xl bg-cyan-500/20 border border-cyan-500/40 flex items-center justify-center text-3xl">
              👵
            </div>
            <div>
              <h4 class="text-xl font-extrabold text-cyan-400 font-mono">玩家二: 李奶奶</h4>
              <p class="text-xs text-slate-400 font-mono">设备: BLE-REHAB-02 (下肢握力拉力计)</p>
            </div>
          </div>

          <!-- 核心指标列 -->
          <div class="grid grid-cols-2 gap-4 font-mono">
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">实时得分</span>
              <span class="text-4xl font-black text-cyan-400">{{ player2.score }}</span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">动作完成度</span>
              <span class="text-4xl font-black text-emerald-400">{{ player2.completion }}%</span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">踏板步频 (Cadence)</span>
              <span class="text-2xl font-bold text-slate-200">{{ player2.cadence }} <span class="text-xs text-slate-500">RPM</span></span>
            </div>
            <div class="bg-slate-900 border border-slate-800 p-4 rounded-xl text-center">
              <span class="text-xs text-slate-400 block uppercase">下肢拉力 (Force)</span>
              <span class="text-2xl font-bold text-slate-200">{{ player2.force }} <span class="text-xs text-slate-500">N</span></span>
            </div>
          </div>
        </div>

      </div>

      <!-- 实时比对柱状分析条 (4K 屏幕增强渲染) -->
      <div class="bg-slate-950 border border-slate-800 rounded-2xl p-6">
        <h4 class="text-lg font-bold text-slate-200 font-mono mb-4">📊 竞技动作标准度可视化对比</h4>
        
        <div class="space-y-4 font-mono text-sm">
          <div>
            <div class="flex justify-between mb-1">
              <span class="text-amber-400">张爷爷 ({{ player1.completion }}%)</span>
              <span class="text-slate-400">膝关节屈伸对称度</span>
              <span class="text-cyan-400">李奶奶 ({{ player2.completion }}%)</span>
            </div>
            <div class="h-4 bg-slate-900 rounded-full flex overflow-hidden border border-slate-800">
              <div class="bg-amber-500 transition-all duration-500" :style="{ width: `${(player1.completion / (player1.completion + player2.completion)) * 100}%` }"></div>
              <div class="bg-cyan-500 transition-all duration-500" :style="{ width: `${(player2.completion / (player1.completion + player2.completion)) * 100}%` }"></div>
            </div>
          </div>
        </div>
      </div>

    </div>

  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';

const activeMode = ref<'single' | 'dual'>('single');

const targetScore = 96;
const targetCompletion = 98;
const displayScore = ref(0);
const displayCompletion = ref(0);
const starsCount = ref(5);

const praiseMessage = ref('太棒了，老人家！您的康复训练完成度达到了 98%，步态动作极其标准，获得 5 颗金星奖励！');

const familyMessages = ref([
  {
    avatar: '👧',
    sender: '孙女 晓琳',
    time: '10:15',
    text: '爷爷加油！看到您的步态恢复得这么好，我们心里可高兴啦！晚上回家陪您看京剧！'
  },
  {
    avatar: '👨',
    sender: '儿子 伟国',
    time: '09:40',
    text: '爸，医生看诊说您最近肌肉力量提升了20%，太赞了！保持好心态！'
  }
]);

// 双人 PK 数据状态
const player1 = ref({
  score: 95,
  completion: 96,
  cadence: 115,
  force: 160.5
});

const player2 = ref({
  score: 92,
  completion: 94,
  cadence: 108,
  force: 148.0
});

const winner = ref<'p1' | 'p2' | 'tie'>('p1');

// 得分动画效果
const animateScore = () => {
  displayScore.value = 0;
  displayCompletion.value = 0;
  
  const scoreStep = Math.ceil(targetScore / 30);
  const compStep = Math.ceil(targetCompletion / 30);

  const timer = setInterval(() => {
    let done = true;
    if (displayScore.value < targetScore) {
      displayScore.value = Math.min(targetScore, displayScore.value + scoreStep);
      done = false;
    }
    if (displayCompletion.value < targetCompletion) {
      displayCompletion.value = Math.min(targetCompletion, displayCompletion.value + compStep);
      done = false;
    }
    if (done) {
      clearInterval(timer);
    }
  }, 30);
};

// 触发 TTS 朗读赞美语
const triggerTTSPraise = () => {
  speakText(praiseMessage.value);
};

const speakText = (text: string) => {
  if ('speechSynthesis' in window) {
    window.speechSynthesis.cancel(); // 停止当前正在播放的语音
    const utterance = new SpeechSynthesisUtterance(text);
    utterance.lang = 'zh-CN';
    utterance.rate = 0.9; // 适老化稍慢语速
    utterance.pitch = 1.1; // 较清亮亲切语调
    utterance.volume = 1.0;
    window.speechSynthesis.speak(utterance);
  } else {
    console.warn('Web Speech API is not supported in this browser.');
  }
};

const triggerConfetti = () => {
  animateScore();
  triggerTTSPraise();
};

let dualTimer: number | null = null;

const startDualBattle = () => {
  if (dualTimer !== null) clearInterval(dualTimer);

  dualTimer = window.setInterval(() => {
    // 动态微调双人实时比对数据
    player1.value.score = Math.min(100, Math.max(70, Math.round(92 + (Math.random() - 0.4) * 8)));
    player1.value.completion = Math.min(100, Math.max(70, Math.round(95 + (Math.random() - 0.4) * 5)));
    player1.value.cadence = Math.round(110 + (Math.random() - 0.5) * 10);
    player1.value.force = parseFloat((155.0 + (Math.random() - 0.5) * 15.0).toFixed(1));

    player2.value.score = Math.min(100, Math.max(70, Math.round(90 + (Math.random() - 0.4) * 8)));
    player2.value.completion = Math.min(100, Math.max(70, Math.round(93 + (Math.random() - 0.4) * 5)));
    player2.value.cadence = Math.round(106 + (Math.random() - 0.5) * 10);
    player2.value.force = parseFloat((145.0 + (Math.random() - 0.5) * 15.0).toFixed(1));

    if (player1.value.score > player2.value.score) {
      winner.value = 'p1';
    } else if (player2.value.score > player1.value.score) {
      winner.value = 'p2';
    } else {
      winner.value = 'tie';
    }
  }, 1000);
};

onMounted(() => {
  animateScore();
  startDualBattle();
});

onUnmounted(() => {
  if (dualTimer !== null) {
    clearInterval(dualTimer);
  }
  if ('speechSynthesis' in window) {
    window.speechSynthesis.cancel();
  }
});
</script>
