<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-cyan-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-emerald-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-cyan-400 animate-pulse"></span>
            DCU Southbound Telemetry Sniffer
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight font-mono">
            DL/T 645 & 698 <span class="text-cyan-400 font-sans">PROTOCOL SNIFFER</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            Real-time Hex Dump frame parsing, A-XDR object tree inspector, and RS485/HPLC bus stream diagnostics.
          </p>
        </div>

        <div class="flex flex-wrap items-center gap-3 font-mono text-xs">
          <button 
            @click="toggleCapture" 
            :class="isCapturing ? 'bg-cyan-500 hover:bg-cyan-600 text-slate-950 font-black' : 'bg-slate-800 hover:bg-slate-700 text-slate-300'"
            class="px-4 py-2.5 rounded-2xl border border-slate-700 transition flex items-center gap-2 shadow-lg"
          >
            <span class="w-2 h-2 rounded-full" :class="isCapturing ? 'bg-slate-950 animate-ping' : 'bg-slate-500'"></span>
            <span>{{ isCapturing ? '⏸ PAUSE SNIFFER' : '▶ RESUME SNIFFER' }}</span>
          </button>
          <button 
            @click="clearPackets" 
            class="px-4 py-2.5 bg-slate-900 hover:bg-slate-800 text-slate-400 hover:text-slate-200 border border-slate-800 rounded-2xl transition"
          >
            🗑️ 清空报文
          </button>
        </div>
      </div>
    </section>

    <!-- Filter & Statistics Bar -->
    <div class="bg-slate-900/90 border border-slate-800 p-4 rounded-2xl flex flex-wrap items-center justify-between gap-4 font-mono text-xs">
      <div class="flex flex-wrap items-center gap-3">
        <!-- Protocol Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Protocol:</span>
          <select v-model="filterProtocol" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer">
            <option value="ALL">ALL (全部规约)</option>
            <option value="698">DL/T 698.45-2017 (A-XDR)</option>
            <option value="645">DL/T 645-2007 (HEX)</option>
          </select>
        </div>

        <!-- Direction Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Direction:</span>
          <select v-model="filterDirection" class="bg-transparent text-slate-200 focus:outline-none cursor-pointer">
            <option value="ALL">ALL (全部方向)</option>
            <option value="TX">TX (主叫请求)</option>
            <option value="RX">RX (从站响应)</option>
          </select>
        </div>

        <!-- Address Search -->
        <div class="relative">
          <input 
            v-model="searchAddress"
            type="text" 
            placeholder="Filter Meter Address..." 
            class="bg-slate-950 border border-slate-800 rounded-xl px-3 py-1.5 text-xs text-slate-200 placeholder-slate-600 focus:outline-none focus:border-cyan-500 w-44"
          />
        </div>
      </div>

      <div class="flex items-center gap-4 text-slate-400 text-[11px]">
        <span>Captured: <strong class="text-cyan-400">{{ packets.length }}</strong> pkts</span>
        <span>DLT698: <strong class="text-emerald-400">{{ count698 }}</strong></span>
        <span>DLT645: <strong class="text-amber-400">{{ count645 }}</strong></span>
      </div>
    </div>

    <!-- Packet Stream Table -->
    <div class="bg-slate-900/90 border border-slate-800/90 rounded-3xl overflow-hidden shadow-2xl">
      <div class="overflow-x-auto">
        <table class="w-full text-left font-mono text-xs">
          <thead>
            <tr class="bg-slate-950 border-b border-slate-800 text-slate-400 uppercase text-[10px] tracking-wider">
              <th class="p-4 w-16">#</th>
              <th class="p-4 w-28">Timestamp</th>
              <th class="p-4 w-20">Dir</th>
              <th class="p-4 w-28">Protocol</th>
              <th class="p-4 w-36">Meter Address</th>
              <th class="p-4 w-44">Function / OAD</th>
              <th class="p-4">Raw Hex Stream (Colorized)</th>
              <th class="p-4 w-20 text-center">Action</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-800/60">
            <tr 
              v-for="(pkt, idx) in filteredPackets" 
              :key="pkt.id"
              @click="selectPacket(pkt)"
              class="hover:bg-slate-800/50 cursor-pointer transition-colors group"
              :class="selectedPacket?.id === pkt.id ? 'bg-cyan-950/30 border-l-4 border-l-cyan-400' : ''"
            >
              <td class="p-4 text-slate-600">{{ idx + 1 }}</td>
              <td class="p-4 text-slate-400 whitespace-nowrap">{{ pkt.timestamp }}</td>
              <td class="p-4 whitespace-nowrap">
                <span 
                  :class="[
                    'px-2 py-0.5 rounded text-[10px] font-bold border',
                    pkt.direction === 'TX' ? 'bg-sky-500/10 border-sky-500/30 text-sky-400' : 'bg-emerald-500/10 border-emerald-500/30 text-emerald-400'
                  ]"
                >
                  {{ pkt.direction }}
                </span>
              </td>
              <td class="p-4 whitespace-nowrap">
                <span :class="pkt.protocol === '698' ? 'text-emerald-400 font-bold' : 'text-amber-400 font-bold'">
                  {{ pkt.protocol === '698' ? 'DL/T 698.45' : 'DL/T 645-07' }}
                </span>
              </td>
              <td class="p-4 text-slate-300 font-bold whitespace-nowrap">{{ pkt.address }}</td>
              <td class="p-4 whitespace-nowrap">
                <span class="text-cyan-300">{{ pkt.functionName }}</span>
              </td>
              <td class="p-4 font-mono text-[11px] leading-relaxed break-all">
                <span 
                  v-for="(chunk, cIdx) in pkt.chunks" 
                  :key="cIdx"
                  :class="chunk.colorClass"
                  class="mr-1 inline-block"
                  :title="chunk.role"
                >
                  {{ chunk.hex }}
                </span>
              </td>
              <td class="p-4 text-center">
                <button 
                  @click.stop="selectPacket(pkt)"
                  class="text-[10px] bg-slate-800 group-hover:bg-cyan-500 group-hover:text-slate-950 text-slate-300 px-2.5 py-1 rounded-lg transition"
                >
                  解析
                </button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Packet Inspector Modal / Detail Drawer -->
    <div v-if="selectedPacket" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-sm animate-in fade-in duration-200">
      <div class="bg-slate-900 border border-slate-700 rounded-3xl p-6 lg:p-8 max-w-2xl w-full shadow-2xl relative font-mono text-xs">
        <div class="flex justify-between items-start mb-6 border-b border-slate-800 pb-4">
          <div>
            <div class="text-[10px] text-cyan-400 uppercase font-bold tracking-widest">
              {{ selectedPacket.protocol === '698' ? 'A-XDR DLT698 APDU TREE' : 'DLT645-2007 FRAME INSPECTOR' }}
            </div>
            <h2 class="text-xl font-black text-white mt-1">
              Frame #{{ selectedPacket.id }} Details
            </h2>
          </div>
          <button @click="selectedPacket = null" class="text-slate-400 hover:text-white text-base">✕</button>
        </div>

        <!-- Frame Breakdown Tree -->
        <div class="space-y-4 max-h-[60vh] overflow-y-auto pr-2">
          <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-2">
            <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
              <span class="text-slate-500">Meter Address:</span>
              <span class="text-slate-200 font-bold">{{ selectedPacket.address }}</span>
            </div>
            <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
              <span class="text-slate-500">APDU Service:</span>
              <span class="text-emerald-400 font-bold">{{ selectedPacket.functionName }}</span>
            </div>
            <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
              <span class="text-slate-500">OAD / Data Identifier:</span>
              <span class="text-amber-400 font-bold">{{ selectedPacket.oadCode }}</span>
            </div>
            <div class="flex justify-between border-b border-slate-800/60 pb-1.5">
              <span class="text-slate-500">Decoded Physical Value:</span>
              <span class="text-cyan-300 font-bold text-sm">{{ selectedPacket.decodedValue }}</span>
            </div>
            <div class="flex justify-between">
              <span class="text-slate-500">Frame Check Sequence:</span>
              <span class="text-emerald-400">FCS VALID (0x{{ selectedPacket.fcs }}) ✓</span>
            </div>
          </div>

          <!-- Raw Stream Breakdown -->
          <div>
            <div class="text-slate-400 text-[10px] font-bold uppercase mb-2">Color-Coded Hex Breakdown</div>
            <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 text-xs leading-relaxed space-y-2">
              <div v-for="(chunk, idx) in selectedPacket.chunks" :key="idx" class="flex justify-between items-center text-[11px]">
                <span class="text-slate-500 w-36">{{ chunk.role }}:</span>
                <span :class="chunk.colorClass" class="font-bold">{{ chunk.hex }}</span>
              </div>
            </div>
          </div>
        </div>

        <div class="mt-6 pt-4 border-t border-slate-800 text-right">
          <button @click="selectedPacket = null" class="px-5 py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 font-bold">
            关闭
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue';

