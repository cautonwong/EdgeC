<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-emerald-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-cyan-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-ping"></span>
            HPLC & Dual-Mode Physical Mesh Topology
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight font-mono">
            CHANNEL <span class="text-emerald-400 font-sans">TOPOLOGY & SNR HEATMAP</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            Interactive physical communication topology, multi-carrier SNR heatmaps, carrier phase crosstalk radar, and node telemetry for HPLC/RF mesh networks.
          </p>
        </div>

        <!-- Polling & Global Control Bar -->
        <div class="flex flex-wrap items-center gap-3 font-mono text-xs">
          <div class="bg-slate-950/90 border border-slate-800 px-3.5 py-2 rounded-2xl flex items-center gap-2.5 shadow-inner">
            <span class="text-slate-500">Scan Rate:</span>
            <select v-model="refreshIntervalMs" class="bg-transparent text-emerald-400 font-bold focus:outline-none cursor-pointer">
              <option :value="1000">1.0s (Live)</option>
              <option :value="2000">2.0s (Normal)</option>
              <option :value="5000">5.0s (Eco)</option>
            </select>
          </div>
          <button 
            @click="togglePolling" 
            :class="isPolling ? 'bg-emerald-500 hover:bg-emerald-600 text-slate-950 font-black' : 'bg-slate-800 hover:bg-slate-700 text-slate-300'"
            class="px-4 py-2.5 rounded-2xl border border-slate-700 transition flex items-center gap-2 shadow-lg cursor-pointer"
          >
            <span class="w-2 h-2 rounded-full" :class="isPolling ? 'bg-slate-950 animate-ping' : 'bg-slate-500'"></span>
            <span>{{ isPolling ? '⏸ PAUSE SCAN' : '▶ RESUME SCAN' }}</span>
          </button>
          <button 
            @click="exportTopologyReport" 
            class="bg-slate-950 hover:bg-slate-800 text-slate-300 px-4 py-2.5 rounded-2xl border border-slate-700 transition font-bold shadow-lg flex items-center gap-1.5 cursor-pointer"
          >
            <span>📥 Export Audit</span>
          </button>
        </div>
      </div>
    </section>

    <!-- Top Key Metrics Cards -->
    <div class="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-4 font-mono text-xs">
      <!-- Total Nodes -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Total Nodes</div>
        <div class="text-2xl lg:text-3xl font-black text-white my-1">{{ topologyStats.totalNodes }}</div>
        <div class="text-[11px] text-slate-400 flex items-center gap-1">
          <span class="text-emerald-400 font-bold">{{ topologyStats.onlineNodes }}</span> Online · <span class="text-rose-400 font-bold">{{ topologyStats.offlineNodes }}</span> Offline
        </div>
      </div>

      <!-- Average SNR -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Average SNR</div>
        <div class="text-2xl lg:text-3xl font-black text-emerald-400 my-1">{{ topologyStats.avgSnrDb.toFixed(1) }} <span class="text-xs font-normal text-slate-500">dB</span></div>
        <div class="text-[11px] text-slate-400">
          Min: <span class="text-slate-200 font-bold">{{ topologyStats.minSnrDb }}dB</span> · Max: <span class="text-cyan-400 font-bold">{{ topologyStats.maxSnrDb }}dB</span>
        </div>
      </div>

      <!-- Max Relay Hops -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Max Relay Hops</div>
        <div class="text-2xl lg:text-3xl font-black text-blue-400 my-1">{{ topologyStats.maxHops }} <span class="text-xs font-normal text-slate-500">Hops</span></div>
        <div class="text-[11px] text-slate-400">
          CCO Level: 0 · Mesh Depth: {{ topologyStats.maxHops + 1 }}
        </div>
      </div>

      <!-- Packet Loss Rate -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Overall Loss Rate</div>
        <div class="text-2xl lg:text-3xl font-black text-white my-1" :class="topologyStats.avgLossPct > 5 ? 'text-amber-400' : 'text-emerald-400'">
          {{ topologyStats.avgLossPct.toFixed(2) }} <span class="text-xs font-normal text-slate-500">%</span>
        </div>
        <div class="text-[11px] text-slate-400">
          Retries: <span class="text-slate-200 font-bold">{{ topologyStats.totalRetries }}</span> frames
        </div>
      </div>

      <!-- Phase Crosstalk Leakage -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Phase Crosstalk</div>
        <div class="text-2xl lg:text-3xl font-black text-amber-400 my-1">{{ topologyStats.crosstalkLevelDb.toFixed(1) }} <span class="text-xs font-normal text-slate-500">dB</span></div>
        <div class="text-[11px] text-slate-400">
          Phase A-B Coupl: <span class="text-slate-200 font-bold">Safe</span>
        </div>
      </div>

      <!-- Active Technology -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col justify-between">
        <div class="text-slate-500 uppercase text-[10px] tracking-wider font-bold">Channel Tech</div>
        <div class="text-2xl lg:text-3xl font-black text-cyan-400 my-1">HPLC+RF</div>
        <div class="text-[11px] text-slate-400">
          Dual-Mode OFDM (SGCC)
        </div>
      </div>
    </div>

    <!-- Filter & View Mode Navigation Bar -->
    <div class="bg-slate-900/90 border border-slate-800 rounded-2xl p-4 shadow-xl flex flex-col lg:flex-row items-start lg:items-center justify-between gap-4 font-mono text-xs">
      <!-- View Modes -->
      <div class="flex items-center gap-1.5 bg-slate-950 p-1.5 rounded-xl border border-slate-800">
        <button 
          v-for="mode in viewModes" 
          :key="mode.id"
          @click="activeViewMode = mode.id"
          :class="activeViewMode === mode.id ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 font-bold' : 'text-slate-400 hover:text-slate-200 border-transparent'"
          class="px-3 py-1.5 rounded-lg border transition flex items-center gap-1.5 cursor-pointer"
        >
          <span>{{ mode.icon }}</span>
          <span>{{ mode.name }}</span>
        </button>
      </div>

      <!-- Filters -->
      <div class="flex flex-wrap items-center gap-3">
        <!-- Tech Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Tech:</span>
          <select v-model="filterTech" class="bg-transparent text-slate-200 font-bold focus:outline-none cursor-pointer">
            <option value="ALL">All Technologies</option>
            <option value="HPLC">HPLC (OFDM)</option>
            <option value="RF">Sub-1GHz RF</option>
            <option value="RS485">RS-485</option>
          </select>
        </div>

        <!-- Phase Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Phase:</span>
          <select v-model="filterPhase" class="bg-transparent text-slate-200 font-bold focus:outline-none cursor-pointer">
            <option value="ALL">All Phases</option>
            <option value="PHASE_A">Phase A (L1)</option>
            <option value="PHASE_B">Phase B (L2)</option>
            <option value="PHASE_C">Phase C (L3)</option>
            <option value="WIRELESS">Wireless/RF</option>
          </select>
        </div>

        <!-- Quality Filter -->
        <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
          <span class="text-slate-500">Quality:</span>
          <select v-model="filterQuality" class="bg-transparent text-slate-200 font-bold focus:outline-none cursor-pointer">
            <option value="ALL">All Qualities</option>
            <option value="GOOD">Excellent (>20dB)</option>
            <option value="MEDIUM">Medium (10~20dB)</option>
            <option value="POOR">Poor (<10dB)</option>
          </select>
        </div>

        <!-- Search Box -->
        <div class="relative">
          <input 
            v-model="searchQuery" 
            placeholder="Search Device / MAC..." 
            class="bg-slate-950 border border-slate-800 px-3.5 py-1.5 rounded-xl text-slate-200 placeholder-slate-600 focus:outline-none focus:border-emerald-500/50 w-44 lg:w-56"
          />
          <span v-if="searchQuery" @click="searchQuery = ''" class="absolute right-2.5 top-2 text-slate-500 hover:text-slate-300 cursor-pointer">×</span>
        </div>
      </div>
    </div>

    <!-- MAIN VIEW AREA -->

    <!-- VIEW 1: Interactive Topology Graph -->
    <div v-if="activeViewMode === 'topology'" class="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-2xl relative overflow-hidden">
      <div class="flex items-center justify-between mb-4 font-mono text-xs">
        <div class="flex items-center gap-2">
          <span class="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse"></span>
          <span class="font-bold text-slate-300">PHYSICAL MESH TOPOLOGY MAP</span>
          <span class="text-slate-500">({{ filteredNodes.length }} Visible Nodes)</span>
        </div>
        <!-- Legend -->
        <div class="flex flex-wrap items-center gap-4 text-[11px] text-slate-400">
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-emerald-400"></span> SNR > 20dB (Good)</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-amber-400"></span> 10~20dB (Medium)</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-rose-500"></span> &lt; 10dB (Poor)</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-cyan-400"></span> Central CCO</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-blue-500"></span> Phase A</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-purple-500"></span> Phase B</span>
          <span class="flex items-center gap-1.5"><span class="w-2.5 h-2.5 rounded-full bg-orange-500"></span> Phase C</span>
        </div>
      </div>

      <!-- SVG Topology Visual Canvas -->
      <div class="w-full h-[540px] bg-slate-950/80 rounded-2xl border border-slate-800/80 relative overflow-hidden flex items-center justify-center select-none">
        <svg class="w-full h-full" viewBox="0 0 1000 500" preserveAspectRatio="xMidYMid meet">
          <!-- Background Grid Lines -->
          <defs>
            <pattern id="topoGrid" width="40" height="40" patternUnits="userSpaceOnUse">
              <path d="M 40 0 L 0 0 0 40" fill="none" stroke="#1e293b" stroke-width="0.5" stroke-dasharray="2,2"/>
            </pattern>
            <linearGradient id="linkGood" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stop-color="#10b981" stop-opacity="0.8"/>
              <stop offset="100%" stop-color="#065f46" stop-opacity="0.3"/>
            </linearGradient>
            <linearGradient id="linkMedium" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stop-color="#f59e0b" stop-opacity="0.8"/>
              <stop offset="100%" stop-color="#78350f" stop-opacity="0.3"/>
            </linearGradient>
            <linearGradient id="linkPoor" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stop-color="#f43f5e" stop-opacity="0.8"/>
              <stop offset="100%" stop-color="#881337" stop-opacity="0.3"/>
            </linearGradient>
          </defs>
          <rect width="100%" height="100%" fill="url(#topoGrid)" />

          <!-- Connection Lines between Nodes -->
          <g class="topology-links">
            <g v-for="link in topologyLinks" :key="link.id">
              <!-- Animated Pulse Particle along link -->
              <line 
                :x1="link.sourceX" 
                :y1="link.sourceY" 
                :x2="link.targetX" 
                :y2="link.targetY" 
                :stroke="getLinkColor(link.snrDb)"
                :stroke-width="getLinkWidth(link.snrDb)"
                stroke-dasharray="4,4"
                class="transition-all duration-300 opacity-70"
              />
              <!-- Midpoint SNR Badge -->
              <rect 
                :x="(link.sourceX + link.targetX) / 2 - 16" 
                :y="(link.sourceY + link.targetY) / 2 - 9" 
                width="32" 
                height="18" 
                rx="4" 
                fill="#0f172a" 
                stroke="#334155" 
                stroke-width="0.8"
              />
              <text 
                :x="(link.sourceX + link.targetX) / 2" 
                :y="(link.sourceY + link.targetY) / 2 + 3" 
                fill="#94a3b8" 
                font-size="9" 
                font-family="monospace" 
                text-anchor="middle"
                font-weight="bold"
              >
                {{ link.snrDb }}dB
              </text>
            </g>
          </g>

          <!-- Node Elements -->
          <g class="topology-nodes">
            <g 
              v-for="node in filteredNodes" 
              :key="node.id"
              :transform="`translate(${node.x}, ${node.y})`"
              @click="selectNode(node)"
              class="cursor-pointer group"
            >
              <!-- Outer Pulse / Selection Halo -->
              <circle 
                v-if="selectedNode?.id === node.id" 
                r="30" 
                fill="none" 
                stroke="#10b981" 
                stroke-width="2" 
                stroke-dasharray="4,4"
                class="animate-spin duration-3000"
              />
              <circle 
                v-if="node.isRoot" 
                r="28" 
                fill="#064e3b" 
                fill-opacity="0.4" 
                stroke="#10b981" 
                stroke-width="1.5"
              />
              
              <!-- Main Node Body -->
              <circle 
                :r="node.isRoot ? 22 : 16" 
                :fill="getNodeFillColor(node)"
                :stroke="getNodeStrokeColor(node)"
                stroke-width="2.5"
                class="transition-transform duration-200 group-hover:scale-110 shadow-lg"
              />

              <!-- Node Icon / Role Character -->
              <text 
                x="0" 
                y="4" 
                fill="#ffffff" 
                font-size="10" 
                font-family="monospace" 
                text-anchor="middle"
                font-weight="900"
                class="pointer-events-none"
              >
                {{ node.isRoot ? 'CCO' : (node.isProxy ? 'PXY' : 'STA') }}
              </text>

              <!-- Node ID & Phase Label Below -->
              <text 
                x="0" 
                :y="node.isRoot ? 36 : 28" 
                fill="#cbd5e1" 
                font-size="10" 
                font-family="monospace" 
                text-anchor="middle"
                font-weight="bold"
                class="pointer-events-none"
              >
                {{ node.name }}
              </text>
              <text 
                x="0" 
                :y="node.isRoot ? 47 : 38" 
                :fill="getPhaseColor(node.phase)" 
                font-size="8.5" 
                font-family="monospace" 
                text-anchor="middle"
                class="pointer-events-none"
              >
                {{ node.phaseLabel }} · {{ node.snrDb }}dB
              </text>
            </g>
          </g>
        </svg>

        <!-- Canvas Zoom/Reset Float Helper -->
        <div class="absolute bottom-4 right-4 bg-slate-900/90 border border-slate-800 rounded-xl px-3 py-1.5 text-[10px] font-mono text-slate-400 flex items-center gap-2">
          <span>🎯 Layered Tree Mesh</span>
          <span>·</span>
          <span>Click Node to Inspect</span>
        </div>
      </div>
    </div>

    <!-- VIEW 2: Sub-Carrier Frequency SNR Heatmap -->
    <div v-else-if="activeViewMode === 'heatmap'" class="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-2xl relative overflow-hidden">
      <div class="flex items-center justify-between mb-4 font-mono text-xs">
        <div class="flex items-center gap-2">
          <span class="w-2.5 h-2.5 rounded-full bg-cyan-400"></span>
          <span class="font-bold text-slate-300">16-OFDM SUB-BAND SNR HEATMAP MATRIX (1.5MHz ~ 12MHz)</span>
        </div>
        <!-- Gradient Legend -->
        <div class="flex items-center gap-2 text-[11px] text-slate-400">
          <span>&lt; 5dB</span>
          <div class="w-28 h-3 rounded-full bg-gradient-to-r from-rose-600 via-amber-500 via-emerald-500 to-cyan-400"></div>
          <span>&gt; 35dB</span>
        </div>
      </div>

      <!-- Heatmap Table Matrix -->
      <div class="overflow-x-auto rounded-2xl border border-slate-800">
        <table class="w-full text-left font-mono text-[11px] border-collapse bg-slate-950/60">
          <thead>
            <tr class="bg-slate-900/90 text-slate-400 border-b border-slate-800">
              <th class="p-3 sticky left-0 bg-slate-900 z-10">Target Node</th>
              <th class="p-3">Phase</th>
              <th class="p-3">Tech</th>
              <th v-for="ch in 16" :key="ch" class="p-2 text-center text-[10px]">
                Ch{{ ch - 1 }}<br/><span class="text-slate-600 font-normal">{{ (1.5 + (ch-1)*0.7).toFixed(1) }}M</span>
              </th>
              <th class="p-3 text-right">Avg SNR</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-850">
            <tr 
              v-for="node in filteredNodes" 
              :key="node.id"
              @click="selectNode(node)"
              class="hover:bg-slate-800/50 transition cursor-pointer"
              :class="selectedNode?.id === node.id ? 'bg-slate-800/70' : ''"
            >
              <td class="p-3 font-bold text-slate-200 sticky left-0 bg-slate-950 z-10 flex items-center gap-2">
                <span class="w-2 h-2 rounded-full" :class="getNodeStatusDotClass(node)"></span>
                {{ node.name }}
              </td>
              <td class="p-3" :class="getPhaseColorClass(node.phase)">{{ node.phaseLabel }}</td>
              <td class="p-3 text-slate-400">{{ node.tech }}</td>
              <td 
                v-for="(subSnr, idx) in node.subCarrierSnr" 
                :key="idx" 
                class="p-1 text-center"
              >
                <div 
                  class="w-full py-1 rounded text-[10px] font-bold transition hover:scale-110 shadow-sm"
                  :style="{ backgroundColor: getSnrHeatmapColor(subSnr), color: subSnr > 15 ? '#020617' : '#ffffff' }"
                  :title="`Ch${idx} (${(1.5 + idx*0.7).toFixed(1)}MHz): ${subSnr} dB`"
                >
                  {{ subSnr }}
                </div>
              </td>
              <td class="p-3 text-right font-black" :class="node.snrDb > 20 ? 'text-emerald-400' : (node.snrDb > 10 ? 'text-amber-400' : 'text-rose-400')">
                {{ node.snrDb }} dB
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- VIEW 3: Phase Crosstalk Radar & Interference Analyzer -->
    <div v-else-if="activeViewMode === 'crosstalk'" class="grid grid-cols-1 lg:grid-cols-2 gap-6 font-mono text-xs">
      <!-- Left: Phase Crosstalk Leakage Matrix -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-2xl space-y-6">
        <div class="flex items-center justify-between">
          <span class="font-bold text-slate-300 flex items-center gap-2">
            <span class="w-2.5 h-2.5 rounded-full bg-amber-400"></span>
            PHASE CROSSTALK LEAKAGE MATRIX (dB)
          </span>
          <span class="text-slate-500 text-[11px]">Carrier Coupling Isolation</span>
        </div>

        <div class="grid grid-cols-3 gap-4">
          <!-- Phase A -> B -->
          <div class="bg-slate-950 border border-slate-800 rounded-2xl p-4 space-y-2">
            <div class="flex justify-between text-slate-400">
              <span class="text-blue-400 font-bold">Phase A → B</span>
              <span class="text-emerald-400 font-bold">12.4 dB</span>
            </div>
            <div class="w-full bg-slate-900 h-2 rounded-full overflow-hidden">
              <div class="bg-emerald-400 h-full rounded-full" style="width: 35%"></div>
            </div>
            <div class="text-[10px] text-slate-500">Status: Safe Coupling (&lt;18dB)</div>
          </div>

          <!-- Phase B -> C -->
          <div class="bg-slate-950 border border-slate-800 rounded-2xl p-4 space-y-2">
            <div class="flex justify-between text-slate-400">
              <span class="text-purple-400 font-bold">Phase B → C</span>
              <span class="text-amber-400 font-bold">21.6 dB</span>
            </div>
            <div class="w-full bg-slate-900 h-2 rounded-full overflow-hidden">
              <div class="bg-amber-400 h-full rounded-full" style="width: 65%"></div>
            </div>
            <div class="text-[10px] text-amber-500 font-bold">Warning: High Inter-Phase Leakage</div>
          </div>

          <!-- Phase C -> A -->
          <div class="bg-slate-950 border border-slate-800 rounded-2xl p-4 space-y-2">
            <div class="flex justify-between text-slate-400">
              <span class="text-orange-400 font-bold">Phase C → A</span>
              <span class="text-emerald-400 font-bold">8.2 dB</span>
            </div>
            <div class="w-full bg-slate-900 h-2 rounded-full overflow-hidden">
              <div class="bg-emerald-400 h-full rounded-full" style="width: 25%"></div>
            </div>
            <div class="text-[10px] text-slate-500">Status: Low Coupling (Optimal)</div>
          </div>
        </div>

        <!-- Crosstalk Diagnostic Advice -->
        <div class="bg-slate-950/80 border border-slate-800/80 rounded-2xl p-4 space-y-2 text-[11px] text-slate-400">
          <div class="font-bold text-slate-200">🔍 Real-Time Crosstalk Root Cause:</div>
          <p>
            Detected capacitive coupling spike between <span class="text-purple-400 font-bold">Phase B</span> and <span class="text-orange-400 font-bold">Phase C</span> at substation feeder #2. Recommended to recalibrate OFDM adaptive notch filters on Ch06-Ch09 to mitigate packet retransmissions.
          </p>
        </div>
      </div>

      <!-- Right: High-Frequency Noise & Inverter PWM Burst Log -->
      <div class="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-2xl space-y-4">
        <div class="flex items-center justify-between">
          <span class="font-bold text-slate-300 flex items-center gap-2">
            <span class="w-2.5 h-2.5 rounded-full bg-rose-400"></span>
            INVERTER PWM NOISE BURSTS & INTERFERENCE
          </span>
          <span class="text-slate-500 text-[11px]">Last 60 Minutes</span>
        </div>

        <div class="space-y-3 max-h-[300px] overflow-y-auto pr-1">
          <div 
            v-for="noise in noiseEvents" 
            :key="noise.id"
            class="bg-slate-950 border border-slate-800 rounded-2xl p-3.5 flex items-start justify-between gap-4"
          >
            <div class="space-y-1">
              <div class="flex items-center gap-2">
                <span 
                  class="px-2 py-0.5 rounded text-[10px] font-bold uppercase"
                  :class="noise.severity === 'CRITICAL' ? 'bg-rose-500/20 text-rose-400 border border-rose-500/30' : 'bg-amber-500/20 text-amber-400 border border-amber-500/30'"
                >
                  {{ noise.type }}
                </span>
                <span class="text-slate-200 font-bold">{{ noise.source }}</span>
                <span class="text-slate-500 text-[10px]">Phase {{ noise.phase }}</span>
              </div>
              <p class="text-[11px] text-slate-400">{{ noise.description }}</p>
            </div>
            <div class="text-right font-mono shrink-0">
              <span class="text-rose-400 font-bold">+{{ noise.noiseFloorRiseDb }} dB Floor</span>
              <div class="text-slate-500 text-[10px]">{{ noise.timestamp }}</div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- VIEW 4: Channel Diagnostics Matrix (Table) -->
    <div v-else-if="activeViewMode === 'table'" class="bg-slate-900/90 border border-slate-800 rounded-3xl p-6 shadow-2xl relative overflow-hidden">
      <div class="flex items-center justify-between mb-4 font-mono text-xs">
        <span class="font-bold text-slate-300 flex items-center gap-2">
          <span class="w-2.5 h-2.5 rounded-full bg-emerald-400"></span>
          PHYSICAL CHANNEL TELEMETRY ROSTER ({{ filteredNodes.length }} Records)
        </span>
      </div>

      <div class="overflow-x-auto rounded-2xl border border-slate-800 font-mono text-xs">
        <table class="w-full text-left divide-y divide-slate-800 bg-slate-950/60">
          <thead class="bg-slate-900/90 text-slate-400 text-[11px]">
            <tr>
              <th class="p-3.5">Device Name</th>
              <th class="p-3.5">MAC / EUI64</th>
              <th class="p-3.5">Role</th>
              <th class="p-3.5">Tech</th>
              <th class="p-3.5">Phase</th>
              <th class="p-3.5">Relay Hops</th>
              <th class="p-3.5">SNR (dB)</th>
              <th class="p-3.5">Attenuation</th>
              <th class="p-3.5">Modulation</th>
              <th class="p-3.5">Loss Rate</th>
              <th class="p-3.5">Retries</th>
              <th class="p-3.5 text-right">Actions</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-slate-850">
            <tr 
              v-for="node in filteredNodes" 
              :key="node.id"
              @click="selectNode(node)"
              class="hover:bg-slate-800/50 transition cursor-pointer"
              :class="selectedNode?.id === node.id ? 'bg-slate-800/70' : ''"
            >
              <td class="p-3.5 font-bold text-white flex items-center gap-2">
                <span class="w-2 h-2 rounded-full" :class="getNodeStatusDotClass(node)"></span>
                {{ node.name }}
              </td>
              <td class="p-3.5 text-slate-400 text-[11px]">{{ node.mac }}</td>
              <td class="p-3.5">
                <span 
                  class="px-2 py-0.5 rounded text-[10px] font-bold"
                  :class="node.isRoot ? 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/30' : (node.isProxy ? 'bg-indigo-500/20 text-indigo-400 border border-indigo-500/30' : 'bg-slate-800 text-slate-300')"
                >
                  {{ node.role }}
                </span>
              </td>
              <td class="p-3.5 text-slate-300">{{ node.tech }}</td>
              <td class="p-3.5 font-bold" :class="getPhaseColorClass(node.phase)">{{ node.phaseLabel }}</td>
              <td class="p-3.5 text-slate-300 font-bold">{{ node.hops }}</td>
              <td class="p-3.5 font-black" :class="node.snrDb > 20 ? 'text-emerald-400' : (node.snrDb > 10 ? 'text-amber-400' : 'text-rose-400')">
                {{ node.snrDb }} dB
              </td>
              <td class="p-3.5 text-slate-400">{{ node.attenuationDbm }} dBm</td>
              <td class="p-3.5 text-cyan-400 font-bold">{{ node.modulation }}</td>
              <td class="p-3.5" :class="node.lossRatePct > 5 ? 'text-rose-400 font-bold' : 'text-slate-300'">
                {{ node.lossRatePct.toFixed(1) }}%
              </td>
              <td class="p-3.5 text-slate-400">{{ node.retries }}</td>
              <td class="p-3.5 text-right">
                <button 
                  @click.stop="selectNode(node)" 
                  class="px-2.5 py-1 rounded-lg bg-emerald-500/20 hover:bg-emerald-500/30 text-emerald-400 font-bold border border-emerald-500/30 cursor-pointer"
                >
                  Inspect
                </button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- RIGHT SLIDE-OVER NODE INSPECTOR DRAWER -->
    <div 
      v-if="selectedNode" 
      class="fixed inset-y-0 right-0 w-full max-w-lg bg-[#0f172a] border-l border-slate-800 shadow-2xl z-50 p-6 lg:p-8 space-y-6 overflow-y-auto font-mono text-xs animate-in slide-in-from-right duration-300"
    >
      <!-- Drawer Header -->
      <div class="flex items-center justify-between pb-4 border-b border-slate-800">
        <div class="space-y-1">
          <div class="flex items-center gap-2">
            <span class="w-2.5 h-2.5 rounded-full" :class="getNodeStatusDotClass(selectedNode)"></span>
            <h2 class="text-xl font-black text-white font-mono">{{ selectedNode.name }}</h2>
            <span class="px-2 py-0.5 rounded text-[10px] font-bold bg-slate-800 text-slate-300 border border-slate-700">
              {{ selectedNode.role }}
            </span>
          </div>
          <p class="text-slate-400 text-[11px]">MAC: {{ selectedNode.mac }} · Protocol: {{ selectedNode.protocol }}</p>
        </div>
        <button 
          @click="selectedNode = null" 
          class="w-8 h-8 rounded-full bg-slate-800 hover:bg-slate-700 text-slate-300 flex items-center justify-center cursor-pointer transition"
        >
          ✕
        </button>
      </div>

      <!-- Physical Parameters Grid -->
      <div class="grid grid-cols-2 gap-3">
        <div class="bg-slate-950 p-3.5 rounded-2xl border border-slate-800 space-y-1">
          <div class="text-slate-500 text-[10px] uppercase">Channel Tech</div>
          <div class="text-sm font-bold text-white">{{ selectedNode.tech }}</div>
          <div class="text-[10px] text-emerald-400">Carrier Phase: {{ selectedNode.phaseLabel }}</div>
        </div>
        <div class="bg-slate-950 p-3.5 rounded-2xl border border-slate-800 space-y-1">
          <div class="text-slate-500 text-[10px] uppercase">SNR / Signal</div>
          <div class="text-sm font-bold text-emerald-400">{{ selectedNode.snrDb }} dB</div>
          <div class="text-[10px] text-slate-400">Atten: {{ selectedNode.attenuationDbm }} dBm</div>
        </div>
        <div class="bg-slate-950 p-3.5 rounded-2xl border border-slate-800 space-y-1">
          <div class="text-slate-500 text-[10px] uppercase">Modulation Scheme</div>
          <div class="text-sm font-bold text-cyan-400">{{ selectedNode.modulation }}</div>
          <div class="text-[10px] text-slate-400">BER: {{ selectedNode.ber }}</div>
        </div>
        <div class="bg-slate-950 p-3.5 rounded-2xl border border-slate-800 space-y-1">
          <div class="text-slate-500 text-[10px] uppercase">Relay Topology</div>
          <div class="text-sm font-bold text-blue-400">{{ selectedNode.hops }} Hops</div>
          <div class="text-[10px] text-slate-400">Parent: {{ selectedNode.parentName || 'None (Root)' }}</div>
        </div>
      </div>

      <!-- Route Path Breadcrumb -->
      <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-2">
        <div class="text-slate-400 font-bold flex items-center justify-between">
          <span>Routing Path to CCO:</span>
          <span class="text-emerald-400 text-[11px]">{{ selectedNode.hops === 0 ? 'Direct Gateway' : 'Mesh Route Active' }}</span>
        </div>
        <div class="flex items-center gap-2 text-slate-300 text-[11px] overflow-x-auto py-1">
          <span class="bg-cyan-500/20 text-cyan-400 px-2 py-1 rounded border border-cyan-500/30 font-bold">CCO-01</span>
          <span v-if="selectedNode.hops > 1" class="text-slate-600">→</span>
          <span v-if="selectedNode.hops > 1" class="bg-indigo-500/20 text-indigo-400 px-2 py-1 rounded border border-indigo-500/30">
            {{ selectedNode.parentName }}
          </span>
          <span v-if="selectedNode.hops > 0" class="text-slate-600">→</span>
          <span v-if="selectedNode.hops > 0" class="bg-emerald-500/20 text-emerald-400 px-2 py-1 rounded border border-emerald-500/30 font-bold">
            {{ selectedNode.name }}
          </span>
        </div>
      </div>

      <!-- 16 Sub-Carrier Spark Bar Chart for Selected Node -->
      <div class="bg-slate-950 p-4 rounded-2xl border border-slate-800 space-y-3">
        <div class="flex items-center justify-between text-slate-400 font-bold">
          <span>OFDM Sub-Carrier SNR Profile:</span>
          <span class="text-slate-500 text-[10px]">Ch00 ~ Ch15</span>
        </div>
        <div class="flex items-end gap-1.5 h-24 pt-4">
          <div 
            v-for="(subSnr, idx) in selectedNode.subCarrierSnr" 
            :key="idx" 
            class="flex-1 flex flex-col items-center gap-1 group"
          >
            <div 
              class="w-full rounded-t transition-all duration-300"
              :style="{ height: `${Math.min(100, Math.max(10, subSnr * 2.5))}%`, backgroundColor: getSnrHeatmapColor(subSnr) }"
            ></div>
            <span class="text-[8px] text-slate-500 font-mono">{{ idx }}</span>
          </div>
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="grid grid-cols-2 gap-3 pt-2">
        <button 
          @click="triggerPingProbe(selectedNode)" 
          class="bg-emerald-500 hover:bg-emerald-600 text-slate-950 font-black py-2.5 rounded-xl transition shadow-lg flex items-center justify-center gap-2 cursor-pointer"
        >
          <span>⚡ Send Echo Probe</span>
        </button>
        <button 
          @click="recalibrateNotch(selectedNode)" 
          class="bg-slate-850 hover:bg-slate-800 text-slate-200 font-bold py-2.5 rounded-xl border border-slate-700 transition flex items-center justify-center gap-2 cursor-pointer"
        >
          <span>🔧 Recalibrate Notch</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';

