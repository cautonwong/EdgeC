<script setup lang="ts">
/**
 * 关于（§8.1 系统 / 附录 B core:about）。
 *
 * 展示的是**构建产物自报的事实**：产品 ID、版本、Core SDK、release 摘要、registry 摘要。
 * 现场排查"这台设备跑的到底是哪个构建"时，这一页比任何日志都快。
 */
import { computed } from 'vue';
import { useAuthz } from '../../app/composables/useAuthz';

const authz = useAuthz();
const registry = computed(() => authz.registry.value);
</script>

<template>
  <section v-if="registry">
    <header class="head">
      <h1>关于</h1>
      <p class="head__hint">以下字段来自随固件交付的静态 Registry，不是运行时从服务端拉取的。</p>
    </header>

    <dl class="facts">
      <dt>产品</dt>
      <dd data-testid="about-product">{{ registry.productName }}（{{ registry.productId }}）</dd>
      <dt>产品版本</dt>
      <dd data-testid="about-version">{{ registry.productVersion }}</dd>
      <dt>Core SDK</dt>
      <dd>{{ registry.coreSdk }}</dd>
      <dt>发布范围</dt>
      <dd>{{ registry.releaseScope.releaseId }}</dd>
      <dt>Registry 摘要</dt>
      <dd class="mono" data-testid="about-digest">{{ registry.digests.registry }}</dd>
      <dt>release-scope 摘要</dt>
      <dd class="mono">{{ registry.digests.releaseScope }}</dd>
      <dt>页面 / 接口</dt>
      <dd>{{ registry.pages.length }} 页面 / {{ registry.api.length }} 接口</dd>
      <dt>已关闭的功能</dt>
      <dd>{{ registry.releaseScope.disabled.join(', ') || '无' }}</dd>
    </dl>
  </section>
</template>

<style scoped>
.head {
  margin-bottom: var(--space-4);
}
.head h1 {
  margin: 0 0 var(--space-1);
  font-size: var(--font-size-xl);
}
.head__hint {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
.facts {
  display: grid;
  grid-template-columns: 160px 1fr;
  gap: var(--space-2) var(--space-4);
  max-width: 760px;
  margin: 0;
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
dt {
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
dd {
  margin: 0;
  font-size: var(--font-size-sm);
}
.mono {
  font-family: var(--font-mono);
  word-break: break-all;
}
</style>
