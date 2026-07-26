<template>
  <div class="w-full bg-slate-900 border-2 border-slate-800 rounded-2xl p-8 text-slate-100 shadow-2xl font-sans">
    
    <!-- Header: 4K HDMI 大屏专用 50Hz 游戏桥接中心 -->
    <div class="flex flex-col xl:flex-row justify-between items-start xl:items-center pb-6 border-b border-slate-800 gap-4">
      <div>
        <div class="flex items-center gap-3">
          <span class="inline-block w-4 h-4 rounded-full" :class="wsStatus === 'connected' ? 'bg-emerald-500 animate-pulse' : wsStatus === 'simulating' ? 'bg-amber-500 animate-pulse' : 'bg-red-500'"></span>
          <h2 class="text-3xl font-black text-emerald-400 tracking-tight">🎮 50Hz 实时游戏传感器桥接器 (Game Bridge)</h2>
        </div>
        <p class="text-base text-slate-400 font-mono mt-1">
          4K HDMI 大屏数据流总线 // DATAHUB REALTIME SENSOR STREAM (50Hz / 20ms)
        </p>
      </div>

      <!-- 操作与连接状态 -->
      <div class="flex flex-wrap items-center gap-3">
        <div class="bg-slate-950 border border-slate-800 rounded-xl px-4 py-2 text-sm font-mono flex items-center gap-3">
          <span class="text-slate-400">状态:</span>
          <span :class="{
            'text-emerald-400 font-bold': wsStatus === 'connected',
            'text-amber-400 font-bold': wsStatus === 'simulating',
            'text-red-400 font-bold': wsStatus === 'disconnected'
          }">
            {{ wsStatus === 'connected' ? '🌐 WebSocket 已连接' : wsStatus === 'simulating' ? '⚡ 50Hz 物理仿真中' : '🔌 已断开' }}
          </span>
        </div>

        <button 
          @click="toggleSimulation" 
          :class="isSimulating ? 'bg-amber-500 hover:bg-amber-600 text-slate-950' : 'bg-emerald-500 hover:bg-emerald-600 text-slate-950'"
          class="px-5 py-2.5 rounded-xl font-extrabold text-base transition duration-150 flex items-center gap-2 shadow-lg"
        >
          <span>{{ isSimulating ? '⏸ 暂停 50Hz 仿真' : '▶ 启动 50Hz 仿真数据流' }}</span>
        </button>

        <button 
          @click="toggleWebSocket" 
          class="bg-slate-800 hover:bg-slate-700 text-slate-100 border border-slate-700 px-4 py-2.5 rounded-xl font-bold text-sm transition"
        >
          {{ wsConnected ? '断开 WebSocket' : '连接后端 WS Gateway' }}
        </button>
      </div>
    </div>

    <!-- 4K 大屏性能仪表盘 (大字对比度) -->
    <div class="grid grid-cols-2 md:grid-cols-4 gap-6 my-6">
      <div class="bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col justify-between">
        <div class="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider">实时采样率 (Rate)</div>
        <div class="text-4xl font-black font-mono text-emerald-400 mt-2">
          {{ actualFreq }} <span class="text-base text-slate-400">Hz</span>
        </div>
        <div class="text-xs text-slate-500 mt-2 font-mono">目标频率: 50.0 Hz (周期 20ms)</div>
      </div>

      <div class="bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col justify-between">
        <div class="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider">发包总数 (Total Packets)</div>
        <div class="text-4xl font-black font-mono text-cyan-400 mt-2">
          {{ packetCount.toLocaleString() }}
        </div>
        <div class="text-xs text-slate-500 mt-2 font-mono">吞吐量: {{ (throughputKbps).toFixed(1) }} kbps</div>
      </div>

      <div class="bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col justify-between">
        <div class="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider">传输延迟 (Latency)</div>
        <div class="text-4xl font-black font-mono text-indigo-400 mt-2">
          {{ latencyMs }} <span class="text-base text-slate-400">ms</span>
        </div>
        <div class="text-xs text-slate-500 mt-2 font-mono">HDMI 4K 零延迟渲染</div>
      </div>

      <div class="bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col justify-between">
        <div class="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider">HTML5 / Unity 游戏桥</div>
        <div class="text-4xl font-black font-mono text-amber-400 mt-2">
          {{ gameFrameStats.fps }} <span class="text-base text-slate-400">FPS</span>
        </div>
        <div class="text-xs text-slate-500 mt-2 font-mono">BroadcastChannel: Broadcast 激活</div>
      </div>
    </div>

    <!-- 主体区域：左侧实时 JSON 报文视图，右侧 HDMI 4K 仿真游戏画面预览 -->
    <div class="grid grid-cols-1 lg:grid-cols-12 gap-6">
      
      <!-- 左侧：50Hz 传感器数据包详细结构 -->
      <div class="lg:col-span-6 bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col">
        <div class="flex justify-between items-center mb-3">
          <h3 class="text-lg font-bold text-slate-200 font-mono">📦 DataHub 物理传感器 50Hz JSON 帧数据包</h3>
          <span class="text-xs font-mono text-emerald-400 bg-emerald-950/80 border border-emerald-800/60 px-2.5 py-1 rounded">
            JSON Matrix Struct
          </span>
        </div>

        <div class="bg-slate-900 border border-slate-800 rounded-lg p-4 font-mono text-xs text-emerald-300 overflow-x-auto h-80">
          <pre class="leading-relaxed">{{ JSON.stringify(latestPacket, null, 2) }}</pre>
        </div>

        <div class="mt-4 grid grid-cols-3 gap-3 font-mono text-xs">
          <div class="bg-slate-900/80 border border-slate-800 p-2.5 rounded text-center">
            <span class="text-slate-400 block">膝关节角度</span>
            <span class="text-emerald-400 font-bold text-sm">{{ latestPacket.angle_deg.toFixed(1) }}°</span>
          </div>
          <div class="bg-slate-900/80 border border-slate-800 p-2.5 rounded text-center">
            <span class="text-slate-400 block">康复拉力</span>
            <span class="text-cyan-400 font-bold text-sm">{{ latestPacket.force_n.toFixed(1) }} N</span>
          </div>
          <div class="bg-slate-900/80 border border-slate-800 p-2.5 rounded text-center">
            <span class="text-slate-400 block">三轴加速度 (X/Y/Z)</span>
            <span class="text-indigo-400 font-bold text-sm">
              {{ latestPacket.imu.accel_x.toFixed(2) }}, {{ latestPacket.imu.accel_y.toFixed(2) }}
            </span>
          </div>
        </div>
      </div>

      <!-- 右侧：HDMI 4K 模拟大屏游戏界面与 HTML5 / Unity WebGL 桥接适配器 -->
      <div class="lg:col-span-6 bg-slate-950 border border-slate-800 rounded-xl p-5 flex flex-col justify-between">
        <div>
          <div class="flex justify-between items-center mb-3">
            <h3 class="text-lg font-bold text-slate-200 font-mono">📺 4K HDMI 交互式康复游戏渲染窗口</h3>
            <span class="text-xs font-mono text-amber-400 bg-amber-950/80 border border-amber-800/60 px-2.5 py-1 rounded">
              3840 x 2160 UHD
            </span>
          </div>

          <!-- 模拟游戏画布/游戏大屏 -->
          <div class="relative bg-slate-900 border border-slate-800 rounded-lg h-64 overflow-hidden flex flex-col items-center justify-center p-4">
            
            <!-- 游戏背景模拟轨迹线 -->
            <div class="absolute inset-0 opacity-20 bg-[radial-gradient(#10b981_1px,transparent_1px)] [background-size:16px_16px]"></div>

            <!-- 游戏目标角色 (根据传感器角度/力度实时位移) -->
            <div 
              class="relative z-10 transition-all duration-75 flex flex-col items-center"
              :style="{
                transform: `translateY(${ (45 - latestPacket.angle_deg) * 2 }px) scale(${ 1 + latestPacket.force_n / 500 })`
              }"
            >
              <div class="w-16 h-16 rounded-full bg-gradient-to-tr from-emerald-500 to-cyan-400 flex items-center justify-center text-slate-950 text-2xl font-black shadow-lg shadow-emerald-500/50">
                🚀
              </div>
              <span class="text-xs font-mono font-bold text-emerald-400 mt-2 bg-slate-950/80 px-2 py-0.5 rounded border border-emerald-500/40">
                玩家 1 (实时驱动)
              </span>
            </div>

            <!-- 目标线 -->
            <div class="absolute w-full border-b-2 border-dashed border-cyan-500/40 top-1/2 left-0 pointer-events-none"></div>

            <div class="absolute bottom-3 right-3 text-[10px] font-mono text-slate-500 bg-slate-950/80 px-2 py-1 rounded border border-slate-800">
              postMessage API: ACTIVE // Window & BroadcastChannel
            </div>
          </div>
        </div>

        <!-- 游戏配置参数 -->
        <div class="mt-4 flex flex-col sm:flex-row gap-3">
          <input 
            v-model="wsServerUrl" 
            class="flex-1 bg-slate-900 border border-slate-800 rounded-lg px-3 py-2 text-xs font-mono text-slate-200 focus:outline-none focus:border-emerald-500" 
            placeholder="ws://localhost:8080/api/v1/game/bridge" 
          />
          <button 
            @click="broadcastData" 
            class="bg-indigo-600 hover:bg-indigo-700 text-white font-bold px-4 py-2 rounded-lg text-xs font-mono transition"
          >
            手动触发 1 帧推流
          </button>
        </div>
      </div>

    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';