export interface TopologyNode {
  id: string;
  name: string;
  mac: string;
  role: 'CCO' | 'Proxy' | 'STA';
  tech: 'HPLC' | 'RF' | 'RS485';
  phase: 'PHASE_A' | 'PHASE_B' | 'PHASE_C' | 'WIRELESS' | 'THREE_PHASE';
  phaseLabel: string;
  hops: number;
  snrDb: number;
  attenuationDbm: number;
  modulation: 'BPSK' | 'QPSK' | '16QAM' | '64QAM' | 'FSK';
  lossRatePct: number;
  retries: number;
  ber: string;
  protocol: 'DLT645-2007' | 'DLT698.45' | 'Modbus-RTU';
  isRoot: boolean;
  isProxy: boolean;
  parentName?: string;
  x: number;
  y: number;
  subCarrierSnr: number[];
}

export interface TopologyLink {
  id: string;
  sourceId: string;
  targetId: string;
  sourceX: number;
  sourceY: number;
  targetX: number;
  targetY: number;
  snrDb: number;
}

export interface NoiseEvent {
  id: string;
  type: string;
  severity: 'WARNING' | 'CRITICAL';
  source: string;
  phase: string;
  noiseFloorRiseDb: number;
  timestamp: string;
  description: string;
}

// State
const isPolling = ref(true);
const refreshIntervalMs = ref(2000);
let pollTimer: ReturnType<typeof setInterval> | null = null;

