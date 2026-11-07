import { nextTick, onBeforeUnmount, watch, type Ref } from 'vue';

const FOCUSABLE =
  'a[href], button:not([disabled]), input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex="-1"])';

export function useDialogA11y(
  open: Ref<boolean>,
  container: Ref<HTMLElement | null>,
  onClose: () => void,
  initialFocus?: Ref<HTMLElement | null>,
): void {
  let previous: HTMLElement | null = null;

  function items(): HTMLElement[] {
    if (!container.value) return [];
    return [...container.value.querySelectorAll<HTMLElement>(FOCUSABLE)].filter(
      (element) => element.offsetParent !== null,
    );
  }

  function onKeydown(event: KeyboardEvent): void {
    if (event.key === 'Escape') {
      event.preventDefault();
      onClose();
      return;
    }
    if (event.key !== 'Tab') return;
    const list = items();
    if (list.length === 0) {
      event.preventDefault();
      container.value?.focus();
      return;
    }
    const first = list[0];
    const last = list[list.length - 1];
    if (event.shiftKey && document.activeElement === first) {
      event.preventDefault();
      last.focus();
    } else if (!event.shiftKey && document.activeElement === last) {
      event.preventDefault();
      first.focus();
    }
  }

  watch(open, async (value) => {
    if (value) {
      previous = document.activeElement instanceof HTMLElement ? document.activeElement : null;
      await nextTick();
      (initialFocus?.value ?? items()[0] ?? container.value)?.focus();
      document.addEventListener('keydown', onKeydown);
    } else {
      document.removeEventListener('keydown', onKeydown);
      previous?.focus();
      previous = null;
    }
  });

  onBeforeUnmount(() => document.removeEventListener('keydown', onKeydown));
}
