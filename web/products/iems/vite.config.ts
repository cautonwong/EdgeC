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
    alias: {
      '@edgeos/web-core': path.resolve(__dirname, '../../core/src/index.ts'),
      '@generated': process.env.VITE_CONTRACT_GEN_DIR || path.resolve(__dirname, '../../../build/products/iems/generated'),
      'vue': resolvePkg('vue'),
      'pinia': resolvePkg('pinia'),
      'vue-router': resolvePkg('vue-router'),
      'axios': resolvePkg('axios'),
      'js-yaml': resolvePkg('js-yaml'),
      'echarts': resolvePkg('echarts')
    }
  },
  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://localhost:8081',
        changeOrigin: true
      }
    }
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true
  }
});