const activeViewMode = ref<'topology' | 'heatmap' | 'crosstalk' | 'table'>('topology');
const filterTech = ref('ALL');
const filterPhase = ref('ALL');
const filterQuality = ref('ALL');
const searchQuery = ref('');
const selectedNode = ref<TopologyNode | null>(null);

const viewModes = [
  { id: 'topology', name: 'Topology Map', icon: '🌐' },
  { id: 'heatmap', name: 'SNR Heatmap', icon: '🗺️' },
  { id: 'crosstalk', name: 'Crosstalk Radar', icon: '📡' },
  { id: 'table', name: 'Diagnostics Table', icon: '📋' },
] as const;

// Physical Nodes Topology Data (SGCC / CSG Dual-Mode compliant)
const nodes = ref<TopologyNode[]>([
  // Root CCO Central Gateway
  {
    id: 'node-cco',
    name: 'CCO-01 (DCU Master)',
    mac: '00:1A:2B:3C:4D:00',
    role: 'CCO',
    tech: 'HPLC',
    phase: 'THREE_PHASE',
    phaseLabel: '3-Phase',
    hops: 0,
    snrDb: 38.5,
    attenuationDbm: -32.0,
    modulation: '64QAM',
    lossRatePct: 0.0,
    retries: 0,
    ber: '1.0e-8',
    protocol: 'DLT698.45',
    isRoot: true,
    isProxy: false,
    x: 500,
    y: 70,
    subCarrierSnr: [38, 37, 39, 38, 36, 39, 40, 38, 37, 39, 38, 37, 39, 40, 38, 39]
  },
  // Layer 1 Proxies / Relays
  {
    id: 'node-pxy-a',
    name: 'STA-PXY-01A',
    mac: '00:1A:2B:3C:4D:01',
    role: 'Proxy',
    tech: 'HPLC',
    phase: 'PHASE_A',
    phaseLabel: 'Phase A',
    hops: 1,
    snrDb: 28.4,
    attenuationDbm: -48.5,
    modulation: '16QAM',
    lossRatePct: 0.1,
    retries: 4,
    ber: '2.5e-6',
    protocol: 'DLT698.45',
    isRoot: false,
    isProxy: true,
    parentName: 'CCO-01',
    x: 220,
    y: 200,
    subCarrierSnr: [28, 29, 27, 26, 28, 30, 29, 28, 27, 26, 28, 29, 30, 28, 27, 29]
  },
  {
    id: 'node-pxy-b',
    name: 'STA-PXY-02B',
    mac: '00:1A:2B:3C:4D:02',
    role: 'Proxy',
    tech: 'HPLC',
    phase: 'PHASE_B',
    phaseLabel: 'Phase B',
    hops: 1,
    snrDb: 24.2,
    attenuationDbm: -54.0,
    modulation: '16QAM',
    lossRatePct: 0.8,
    retries: 12,
    ber: '8.1e-6',
    protocol: 'DLT698.45',
    isRoot: false,
    isProxy: true,
    parentName: 'CCO-01',
    x: 500,
    y: 200,
    subCarrierSnr: [24, 23, 22, 25, 26, 24, 23, 25, 24, 22, 23, 25, 24, 26, 25, 23]
  },
  {
    id: 'node-pxy-c',
    name: 'STA-PXY-03C',
    mac: '00:1A:2B:3C:4D:03',
    role: 'Proxy',
    tech: 'HPLC',
    phase: 'PHASE_C',
    phaseLabel: 'Phase C',
    hops: 1,
    snrDb: 26.8,
    attenuationDbm: -51.2,
    modulation: '16QAM',
    lossRatePct: 0.3,
    retries: 6,
    ber: '4.2e-6',
    protocol: 'DLT698.45',
    isRoot: false,
    isProxy: true,
    parentName: 'CCO-01',
    x: 780,
    y: 200,
    subCarrierSnr: [27, 26, 28, 27, 25, 28, 29, 26, 27, 25, 28, 27, 26, 28, 27, 26]
  },
  // Layer 2 Leaf Nodes / Smart Meters
  {
    id: 'node-mtr-101',
    name: 'MTR-101A (Feeder 1)',
    mac: '00:1A:2B:3C:4D:11',
    role: 'STA',
    tech: 'HPLC',
    phase: 'PHASE_A',
    phaseLabel: 'Phase A',
    hops: 2,
    snrDb: 22.1,
    attenuationDbm: -62.4,
    modulation: 'QPSK',
    lossRatePct: 1.2,
    retries: 18,
    ber: '3.4e-5',
    protocol: 'DLT645-2007',
    isRoot: false,
    isProxy: false,
    parentName: 'STA-PXY-01A',
    x: 120,
    y: 350,
    subCarrierSnr: [22, 21, 23, 20, 19, 22, 24, 21, 22, 20, 21, 23, 22, 21, 23, 22]
  },
  {
    id: 'node-mtr-102',
    name: 'MTR-102A (Feeder 1)',
    mac: '00:1A:2B:3C:4D:12',
    role: 'STA',
    tech: 'HPLC',
    phase: 'PHASE_A',
    phaseLabel: 'Phase A',
    hops: 2,
    snrDb: 25.6,
    attenuationDbm: -58.0,
    modulation: '16QAM',
    lossRatePct: 0.5,
    retries: 7,
    ber: '1.2e-5',
    protocol: 'DLT645-2007',
    isRoot: false,
    isProxy: false,
    parentName: 'STA-PXY-01A',
    x: 260,
    y: 350,
    subCarrierSnr: [26, 25, 27, 24, 25, 27, 28, 25, 26, 24, 25, 27, 26, 25, 26, 25]
  },
  {
    id: 'node-mtr-201',
    name: 'MTR-201B (Feeder 2)',
    mac: '00:1A:2B:3C:4D:21',
    role: 'STA',
    tech: 'HPLC',
    phase: 'PHASE_B',
    phaseLabel: 'Phase B',
    hops: 2,
    snrDb: 18.5,
    attenuationDbm: -68.2,
    modulation: 'QPSK',
    lossRatePct: 3.8,
    retries: 42,
    ber: '8.9e-5',
    protocol: 'DLT645-2007',
    isRoot: false,
    isProxy: false,
    parentName: 'STA-PXY-02B',
    x: 420,
    y: 350,
    subCarrierSnr: [18, 17, 19, 16, 15, 18, 20, 17, 18, 16, 17, 19, 18, 17, 19, 18]
  },
  {
    id: 'node-mtr-202',
    name: 'MTR-202B (Feeder 2)',
    mac: '00:1A:2B:3C:4D:22',
    role: 'STA',
    tech: 'HPLC',
    phase: 'PHASE_B',
    phaseLabel: 'Phase B',
    hops: 2,
    snrDb: 9.4,
    attenuationDbm: -79.5,
    modulation: 'BPSK',
    lossRatePct: 8.4,
    retries: 96,
    ber: '4.5e-4',
    protocol: 'DLT645-2007',
    isRoot: false,
    isProxy: false,
    parentName: 'STA-PXY-02B',
    x: 560,
    y: 350,
    subCarrierSnr: [9, 8, 10, 7, 6, 9, 11, 8, 9, 7, 8, 10, 9, 8, 10, 9]
  },
  {
    id: 'node-mtr-301',
    name: 'MTR-301C (Feeder 3)',
    mac: '00:1A:2B:3C:4D:31',
    role: 'STA',
    tech: 'HPLC',
    phase: 'PHASE_C',
    phaseLabel: 'Phase C',
    hops: 2,
    snrDb: 23.4,
    attenuationDbm: -61.0,
    modulation: '16QAM',
    lossRatePct: 0.9,
    retries: 14,
    ber: '2.8e-5',
    protocol: 'DLT645-2007',
    isRoot: false,
    isProxy: false,
    parentName: 'STA-PXY-03C',
    x: 720,
    y: 350,
    subCarrierSnr: [23, 24, 22, 21, 23, 25, 24, 23, 22, 21, 23, 24, 25, 23, 22, 24]
  },
  {
    id: 'node-rf-401',
    name: 'SEN-RF-401 (Micro-Power)',
    mac: '00:1A:2B:3C:4D:41',
    role: 'STA',
    tech: 'RF',
    phase: 'WIRELESS',
    phaseLabel: 'RF Sub-1G',
    hops: 1,
    snrDb: 21.0,
    attenuationDbm: -72.0,
    modulation: 'FSK',
    lossRatePct: 1.5,
    retries: 22,
    ber: '5.6e-5',
    protocol: 'Modbus-RTU',
    isRoot: false,
    isProxy: false,
    parentName: 'CCO-01',
    x: 880,
    y: 200,
    subCarrierSnr: [21, 21, 21, 21, 20, 20, 22, 22, 21, 21, 20, 21, 21, 22, 21, 21]
  }
]);

