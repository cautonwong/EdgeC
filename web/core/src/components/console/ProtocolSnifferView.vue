<template>
  <div class="h-full flex flex-col bg-[#050b14] text-[#f8fafc] font-mono text-xs overflow-hidden select-text">
    <!-- Top Stream Controls & Statistics Bar -->
    <header class="px-4 py-2.5 bg-slate-950 border-b border-slate-850 flex flex-wrap items-center justify-between gap-3 shrink-0 text-xs">
      <!-- Left: Status & Counters -->
      <div class="flex items-center gap-3">
        <div class="flex items-center gap-2">
          <span 
            class="w-2 h-2 rounded-full" 
            :class="isStreaming ? 'bg-emerald-400 animate-pulse' : 'bg-amber-400'"
          ></span>
          <span class="font-bold text-white">
            {{ isStreaming ? 'Live Sniffer (50Hz)' : 'Capture Paused' }}
          </span>
        </div>
        <div class="hidden sm:flex items-center gap-2 text-slate-500 text-[11px] border-l border-slate-800 pl-3">
          <span>Frames: <strong class="text-slate-300">{{ totalFrames }}</strong></span>
          <span>Filtered: <strong class="text-cyan-400">{{ filteredPackets.length }}</strong></span>
          <span>Rate: <strong class="text-emerald-400">{{ throughputKbps.toFixed(1) }} KB/s</strong></span>
        </div>
      </div>

      <!-- Right: Action Buttons (Pause, Scroll, Clear, Export) -->
      <div class="flex items-center gap-2">
        <!-- Pause / Resume -->
        <button 
          @click="toggleStreaming" 
          class="px-2.5 py-1 rounded-lg border text-xs font-bold transition cursor-pointer flex items-center gap-1.5"
          :class="isStreaming ? 'bg-amber-500/10 border-amber-500/30 text-amber-400 hover:bg-amber-500/20' : 'bg-emerald-500/10 border-emerald-500/30 text-emerald-400 hover:bg-emerald-500/20'"
        >
          <span>{{ isStreaming ? '⏸ Pause' : '▶ Resume' }}</span>
        </button>

        <!-- Auto-scroll Toggle -->
        <button 
          @click="autoScroll = !autoScroll" 
          class="px-2.5 py-1 rounded-lg border text-xs transition cursor-pointer"
          :class="autoScroll ? 'bg-indigo-500/20 border-indigo-500/40 text-indigo-300' : 'bg-slate-900 border-slate-800 text-slate-400'"
          title="Toggle Auto-Scroll"
        >
          Auto-Scroll: {{ autoScroll ? 'ON' : 'OFF' }}
        </button>

        <!-- Clear -->
        <button 
          @click="clearPackets" 
          class="px-2.5 py-1 rounded-lg bg-slate-900 hover:bg-slate-800 border border-slate-800 text-slate-400 hover:text-slate-200 transition cursor-pointer text-xs"
        >
          Clear
        </button>

        <!-- Export Dropdown / Buttons -->
        <div class="flex items-center gap-1 border-l border-slate-800 pl-2">
          <button 
            @click="exportJSON" 
            class="px-2.5 py-1 rounded-lg bg-slate-900 hover:bg-cyan-950/60 border border-slate-800 hover:border-cyan-500/40 text-cyan-400 transition cursor-pointer text-xs"
            title="Export captured packets as JSON"
          >
            Export JSON
          </button>
          <button 
            @click="exportPCAP" 
            class="px-2.5 py-1 rounded-lg bg-slate-900 hover:bg-purple-950/60 border border-slate-800 hover:border-purple-500/40 text-purple-400 transition cursor-pointer text-xs"
            title="Export captured packets as standard Libpcap binary format"
          >
            Export PCAP
          </button>
        </div>
      </div>
    </header>

    <!-- Filter & Search Toolbar -->
    <div class="px-4 py-2 bg-slate-950/70 border-b border-slate-900 flex flex-wrap items-center justify-between gap-3 shrink-0 text-xs">
      <div class="flex flex-wrap items-center gap-3">
        <!-- Physical Channel Filter -->
        <div class="flex items-center gap-1.5">
          <span class="text-slate-500 text-[11px]">Channel:</span>
          <select v-model="selectedChannel" class="bg-slate-900 border border-slate-800 text-slate-200 rounded px-2 py-1 text-xs">
            <option value="ALL">ALL Channels</option>
            <option value="RS485-0">RS485-0 / COM1</option>
            <option value="RS485-1">RS485-1 / COM2</option>
            <option value="HPLC-Mesh">HPLC Mesh</option>
            <option value="TCP-502">TCP / Ethernet</option>
          </select>
        </div>

        <!-- Protocol Filter -->
        <div class="flex items-center gap-1.5">
          <span class="text-slate-500 text-[11px]">Protocol:</span>
          <select v-model="selectedProtocol" class="bg-slate-900 border border-slate-800 text-slate-200 rounded px-2 py-1 text-xs">
            <option value="ALL">ALL Protocols</option>
            <option value="DLT645-2007">DL/T 645-2007</option>
            <option value="DLT698.45">DL/T 698.45</option>
            <option value="Modbus-RTU">Modbus-RTU</option>
            <option value="Cyclone DDS">Cyclone DDS</option>
          </select>
        </div>

        <!-- Direction Filter -->
        <div class="flex items-center gap-1.5">
          <span class="text-slate-500 text-[11px]">Dir:</span>
          <select v-model="selectedDirection" class="bg-slate-900 border border-slate-800 text-slate-200 rounded px-2 py-1 text-xs">
            <option value="ALL">ALL (TX+RX)</option>
            <option value="TX">TX (Transmit)</option>
            <option value="RX">RX (Receive)</option>
          </select>
        </div>
      </div>

      <!-- Keyword Search -->
      <div class="flex items-center gap-2">
        <input 
          v-model="searchKeyword" 
          placeholder="Filter hex, summary, tag..." 
          class="bg-slate-900 border border-slate-800 text-white placeholder-slate-600 rounded-lg px-2.5 py-1 text-xs focus:outline-none focus:border-cyan-500 w-48 sm:w-64"
        />
      </div>
    </div>

    <!-- Main Content Area: Packet Stream Table + Detail Inspector -->
    <div class="flex-1 min-h-0 flex flex-col lg:flex-row overflow-hidden">
      <!-- Left / Top: Packet Stream Table List -->
      <div 
        ref="packetTableRef"
        class="flex-1 overflow-y-auto scrollbar-thin scrollbar-thumb-slate-800 scrollbar-track-transparent border-b lg:border-b-0 lg:border-r border-slate-850"
      >
        <table class="w-full text-left border-collapse text-xs">
          <thead class="sticky top-0 bg-[#070e1b] border-b border-slate-800 text-[10px] text-slate-500 font-bold uppercase tracking-wider select-none z-10">
            <tr>
              <th class="py-2 px-3 w-12 text-center">#</th>
              <th class="py-2 px-3 w-28">MTT Time</th>
              <th class="py-2 px-2 w-16 text-center">Dir</th>
              <th class="py-2 px-2 w-24">Channel</th>
              <th class="py-2 px-2 w-28">Protocol</th>
              <th class="py-2 px-2 w-16 text-center">Len</th>
              <th class="py-2 px-3">Decoded Summary & Hex Payload</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-900 font-mono">
            <tr 
              v-for="(pkt, idx) in filteredPackets" 
              :key="pkt.id || idx"
              @click="selectPacket(pkt)"
              class="cursor-pointer transition hover:bg-slate-900/60"
              :class="selectedPacket?.id === pkt.id ? 'bg-cyan-950/40 border-l-2 border-cyan-400' : ''"
            >
              <!-- Index -->
              <td class="py-2 px-3 text-slate-600 text-center text-[10px]">{{ idx + 1 }}</td>
              <!-- MTT Time -->
              <td class="py-2 px-3 text-slate-400 text-[11px] whitespace-nowrap">{{ pkt.time }}</td>
              <!-- Direction -->
              <td class="py-2 px-2 text-center">
                <span 
                  class="px-1.5 py-0.5 rounded text-[9px] font-black uppercase"
                  :class="pkt.direction === 'TX' ? 'bg-blue-500/20 text-blue-400 border border-blue-500/30' : 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30'"
                >
                  {{ pkt.direction }}
                </span>
              </td>
              <!-- Channel -->
              <td class="py-2 px-2 text-slate-400 text-[11px] whitespace-nowrap">{{ pkt.channel }}</td>
              <!-- Protocol -->
              <td class="py-2 px-2 whitespace-nowrap">
                <span class="text-cyan-400 font-bold text-[11px]">{{ pkt.protocol }}</span>
              </td>
              <!-- Length -->
              <td class="py-2 px-2 text-center text-slate-500 text-[11px]">{{ pkt.rawBytes.length }}B</td>
              <!-- Summary & Hex -->
              <td class="py-2 px-3">
                <div class="font-bold text-slate-200 text-[11px] flex items-center gap-2">
                  <span>{{ pkt.decodedSummary }}</span>
                  <span 
                    v-if="pkt.isValidChecksum" 
                    class="text-[9px] px-1 rounded bg-emerald-500/10 text-emerald-400 border border-emerald-500/20"
                  >
                    CRC OK
                  </span>
                  <span 
                    v-else 
                    class="text-[9px] px-1 rounded bg-rose-500/10 text-rose-400 border border-rose-500/20"
                  >
                    CRC ERR
                  </span>
                </div>
                <div class="text-[10px] text-amber-400/80 tracking-wider truncate max-w-xl font-mono pt-0.5">
                  {{ pkt.hexString }}
                </div>
              </td>
            </tr>

            <!-- Empty State -->
            <tr v-if="filteredPackets.length === 0">
              <td colspan="7" class="py-12 text-center text-slate-600 space-y-2">
                <div>📡 No protocol frames captured matching the current filters.</div>
                <div class="text-[11px] text-slate-700">Check physical channel connections or resume the capture stream.</div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <!-- Right / Bottom: Selected Frame Dissection & Hex Inspector Drawer -->
      <div 
        v-if="selectedPacket" 
        class="w-full lg:w-[480px] xl:w-[560px] bg-[#050b14] flex flex-col shrink-0 overflow-y-auto p-4 space-y-4 border-t lg:border-t-0 scrollbar-thin"
      >
        <!-- Dissection Overview Card -->
        <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-3">
          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
              <span class="text-cyan-400 font-bold text-sm">{{ selectedPacket.protocol }} Dissection</span>
              <span 
                class="px-1.5 py-0.5 rounded text-[10px] font-bold uppercase"
                :class="selectedPacket.direction === 'TX' ? 'bg-blue-500/20 text-blue-400' : 'bg-emerald-500/20 text-emerald-400'"
              >
                {{ selectedPacket.direction }}
              </span>
            </div>
            <button 
              @click="selectedPacket = null" 
              class="w-6 h-6 rounded-lg bg-slate-900 hover:bg-slate-800 text-slate-400 text-xs flex items-center justify-center cursor-pointer"
            >
              ✕
            </button>
          </div>

          <div class="grid grid-cols-2 gap-2 text-[11px] text-slate-400 pt-1 border-t border-slate-900">
            <div>Channel: <strong class="text-white">{{ selectedPacket.channel }}</strong></div>
            <div>Length: <strong class="text-white">{{ selectedPacket.rawBytes.length }} Bytes</strong></div>
            <div>Timestamp: <strong class="text-white">{{ selectedPacket.time }}</strong></div>
            <div>Checksum: <strong :class="selectedPacket.isValidChecksum ? 'text-emerald-400' : 'text-rose-400'">{{ selectedPacket.isValidChecksum ? 'Valid (Pass)' : 'Invalid (Failed)' }}</strong></div>
          </div>

          <div class="p-2.5 rounded-xl bg-slate-900/90 border border-slate-850 text-xs text-slate-200">
            <span class="text-slate-500 font-bold mr-1">Decoded Summary:</span>
            <span>{{ selectedPacket.decodedSummary }}</span>
          </div>
        </div>

        <!-- Hex + ASCII Inspector Component -->
        <HexPayloadViewer 
          :raw-bytes="selectedPacket.rawBytes" 
          :segments="selectedPacket.segments" 
        />

        <!-- Protocol Fields Tree / Segments List -->
        <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-3">
          <div class="text-xs font-bold text-slate-300">Protocol Dissected Fields</div>
          <div class="space-y-1.5">
            <div 
              v-for="(seg, sIdx) in selectedPacket.segments" 
              :key="sIdx"
              class="p-2 rounded-xl bg-slate-900/60 border border-slate-850 flex items-center justify-between text-[11px]"
            >
              <div class="space-y-0.5">
                <div class="font-bold text-slate-200 flex items-center gap-2">
                  <span class="w-1.5 h-1.5 rounded-full" :class="getSegmentDotColor(seg.type)"></span>
                  <span>{{ seg.name }}</span>
                </div>
                <div class="text-slate-500 text-[10px] pl-3.5">{{ seg.description }}</div>
              </div>
              <div class="text-right text-[10px] text-slate-500">
                <div>Offset: 0x{{ seg.start_offset.toString(16).padStart(2, '0').toUpperCase() }}</div>
                <div>Len: {{ seg.length }}B</div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue';
