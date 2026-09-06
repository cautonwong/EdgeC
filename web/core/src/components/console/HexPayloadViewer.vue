<template>
  <div class="bg-[#030712] border border-slate-800/80 rounded-2xl p-4 font-mono text-xs text-slate-300 space-y-4 select-text">
    <!-- Header Controls & Legend -->
    <div class="flex flex-wrap items-center justify-between gap-3 pb-3 border-b border-slate-800 text-[11px]">
      <div class="flex items-center gap-2">
        <span class="text-cyan-400 font-bold">Hex / ASCII Inspector</span>
        <span class="text-slate-500">({{ rawBytes.length }} Bytes)</span>
      </div>

      <!-- Protocol Segment Color Legend -->
      <div class="flex flex-wrap items-center gap-2 text-[10px]">
        <span class="px-2 py-0.5 rounded bg-indigo-500/20 text-indigo-400 border border-indigo-500/30">Header</span>
        <span class="px-2 py-0.5 rounded bg-cyan-500/20 text-cyan-400 border border-cyan-500/30">Address</span>
        <span class="px-2 py-0.5 rounded bg-amber-500/20 text-amber-400 border border-amber-500/30">Function</span>
        <span class="px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">Payload</span>
        <span class="px-2 py-0.5 rounded bg-purple-500/20 text-purple-400 border border-purple-500/30">Checksum</span>
        <span class="px-2 py-0.5 rounded bg-slate-700/50 text-slate-400 border border-slate-700">Trail</span>
      </div>

      <div class="flex items-center gap-2">
        <button 
          @click="copyHexString" 
          class="px-2 py-1 rounded bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-800 text-[10px] transition cursor-pointer flex items-center gap-1"
        >
          <span>📋</span>
          <span>{{ copied ? 'Copied!' : 'Copy Hex' }}</span>
        </button>
      </div>
    </div>

    <!-- Dual-Pane Hex + ASCII Grid -->
    <div class="overflow-x-auto scrollbar-thin">
      <div class="min-w-[640px] space-y-1">
        <!-- Grid Header Row -->
        <div class="grid grid-cols-[60px_1fr_180px] gap-4 text-[10px] font-bold text-slate-500 pb-1 border-b border-slate-900">
          <div>OFFSET</div>
          <div class="grid grid-cols-16 gap-1 text-center">
            <span v-for="i in 16" :key="i">{{ (i - 1).toString(16).toUpperCase() }}</span>
          </div>
          <div>ASCII</div>
        </div>

        <!-- Byte Rows -->
        <div 
          v-for="(row, rowIdx) in byteRows" 
          :key="rowIdx" 
          class="grid grid-cols-[60px_1fr_180px] gap-4 items-center py-0.5 hover:bg-slate-900/40 rounded transition"
        >
          <!-- Offset (e.g. 0000, 0010, ...) -->
          <div class="text-slate-500 select-none">
            {{ formatOffset(rowIdx * 16) }}
          </div>

          <!-- 16 Hex Bytes -->
          <div class="grid grid-cols-16 gap-1 text-center">
            <div 
              v-for="(b, colIdx) in row" 
              :key="colIdx"
              @mouseenter="hoveredByteIndex = rowIdx * 16 + colIdx"
              @mouseleave="hoveredByteIndex = null"
              class="px-1 py-0.5 rounded transition cursor-pointer font-bold select-all"
              :class="[
                getByteSegmentClass(rowIdx * 16 + colIdx),
                hoveredByteIndex === rowIdx * 16 + colIdx ? 'ring-2 ring-white scale-110 shadow-lg z-10' : ''
              ]"
            >
              {{ b.hex }}
            </div>
            <!-- Empty placeholders if row has < 16 bytes -->
            <div v-for="pad in (16 - row.length)" :key="'pad-' + pad" class="text-slate-800 select-none">
              ..
            </div>
          </div>

          <!-- 16 ASCII Characters -->
          <div class="tracking-widest flex items-center gap-0.5 text-slate-400 select-all">
            <span 
              v-for="(b, colIdx) in row" 
              :key="'asc-' + colIdx"
              @mouseenter="hoveredByteIndex = rowIdx * 16 + colIdx"
              @mouseleave="hoveredByteIndex = null"
              class="inline-block w-2.5 text-center transition"
              :class="hoveredByteIndex === rowIdx * 16 + colIdx ? 'text-cyan-400 font-bold bg-cyan-950/80 rounded' : ''"
            >
              {{ b.ascii }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- Active Byte Inspector / Tooltip Bar -->
    <div 
      v-if="activeByteInfo" 
      class="p-3 bg-slate-950 rounded-xl border border-slate-800 flex flex-wrap items-center justify-between gap-4 text-[11px] animate-fadeIn"
    >
      <div class="flex items-center gap-4">
        <div>
          <span class="text-slate-500">Offset:</span>
          <span class="text-cyan-400 font-bold ml-1">0x{{ activeByteInfo.offset.toString(16).padStart(4, '0').toUpperCase() }} ({{ activeByteInfo.offset }})</span>
        </div>
        <div>
          <span class="text-slate-500">Hex:</span>
          <span class="text-amber-400 font-bold ml-1">0x{{ activeByteInfo.hex }}</span>
        </div>
        <div>
          <span class="text-slate-500">Dec:</span>
          <span class="text-emerald-400 font-bold ml-1">{{ activeByteInfo.dec }}</span>
        </div>
        <div>
          <span class="text-slate-500">Bin:</span>
          <span class="text-purple-400 font-bold ml-1">{{ activeByteInfo.bin }}</span>
        </div>
        <div>
          <span class="text-slate-500">Char:</span>
          <span class="text-white font-bold ml-1">'{{ activeByteInfo.ascii }}'</span>
        </div>
      </div>

      <!-- Segment Meaning -->
      <div v-if="activeByteInfo.segment" class="flex items-center gap-2">
        <span 
          class="px-2 py-0.5 rounded text-[10px] font-bold uppercase"
          :class="getSegmentBadgeClass(activeByteInfo.segment.type)"
        >
          {{ activeByteInfo.segment.name }}
        </span>
        <span class="text-slate-400 text-[10px]">{{ activeByteInfo.segment.description }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';

export interface SnifferSegment {
  type: number; // 0=Header, 1=Address, 2=Function, 3=Payload, 4=Checksum, 5=Trail
  start_offset: number;
  length: number;
  name: string;
  description: string;
}

export interface ByteEntry {
  offset: number;
  hex: string;
  dec: number;
  bin: string;
  ascii: string;
  segment?: SnifferSegment;
}

const props = withDefaults(defineProps<{
  rawBytes: number[];
  segments?: SnifferSegment[];
}>(), {
  rawBytes: () => [],
  segments: () => []
});

const hoveredByteIndex = ref<number | null>(null);
const copied = ref(false);

const byteRows = computed(() => {
  const rows: ByteEntry[][] = [];
  const bytes = props.rawBytes;

  for (let i = 0; i < bytes.length; i += 16) {
    const chunk = bytes.slice(i, i + 16);
    const rowEntries: ByteEntry[] = chunk.map((b, idx) => {
      const offset = i + idx;
      const hex = b.toString(16).padStart(2, '0').toUpperCase();
      const dec = b;
      const bin = b.toString(2).padStart(8, '0');
      const ascii = (b >= 32 && b <= 126) ? String.fromCharCode(b) : '.';
      const segment = props.segments.find(s => offset >= s.start_offset && offset < s.start_offset + s.length);

      return { offset, hex, dec, bin, ascii, segment };
    });
    rows.push(rowEntries);
  }
  return rows;
});

const activeByteInfo = computed<ByteEntry | null>(() => {
  if (hoveredByteIndex.value === null) {
    // Default to first byte if available
    if (props.rawBytes.length > 0) {
      const b = props.rawBytes[0];
      const segment = props.segments.find(s => 0 >= s.start_offset && 0 < s.start_offset + s.length);
      return {
        offset: 0,
        hex: b.toString(16).padStart(2, '0').toUpperCase(),
        dec: b,
        bin: b.toString(2).padStart(8, '0'),
        ascii: (b >= 32 && b <= 126) ? String.fromCharCode(b) : '.',
        segment
      };
    }
    return null;
  }
  const idx = hoveredByteIndex.value;
  if (idx >= 0 && idx < props.rawBytes.length) {
    const b = props.rawBytes[idx];
    const segment = props.segments.find(s => idx >= s.start_offset && idx < s.start_offset + s.length);
    return {
      offset: idx,
      hex: b.toString(16).padStart(2, '0').toUpperCase(),
      dec: b,
      bin: b.toString(2).padStart(8, '0'),
      ascii: (b >= 32 && b <= 126) ? String.fromCharCode(b) : '.',
      segment
    };
  }
  return null;
});

function formatOffset(offset: number): string {
  return offset.toString(16).padStart(4, '0').toUpperCase() + ':';
}

function getByteSegmentClass(byteIndex: number): string {
  const seg = props.segments.find(s => byteIndex >= s.start_offset && byteIndex < s.start_offset + s.length);
  if (!seg) return 'bg-slate-900 text-slate-300 hover:bg-slate-800';

  switch (seg.type) {
    case 0: return 'bg-indigo-950/80 text-indigo-300 border border-indigo-500/30'; // Header
    case 1: return 'bg-cyan-950/80 text-cyan-300 border border-cyan-500/30';     // Address
    case 2: return 'bg-amber-950/80 text-amber-300 border border-amber-500/30';   // Function
    case 3: return 'bg-emerald-950/80 text-emerald-300 border border-emerald-500/30'; // Payload
    case 4: return 'bg-purple-950/80 text-purple-300 border border-purple-500/30'; // Checksum
    case 5: return 'bg-slate-800 text-slate-400 border border-slate-700';        // Trail
    default: return 'bg-slate-900 text-slate-300';
  }
}

function getSegmentBadgeClass(type: number): string {
  switch (type) {
    case 0: return 'bg-indigo-500/20 text-indigo-400 border border-indigo-500/40';
    case 1: return 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/40';
    case 2: return 'bg-amber-500/20 text-amber-400 border border-amber-500/40';
    case 3: return 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/40';
    case 4: return 'bg-purple-500/20 text-purple-400 border border-purple-500/40';
    default: return 'bg-slate-800 text-slate-400';
  }
}

function copyHexString() {
  const hex = props.rawBytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
  navigator.clipboard.writeText(hex);
  copied.value = true;
  setTimeout(() => {
    copied.value = false;
  }, 2000);
}
</script>

<style scoped>
.grid-cols-16 {
  grid-template-columns: repeat(16, minmax(0, 1fr));
}
</style>