// Noise Events
const noiseEvents = ref<NoiseEvent[]>([
  {
    id: 'noise-01',
    type: 'PWM_HARMONIC',
    severity: 'WARNING',
    source: 'Solar Inverter #01',
    phase: 'B',
    noiseFloorRiseDb: 18.2,
    timestamp: '14:32:05',
    description: '20kHz PWM switching noise burst detected on Feeder #2.'
  },
  {
    id: 'noise-02',
    type: 'IMPULSE_SPIKE',
    severity: 'CRITICAL',
    source: 'BTM Arc Welder Feeder',
    phase: 'B',
    noiseFloorRiseDb: 24.5,
    timestamp: '14:18:22',
    description: 'Transient impulsive high-frequency arc interference exceeding 30MHz band.'
  },
  {
    id: 'noise-03',
    type: 'CAPACITIVE_LEAKAGE',
    severity: 'WARNING',
    source: 'Underground Cable Box',
    phase: 'C',
    noiseFloorRiseDb: 12.0,
    timestamp: '13:55:10',
    description: 'Phase B to C inter-carrier crosstalk elevated during heavy load shift.'
  }
]);

// Computed Filtered Nodes
const filteredNodes = computed(() => {
  return nodes.value.filter(n => {
    // Tech filter
    if (filterTech.value !== 'ALL' && n.tech !== filterTech.value) return false;
    // Phase filter
    if (filterPhase.value !== 'ALL' && n.phase !== filterPhase.value) return false;
    // Quality filter
    if (filterQuality.value === 'GOOD' && n.snrDb <= 20) return false;
    if (filterQuality.value === 'MEDIUM' && (n.snrDb < 10 || n.snrDb > 20)) return false;
    if (filterQuality.value === 'POOR' && n.snrDb >= 10) return false;
    // Search query
    if (searchQuery.value) {
      const q = searchQuery.value.toLowerCase();
      return n.name.toLowerCase().includes(q) || n.mac.toLowerCase().includes(q) || n.phaseLabel.toLowerCase().includes(q);
    }
    return true;
  });
});