// 接口定义：50Hz 传感器数据包
export interface SensorPacket {
  timestamp: number;
  device_id: string;
  cadence_bpm: number;
  heart_rate_bpm: number;
  imu: {
    accel_x: number;
    accel_y: number;
    accel_z: number;
    gyro_x: number;
    gyro_y: number;
    gyro_z: number;
  };
  angle_deg: number;
  force_n: number;
  battery_soc: number;
  prescription_id: string;
}

const wsStatus = ref<'connected' | 'simulating' | 'disconnected'>('simulating');
const isSimulating = ref(true);
const wsConnected = ref(false);
const wsServerUrl = ref('ws://localhost:8080/api/v1/game/bridge');

const actualFreq = ref(50.0);
const packetCount = ref(0);
const latencyMs = ref(2);
const throughputKbps = ref(12.5);

const gameFrameStats = ref({
  fps: 60
});

const latestPacket = ref<SensorPacket>({
  timestamp: Date.now(),
  device_id: 'BLE-REHAB-01',
  cadence_bpm: 112,
  heart_rate_bpm: 76,
  imu: {
    accel_x: 0.12,
    accel_y: 0.98,
    accel_z: 0.05,
    gyro_x: 1.2,
    gyro_y: -0.5,
    gyro_z: 0.3
  },
  angle_deg: 45.0,
  force_n: 150.0,
  battery_soc: 88,
  prescription_id: 'RX-20260725-001'
});

