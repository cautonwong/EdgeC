/**
 * One-shot migration for pages written against the old hard-coded deep-black
 * palette. Rewrites only Tailwind arbitrary values ([#hex]) so script-side
 * chart colours stay explicit and are handled by the chart theme helper.
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { globSync } from 'node:fs';

const ROOTS = ['core/src', 'products/smart_box/src'];

const HEX = {
  '020617': 'var(--bg-page)',
  '050b14': 'var(--bg-page)',
  '0f172a': 'var(--bg-surface)',
  '1e293b': 'var(--bg-elevated)',
  '334155': 'var(--border-default)',
  f8fafc: 'var(--text-primary)',
  f1f5f9: 'var(--text-primary)',
  ffffff: 'var(--text-primary)',
  '94a3b8': 'var(--text-secondary)',
  '64748b': 'var(--text-disabled)',
  '38bdf8': 'var(--color-primary)',
  '3b82f6': 'var(--color-primary)',
  '60a5fa': 'var(--color-primary)',
  '22d3ee': 'var(--color-primary)',
  '06b6d4': 'var(--color-primary)',
  '10b981': 'var(--color-success)',
  '34d399': 'var(--color-success)',
  '22c55e': 'var(--color-success)',
  '065f46': 'var(--color-success-bg)',
  '064e3b': 'var(--color-success-bg)',
  f59e0b: 'var(--color-warning)',
  fbbf24: 'var(--color-warning)',
  fb923c: 'var(--color-warning)',
  '78350f': 'var(--color-warning-bg)',
  ef4444: 'var(--color-danger)',
  f43f5e: 'var(--color-danger)',
  e11d48: 'var(--color-danger)',
  fb7185: 'var(--color-danger)',
  f472b6: 'var(--color-danger)',
  ec4899: 'var(--color-danger)',
  '881337': 'var(--color-danger-bg)',
  a855f7: 'var(--color-info)',
  c084fc: 'var(--color-info)',
  '6366f1': 'var(--color-info)',
};

const files = ROOTS.flatMap((root) => globSync(`${root}/**/*.{vue,ts,html}`, { cwd: process.cwd() }));

for (const file of files) {
  let source = readFileSync(file, 'utf8');
  const before = source;

  source = source.replace(/\[#([0-9a-fA-F]{6})\](?:\/\d+)?/g, (match, hex) => {
    const token = HEX[hex.toLowerCase()];
    return token ? `[${token}]` : match;
  });

  source = source.replace(/text-\[(?:9|10|11)px\]/g, 'text-xs');
  source = source.replace(/rounded-\[(?:1\.5|2|2\.5|3)rem\]/g, 'rounded-xl');
  source = source.replace(/ animate-ping\b/g, '');

  source = source.replace(/class="([^"]*)"/g, (_match, classes) => {
    let next = classes.replace(/\bitalic\b\s?/g, '');
    if (!/focus:(?:ring|border|outline-)/.test(next)) next = next.replace(/\bfocus:outline-none\b\s?/g, '');
    return `class="${next.trim()}"`;
  });
  source = source.replace(/:class="\[([^\]]*)\]"/g, (match, classes) =>
    match.replace(/\bitalic\b\s?/g, ''),
  );

  if (source !== before) writeFileSync(file, source);
}
console.log(`scanned ${files.length} files`);