// Computed Topology Links
const topologyLinks = computed<TopologyLink[]>(() => {
  const links: TopologyLink[] = [];
  const nodeMap = new Map(nodes.value.map(n => [n.name, n]));
  
  for (const node of filteredNodes.value) {
    if (node.parentName && nodeMap.has(node.parentName)) {
      const parent = nodeMap.get(node.parentName)!;
      links.push({
        id: `link-${parent.id}-${node.id}`,
        sourceId: parent.id,
        targetId: node.id,
        sourceX: parent.x,
        sourceY: parent.y,
        targetX: node.x,
        targetY: node.y,
        snrDb: node.snrDb
      });
    }
  }
  return links;
});

// Computed Topology Overall Statistics
const topologyStats = computed(() => {
  const total = nodes.value.length;
  const online = nodes.value.filter(n => n.snrDb > 5).length;
  const offline = total - online;
  const snrList = nodes.value.map(n => n.snrDb);
  const avgSnr = snrList.reduce((acc, s) => acc + s, 0) / (total || 1);
  const minSnr = Math.min(...snrList);
  const maxSnr = Math.max(...snrList);
  const maxHops = Math.max(...nodes.value.map(n => n.hops));
  const avgLoss = nodes.value.reduce((acc, n) => acc + n.lossRatePct, 0) / (total || 1);
  const totalRetries = nodes.value.reduce((acc, n) => acc + n.retries, 0);
  const crosstalk = 14.8;

  return {
    totalNodes: total,
    onlineNodes: online,
    offlineNodes: offline,
    avgSnrDb: avgSnr,
    minSnrDb: minSnr,
    maxSnrDb: maxSnr,
    maxHops,
    avgLossPct: avgLoss,
    totalRetries,
    crosstalkLevelDb: crosstalk
  };
});