import HexPayloadViewer, { SnifferSegment } from './HexPayloadViewer.vue';

export interface CapturedPacket {
  id: string;
  time: string;
  timestampNs: number;
  direction: 'TX' | 'RX';
  channel: string;
  protocol: string;
  rawBytes: number[];
  hexString: string;
  decodedSummary: string;
  isValidChecksum: boolean;
  segments: SnifferSegment[];
}

const isStreaming = ref(true);
const autoScroll = ref(true);
const totalFrames = ref(0);
const throughputKbps = ref(18.4);

const selectedChannel = ref('ALL');
const selectedProtocol = ref('ALL');
const selectedDirection = ref('ALL');
const searchKeyword = ref('');

const packetTableRef = ref<HTMLElement | null>(null);
const selectedPacket = ref<CapturedPacket | null>(null);

const rawPackets = ref<CapturedPacket[]>([]);

// Mock Initial Ground-Truth Captured Packets
const seedPackets: CapturedPacket[] = [
  {
    id: 'pkt-1',
    time: '21:40:01.120.450',
    timestampNs: Date.now() * 1000000,
    direction: 'TX',
    channel: 'RS485-0',
    protocol: 'DLT645-2007',
    rawBytes: [0x68, 0x11, 0x4D, 0x3C, 0x2B, 0x1A, 0x00, 0x68, 0x11, 0x04, 0x33, 0x33, 0x34, 0x33, 0x91, 0x16],
    hexString: '68 11 4D 3C 2B 1A 00 68 11 04 33 33 34 33 91 16',
    decodedSummary: 'Read Tag: Phase A Voltage (DI=0x02010100, Meter 001A2B3C4D11)',
    isValidChecksum: true,
    segments: [
      { type: 0, start_offset: 0, length: 1, name: 'Start Frame 1 (0x68)', description: 'DL/T 645 Header' },
      { type: 1, start_offset: 1, length: 6, name: 'Address BCD', description: 'Meter ID: 001A2B3C4D11' },
      { type: 0, start_offset: 7, length: 1, name: 'Start Frame 2 (0x68)', description: 'Address Delimiter' },
      { type: 2, start_offset: 8, length: 1, name: 'Control (0x11)', description: 'Read Data Request' },
      { type: 0, start_offset: 9, length: 1, name: 'Length (4 B)', description: 'DI Data Length' },
      { type: 3, start_offset: 10, length: 4, name: 'Data Payload (+0x33)', description: 'DI: 0x02010100' },
      { type: 4, start_offset: 14, length: 1, name: 'Checksum CS (0x91)', description: 'Sum Modulo 256 Valid' },
      { type: 5, start_offset: 15, length: 1, name: 'End Frame (0x16)', description: 'End Delimiter' }
    ]
  },
  {
    id: 'pkt-2',
    time: '21:40:01.162.890',
    timestampNs: Date.now() * 1000000 + 42000000,
    direction: 'RX',
    channel: 'RS485-0',
    protocol: 'DLT645-2007',
    rawBytes: [0x68, 0x11, 0x4D, 0x3C, 0x2B, 0x1A, 0x00, 0x68, 0x91, 0x06, 0x33, 0x33, 0x34, 0x33, 0x85, 0x45, 0xDD, 0x16],
    hexString: '68 11 4D 3C 2B 1A 00 68 91 06 33 33 34 33 85 45 DD 16',
    decodedSummary: 'Read Response: Phase A Voltage = 220.5 V (Success)',
    isValidChecksum: true,
    segments: [
      { type: 0, start_offset: 0, length: 1, name: 'Start Frame 1 (0x68)', description: 'DL/T 645 Header' },
      { type: 1, start_offset: 1, length: 6, name: 'Address BCD', description: 'Meter ID: 001A2B3C4D11' },
      { type: 0, start_offset: 7, length: 1, name: 'Start Frame 2 (0x68)', description: 'Address Delimiter' },
      { type: 2, start_offset: 8, length: 1, name: 'Control (0x91)', description: 'Read Normal Response' },
      { type: 0, start_offset: 9, length: 1, name: 'Length (6 B)', description: 'Payload Length' },
      { type: 3, start_offset: 10, length: 6, name: 'Data Payload (+0x33)', description: 'DI + Voltage 220.5V' },
      { type: 4, start_offset: 16, length: 1, name: 'Checksum CS (0xDD)', description: 'Sum Modulo 256 Valid' },
      { type: 5, start_offset: 17, length: 1, name: 'End Frame (0x16)', description: 'End Delimiter' }
    ]
  },
  {
    id: 'pkt-3',
    time: '21:40:01.320.100',
    timestampNs: Date.now() * 1000000 + 200000000,
    direction: 'TX',
    channel: 'RS485-1',
    protocol: 'Modbus-RTU',
    rawBytes: [0x01, 0x03, 0x00, 0x64, 0x00, 0x02, 0x85, 0xD4],
    hexString: '01 03 00 64 00 02 85 D4',
    decodedSummary: 'Read Holding Registers: Slave=1, Addr=0x0064, Qty=2 (PCS Inverter Power)',
    isValidChecksum: true,
    segments: [
      { type: 1, start_offset: 0, length: 1, name: 'Slave ID (0x01)', description: 'Target Unit Address' },
      { type: 2, start_offset: 1, length: 1, name: 'Function (0x03)', description: 'Read Holding Registers' },
      { type: 3, start_offset: 2, length: 4, name: 'Register Range', description: 'Start: 100, Count: 2' },
      { type: 4, start_offset: 6, length: 2, name: 'CRC16 (0xD485)', description: 'Modbus CRC-16 Valid' }
    ]
  },
  {
    id: 'pkt-4',
    time: '21:40:01.340.550',
    timestampNs: Date.now() * 1000000 + 220000000,
    direction: 'RX',
    channel: 'RS485-1',
    protocol: 'Modbus-RTU',
    rawBytes: [0x01, 0x03, 0x04, 0x03, 0xE8, 0x00, 0x00, 0x7A, 0x43],
    hexString: '01 03 04 03 E8 00 00 7A 43',
    decodedSummary: 'Read Response: Active Power = 100.0 kW, Reactive Power = 0.0 kvar',
    isValidChecksum: true,
    segments: [
      { type: 1, start_offset: 0, length: 1, name: 'Slave ID (0x01)', description: 'Unit Address' },
      { type: 2, start_offset: 1, length: 1, name: 'Function (0x03)', description: 'Read Response' },
      { type: 0, start_offset: 2, length: 1, name: 'Byte Count (4)', description: '4 Bytes Payload' },
      { type: 3, start_offset: 3, length: 4, name: 'Register Data', description: 'Power: 1000 (100.0 kW)' },
      { type: 4, start_offset: 7, length: 2, name: 'CRC16 (0x437A)', description: 'Modbus CRC-16 Valid' }
    ]
  },
  {
    id: 'pkt-5',
    time: '21:40:01.500.200',
    timestampNs: Date.now() * 1000000 + 380000000,
    direction: 'RX',
    channel: 'HPLC-Mesh',
    protocol: 'DLT698.45',
    rawBytes: [0x68, 0x17, 0x00, 0x43, 0x05, 0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0x05, 0x01, 0x00, 0x20, 0x01, 0x02, 0x00, 0x00, 0x4A, 0x8C, 0x16],
    hexString: '68 17 00 43 05 12 34 56 78 90 00 05 01 00 20 01 02 00 00 4A 8C 16',
    decodedSummary: 'DLT698.45 GET-Response: OAD=0x20010200 (Active Energy Rate 1 = 1248.5 kWh)',
    isValidChecksum: true,
    segments: [
      { type: 0, start_offset: 0, length: 3, name: 'Header + Length (0x17)', description: 'DLT698 Frame Length 23' },
      { type: 2, start_offset: 3, length: 1, name: 'Control (0x43)', description: 'Response Frame' },
      { type: 1, start_offset: 4, length: 7, name: 'Server Address', description: 'Meter SA: 1234567890' },
      { type: 3, start_offset: 11, length: 8, name: 'AXDR APDU', description: 'OAD 0x20010200' },
      { type: 4, start_offset: 19, length: 2, name: 'FCS-16 (0x8C4A)', description: 'CRC-16-CCITT Valid' },
      { type: 5, start_offset: 21, length: 1, name: 'End Frame (0x16)', description: 'End Delimiter' }
    ]
  }
];

