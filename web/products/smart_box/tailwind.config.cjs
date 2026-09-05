module.exports = {
  content: [
    "./index.html",
    "./src/**/*.{vue,js,ts,jsx,tsx}",
    "../../core/src/**/*.{vue,js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        edge: {
          bg: {
            base: '#020617',
            surface: '#0f172a',
            elevated: '#1e293b',
            subtle: '#334155',
          },
          border: {
            subtle: '#1e293b',
            default: '#334155',
            focus: '#38bdf8',
          },
          text: {
            primary: '#f8fafc',
            secondary: '#94a3b8',
            muted: '#64748b',
          },
          status: {
            normal: '#10b981',
            warning: '#f59e0b',
            fault: '#ef4444',
            offline: '#64748b',
            standby: '#38bdf8',
          }
        }
      },
      fontFamily: {
        ui: ['Inter', '-apple-system', 'BlinkMacSystemFont', 'Segoe UI', 'Roboto', 'sans-serif'],
        mono: ['JetBrains Mono', 'SF Mono', 'Consolas', 'monospace'],
      }
    },
  },
  plugins: [],
}
