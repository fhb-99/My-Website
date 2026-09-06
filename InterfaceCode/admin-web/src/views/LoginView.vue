<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { authApi } from '../api'
import { useSessionStore } from '../stores/session'

const session = useSessionStore()
const router = useRouter()
const username = ref('')
const password = ref('')
const notice = ref('使用 PocketBase 超级管理员账号登录。')
const loading = ref(false)

async function login() {
  if (!username.value.trim() || !password.value) {
    notice.value = '请输入管理员邮箱和密码'
    return
  }

  try {
    loading.value = true
    notice.value = '正在登录...'
    const result = await authApi.login({ username: username.value.trim(), password: password.value })
    session.setToken(result.token, result.email)
    notice.value = '登录成功'
    router.push('/dashboard')
  } catch (error) {
    console.warn('[admin-login] failed:', error)
    notice.value = error instanceof Error ? error.message : '登录失败，请检查账号和密码'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <main class="login-page">
    <form class="login-card" @submit.prevent="login">
      <div class="login-brand"><span class="brand-mark">PB</span><span>博客管理端</span></div>
      <h1>欢迎回来</h1>
      <p class="muted">一个中文、轻量的 PocketBase 内容管理界面。</p>
      <label class="field"><span>管理员邮箱</span><input v-model="username" type="email" autocomplete="username" placeholder="admin@example.com" /></label>
      <label class="field"><span>密码</span><input v-model="password" type="password" autocomplete="current-password" /></label>
      <button class="btn primary login-button" type="submit" :disabled="loading">{{ loading ? '登录中...' : '登录' }}</button>
      <p class="login-notice">{{ notice }}</p>
    </form>
  </main>
</template>

<style scoped>
.login-page { min-height: 100vh; display: grid; place-items: center; padding: 24px; background: #181818; }
.login-card { width: min(430px, 100%); display: grid; gap: 18px; padding: 36px; border: 1px solid #383838; border-radius: 8px; background: #222; box-shadow: 0 26px 80px rgba(0,0,0,.34); }
.login-brand { display: flex; align-items: center; gap: 12px; color: #f4f4f4; font-size: 18px; font-weight: 800; }
.brand-mark { display: grid; place-items: center; width: 40px; height: 40px; border-radius: 7px; color: #1769d2; background: white; font-size: 14px; font-weight: 900; }
h1 { margin: 10px 0 -8px; color: #f4f4f4; font-size: 30px; }
.login-button { width: 100%; justify-content: center; }
.login-notice { min-height: 24px; margin: 0; color: #9ca3af; font-size: 13px; }
</style>
