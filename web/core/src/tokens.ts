/**
 * EdgeOS Web Design System Tokens (AEP-490)
 * Industrial Edge OLED Deep Black & Domain-Specific Color Palette
 */

export const EdgeColors = {
  // Base OLED Surface Palette (Zero-Power Energy Saving)
  bg: {
    base: '#020617',       // Slate 950 OLED pure deep background
    surface: '#0f172a',    // Slate 900 primary card / panel surface
    elevated: '#1e293b',   // Slate 800 modal / popover / floating widget
    subtle: '#334155',     // Slate 700 hover / active state
  },

  // Border & Dividers
  border: {
    subtle: '#1e293b',
    default: '#334155',
    focus: '#38bdf8',
  },

  // Text Hierarchy
  text: {
    primary: '#f8fafc',    // Slate 50 high-contrast primary text
    secondary: '#94a3b8',  // Slate 400 secondary / label text
    muted: '#64748b',      // Slate 500 disabled / placeholder text
    inverse: '#020617',    // Contrast inverse
  },

  // Industrial Domain Status Tokens (IEC 60073 / SGCC Standard)
  status: {
    normal: '#10b981',     // Emerald 500 - Running / Healthy / Connected
    warning: '#f59e0b',    // Amber 500 - Alert / Over-threshold / Transients
    fault: '#ef4444',      // Rose 500 - Trip / Fault / Alarm / Emergency
    offline: '#64748b',    // Slate 500 - Disconnected / Heartbeat Stale
    standby: '#38bdf8',    // Sky 400 - Standby / Ready
  },

  // Microgrid & Energy Flow Semantic Colors (IEMS Sankey / Power Vectors)
  powerFlow: {
    grid: '#38bdf8',        // Sky 400 - Main Grid AC Flow
    pv: '#fbbf24',          // Amber 400 - Solar PV Generation
    essCharge: '#34d399',   // Emerald 400 - BESS Storage Charging
    essDischarge: '#f97316',// Orange 500 - BESS Storage Discharging
    load: '#a78bfa',        // Violet 400 - Critical / Facility Load
    diesel: '#fb7185',      // Rose 400 - Backup Generator
  },
} as const;

export const EdgeTypography = {
  fontFamily: {
    ui: 'Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    mono: '"JetBrains Mono", "SF Mono", Consolas, "Liberation Mono", Menlo, monospace',
  },
  fontSize: {
    xs: '0.75rem',    // 12px
    sm: '0.875rem',   // 14px
    base: '1rem',      // 16px
    lg: '1.125rem',   // 18px
    xl: '1.25rem',    // 20px
    '2xl': '1.5rem',  // 24px
    '3xl': '1.875rem',// 30px
    '4xl': '2.25rem', // 36px
  },
} as const;

export const EdgeMotion = {
  duration: {
    instant: '100ms',
    fast: '200ms',
    normal: '400ms',  // Standard GSAP high-refresh data transition
    slow: '800ms',
  },
  easing: {
    standard: 'cubic-bezier(0.4, 0, 0.2, 1)',
    decelerate: 'cubic-bezier(0, 0, 0.2, 1)',
    accelerate: 'cubic-bezier(0.4, 0, 1, 1)',
  },
} as const;

export default {
  colors: EdgeColors,
  typography: EdgeTypography,
  motion: EdgeMotion,
};
