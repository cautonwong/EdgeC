import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { createEdgeRouter } from '@edgeos/web-core';
import App from './App.vue';
import './style.css';

const app = createApp(App);
const pinia = createPinia();

app.use(pinia);
app.use(createEdgeRouter());

app.mount('#app');