/**
 * Legacy Tailwind surface -> semantic token bridge.
 *
 * The Core pages predate the token system and still spell colours as Tailwind
 * utilities. Mapping the palette here keeps every product on one theme instead
 * of copying the deep-black palette into each product config.
 */
const color = (name) => `rgb(var(--${name}-rgb) / <alpha-value>)`;
const tint = (name) => `rgb(var(--${name}-bg-rgb) / <alpha-value>)`;

const accent = (name) => ({
  50: tint(name),
  100: tint(name),
  200: tint(name),
  300: color(name),
  400: color(name),
  500: color(name),
  600: color(name),
  700: color(name),
  800: tint(name),
  900: tint(name),
  950: tint(name),
});

const neutral = {
  50: color('bg-page'),
  100: color('text-primary'),
  200: color('text-primary'),
  300: color('text-secondary'),
  400: color('text-secondary'),
  500: color('text-disabled'),
  600: color('text-disabled'),
  700: color('border-strong'),
  800: color('bg-elevated'),
  850: color('bg-elevated'),
  900: color('bg-surface'),
  950: color('bg-page'),
};

module.exports = {
  theme: {
    extend: {
      colors: {
        slate: neutral,
        gray: neutral,
        zinc: neutral,
        neutral: neutral,
        stone: neutral,
        emerald: accent('color-success'),
        green: accent('color-success'),
        rose: accent('color-danger'),
        red: accent('color-danger'),
        pink: accent('color-danger'),
        amber: accent('color-warning'),
        yellow: accent('color-warning'),
        orange: accent('color-warning'),
        blue: accent('color-primary'),
        sky: accent('color-primary'),
        cyan: accent('color-primary'),
        teal: accent('color-info'),
        indigo: accent('color-info'),
        violet: accent('color-info'),
        purple: accent('color-info'),
        fuchsia: accent('color-info'),
      },
      textColor: {
        white: color('text-primary'),
        black: color('text-primary'),
      },
      backgroundColor: {
        white: color('bg-surface'),
        black: color('bg-page'),
      },
      borderColor: {
        white: color('border-default'),
        black: color('border-strong'),
      },
      fontFamily: {
        ui: ['Edge Sans', 'Edge Emoji', 'sans-serif'],
        mono: ['Edge Mono', 'Edge Sans', 'Edge Emoji', 'monospace'],
        sans: ['Edge Sans', 'Edge Emoji', 'sans-serif'],
      },
      fontSize: {
        xs: ['12px', '18px'],
        sm: ['13px', '20px'],
        base: ['14px', '22px'],
        lg: ['16px', '24px'],
        xl: ['20px', '28px'],
        '2xl': ['24px', '32px'],
        '3xl': ['24px', '32px'],
        '4xl': ['30px', '38px'],
      },
      fontWeight: {
        extrabold: '600',
        black: '600',
      },
      borderRadius: {
        '2xl': '12px',
        '3xl': '12px',
      },
      boxShadow: {
        lg: 'var(--shadow-1)',
        xl: 'var(--shadow-2)',
        '2xl': 'var(--shadow-2)',
      },
    },
  },
};