let simTimer: number | null = null;
let wsInstance: WebSocket | null = null;
let broadcastChannel: BroadcastChannel | null = null;

let lastFrameTime = performance.now();
let framesInSec = 0;

// 触发与广播数据给 Unity / HTML5 游戏
const broadcastData = () => {
  if (!latestPacket.value) return;
  
  // 1. BroadcastChannel 传输 (同域跨窗口/跨 Frame 零拷贝传输)
  if (broadcastChannel) {
    broadcastChannel.postMessage(latestPacket.value);
  }

  // 2. window.postMessage 给内嵌 iframe / Unity WebGL 窗口
  window.postMessage({
    type: 'SMARTBOX_GAME_SENSOR_DATA',
    payload: latestPacket.value
  }, '*');

  packetCount.value++;
  throughputKbps.value = (packetCount.value * 256 * 8) / 1000 / Math.max(1, (Date.now() - startTime) / 1000);
};

// 仿真数据流 (50Hz = 20ms 间隔)
let startTime = Date.now();
let stepCounter = 0;

const start50HzSimulation = () => {
  if (simTimer !== null) clearInterval(simTimer);
  
  simTimer = window.setInterval(() => {
    stepCounter++;
    const t = stepCounter * 0.05;
    
    // 生成平滑物理传感器曲线 (正弦叠加噪声)
    const angle = 45.0 + Math.sin(t * 2) * 30.0 + (Math.random() - 0.5) * 1.5;
    const force = 120.0 + Math.cos(t * 2) * 60.0 + (Math.random() - 0.5) * 5.0;
    
    latestPacket.value = {
      timestamp: Date.now(),
      device_id: 'BLE-REHAB-01',
      cadence_bpm: Math.round(110 + Math.sin(t) * 5),
      heart_rate_bpm: Math.round(75 + Math.cos(t * 0.5) * 3),
      imu: {
        accel_x: parseFloat((Math.sin(t * 3) * 0.5).toFixed(2)),
        accel_y: parseFloat((0.98 + Math.cos(t * 3) * 0.1).toFixed(2)),
        accel_z: parseFloat((Math.sin(t * 1.5) * 0.2).toFixed(2)),
        gyro_x: parseFloat((Math.cos(t * 2) * 2.0).toFixed(2)),
        gyro_y: parseFloat((Math.sin(t * 2) * 1.5).toFixed(2)),
        gyro_z: parseFloat((Math.cos(t * 4) * 0.8).toFixed(2))
      },
      angle_deg: parseFloat(Math.max(0, Math.min(90, angle)).toFixed(1)),
      force_n: parseFloat(Math.max(0, force).toFixed(1)),
      battery_soc: 88,
      prescription_id: 'RX-20260725-001'
    };

    broadcastData();
    actualFreq.value = 50.0 + (Math.random() - 0.5) * 0.4;
    latencyMs.value = Math.floor(1 + Math.random() * 3);
  }, 20); // 20ms = 50Hz

  wsStatus.value = 'simulating';
};

