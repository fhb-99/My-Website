import { defineStore } from 'pinia'
const EMAIL_KEY = 'blog-visitor-email'
function isValidEmail(value: string) { return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value) }
export const useVisitorStore = defineStore('visitor', {
  state: () => ({ email: localStorage.getItem(EMAIL_KEY) || '' }),
  getters: {
    hasEmail: (state) => isValidEmail(state.email),
    maskedEmail: (state) => { const [name, domain] = state.email.split('@'); if (!name || !domain) return ''; return `${name.slice(0, Math.min(2, name.length))}***@${domain}` }
  },
  actions: {
    registerEmail(email: string) { if (!isValidEmail(email)) throw new Error('请填写有效邮箱'); this.email = email; localStorage.setItem(EMAIL_KEY, email) },
    clearEmail() { this.email = ''; localStorage.removeItem(EMAIL_KEY) }
  }
})
