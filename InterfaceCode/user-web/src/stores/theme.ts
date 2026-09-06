import { defineStore } from 'pinia'
export type ThemeMode = 'day' | 'night'
const KEY = 'blog-ui-theme'
export const useThemeStore = defineStore('theme', {
  state: () => ({ mode: (localStorage.getItem(KEY) === 'night' ? 'night' : 'day') as ThemeMode }),
  actions: { toggle() { this.mode = this.mode === 'day' ? 'night' : 'day'; localStorage.setItem(KEY, this.mode) } }
})
