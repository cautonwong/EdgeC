/** ECharts draws on canvas, so it cannot read CSS variables itself. */
export function cssVar(name: string): string {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

export function withAlpha(color: string, alpha: number): string {
  const hex = color.replace('#', '');
  if (hex.length !== 6) return color;
  const [r, g, b] = [0, 2, 4].map((offset) => parseInt(hex.slice(offset, offset + 2), 16));
  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

export function chartPalette(): string[] {
  return [1, 2, 3, 4, 5, 6].map((index) => cssVar(`--chart-${index}`));
}

export function chartChrome() {
  return {
    surface: cssVar('--bg-surface'),
    elevated: cssVar('--bg-elevated'),
    border: cssVar('--border-default'),
    text: cssVar('--text-primary'),
    muted: cssVar('--text-secondary'),
  };
}