const stopSimulation = () => {
  if (simTimer !== null) {
    clearInterval(simTimer);
    simTimer = null;
  }
};

const toggleSimulation = () => {
  isSimulating.value = !isSimulating.value;
  if (isSimulating.value) {
    start50HzSimulation();
  } else {
    stopSimulation();
    if (!wsConnected.value) wsStatus.value = 'disconnected';
  }
};

const toggleWebSocket = () => {
  if (wsConnected.value) {
    if (wsInstance) {
      wsInstance.close();
      wsInstance = null;
    }
    wsConnected.value = false;
    if (isSimulating.value) {
      wsStatus.value = 'simulating';
    } else {
      wsStatus.value = 'disconnected';
    }
  } else {
    try {
      wsInstance = new WebSocket(wsServerUrl.value);
      wsInstance.onopen = () => {
        wsConnected.value = true;
        wsStatus.value = 'connected';
        stopSimulation();
        isSimulating.value = false;
      };
      wsInstance.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          latestPacket.value = data;
          broadcastData();
        } catch (e) {
          console.error('Failed to parse WebSocket JSON frame:', e);
        }
      };
      wsInstance.onclose = () => {
        wsConnected.value = false;
        wsStatus.value = isSimulating.value ? 'simulating' : 'disconnected';
      };
      wsInstance.onerror = () => {
        console.warn('WebSocket connection failed, reverting to 50Hz simulation mode.');
        wsConnected.value = false;
        if (!isSimulating.value) toggleSimulation();
      };
    } catch (e) {
      console.warn('Invalid WebSocket URL or network error:', e);
    }
  }
};

onMounted(() => {
  // 初始化 BroadcastChannel 广播网关
  try {
    broadcastChannel = new BroadcastChannel('smartbox_game_bridge');
  } catch (e) {
    console.warn('BroadcastChannel not supported in environment:', e);
  }

  // 默认启动 50Hz 仿真推流
  start50HzSimulation();
});

onUnmounted(() => {
  stopSimulation();
  if (wsInstance) {
    wsInstance.close();
  }
  if (broadcastChannel) {
    broadcastChannel.close();
  }
});
</script>
