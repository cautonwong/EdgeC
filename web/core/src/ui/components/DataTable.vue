<script lang="ts">
/** 列定义：与具体页面无关，所以放在普通 script 块里导出类型。 */
export interface Column {
  key: string;
  title: string;
  align?: 'left' | 'right';
  width?: string;
}
</script>

<script setup lang="ts">
/** L1 表格：行高 40px、数值右对齐、空态与加载态明确（§9.3 / §6.8）。 */

withDefaults(
  defineProps<{
    columns: Column[];
    rows: Record<string, unknown>[];
    rowKey: string;
    loading?: boolean;
    emptyText?: string;
    caption?: string;
  }>(),
  { loading: false, emptyText: '暂无数据' },
);
</script>

<template>
  <div class="table-wrap">
    <table class="table">
      <caption v-if="caption" class="table__caption">{{ caption }}</caption>
      <thead>
        <tr>
          <th
            v-for="column in columns"
            :key="column.key"
            :style="{ width: column.width, textAlign: column.align ?? 'left' }"
            scope="col"
          >
            {{ column.title }}
          </th>
        </tr>
      </thead>
      <tbody>
        <tr v-if="loading">
          <td :colspan="columns.length" class="table__state">加载中…</td>
        </tr>
        <tr v-else-if="rows.length === 0">
          <td :colspan="columns.length" class="table__state">{{ emptyText }}</td>
        </tr>
        <tr
          v-for="row in rows"
          v-else
          :key="String(row[rowKey])"
          class="table__row"
        >
          <td
            v-for="column in columns"
            :key="column.key"
            :style="{ textAlign: column.align ?? 'left' }"
            :class="{ 'table__cell--num': column.align === 'right' }"
          >
            <slot :name="`cell-${column.key}`" :row="row">{{ row[column.key] ?? '—' }}</slot>
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<style scoped>
.table-wrap {
  overflow: auto;
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
}
.table {
  width: 100%;
  border-collapse: collapse;
  font-size: var(--font-size-base);
}
.table__caption {
  text-align: left;
  padding: var(--space-3);
  color: var(--text-secondary);
  font-size: var(--font-size-sm);
}
th {
  position: sticky;
  top: 0;
  background: var(--bg-page);
  color: var(--text-secondary);
  font-weight: 600;
  font-size: var(--font-size-sm);
  text-align: left;
  padding: var(--space-2) var(--space-3);
  border-bottom: 1px solid var(--border-default);
}
td {
  height: var(--table-row-height);
  padding: 0 var(--space-3);
  border-bottom: 1px solid var(--border-default);
}
.table__cell--num {
  font-variant-numeric: tabular-nums;
}
.table__row:hover,
.table__row:focus-visible {
  background: color-mix(in srgb, var(--color-primary) 8%, transparent);
}
.table__state {
  text-align: center;
  color: var(--text-secondary);
  height: 72px;
}
</style>
