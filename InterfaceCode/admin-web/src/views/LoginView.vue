<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useSessionStore } from '../stores/session'
const session = useSessionStore(); const router = useRouter(); const username = ref('admin'); const password = ref(''); const notice = ref('当前登录接口是占位框架，点击登录只写入本地演示 token。')
function login(){ if(!username.value || !password.value){ notice.value='请输入用户名和密码'; return } session.setToken('demo-admin-token'); router.push('/dashboard') }
</script>
<template><main class="login-page"><form class="card login-card" @submit.prevent="login"><p class="muted">Admin</p><h1>后台管理</h1><p class="muted">明亮、克制、聚焦内容管理。真实登录后续接 authApi。</p><label class="field"><span>用户名</span><input v-model="username" autocomplete="username" /></label><label class="field"><span>密码</span><input v-model="password" type="password" autocomplete="current-password" /></label><button class="btn primary" type="submit">进入管理端</button><p class="muted">{{ notice }}</p></form></main></template>
<style scoped>.login-page{min-height:100vh;display:grid;place-items:center;padding:24px}.login-card{width:min(480px,100%);display:grid;gap:14px}h1{margin:0;font-size:42px}</style>