interface HexChunk {
  hex: string;
  role: string;
  colorClass: string;
}

interface Packet {
  id: number;
  timestamp: string;
  direction: 'TX' | 'RX';
  protocol: '698' | '645';
  address: string;
  functionName: string;
  oadCode: string;
  decodedValue: string;
  fcs: string;
  chunks: HexChunk[];
}

const isCapturing = ref(true);
const filterProtocol = ref('ALL');
const filterDirection = ref('ALL');
const searchAddress = ref('');
const selectedPacket = ref<Packet | null>(null);
let streamTimer: any = null;

const packets = ref<Packet[]>([
  {
    id: 101,
    timestamp: '16:08:12.450',
    direction: 'TX',
    protocol: '698',
    address: '000000000001',
    functionName: 'GET_REQUEST_NORMAL (0x0501)',
    oadCode: '0x20000200 (Phase A Voltage)',
    decodedValue: 'Requesting U_a',
    fcs: 'A2B4',
    chunks: [
      { hex: '68', role: 'Header', colorClass: 'text-cyan-400 font-bold' },
      { hex: '1D 00', role: 'Length', colorClass: 'text-sky-300' },
      { hex: '43', role: 'Ctrl', colorClass: 'text-purple-400' },
      { hex: '05 01 00 00 00 00 00 01', role: 'Address', colorClass: 'text-emerald-400' },
      { hex: '10', role: 'CA', colorClass: 'text-slate-400' },
      { hex: '05 01 01', role: 'APDU GetReq', colorClass: 'text-amber-400' },
      { hex: '20 00 02 00', role: 'OAD (Voltage)', colorClass: 'text-rose-400 font-bold' },
      { hex: '00', role: 'TimeTag', colorClass: 'text-slate-400' },
      { hex: 'A2 B4', role: 'FCS', colorClass: 'text-blue-400 font-bold' },
      { hex: '16', role: 'End', colorClass: 'text-cyan-400 font-bold' }
    ]
  },
  {
    id: 102,
    timestamp: '16:08:12.512',
    direction: 'RX',
    protocol: '698',
    address: '000000000001',
    functionName: 'GET_RESPONSE_NORMAL (0x8501)',
    oadCode: '0x20000200 (Phase A Voltage)',
    decodedValue: '220.4 V (Scale 0.1)',
    fcs: '34DF',
    chunks: [
      { hex: '68', role: 'Header', colorClass: 'text-cyan-400 font-bold' },
      { hex: '22 00', role: 'Length', colorClass: 'text-sky-300' },
      { hex: '83', role: 'Ctrl', colorClass: 'text-purple-400' },
      { hex: '05 01 00 00 00 00 00 01', role: 'Address', colorClass: 'text-emerald-400' },
      { hex: '10', role: 'CA', colorClass: 'text-slate-400' },
      { hex: '85 01 01', role: 'APDU GetResp', colorClass: 'text-amber-400' },
      { hex: '20 00 02 00 01', role: 'OAD Result', colorClass: 'text-rose-400' },
      { hex: '12 08 9C', role: 'Data (2204 -> 220.4V)', colorClass: 'text-white font-bold bg-slate-800 px-1 rounded' },
      { hex: '34 DF', role: 'FCS', colorClass: 'text-blue-400 font-bold' },
      { hex: '16', role: 'End', colorClass: 'text-cyan-400 font-bold' }
    ]
  },
  {
    id: 103,
    timestamp: '16:08:13.120',
    direction: 'TX',
    protocol: '645',
    address: '123456789012',
    functionName: 'READ_DATA (0x11)',
    oadCode: 'DI 0x00010000 (Active Energy)',
    decodedValue: 'Read Positive Active kWh',
    fcs: '3A',
    chunks: [
      { hex: '68', role: 'Header', colorClass: 'text-cyan-400 font-bold' },
      { hex: '12 90 78 56 34 12', role: 'Address (BCD)', colorClass: 'text-emerald-400' },
      { hex: '68', role: 'Header2', colorClass: 'text-cyan-400 font-bold' },
      { hex: '11', role: 'Ctrl (Read)', colorClass: 'text-purple-400' },
      { hex: '04', role: 'Length', colorClass: 'text-sky-300' },
      { hex: '33 33 34 33', role: 'DI (+0x33)', colorClass: 'text-amber-400 font-bold' },
      { hex: '3A', role: 'CS', colorClass: 'text-blue-400 font-bold' },
      { hex: '16', role: 'End', colorClass: 'text-cyan-400 font-bold' }
    ]
  }
]);

