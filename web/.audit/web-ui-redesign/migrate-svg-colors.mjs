import { readFileSync, writeFileSync } from 'node:fs';

const FILES = [
  'core/src/components/SankeyEnergyFlow.vue',
  'core/src/components/TOUArbitrageWidget.vue',
  'core/src/components/AntiReversePIDGauge.vue',
  'core/src/components/DemandPeakShavingWidget.vue',
];

const MAP = {
  '#3B82F6': 'var(--chart-1)',
  '#60A5FA': 'var(--chart-1)',
  '#7DD3FC': 'var(--chart-1)',
  '#38BDF8': 'var(--chart-1)',
  '#A855F7': 'var(--chart-4)',
  '#C084FC': 'var(--chart-4)',
  '#D8B4FE': 'var(--chart-4)',
  '#8B5CF6': 'var(--chart-4)',
  '#EC4899': 'var(--chart-4)',
  '#F472B6': 'var(--chart-4)',
  '#10B981': 'var(--chart-3)',
  '#34D399': 'var(--chart-3)',
  '#6EE7B7': 'var(--chart-3)',
  '#F97316': 'var(--chart-6)',
  '#FB923C': 'var(--chart-6)',
  '#FDBA74': 'var(--chart-6)',
  '#EF4444': 'var(--color-danger)',
  '#F59E0B': 'var(--color-warning)',
  '#0F172A': 'var(--bg-surface)',
  '#0B1329': 'var(--bg-surface)',
  '#1E293B': 'var(--bg-elevated)',
  '#334155': 'var(--border-default)',
  '#64748B': 'var(--text-disabled)',
  '#94A3B8': 'var(--text-secondary)',
  '#FFFFFF': 'var(--text-primary)',
  '#F8FAFC': 'var(--text-primary)',
};

const LABELS = {
  '☀️ SOLAR PV': '光伏',
  '🏢 UTILITY GRID': '电网',
  '🔋 ESS BESS': '储能',
  '⚡ MG BUS': '母线',
  '🏭 LOAD': '负荷',
  '🔌 EV CHARGER': '充电桩',
};

for (const file of FILES) {
  let source = readFileSync(file, 'utf8');
  source = source.replace(/#[0-9A-Fa-f]{6}/g, (hex) => MAP[hex.toUpperCase()] ?? hex);
  source = source.replace(/rgba\(255,255,255,0\.06\)/g, 'var(--border-default)');
  source = source.replace(/shadow-\[0_0_8px_var\(--text-primary\)\]/g, 'shadow-[0_0_8px_var(--color-primary)]');
  source = source.replace(/shadow-\[0_0_6px_var\(--color-danger\)\]/g, 'shadow-[0_0_6px_var(--color-danger)]');
  for (const [from, to] of Object.entries(LABELS)) source = source.split(from).join(to);
  source = source.replace(/font-weight="900"/g, 'font-weight="600"');
  source = source.replace(/font-size="(?:9|10|11)"/g, 'font-size="12"');
  source = source.replace(/font-family="JetBrains Mono, monospace"/g, 'font-family="Edge Mono, monospace"');
  source = source.replace(/font-family="sans-serif"/g, 'font-family="Edge Sans, sans-serif"');
  source = source.replace(/ class="animate-ping"/g, '');
  writeFileSync(file, source);
}
console.log('done');