// Methods & Helpers
function togglePolling() {
  isPolling.value = !isPolling.value;
}

function selectNode(node: TopologyNode) {
  selectedNode.value = node;
}

function getNodeFillColor(node: TopologyNode) {
  if (node.isRoot) return '#064e3b';
  if (node.snrDb > 20) return '#065f46';
  if (node.snrDb > 10) return '#78350f';
  return '#881337';
}

function getNodeStrokeColor(node: TopologyNode) {
  if (node.isRoot) return '#10b981';
  if (node.snrDb > 20) return '#34d399';
  if (node.snrDb > 10) return '#fbbf24';
  return '#f43f5e';
}

function getNodeStatusDotClass(node: TopologyNode) {
  if (node.snrDb > 20) return 'bg-emerald-400';
  if (node.snrDb > 10) return 'bg-amber-400';
  return 'bg-rose-500 animate-ping';
}

function getPhaseColor(phase: TopologyNode['phase']) {
  switch (phase) {
    case 'PHASE_A': return '#60a5fa';
    case 'PHASE_B': return '#c084fc';
    case 'PHASE_C': return '#fb923c';
    case 'WIRELESS': return '#22d3ee';
    default: return '#34d399';
  }
}

function getPhaseColorClass(phase: TopologyNode['phase']) {
  switch (phase) {
    case 'PHASE_A': return 'text-blue-400 font-bold';
    case 'PHASE_B': return 'text-purple-400 font-bold';
    case 'PHASE_C': return 'text-orange-400 font-bold';
    case 'WIRELESS': return 'text-cyan-400 font-bold';
    default: return 'text-emerald-400 font-bold';
  }
}

