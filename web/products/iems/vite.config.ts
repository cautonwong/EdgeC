import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import path from 'path';
import fs from 'fs';

const resolvePkg = (pkg: string) => {
  const localPath = path.resolve(__dirname, 'node_modules', pkg);
  if (fs.existsSync(localPath)) return localPath;
  return path.resolve(__dirname, '../../node_modules', pkg);
};

export default defineConfig({
  plugins: [vue()],
  // 每个产品独立缓存：单仓提升 node_modules 时避免两个 dev server 互相覆盖依赖预构建。
  cacheDir: path.resolve(__dirname, 'node_modules/.vite-iems'),
  resolve: {
    // 顺序重要：更具体的子路径别名必须排在包根别名之前
    alias: [
      { find: '@edgeos/web-core/app', replacement: path.resolve(__dirname, '../../core/src/app/index.ts') },
      { find: '@edgeos/web-core/sdk', replacement: path.resolve(__dirname, '../../core/src/sdk/index.ts') },
      { find: '@edgeos/web-core/ui', replacement: path.resolve(__dirname, '../../core/src/ui/index.ts') },
      { find: '@edgeos/web-core', replacement: path.resolve(__dirname, '../../core/src/index.ts') },
      { find: '@generated', replacement: process.env.VITE_CONTRACT_GEN_DIR || path.resolve(__dirname, '../../build/products/iems/generated') },
      { find: 'vue', replacement: resolvePkg('vue') },
      { find: 'pinia', replacement: resolvePkg('pinia') },
      { find: 'vue-router', replacement: resolvePkg('vue-router') },
      { find: 'axios', replacement: resolvePkg('axios') },
      { find: 'js-yaml', replacement: resolvePkg('js-yaml') },
      { find: 'echarts', replacement: resolvePkg('echarts') },
    ],
  },
  server: {
    port: 5173,
    proxy: {
      // 开发期把 /api 交给 mock-server（webd 的行为替身）
      '/api': {
        // 开发/E2E 把 /api 交给 mock-server（webd 的行为替身）；产品之间端口不同，用环境变量指定
        target: process.env.EDGEOS_API_TARGET ?? 'http://127.0.0.1:8081',
        changeOrigin: false,
        // /api/v1/ws 是 WebSocket：dev server 必须显式开启 ws 代理，否则升级请求被丢弃
        ws: true,
      },
    },
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    // 首屏预算靠分包控制：图表、编辑器按需异步加载（§6.8）
    rollupOptions: {
      output: {
        manualChunks: {
          echarts: ['echarts'],
        },
      },
    },
  },
});
