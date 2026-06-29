import { defineStore } from 'pinia'
import { adminApiClient } from '../api'

const TOKEN_KEY = 'blog-admin-token'
const BASE_URL_KEY = 'blog-api-base'

export const useSessionStore = defineStore('session', {
  state: () => ({
    token: localStorage.getItem(TOKEN_KEY) || '',
    baseUrl: localStorage.getItem(BASE_URL_KEY) || adminApiClient.config.baseUrl
  }),

  getters: {
    isLoggedIn: (state) => Boolean(state.token)
  },

  actions: {
    /** 修改后端地址时同步到 API 客户端，确保下一次请求立即生效。 */
    setBaseUrl(value: string) {
      const baseUrl = value.replace(/\/+$/, '')
      this.baseUrl = baseUrl
      adminApiClient.setBaseUrl(baseUrl)
    },

    /** 登录成功后保存 token，并同步到 API 客户端的 Authorization 请求头。 */
    setToken(token: string) {
      this.token = token
      adminApiClient.setToken(token)
    },

    /** 退出登录时同时清空页面状态、localStorage 和 API 客户端 token。 */
    logout() {
      this.token = ''
      adminApiClient.setToken(undefined)
    }
  }
})
