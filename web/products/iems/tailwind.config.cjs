module.exports = {
  presets: [require('../../tooling/tailwind.preset.cjs')],
  content: [
    './index.html',
    './src/**/*.{vue,js,ts,jsx,tsx}',
    './pages/**/*.{vue,js,ts,jsx,tsx}',
    './components/**/*.{vue,js,ts,jsx,tsx}',
    '../../core/src/**/*.{vue,js,ts,jsx,tsx}',
  ],
};
