import { defineStore } from 'pinia'

const EMAIL_KEY = 'blog-visitor-email'
const NICKNAME_KEY = 'blog-visitor-nickname'

function isValidEmail(value: string) {
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value)
}

function normalizeNickname(value: string) {
  return value.trim().slice(0, 32)
}

export const useVisitorStore = defineStore('visitor', {
  state: () => ({
    email: localStorage.getItem(EMAIL_KEY) || '',
    nickname: localStorage.getItem(NICKNAME_KEY) || ''
  }),

  getters: {
    hasEmail: (state) => isValidEmail(state.email),
    hasNickname: (state) => normalizeNickname(state.nickname).length > 0,
    hasProfile(): boolean {
      return this.hasEmail && this.hasNickname
    },
    maskedEmail: (state) => {
      const [name, domain] = state.email.split('@')
      if (!name || !domain) return ''
      return `${name.slice(0, Math.min(2, name.length))}***@${domain}`
    }
  },

  actions: {
    registerEmail(email: string) {
      if (!isValidEmail(email)) throw new Error('请填写有效邮箱')
      this.email = email
      localStorage.setItem(EMAIL_KEY, email)
    },

    registerProfile(email: string, nickname: string) {
      const cleanNickname = normalizeNickname(nickname)
      if (!cleanNickname) throw new Error('请填写昵称')
      this.registerEmail(email)
      this.nickname = cleanNickname
      localStorage.setItem(NICKNAME_KEY, cleanNickname)
    },

    clearProfile() {
      this.email = ''
      this.nickname = ''
      localStorage.removeItem(EMAIL_KEY)
      localStorage.removeItem(NICKNAME_KEY)
    },

    clearEmail() {
      this.clearProfile()
    }
  }
})
