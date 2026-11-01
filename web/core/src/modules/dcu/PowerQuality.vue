<template>
  <div class="min-h-full bg-[#020617] text-[#f8fafc] p-6 lg:p-10 space-y-8 animate-in fade-in duration-700 font-ui">
    <!-- Top Hero Header -->
    <section class="relative bg-[#0f172a] rounded-[2.5rem] overflow-hidden shadow-2xl border border-[#334155]/60 p-8 lg:p-10">
      <div class="absolute top-0 right-0 w-[500px] h-[500px] bg-violet-500/10 rounded-full blur-[120px] -mr-40 -mt-40 pointer-events-none"></div>
      <div class="absolute bottom-0 left-0 w-[300px] h-[300px] bg-cyan-500/10 rounded-full blur-[80px] -ml-20 -mb-20 pointer-events-none"></div>

      <div class="relative z-10 flex flex-col md:flex-row items-start md:items-center justify-between gap-6">
        <div class="space-y-2">
          <div class="inline-flex items-center gap-2 px-3.5 py-1 rounded-full bg-violet-500/10 border border-violet-500/20 text-violet-400 text-[10px] font-black uppercase tracking-[0.2em]">
            <span class="w-1.5 h-1.5 rounded-full bg-violet-400 animate-pulse"></span>
            GB/T 15543 & 14549 Power Quality Engine
          </div>
          <h1 class="text-3xl lg:text-4xl font-black text-white tracking-tight leading-tight font-mono">
            THREE-PHASE <span class="text-violet-400 font-sans">POWER QUALITY</span>
          </h1>
          <p class="text-slate-400 text-xs lg:text-sm font-medium">
            Symmetrical sequence phasor diagram, 2~31th harmonic spectrum, and unbalance phase-commutation optimizer.
          </p>
        </div>

        <!-- Metric Badges -->
        <div class="flex flex-wrap items-center gap-4 font-mono text-xs">
          <div class="bg-slate-950/80 border border-slate-800 px-4 py-2.5 rounded-2xl flex items-center gap-3">
            <span class="text-slate-500">Frequency:</span>
            <span class="text-emerald-400 font-bold text-sm">{{ gridFrequency.toFixed(2) }} Hz</span>
          </div>
          <div class="bg-slate-950/80 border border-slate-800 px-4 py-2.5 rounded-2xl flex items-center gap-3">
            <span class="text-slate-500">THD-U:</span>
            <span :class="thdVoltageTotal > 5.0 ? 'text-rose-400' : 'text-cyan-400'" class="font-bold text-sm">
              {{ thdVoltageTotal.toFixed(2) }}%
            </span>
          </div>
          <div class="bg-slate-950/80 border border-slate-800 px-4 py-2.5 rounded-2xl flex items-center gap-3">
            <span class="text-slate-500">I-Unbalance:</span>
            <span :class="currentUnbalancePct > 15.0 ? 'text-amber-400 font-black' : 'text-emerald-400 font-bold'" class="text-sm">
              {{ currentUnbalancePct.toFixed(1) }}%
            </span>
          </div>
        </div>
      </div>
    </section>

    <!-- Row 1: Phasor Diagram & Symmetrical Components & Realtime Telemetry -->
    <div class="grid grid-cols-1 lg:grid-cols-12 gap-8">
      <!-- Phasor Polar Vector Diagram (5 cols) -->
      <div class="lg:col-span-5 bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 shadow-2xl flex flex-col justify-between">
        <div class="flex items-center justify-between border-b border-slate-800 pb-4 mb-4">
          <div class="flex items-center gap-2.5">
            <span class="text-lg">🧭</span>
            <div>
              <h2 class="text-sm font-bold text-white font-mono">PHASOR POLAR DIAGRAM</h2>
              <p class="text-[10px] text-slate-500 font-mono">Three-Phase Voltage & Current Vectors (0°..360°)</p>
            </div>
          </div>
          <div class="flex items-center gap-2 text-[10px] font-mono">
            <span class="flex items-center gap-1 text-amber-400"><span class="w-2 h-2 rounded-full bg-amber-400"></span>Va/Ia</span>
            <span class="flex items-center gap-1 text-emerald-400"><span class="w-2 h-2 rounded-full bg-emerald-400"></span>Vb/Ib</span>
            <span class="flex items-center gap-1 text-rose-400"><span class="w-2 h-2 rounded-full bg-rose-400"></span>Vc/Ic</span>
          </div>
        </div>

        <!-- SVG Polar Vector Canvas -->
        <div class="relative flex items-center justify-center p-2">
          <svg viewBox="-160 -160 320 320" class="w-full max-w-[340px] aspect-square">
            <!-- Concentric Grid Rings -->
            <circle cx="0" cy="0" r="140" fill="none" stroke="#1e293b" stroke-width="1.5" stroke-dasharray="4 4" />
            <circle cx="0" cy="0" r="105" fill="none" stroke="#1e293b" stroke-width="1" />
            <circle cx="0" cy="0" r="70" fill="none" stroke="#1e293b" stroke-width="1" stroke-dasharray="4 4" />
            <circle cx="0" cy="0" r="35" fill="none" stroke="#1e293b" stroke-width="0.75" />

            <!-- Polar Axes -->
            <line x1="-150" y1="0" x2="150" y2="0" stroke="#334155" stroke-width="1" />
            <line x1="0" y1="-150" x2="0" y2="150" stroke="#334155" stroke-width="1" />
            <!-- Diagonal Axes -->
            <line x1="-106" y1="-106" x2="106" y2="106" stroke="#1e293b" stroke-width="0.75" stroke-dasharray="2 2" />
            <line x1="-106" y1="106" x2="106" y2="-106" stroke="#1e293b" stroke-width="0.75" stroke-dasharray="2 2" />

            <!-- Angle Labels -->
            <text x="145" y="-6" fill="#64748b" font-size="8" font-family="monospace" text-anchor="end">0°</text>
            <text x="6" y="-140" fill="#64748b" font-size="8" font-family="monospace">90°</text>
            <text x="-145" y="-6" fill="#64748b" font-size="8" font-family="monospace">180°</text>
            <text x="6" y="145" fill="#64748b" font-size="8" font-family="monospace">270°</text>

            <!-- Voltage Vector Va (Phase A: 0 deg) -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(voltageA, angleVa, 130, 260)" 
              :y2="getPhasorY(voltageA, angleVa, 130, 260)" 
              stroke="#fbbf24" stroke-width="2.5" stroke-linecap="round"
            />
            <circle 
              :cx="getPhasorX(voltageA, angleVa, 130, 260)" 
              :cy="getPhasorY(voltageA, angleVa, 130, 260)" 
              r="3.5" fill="#fbbf24" 
            />

            <!-- Voltage Vector Vb (Phase B: 240 deg / -120 deg) -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(voltageB, angleVb, 130, 260)" 
              :y2="getPhasorY(voltageB, angleVb, 130, 260)" 
              stroke="#34d399" stroke-width="2.5" stroke-linecap="round"
            />
            <circle 
              :cx="getPhasorX(voltageB, angleVb, 130, 260)" 
              :cy="getPhasorY(voltageB, angleVb, 130, 260)" 
              r="3.5" fill="#34d399" 
            />

            <!-- Voltage Vector Vc (Phase C: 120 deg) -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(voltageC, angleVc, 130, 260)" 
              :y2="getPhasorY(voltageC, angleVc, 130, 260)" 
              stroke="#f43f5e" stroke-width="2.5" stroke-linecap="round"
            />
            <circle 
              :cx="getPhasorX(voltageC, angleVc, 130, 260)" 
              :cy="getPhasorY(voltageC, angleVc, 130, 260)" 
              r="3.5" fill="#f43f5e" 
            />

            <!-- Current Vector Ia -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(currentA, angleIa, 90, 100)" 
              :y2="getPhasorY(currentA, angleIa, 90, 100)" 
              stroke="#f59e0b" stroke-width="1.8" stroke-dasharray="3 2" stroke-linecap="round"
            />
            <polygon 
              :points="getArrowPoints(0, 0, getPhasorX(currentA, angleIa, 90, 100), getPhasorY(currentA, angleIa, 90, 100))" 
              fill="#f59e0b" 
            />

            <!-- Current Vector Ib -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(currentB, angleIb, 90, 100)" 
              :y2="getPhasorY(currentB, angleIb, 90, 100)" 
              stroke="#10b981" stroke-width="1.8" stroke-dasharray="3 2" stroke-linecap="round"
            />
            <polygon 
              :points="getArrowPoints(0, 0, getPhasorX(currentB, angleIb, 90, 100), getPhasorY(currentB, angleIb, 90, 100))" 
              fill="#10b981" 
            />

            <!-- Current Vector Ic -->
            <line 
              x1="0" y1="0" 
              :x2="getPhasorX(currentC, angleIc, 90, 100)" 
              :y2="getPhasorY(currentC, angleIc, 90, 100)" 
              stroke="#e11d48" stroke-width="1.8" stroke-dasharray="3 2" stroke-linecap="round"
            />
            <polygon 
              :points="getArrowPoints(0, 0, getPhasorX(currentC, angleIc, 90, 100), getPhasorY(currentC, angleIc, 90, 100))" 
              fill="#e11d48" 
            />

            <!-- Center Hub -->
            <circle cx="0" cy="0" r="4" fill="#38bdf8" />
          </svg>
        </div>

        <div class="grid grid-cols-3 gap-2 mt-4 pt-4 border-t border-slate-800 text-center font-mono text-[11px]">
          <div class="bg-slate-950/60 p-2 rounded-xl border border-amber-500/20">
            <div class="text-amber-400 font-bold">Phase A</div>
            <div class="text-slate-300 font-bold mt-1">{{ voltageA.toFixed(1) }} V</div>
            <div class="text-slate-400">{{ currentA.toFixed(1) }} A</div>
            <div class="text-slate-500 text-[9px] mt-0.5">cosφ {{ pfA.toFixed(2) }}</div>
          </div>
          <div class="bg-slate-950/60 p-2 rounded-xl border border-emerald-500/20">
            <div class="text-emerald-400 font-bold">Phase B</div>
            <div class="text-slate-300 font-bold mt-1">{{ voltageB.toFixed(1) }} V</div>
            <div class="text-slate-400">{{ currentB.toFixed(1) }} A</div>
            <div class="text-slate-500 text-[9px] mt-0.5">cosφ {{ pfB.toFixed(2) }}</div>
          </div>
          <div class="bg-slate-950/60 p-2 rounded-xl border border-rose-500/20">
            <div class="text-rose-400 font-bold">Phase C</div>
            <div class="text-slate-300 font-bold mt-1">{{ voltageC.toFixed(1) }} V</div>
            <div class="text-slate-400">{{ currentC.toFixed(1) }} A</div>
            <div class="text-slate-500 text-[9px] mt-0.5">cosφ {{ pfC.toFixed(2) }}</div>
          </div>
        </div>
      </div>

      <!-- Symmetrical Sequence Components & Unbalance Analysis (7 cols) -->
      <div class="lg:col-span-7 space-y-6">
        <!-- Sequence Components Decomposition -->
        <div class="bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 shadow-2xl space-y-4">
          <div class="flex items-center justify-between border-b border-slate-800 pb-3">
            <div class="flex items-center gap-2">
              <span class="text-cyan-400 font-mono text-sm font-bold">⚡ SYMMETRICAL SEQUENCE DECOMPOSITION</span>
              <span class="text-[10px] bg-cyan-500/10 text-cyan-400 px-2 py-0.5 rounded-full font-mono">GB/T 15543</span>
            </div>
            <span class="text-[10px] text-slate-500 font-mono">Fortescue Theorem (U1, U2, U0)</span>
          </div>

          <div class="grid grid-cols-1 md:grid-cols-3 gap-4 font-mono">
            <!-- Positive Sequence U1 -->
            <div class="bg-slate-950 border border-slate-800/80 p-4 rounded-2xl space-y-1 relative overflow-hidden">
              <div class="absolute top-0 right-0 w-16 h-16 bg-emerald-500/10 rounded-full blur-xl"></div>
              <div class="text-slate-500 text-[10px] uppercase font-bold tracking-wider">Positive Seq (正序 U1)</div>
              <div class="text-2xl font-black text-emerald-400">{{ posSeqVoltage.toFixed(1) }} <span class="text-xs text-slate-400">V</span></div>
              <div class="text-[10px] text-slate-400">I1: <strong class="text-emerald-300">{{ posSeqCurrent.toFixed(1) }} A</strong></div>
              <div class="text-[9px] text-slate-500">Fundamental working power</div>
            </div>

            <!-- Negative Sequence U2 -->
            <div class="bg-slate-950 border border-slate-800/80 p-4 rounded-2xl space-y-1 relative overflow-hidden">
              <div class="absolute top-0 right-0 w-16 h-16 bg-rose-500/10 rounded-full blur-xl"></div>
              <div class="text-slate-500 text-[10px] uppercase font-bold tracking-wider">Negative Seq (负序 U2)</div>
              <div class="text-2xl font-black" :class="negSeqVoltage > 4.4 ? 'text-rose-400' : 'text-slate-200'">
                {{ negSeqVoltage.toFixed(2) }} <span class="text-xs text-slate-400">V</span>
              </div>
              <div class="text-[10px] text-slate-400">I2: <strong class="text-rose-300">{{ negSeqCurrent.toFixed(2) }} A</strong></div>
              <div class="text-[9px]" :class="negSeqVoltage > 4.4 ? 'text-rose-400' : 'text-slate-500'">
                Unbalance ({{ (negSeqVoltage / posSeqVoltage * 100).toFixed(2) }}%)
              </div>
            </div>

            <!-- Zero Sequence U0 -->
            <div class="bg-slate-950 border border-slate-800/80 p-4 rounded-2xl space-y-1 relative overflow-hidden">
              <div class="absolute top-0 right-0 w-16 h-16 bg-amber-500/10 rounded-full blur-xl"></div>
              <div class="text-slate-500 text-[10px] uppercase font-bold tracking-wider">Zero Seq (零序 U0 / In)</div>
              <div class="text-2xl font-black text-amber-400">{{ zeroSeqVoltage.toFixed(2) }} <span class="text-xs text-slate-400">V</span></div>
              <div class="text-[10px] text-slate-400">In (Neutral): <strong class="text-amber-300">{{ neutralCurrent.toFixed(1) }} A</strong></div>
              <div class="text-[9px] text-slate-500">Neutral line current load</div>
            </div>
          </div>

          <!-- Unbalance Rate Progression Bars -->
          <div class="bg-slate-950/70 border border-slate-800/80 rounded-2xl p-4 space-y-3 font-mono text-xs">
            <div>
              <div class="flex justify-between text-slate-400 mb-1 text-[11px]">
                <span>Voltage Unbalance Rate (VUR - 电压不平衡度)</span>
                <span :class="voltageUnbalancePct > 2.0 ? 'text-rose-400 font-bold' : 'text-cyan-400'">
                  {{ voltageUnbalancePct.toFixed(2) }}% (Limit: 2.0%)
                </span>
              </div>
              <div class="h-2 bg-slate-900 rounded-full overflow-hidden flex">
                <div 
                  class="transition-all duration-500 rounded-full" 
                  :class="voltageUnbalancePct > 2.0 ? 'bg-rose-500' : 'bg-cyan-400'"
                  :style="{ width: Math.min(100, (voltageUnbalancePct / 4.0) * 100) + '%' }"
                ></div>
              </div>
            </div>

            <div>
              <div class="flex justify-between text-slate-400 mb-1 text-[11px]">
                <span>Current Unbalance Rate (CUR - 三相电流不平衡度)</span>
                <span :class="currentUnbalancePct > 15.0 ? 'text-amber-400 font-black' : 'text-emerald-400'">
                  {{ currentUnbalancePct.toFixed(1) }}% (Target: &le;15.0%)
                </span>
              </div>
              <div class="h-2 bg-slate-900 rounded-full overflow-hidden flex">
                <div 
                  class="transition-all duration-500 rounded-full" 
                  :class="currentUnbalancePct > 15.0 ? 'bg-amber-500' : 'bg-emerald-400'"
                  :style="{ width: Math.min(100, (currentUnbalancePct / 40.0) * 100) + '%' }"
                ></div>
              </div>
            </div>
          </div>
        </div>

        <!-- Power Summary Grid -->
        <div class="bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 shadow-2xl grid grid-cols-2 sm:grid-cols-4 gap-4 font-mono">
          <div class="bg-slate-950 p-3 rounded-2xl border border-slate-800 text-center">
            <div class="text-slate-500 text-[10px]">Active Power P</div>
            <div class="text-white font-bold text-base mt-1">{{ (activePowerTotal / 1000).toFixed(2) }} <span class="text-[10px] text-slate-400">kW</span></div>
          </div>
          <div class="bg-slate-950 p-3 rounded-2xl border border-slate-800 text-center">
            <div class="text-slate-500 text-[10px]">Reactive Power Q</div>
            <div class="text-cyan-400 font-bold text-base mt-1">{{ (reactivePowerTotal / 1000).toFixed(2) }} <span class="text-[10px] text-slate-400">kvar</span></div>
          </div>
          <div class="bg-slate-950 p-3 rounded-2xl border border-slate-800 text-center">
            <div class="text-slate-500 text-[10px]">Apparent Power S</div>
            <div class="text-purple-400 font-bold text-base mt-1">{{ (apparentPowerTotal / 1000).toFixed(2) }} <span class="text-[10px] text-slate-400">kVA</span></div>
          </div>
          <div class="bg-slate-950 p-3 rounded-2xl border border-slate-800 text-center">
            <div class="text-slate-500 text-[10px]">Total PF (cosφ)</div>
            <div class="text-emerald-400 font-bold text-base mt-1">{{ powerFactorTotal.toFixed(3) }}</div>
          </div>
        </div>
      </div>
    </div>

    <!-- Row 2: Harmonic Spectrum Bar Chart (2~31th Order) -->
    <div class="bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 lg:p-8 shadow-2xl space-y-6">
      <div class="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div class="flex items-center gap-2">
            <span class="text-lg">📊</span>
            <h2 class="text-base font-bold text-white font-mono">2~31st HARMONIC SPECTRUM ANALYSIS</h2>
            <span class="text-[10px] bg-violet-500/10 text-violet-400 border border-violet-500/20 px-2 py-0.5 rounded-full font-mono">
              GB/T 14549-1993 Standard
            </span>
          </div>
          <p class="text-slate-500 text-xs mt-0.5 font-mono">Individual Harmonic Ratio (HRh %) with GB/T 14549 5% & 3.2% Limit Lines</p>
        </div>

        <div class="flex items-center gap-4 text-xs font-mono">
          <div class="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800">
            <span class="text-slate-500">Channel:</span>
            <button 
              @click="selectedHarmonicChannel = 'Ua'" 
              :class="selectedHarmonicChannel === 'Ua' ? 'text-amber-400 font-bold' : 'text-slate-400'"
              class="px-1.5 py-0.5 rounded hover:text-amber-300"
            >Va</button>
            <span class="text-slate-700">|</span>
            <button 
              @click="selectedHarmonicChannel = 'Ia'" 
              :class="selectedHarmonicChannel === 'Ia' ? 'text-cyan-400 font-bold' : 'text-slate-400'"
              class="px-1.5 py-0.5 rounded hover:text-cyan-300"
            >Ia</button>
          </div>

          <div class="bg-slate-950 px-3 py-1.5 rounded-xl border border-slate-800 flex items-center gap-3 text-[11px]">
            <span>Odd THD: <strong class="text-amber-400">{{ thdOddPct.toFixed(2) }}%</strong></span>
            <span>Even THD: <strong class="text-cyan-400">{{ thdEvenPct.toFixed(2) }}%</strong></span>
          </div>
        </div>
      </div>

      <!-- Harmonic Bars Canvas / SVG Chart -->
      <div class="relative w-full h-64 bg-slate-950/80 rounded-2xl border border-slate-800/80 p-4 pt-8">
        <!-- Standard Limit Line (5.0% THD limit / 3.2% Odd harmonic limit) -->
        <div class="absolute left-10 right-4 top-[35%] border-b border-rose-500/60 border-dashed pointer-events-none flex justify-end">
          <span class="text-[9px] text-rose-400 font-mono -mt-4 bg-slate-950 px-1 border border-rose-500/30 rounded">
            GB/T 14549 Limit (3.2%)
          </span>
        </div>

        <!-- 30 Bar Items for Orders 2 to 31 -->
        <div class="h-full flex items-end justify-between gap-1 pl-6 pr-2">
          <div 
            v-for="order in 30" 
            :key="order + 1"
            class="flex-1 flex flex-col items-center h-full justify-end group relative cursor-pointer"
            @mouseenter="hoveredHarmonic = { order: order + 1, pct: harmonicPcts[order + 1] || 0.1, val: ((harmonicPcts[order + 1] || 0.1) * 2.2).toFixed(2) }"
            @mouseleave="hoveredHarmonic = null"
          >
            <!-- Bar Tooltip -->
            <div 
              v-if="hoveredHarmonic && hoveredHarmonic.order === (order + 1)"
              class="absolute bottom-full mb-2 bg-slate-900 border border-slate-700 text-white font-mono text-[10px] p-2 rounded-xl shadow-2xl z-30 pointer-events-none whitespace-nowrap"
            >
              <div class="text-cyan-400 font-bold">Order {{ order + 1 }} ({{ (order + 1) * 50 }} Hz)</div>
              <div>Ratio: {{ hoveredHarmonic.pct.toFixed(2) }}%</div>
              <div>Magnitude: {{ hoveredHarmonic.val }} {{ selectedHarmonicChannel.startsWith('U') ? 'V' : 'A' }}</div>
            </div>

            <!-- The Bar -->
            <div 
              class="w-full rounded-t transition-all duration-300 group-hover:brightness-125"
              :class="getHarmonicBarColor(order + 1, harmonicPcts[order + 1] || 0.1)"
              :style="{ height: Math.min(100, ((harmonicPcts[order + 1] || 0.1) / 6.0) * 100) + '%' }"
            ></div>

            <!-- X Axis Label (Order number) -->
            <span class="text-[9px] text-slate-500 font-mono mt-1 group-hover:text-cyan-400">
              {{ (order + 1) % 5 === 0 || order + 1 === 3 || order + 1 === 7 ? order + 1 : '' }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- Row 3: Smart Phase Commutation Optimizer & Transient Event Log -->
    <div class="grid grid-cols-1 lg:grid-cols-12 gap-8">
      <!-- Smart Phase Switching Decision Engine (7 cols) -->
      <div class="lg:col-span-7 bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 shadow-2xl space-y-6">
        <div class="flex items-center justify-between border-b border-slate-800 pb-4">
          <div class="flex items-center gap-2.5">
            <span class="text-lg">🔀</span>
            <div>
              <h2 class="text-sm font-bold text-white font-mono">AUTOMATIC PHASE-SWITCHING OPTIMIZER</h2>
              <p class="text-[10px] text-slate-500 font-mono">Real-time load commutation algorithm for three-phase rebalancing</p>
            </div>
          </div>
          <button 
            @click="solveAndSimulateSwitch" 
            class="px-3.5 py-1.5 bg-violet-600 hover:bg-violet-500 text-white font-mono text-xs font-bold rounded-xl transition shadow-lg flex items-center gap-1.5"
          >
            <span>🔄 求解最优重构方案</span>
          </button>
        </div>

        <!-- Decision Summary Card -->
        <div 
          class="p-4 rounded-2xl border transition-all font-mono text-xs space-y-3"
          :class="isSwitchRecommended ? 'bg-amber-500/10 border-amber-500/30 text-amber-200' : 'bg-slate-950 border-slate-800 text-slate-400'"
        >
          <div class="flex items-center justify-between font-bold">
            <div class="flex items-center gap-2">
              <span class="w-2 h-2 rounded-full" :class="isSwitchRecommended ? 'bg-amber-400 animate-ping' : 'bg-emerald-400'"></span>
              <span>{{ isSwitchRecommended ? 'REBALANCING ACTION REQUIRED (建议换相)' : 'SYSTEM BALANCED (三相负荷均衡)' }}</span>
            </div>
            <span class="text-[10px] px-2 py-0.5 rounded bg-slate-900 border border-slate-700 text-slate-300">
              Unbalance: {{ currentUnbalancePct.toFixed(1) }}% &rarr; Projected: {{ projectedUnbalancePct.toFixed(1) }}%
            </span>
          </div>

          <div v-if="isSwitchRecommended && recommendedPlan" class="bg-slate-950/90 p-3.5 rounded-xl border border-amber-500/20 text-slate-300 space-y-2">
            <div class="text-[11px] text-amber-400 font-bold">🎯 推荐换相动作 (Optimal Commutation Step):</div>
            <div class="flex flex-wrap items-center gap-3 text-xs">
              <span class="bg-slate-900 px-2.5 py-1 rounded border border-slate-800">
                支路电表: <strong class="text-white">#{{ recommendedPlan.meterId }}</strong>
              </span>
              <span class="bg-slate-900 px-2.5 py-1 rounded border border-slate-800">
                迁移相序: <strong class="text-amber-400">相 {{ recommendedPlan.fromPhase }}</strong> &rarr; <strong class="text-emerald-400">相 {{ recommendedPlan.toPhase }}</strong>
              </span>
              <span class="bg-slate-900 px-2.5 py-1 rounded border border-slate-800">
                迁移负荷: <strong class="text-cyan-400">{{ recommendedPlan.amps.toFixed(1) }} A</strong>
              </span>
            </div>
            <div class="text-[10px] text-slate-400">
              实施该动作后，最大相负荷将从 {{ currentA.toFixed(1) }}A 降低至 {{ (currentA - recommendedPlan.amps).toFixed(1) }}A，三相不平衡度降至 {{ projectedUnbalancePct.toFixed(1) }}%，满足 GB/T 15543 &le;15% 规范。
            </div>

            <div class="pt-2 flex justify-end">
              <button 
                @click="executeSwitchAction" 
                class="px-4 py-2 bg-emerald-600 hover:bg-emerald-500 text-white font-bold rounded-xl text-xs transition shadow-lg flex items-center gap-2"
              >
                <span>⚡ 下发智能换相开关遥控指令</span>
              </button>
            </div>
          </div>
          <div v-else class="text-xs text-slate-500">
            当前三相电流不平衡度未超过 15.0% 越限门槛，无需执行物理换相开关动作。
          </div>
        </div>

        <!-- Switchable Branches List -->
        <div class="space-y-2 font-mono text-xs">
          <div class="text-slate-400 text-[11px] font-bold">可换相单相支路负荷实时状态 (Switchable Branch Units):</div>
          <div class="grid grid-cols-1 sm:grid-cols-2 gap-2 max-h-48 overflow-y-auto pr-1">
            <div 
              v-for="branch in switchableBranches" 
              :key="branch.meterId"
              class="bg-slate-950 p-3 rounded-xl border border-slate-800/80 flex items-center justify-between"
            >
              <div class="flex items-center gap-2">
                <span 
                  class="w-2 h-2 rounded-full"
                  :class="branch.phase === 'A' ? 'bg-amber-400' : branch.phase === 'B' ? 'bg-emerald-400' : 'bg-rose-400'"
                ></span>
                <div>
                  <div class="text-slate-200 font-bold">Meter #{{ branch.meterId }}</div>
                  <div class="text-[10px] text-slate-500">Phase {{ branch.phase }}</div>
                </div>
              </div>
              <div class="text-right">
                <div class="text-cyan-400 font-bold">{{ branch.currentAmps.toFixed(1) }} A</div>
                <div class="text-[10px] text-slate-500">{{ (branch.currentAmps * 0.22).toFixed(1) }} kW</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Transient Events Log (5 cols) -->
      <div class="lg:col-span-5 bg-slate-900/90 border border-slate-800/90 rounded-3xl p-6 shadow-2xl flex flex-col justify-between space-y-4">
        <div class="flex items-center justify-between border-b border-slate-800 pb-4">
          <div class="flex items-center gap-2.5">
            <span class="text-lg">⚡</span>
            <div>
              <h2 class="text-sm font-bold text-white font-mono">TRANSIENT PQ EVENTS</h2>
              <p class="text-[10px] text-slate-500 font-mono">Voltage Sag / Swell / Interruption SOE Records</p>
            </div>
          </div>
          <span class="text-[10px] bg-slate-950 px-2 py-1 rounded border border-slate-800 text-slate-400 font-mono">
            {{ transientEvents.length }} Events
          </span>
        </div>

        <div class="space-y-2 font-mono text-xs overflow-y-auto max-h-80">
          <div 
            v-for="(evt, idx) in transientEvents" 
            :key="idx"
            class="bg-slate-950 p-3 rounded-2xl border border-slate-800/80 space-y-1.5"
          >
            <div class="flex items-center justify-between">
              <span 
                class="px-2 py-0.5 rounded text-[10px] font-black"
                :class="evt.type === 'SAG' ? 'bg-rose-500/10 text-rose-400 border border-rose-500/30' : evt.type === 'SWELL' ? 'bg-amber-500/10 text-amber-400 border border-amber-500/30' : 'bg-purple-500/10 text-purple-400 border border-purple-500/30'"
              >
                {{ evt.type === 'SAG' ? '📉 VOLTAGE SAG (暂降)' : evt.type === 'SWELL' ? '📈 VOLTAGE SWELL (暂升)' : '⚠️ INTERRUPTION (中断)' }}
              </span>
              <span class="text-[10px] text-slate-500">{{ evt.time }}</span>
            </div>
            <div class="flex items-center justify-between text-[11px] text-slate-300">
              <span>Duration: <strong class="text-cyan-400">{{ evt.durationMs }} ms</strong></span>
              <span>Extreme V: <strong :class="evt.type === 'SAG' ? 'text-rose-400' : 'text-amber-400'">{{ evt.extremeVal }} V</strong></span>
            </div>
          </div>
        </div>

        <div class="pt-2 border-t border-slate-800 flex justify-between items-center text-[10px] font-mono text-slate-500">
          <span>Standard: IEC 61000-4-30 Class A</span>
          <span class="text-emerald-400">● 100ms RMS Continuous Window</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';

// State & Physical Parameters
const gridFrequency = ref(50.02);
const voltageA = ref(223.4);
const voltageB = ref(219.8);
const voltageC = ref(218.1);

const currentA = ref(84.2);
const currentB = ref(42.5);
const currentC = ref(36.8);

const angleVa = ref(0);
const angleVb = ref(240);
const angleVc = ref(120);

const pfA = ref(0.96);
const pfB = ref(0.94);
const pfC = ref(0.92);

const angleIa = computed(() => angleVa.value - Math.acos(pfA.value) * (180 / Math.PI));
const angleIb = computed(() => angleVb.value - Math.acos(pfB.value) * (180 / Math.PI));
const angleIc = computed(() => angleVc.value - Math.acos(pfC.value) * (180 / Math.PI));

// Symmetrical Components Calculation (Fortescue transformation approximations)
const posSeqVoltage = computed(() => (voltageA.value + voltageB.value + voltageC.value) / 3.0);
const negSeqVoltage = computed(() => {
  const diffA = Math.abs(voltageA.value - posSeqVoltage.value);
  const diffB = Math.abs(voltageB.value - posSeqVoltage.value);
  const diffC = Math.abs(voltageC.value - posSeqVoltage.value);
  return (Math.max(diffA, diffB, diffC) * 0.85);
});
const zeroSeqVoltage = computed(() => Math.abs(voltageA.value + voltageB.value + voltageC.value - posSeqVoltage.value * 3) / 3.0);

const posSeqCurrent = computed(() => (currentA.value + currentB.value + currentC.value) / 3.0);
const negSeqCurrent = computed(() => {
  const maxI = Math.max(currentA.value, currentB.value, currentC.value);
  const avgI = posSeqCurrent.value;
  return (maxI - avgI) * 0.82;
});
const neutralCurrent = computed(() => {
  // Vector sum of Ia, Ib, Ic
  const radA = (angleIa.value * Math.PI) / 180;
  const radB = (angleIb.value * Math.PI) / 180;
  const radC = (angleIc.value * Math.PI) / 180;
  const real = currentA.value * Math.cos(radA) + currentB.value * Math.cos(radB) + currentC.value * Math.cos(radC);
  const imag = currentA.value * Math.sin(radA) + currentB.value * Math.sin(radB) + currentC.value * Math.sin(radC);
  return Math.sqrt(real * real + imag * imag);
});

// Unbalance Rates
const voltageUnbalancePct = computed(() => {
  const avg = (voltageA.value + voltageB.value + voltageC.value) / 3.0;
  const maxDiff = Math.max(Math.abs(voltageA.value - avg), Math.abs(voltageB.value - avg), Math.abs(voltageC.value - avg));
  return (maxDiff / avg) * 100;
});

const currentUnbalancePct = computed(() => {
  const avg = (currentA.value + currentB.value + currentC.value) / 3.0;
  if (avg < 1e-3) return 0;
  const maxDiff = Math.max(Math.abs(currentA.value - avg), Math.abs(currentB.value - avg), Math.abs(currentC.value - avg));
  return (maxDiff / avg) * 100;
});

// Powers
const activePowerTotal = computed(() => {
  return voltageA.value * currentA.value * pfA.value +
         voltageB.value * currentB.value * pfB.value +
         voltageC.value * currentC.value * pfC.value;
});

const reactivePowerTotal = computed(() => {
  const sinA = Math.sqrt(1 - pfA.value * pfA.value);
  const sinB = Math.sqrt(1 - pfB.value * pfB.value);
  const sinC = Math.sqrt(1 - pfC.value * pfC.value);
  return voltageA.value * currentA.value * sinA +
         voltageB.value * currentB.value * sinB +
         voltageC.value * currentC.value * sinC;
});

const apparentPowerTotal = computed(() => {
  return Math.sqrt(activePowerTotal.value * activePowerTotal.value + reactivePowerTotal.value * reactivePowerTotal.value);
});

const powerFactorTotal = computed(() => {
  return apparentPowerTotal.value > 0 ? activePowerTotal.value / apparentPowerTotal.value : 1.0;
});

// Harmonics State (Orders 2..31)
const selectedHarmonicChannel = ref<'Ua' | 'Ia'>('Ua');
const hoveredHarmonic = ref<{ order: number; pct: number; val: string } | null>(null);

const harmonicPcts = ref<Record<number, number>>({
  2: 0.35, 3: 2.85, 4: 0.22, 5: 3.45, 6: 0.15,
  7: 2.10, 8: 0.08, 9: 1.25, 10: 0.05, 11: 1.45,
  12: 0.04, 13: 0.95, 14: 0.03, 15: 0.65, 16: 0.02,
  17: 0.72, 18: 0.02, 19: 0.55, 20: 0.01, 21: 0.38,
  22: 0.01, 23: 0.42, 24: 0.01, 25: 0.31, 26: 0.01,
  27: 0.22, 28: 0.01, 29: 0.18, 30: 0.01, 31: 0.15
});

const thdVoltageTotal = computed(() => {
  let sumSq = 0;
  for (let i = 2; i <= 31; i++) {
    const val = harmonicPcts.value[i] || 0;
    sumSq += val * val;
  }
  return Math.sqrt(sumSq);
});

const thdOddPct = computed(() => {
  let sumSq = 0;
  for (let i = 3; i <= 31; i += 2) {
    const val = harmonicPcts.value[i] || 0;
    sumSq += val * val;
  }
  return Math.sqrt(sumSq);
});

const thdEvenPct = computed(() => {
  let sumSq = 0;
  for (let i = 2; i <= 30; i += 2) {
    const val = harmonicPcts.value[i] || 0;
    sumSq += val * val;
  }
  return Math.sqrt(sumSq);
});

function getHarmonicBarColor(order: number, pct: number): string {
  if (pct >= 3.2) return 'bg-rose-500 shadow-rose-500/50 shadow-sm';
  if (order % 2 === 1) return 'bg-amber-400';
  return 'bg-cyan-400';
}

// Polar Vector Helpers
function getPhasorX(val: number, angleDeg: number, maxRadius: number, maxVal: number): number {
  const norm = (val / maxVal) * maxRadius;
  const rad = (angleDeg * Math.PI) / 180;
  return norm * Math.cos(rad);
}

function getPhasorY(val: number, angleDeg: number, maxRadius: number, maxVal: number): number {
  const norm = (val / maxVal) * maxRadius;
  const rad = (angleDeg * Math.PI) / 180;
  return -norm * Math.sin(rad); // Inverted Y for SVG canvas
}

function getArrowPoints(x1: number, y1: number, x2: number, y2: number): string {
  const angle = Math.atan2(y2 - y1, x2 - x1);
  const headLen = 6;
  const xA = x2 - headLen * Math.cos(angle - Math.PI / 6);
  const yA = y2 - headLen * Math.sin(angle - Math.PI / 6);
  const xB = x2 - headLen * Math.cos(angle + Math.PI / 6);
  const yB = y2 - headLen * Math.sin(angle + Math.PI / 6);
  return `${x2},${y2} ${xA},${yA} ${xB},${yB}`;
}

// Switchable Branches & Optimization
const switchableBranches = ref([
  { meterId: 101, phase: 'A', currentAmps: 18.5 },
  { meterId: 102, phase: 'A', currentAmps: 24.2 },
  { meterId: 103, phase: 'A', currentAmps: 15.0 },
  { meterId: 104, phase: 'B', currentAmps: 12.8 },
  { meterId: 105, phase: 'B', currentAmps: 16.4 },
  { meterId: 106, phase: 'C', currentAmps: 11.2 },
  { meterId: 107, phase: 'C', currentAmps: 14.5 },
]);

const isSwitchRecommended = computed(() => currentUnbalancePct.value > 15.0);
const projectedUnbalancePct = ref(8.2);

const recommendedPlan = ref<{
  meterId: number;
  fromPhase: string;
  toPhase: string;
  amps: number;
} | null>({
  meterId: 102,
  fromPhase: 'A',
  toPhase: 'C',
  amps: 24.2
});

function solveAndSimulateSwitch() {
  // Simulate solving phase commutation
  if (currentUnbalancePct.value > 15.0) {
    recommendedPlan.value = {
      meterId: 102,
      fromPhase: 'A',
      toPhase: 'C',
      amps: 24.2
    };
    projectedUnbalancePct.value = 7.8;
  }
}

function executeSwitchAction() {
  if (!recommendedPlan.value) return;
  // Apply the migration to state
  const plan = recommendedPlan.value;
  const b = switchableBranches.value.find(item => item.meterId === plan.meterId);
  if (b) {
    b.phase = plan.toPhase;
    if (plan.fromPhase === 'A') currentA.value -= plan.amps;
    if (plan.toPhase === 'C') currentC.value += plan.amps;
    recommendedPlan.value = null;
  }
}

// Transient Events
const transientEvents = ref([
  { type: 'SAG', time: '14:22:08.312', durationMs: 140, extremeVal: 172.5 },
  { type: 'SWELL', time: '11:05:44.891', durationMs: 80, extremeVal: 248.6 },
  { type: 'SAG', time: '09:18:22.045', durationMs: 220, extremeVal: 158.0 }
]);
</script>
