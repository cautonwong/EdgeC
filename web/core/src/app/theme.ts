/**
 * Theme preference: system by default, user override persisted.
 * The inline script in each product's index.html reads the same key before
 * first paint so the first frame does not flash the wrong surface.
 */
import { ref } from 'vue';

export type ThemePreference = 'light' | 'dark' | 'system';
export type ResolvedTheme = 'light' | 'dark';

const STORAGE_KEY = 'edgeos.theme';
const ORDER: ThemePreference[] = ['system', 'light', 'dark'];

export const themePreference = ref<ThemePreference>('system');
export const resolvedTheme = ref<ResolvedTheme>('light');

let initialized = false;

function readPreference(): ThemePreference {
  try {
    const value = localStorage.getItem(STORAGE_KEY);
    if (value === 'light' || value === 'dark' || value === 'system') return value;
  } catch {
    // private mode or storage disabled: system preference still works
  }
  return 'system';
}

function systemTheme(): ResolvedTheme {
  return window.matchMedia?.('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

export function applyTheme(preference: ThemePreference): void {
  const resolved = preference === 'system' ? systemTheme() : preference;
  document.documentElement.dataset.theme = resolved;
  document.documentElement.style.colorScheme = resolved;
  themePreference.value = preference;
  resolvedTheme.value = resolved;
}

export function setThemePreference(preference: ThemePreference): void {
  try {
    localStorage.setItem(STORAGE_KEY, preference);
  } catch {
    // the current tab still switches; only persistence is lost
  }
  applyTheme(preference);
}

export function cycleTheme(): void {
  const next = ORDER[(ORDER.indexOf(themePreference.value) + 1) % ORDER.length];
  setThemePreference(next);
}

export function initTheme(): void {
  applyTheme(readPreference());
  if (initialized) return;
  initialized = true;
  window.matchMedia?.('(prefers-color-scheme: dark)').addEventListener('change', () => {
    if (themePreference.value === 'system') applyTheme('system');
  });
}