const filteredPackets = computed(() => {
  return rawPackets.value.filter(pkt => {
    if (selectedChannel.value !== 'ALL' && pkt.channel !== selectedChannel.value) return false;
    if (selectedProtocol.value !== 'ALL' && pkt.protocol !== selectedProtocol.value) return false;
    if (selectedDirection.value !== 'ALL' && pkt.direction !== selectedDirection.value) return false;
    if (searchKeyword.value.trim()) {
      const kw = searchKeyword.value.trim().toLowerCase();
      const matchSummary = pkt.decodedSummary.toLowerCase().includes(kw);
      const matchHex = pkt.hexString.toLowerCase().includes(kw);
      const matchChannel = pkt.channel.toLowerCase().includes(kw);
      if (!matchSummary && !matchHex && !matchChannel) return false;
    }
    return true;
  });
});

function toggleStreaming() {
  isStreaming.value = !isStreaming.value;
}

function clearPackets() {
  rawPackets.value = [];
  selectedPacket.value = null;
  totalFrames.value = 0;
}

function selectPacket(pkt: CapturedPacket) {
  selectedPacket.value = pkt;
}

function getSegmentDotColor(type: number): string {
  switch (type) {
    case 0: return 'bg-indigo-400';
    case 1: return 'bg-cyan-400';
    case 2: return 'bg-amber-400';
    case 3: return 'bg-emerald-400';
    case 4: return 'bg-purple-400';
    default: return 'bg-slate-500';
  }
}

