<template>
  <section class="nf">
    <h1>404 · 页面不存在</h1>
    <p>该路径不在本次构建的静态 Registry 中。运行期不会为未知路径注册路由。</p>
    <router-link to="/">返回概览</router-link>
  </section>
</template>

<style scoped>
.nf {
  padding: var(--space-8);
  max-width: 560px;
}
.nf h1 {
  font-size: var(--font-size-xl);
  margin: 0 0 var(--space-2);
}
.nf p {
  color: var(--text-secondary);
  margin: 0 0 var(--space-4);
}
</style>
