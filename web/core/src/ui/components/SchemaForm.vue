<script lang="ts">
/** 表单字段控件类型：由 Schema 推导，页面不手写表单。 */
export interface SchemaFormValue {
  [field: string]: unknown;
}
</script>

<script setup lang="ts">
/**
 * Schema 驱动的配置表单（§6.5）。
 *
 * 目的：配置页不再各写一套 input/label/校验挂载逻辑。字段类型、标题、单位、提示、
 * 枚举、范围全部来自 Registry 里的 Schema；服务端 422 的 fieldErrors 直接挂在对应控件下。
 *
 * 只支持四种字段类型（string / integer / boolean / array<string>）——够覆盖当前所有
 * 配置分区，且故意不引入第三方表单库（离线设备装不了）。
 */
import { computed } from 'vue';
import type { ConfigSchema } from '../../sdk/types';

const props = withDefaults(
  defineProps<{
    schema: ConfigSchema;
    modelValue: SchemaFormValue;
    errors?: Record<string, string>;
    disabled?: boolean;
  }>(),
  { errors: () => ({}), disabled: false },
);

const emit = defineEmits<{
  'update:modelValue': [SchemaFormValue];
  change: [field: string, value: unknown];
}>();

const fields = computed(() => Object.entries(props.schema.properties));

function valueOf(name: string): unknown {
  return props.modelValue[name];
}

function set(name: string, value: unknown): void {
  emit('update:modelValue', { ...props.modelValue, [name]: value });
  emit('change', name, value);
}

function asText(name: string): string {
  const value = valueOf(name);
  return value === undefined || value === null ? '' : String(value);
}

function asList(name: string): string {
  const value = valueOf(name);
  return Array.isArray(value) ? value.join('\n') : '';
}

function setList(name: string, text: string): void {
  set(
    name,
    text
      .split('\n')
      .map((line) => line.trim())
      .filter((line) => line.length > 0),
  );
}
</script>

<template>
  <form class="schema-form" data-testid="schema-form" @submit.prevent>
    <label v-for="[name, field] in fields" :key="name" class="field">
      <span class="field__label">
        {{ field.title }}
        <span v-if="field.unit" class="field__unit">（{{ field.unit }}）</span>
      </span>

      <!-- 布尔：复选框 -->
      <input
        v-if="field.type === 'boolean'"
        class="field__check"
        type="checkbox"
        :checked="valueOf(name) === true"
        :disabled="disabled"
        :data-testid="`field-${name}`"
        @change="set(name, ($event.target as HTMLInputElement).checked)"
      />

      <!-- 枚举：下拉 -->
      <select
        v-else-if="field.enum"
        class="field__input"
        :value="asText(name)"
        :disabled="disabled"
        :data-testid="`field-${name}`"
        @change="set(name, ($event.target as HTMLSelectElement).value)"
      >
        <option v-for="option in field.enum" :key="String(option)" :value="String(option)">{{ option }}</option>
      </select>

      <!-- 数组：一行一个 -->
      <textarea
        v-else-if="field.type === 'array'"
        class="field__input field__input--area"
        rows="3"
        :value="asList(name)"
        :disabled="disabled"
        :data-testid="`field-${name}`"
        :aria-invalid="errors[name] ? 'true' : undefined"
        @input="setList(name, ($event.target as HTMLTextAreaElement).value)"
      />

      <!-- 整数：number -->
      <input
        v-else-if="field.type === 'integer'"
        class="field__input"
        type="number"
        :min="field.minimum"
        :max="field.maximum"
        :value="asText(name)"
        :disabled="disabled"
        :data-testid="`field-${name}`"
        :aria-invalid="errors[name] ? 'true' : undefined"
        @input="set(name, Number(($event.target as HTMLInputElement).value))"
      />

      <!-- 其余按字符串处理（含 format 提示） -->
      <input
        v-else
        class="field__input"
        :value="asText(name)"
        :disabled="disabled"
        :data-testid="`field-${name}`"
        :placeholder="field.format"
        :aria-invalid="errors[name] ? 'true' : undefined"
        @input="set(name, ($event.target as HTMLInputElement).value)"
      />

      <span v-if="errors[name]" class="field__error" :data-testid="`error-${name}`">{{ errors[name] }}</span>
      <span v-else-if="field.hint" class="field__hint">{{ field.hint }}</span>
    </label>
  </form>
</template>

<style scoped>
.schema-form {
  max-width: 560px;
  padding: var(--space-4);
  background: var(--bg-surface);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
}
.field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
  margin-bottom: var(--space-3);
}
.field__label {
  font-size: var(--font-size-sm);
  color: var(--text-secondary);
}
.field__unit {
  color: var(--text-disabled);
}
.field__input {
  min-height: var(--input-height);
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
}
.field__input--area {
  padding: var(--space-2) var(--space-3);
  font-family: var(--font-mono);
  font-size: var(--font-size-sm);
}
.field__input[aria-invalid='true'] {
  border-color: var(--color-danger);
}
.field__check {
  width: 18px;
  height: 18px;
}
.field__error {
  color: var(--color-danger);
  font-size: var(--font-size-xs);
}
.field__hint {
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
</style>