function getLinkColor(snr: number) {
  if (snr > 20) return '#10b981';
  if (snr > 10) return '#f59e0b';
  return '#f43f5e';
}

function getLinkWidth(snr: number) {
  if (snr > 20) return 2.0;
  if (snr > 10) return 1.5;
  return 1.0;
}

function getSnrHeatmapColor(snr: number) {
  if (snr <= 5) return '#e11d48';
  if (snr <= 10) return '#f43f5e';
  if (snr <= 15) return '#f59e0b';
  if (snr <= 20) return '#eab308';
  if (snr <= 25) return '#10b981';
  if (snr <= 30) return '#059669';
  return '#06b6d4';
}

function triggerPingProbe(node: TopologyNode) {
  alert(`[EdgeOS DCU] Dispatched high-priority DLT698.45 Echo Probe to ${node.name} (${node.mac}). Result: Round-trip 42ms, SNR ${node.snrDb}dB.`);
}

function recalibrateNotch(node: TopologyNode) {
  alert(`[EdgeOS DCU] Initiated OFDM Adaptive Notch Recalibration on ${node.name}. Subcarrier mask optimized.`);
}

function exportTopologyReport() {
  const payload = {
    exportTime: new Date().toISOString(),
    standard: 'SGCC/CSG HPLC Dual-Mode Mesh Specification',
    stats: topologyStats.value,
    nodes: nodes.value,
    sha256Digest: 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `channel_topology_audit_${Date.now()}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

// Dynamic polling jitter
function runLivePoll() {
  if (!isPolling.value) return;
  for (const node of nodes.value) {
    if (!node.isRoot) {
      // Small physical jitter on SNR
      const delta = (Math.random() - 0.5) * 0.4;
      node.snrDb = Number(Math.max(5, Math.min(45, node.snrDb + delta)).toFixed(1));
    }
  }
}

onMounted(() => {
  pollTimer = setInterval(runLivePoll, refreshIntervalMs.value);
});

onUnmounted(() => {
  if (pollTimer) clearInterval(pollTimer);
});
</script>

<style scoped>
.font-ui {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
}
</style>
