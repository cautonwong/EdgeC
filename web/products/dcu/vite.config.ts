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
  resolve: {
    alias: [
      { find: '@edgeos/web-core/sdk', replacement: path.resolve(__dirname, '../../core/src/sdk/index.ts') },
      { find: '@edgeos/web-core/ui', replacement: path.resolve(__dirname, '../../core/src/ui/index.ts') },
      { find: '@edgeos/web-core', replacement: path.resolve(__dirname, '../../core/src/index.ts') },
      { find: 'vue', replacement: resolvePkg('vue') },
      { find: 'pinia', replacement: resolvePkg('pinia') },
      { find: 'vue-router', replacement: resolvePkg('vue-router') },
      { find: 'axios', replacement: resolvePkg('axios') },
      { find: 'js-yaml', replacement: resolvePkg('js-yaml') },
      { find: 'echarts', replacement: resolvePkg('echarts') },
    ],
  },
  server: {
    port: 5174,
    proxy: {
      '/api': {
        // 开发/E2E 把 /api 交给 mock-server（webd 的行为替身）；产品之间端口不同，用环境变量指定
        target: process.env.EDGEOS_API_TARGET ?? 'http://127.0.0.1:8081',
        changeOrigin: false,
      },
    },
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    rollupOptions: {
      output: {
        // 图表库单独成 vendor chunk：既便于缓存，也让 entry 预算只度量真正的首屏代码
        manualChunks: {
          echarts: ['echarts'],
        },
      },
    },
  },
});
