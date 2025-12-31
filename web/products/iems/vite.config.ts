import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import path from 'path';

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@edgeos/web-core': path.resolve(__dirname, '../../core/src/index.ts'),
      'vue': path.resolve(__dirname, 'node_modules/vue'),
      'pinia': path.resolve(__dirname, 'node_modules/pinia'),
      'vue-router': path.resolve(__dirname, 'node_modules/vue-router'),
      'axios': path.resolve(__dirname, 'node_modules/axios'),
      'js-yaml': path.resolve(__dirname, 'node_modules/js-yaml')
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