function exportJSON() {
  const jsonStr = JSON.stringify(rawPackets.value, null, 2);
  const blob = new Blob([jsonStr], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `edgeos_sniffer_capture_${Date.now()}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

function exportPCAP() {
  // Generate Libpcap standard binary capture format
  const globalHeaderSize = 24;
  let totalSize = globalHeaderSize;
  rawPackets.value.forEach(p => {
    totalSize += 16 + p.rawBytes.length;
  });

  const buffer = new ArrayBuffer(totalSize);
  const view = new DataView(buffer);
  const uint8 = new Uint8Array(buffer);

  // 1. Libpcap Global Header (24 bytes)
  view.setUint32(0, 0xa1b2c3d4, true); // Magic Number
  view.setUint16(4, 2, true);          // Version Major
  view.setUint16(6, 4, true);          // Version Minor
  view.setInt32(8, 0, true);           // ThisZone (GMT)
  view.setUint32(12, 0, true);         // SigFigs
  view.setUint32(16, 65535, true);     // SnapLen
  view.setUint32(20, 147, true);       // Network LinkType: USER_0 (Industrial)

  // 2. Write Each Packet Header + Data
  let offset = globalHeaderSize;
  rawPackets.value.forEach(pkt => {
    const tsSec = Math.floor(pkt.timestampNs / 1000000000);
    const tsUsec = Math.floor((pkt.timestampNs % 1000000000) / 1000);
    const len = pkt.rawBytes.length;

    view.setUint32(offset, tsSec, true);
    view.setUint32(offset + 4, tsUsec, true);
    view.setUint32(offset + 8, len, true);   // Incl Len
    view.setUint32(offset + 12, len, true);  // Orig Len
    offset += 16;

    for (let i = 0; i < len; i++) {
      uint8[offset + i] = pkt.rawBytes[i];
    }
    offset += len;
  });

  const blob = new Blob([buffer], { type: 'application/vnd.tcpdump.pcap' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `edgeos_sniffer_capture_${Date.now()}.pcap`;
  a.click();
  URL.revokeObjectURL(url);
}

let mockInterval: number | null = null;

onMounted(() => {
  rawPackets.value = [...seedPackets];
  totalFrames.value = seedPackets.length;
  if (seedPackets.length > 0) {
    selectedPacket.value = seedPackets[0];
  }

  // Periodic frame feed simulation if streaming
  mockInterval = window.setInterval(() => {
    if (!isStreaming.value) return;

    // Pick random seed to duplicate with new time
    const seed = seedPackets[Math.floor(Math.random() * seedPackets.length)];
    const now = new Date();
    const timeStr = `${now.toTimeString().split(' ')[0]}.${String(now.getMilliseconds()).padStart(3, '0')}.120`;
    const newPkt: CapturedPacket = {
      ...seed,
      id: `pkt-${Date.now()}-${Math.floor(Math.random() * 1000)}`,
      time: timeStr,
      timestampNs: Date.now() * 1000000
    };

    rawPackets.value.push(newPkt);
    totalFrames.value++;

    if (rawPackets.value.length > 500) {
      rawPackets.value.shift();
    }

    if (autoScroll.value) {
      nextTick(() => {
        if (packetTableRef.value) {
          packetTableRef.value.scrollTop = packetTableRef.value.scrollHeight;
        }
      });
    }
  }, 2000);
});

onUnmounted(() => {
  if (mockInterval) clearInterval(mockInterval);
});
</script>
