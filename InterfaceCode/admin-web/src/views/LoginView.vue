<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { authApi } from '../api'
import { useSessionStore } from '../stores/session'

const session = useSessionStore()
const router = useRouter()
const username = ref('admin')
const password = ref('')
const notice = ref('请输入管理员账号和密码。')
const loading = ref(false)

async function login() {
  if (!username.value.trim() || !password.value) {
    notice.value = '请输入用户名和密码'
    return
  }

  try {
    loading.value = true
    notice.value = '正在登录...'
    const result = await authApi.login({ username: username.value.trim(), password: password.value })
    session.setToken(result.token)
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
    <form class="card login-card" @submit.prevent="login">
      <p class="muted">Admin</p>
      <h1>后台管理</h1>
      <p class="muted">登录后可以管理文章、上传内容和查看互动信息。</p>
      <label class="field"><span>用户名</span><input v-model="username" autocomplete="username" /></label>
      <label class="field"><span>密码</span><input v-model="password" type="password" autocomplete="current-password" /></label>
      <button class="btn primary" type="submit" :disabled="loading">{{ loading ? '登录中...' : '进入管理端' }}</button>
      <p class="muted">{{ notice }}</p>
    </form>
  </main>
</template>

<style scoped>
.login-page { min-height: 100vh; display: grid; place-items: center; padding: 24px; }
.login-card { width: min(480px, 100%); display: grid; gap: 14px; }
h1 { margin: 0; font-size: 42px; }
</style>