const count698 = computed(() => packets.value.filter(p => p.protocol === '698').length);
const count645 = computed(() => packets.value.filter(p => p.protocol === '645').length);

const filteredPackets = computed(() => {
  return packets.value.filter(p => {
    if (filterProtocol.value !== 'ALL' && p.protocol !== filterProtocol.value) return false;
    if (filterDirection.value !== 'ALL' && p.direction !== filterDirection.value) return false;
    if (searchAddress.value && !p.address.includes(searchAddress.value)) return false;
    return true;
  });
});

function toggleCapture() {
  isCapturing.value = !isCapturing.value;
}

function clearPackets() {
  packets.value = [];
  selectedPacket.value = null;
}

function selectPacket(pkt: Packet) {
  selectedPacket.value = pkt;
}

function generateMockPacket() {
  if (!isCapturing.value) return;
  const now = new Date();
  const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}.${String(now.getMilliseconds()).padStart(3, '0')}`;
  
  const is698 = Math.random() > 0.4;
  const isTx = Math.random() > 0.5;
  const id = Date.now() % 10000;
  
  if (is698) {
    const voltage = (219.5 + Math.random() * 2).toFixed(1);
    packets.value.unshift({
      id,
      timestamp: timeStr,
      direction: isTx ? 'TX' : 'RX',
      protocol: '698',
      address: '000000000002',
      functionName: isTx ? 'GET_REQUEST_NORMAL (0x0501)' : 'GET_RESPONSE_NORMAL (0x8501)',
      oadCode: '0x20000200 (Phase A Voltage)',
      decodedValue: isTx ? 'Requesting U_a' : `${voltage} V`,
      fcs: (Math.random() * 0xFFFF).toString(16).toUpperCase().padStart(4, '0'),
      chunks: [
        { hex: '68', role: 'Header', colorClass: 'text-cyan-400 font-bold' },
        { hex: '22 00', role: 'Length', colorClass: 'text-sky-300' },
        { hex: isTx ? '43' : '83', role: 'Ctrl', colorClass: 'text-purple-400' },
        { hex: '05 01 00 00 00 00 00 02', role: 'Address', colorClass: 'text-emerald-400' },
        { hex: '10', role: 'CA', colorClass: 'text-slate-400' },
        { hex: isTx ? '05 01 01' : '85 01 01', role: 'APDU', colorClass: 'text-amber-400' },
        { hex: '20 00 02 00', role: 'OAD', colorClass: 'text-rose-400 font-bold' },
        { hex: '9C', role: 'Payload', colorClass: 'text-white' },
        { hex: '16', role: 'End', colorClass: 'text-cyan-400 font-bold' }
      ]
    });
  } else {
    packets.value.unshift({
      id,
      timestamp: timeStr,
      direction: isTx ? 'TX' : 'RX',
      protocol: '645',
      address: '987654321098',
      functionName: isTx ? 'READ_DATA (0x11)' : 'READ_DATA_RESP (0x91)',
      oadCode: 'DI 0x00010000 (Active Energy)',
      decodedValue: isTx ? 'Query Active kWh' : '14285.32 kWh',
      fcs: (Math.random() * 0xFF).toString(16).toUpperCase().padStart(2, '0'),
      chunks: [
        { hex: '68', role: 'Header', colorClass: 'text-cyan-400 font-bold' },
        { hex: '98 10 32 54 76 98', role: 'Address', colorClass: 'text-emerald-400' },
        { hex: '68', role: 'Header2', colorClass: 'text-cyan-400 font-bold' },
        { hex: isTx ? '11' : '91', role: 'Ctrl', colorClass: 'text-purple-400' },
        { hex: '04', role: 'Length', colorClass: 'text-sky-300' },
        { hex: '33 33 34 33', role: 'DI (+0x33)', colorClass: 'text-amber-400 font-bold' },
        { hex: '16', role: 'End', colorClass: 'text-cyan-400 font-bold' }
      ]
    });
  }

  if (packets.value.length > 50) {
    packets.value.pop();
  }
}

onMounted(() => {
  streamTimer = setInterval(generateMockPacket, 2000);
});

onBeforeUnmount(() => {
  if (streamTimer) {
    clearInterval(streamTimer);
    streamTimer = null;
  }
});
</script>
