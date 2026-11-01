<script setup lang="ts">
/** L1 文本输入。标签在上（窄屏）/左侧由父容器决定；错误信息带图标，不只靠颜色（§9.3/§9.5）。 */
import { computed, useId } from 'vue';

const props = withDefaults(
  defineProps<{
    modelValue: string;
    label?: string;
    type?: 'text' | 'password' | 'number' | 'search';
    hint?: string;
    error?: string;
    required?: boolean;
    disabled?: boolean;
    autocomplete?: string;
    placeholder?: string;
  }>(),
  { type: 'text', required: false, disabled: false },
);

const emit = defineEmits<{ 'update:modelValue': [string] }>();
const inputId = useId();
const describedBy = computed(() => (props.error ? `${inputId}-error` : props.hint ? `${inputId}-hint` : undefined));
</script>

<template>
  <div class="ui-field">
    <label v-if="label" class="ui-field__label" :for="inputId">
      {{ label }}<span v-if="required" class="ui-field__required" aria-hidden="true">*</span>
    </label>
    <input
      :id="inputId"
      class="ui-field__input"
      :class="{ 'ui-field__input--error': !!error }"
      :type="type"
      :value="modelValue"
      :disabled="disabled"
      :required="required"
      :placeholder="placeholder"
      :autocomplete="autocomplete"
      :aria-invalid="error ? 'true' : undefined"
      :aria-describedby="describedBy"
      @input="emit('update:modelValue', ($event.target as HTMLInputElement).value)"
    />
    <p v-if="error" :id="`${inputId}-error`" class="ui-field__error" role="alert">⚠ {{ error }}</p>
    <p v-else-if="hint" :id="`${inputId}-hint`" class="ui-field__hint">{{ hint }}</p>
  </div>
</template>

<style scoped>
.ui-field {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
  margin-bottom: var(--space-4);
}
.ui-field__label {
  font-size: var(--font-size-sm);
  color: var(--text-secondary);
}
.ui-field__required {
  color: var(--color-danger);
  margin-left: 2px;
}
.ui-field__input {
  min-height: var(--input-height);
  padding: 0 var(--space-3);
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  background: var(--bg-surface);
  color: var(--text-primary);
  font: inherit;
  font-variant-numeric: tabular-nums;
}
.ui-field__input:focus {
  border-color: var(--color-primary);
  outline: 2px solid color-mix(in srgb, var(--color-primary) 35%, transparent);
  outline-offset: 1px;
}
.ui-field__input--error {
  border-color: var(--color-danger);
}
.ui-field__error {
  margin: 0;
  color: var(--color-danger);
  font-size: var(--font-size-xs);
}
.ui-field__hint {
  margin: 0;
  color: var(--text-secondary);
  font-size: var(--font-size-xs);
}
</style>
