import { readFileSync, writeFileSync } from 'node:fs';
const FILES = [
  'core/src/modules/devices/ChannelTopologyHeatmap.vue',
  'core/src/modules/dcu/PowerQuality.vue',
  'core/src/modules/metrics/MetricsMatrix.vue',
];
const MAP = {
  '#1e293b': 'var(--border-default)',
  '#334155': 'var(--border-default)',
  '#0f172a': 'var(--bg-surface)',
  '#94a3b8': 'var(--text-secondary)',
  '#64748b': 'var(--text-secondary)',
  '#cbd5e1': 'var(--text-secondary)',
  '#ffffff': 'var(--text-primary)',
  '#020617': 'var(--bg-page)',
  '#10b981': 'var(--chart-3)',
  '#34d399': 'var(--chart-3)',
  '#064e3b': 'var(--color-success-bg)',
  '#065f46': 'var(--color-success-bg)',
  '#f59e0b': 'var(--chart-2)',
  '#fbbf24': 'var(--chart-2)',
  '#78350f': 'var(--color-warning-bg)',
  '#f43f5e': 'var(--chart-4)',
  '#e11d48': 'var(--color-danger)',
  '#fb7185': 'var(--chart-4)',
  '#881337': 'var(--color-danger-bg)',
  '#60a5fa': 'var(--chart-1)',
  '#22d3ee': 'var(--chart-1)',
  '#06b6d4': 'var(--chart-1)',
  '#3b82f6': 'var(--chart-1)',
  '#818cf8': 'var(--chart-4)',
  '#a855f7': 'var(--chart-4)',
  '#c084fc': 'var(--chart-4)',
};
for (const file of FILES) {
  let s = readFileSync(file, 'utf8');
  s = s.replace(/#[0-9A-Fa-f]{6}/g, (hex) => MAP[hex.toLowerCase()] ?? hex);
  s = s.replace(/font-family="monospace"/g, 'font-family="Edge Mono, monospace"');
  s = s.replace(/font-size="8"/g, 'font-size="12"');
  s = s.replace(/font-weight="900"/g, 'font-weight="600"');
  writeFileSync(file, s);
}
console.log('done');
