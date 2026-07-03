<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useSessionStore } from '../stores/session'

const session = useSessionStore()
const router = useRouter()

function logout() {
  session.logout()
  router.push('/login')
}
</script>

<template>
  <div class="admin-layout">
    <aside class="sidebar">
      <div class="brand">Blog Admin</div>
      <nav class="nav-list">
        <RouterLink to="/dashboard">概览</RouterLink>
        <RouterLink to="/posts">文章管理</RouterLink>
        <RouterLink to="/uploads">上传管理</RouterLink>
        <RouterLink to="/moderation">留言/评论</RouterLink>
        <RouterLink to="/ai-moderation">AI 辅助审核</RouterLink>
        <RouterLink to="/settings">基础设置</RouterLink>
      </nav>
    </aside>
    <main class="main">
      <div class="top"><slot name="title" /><button class="btn" type="button" @click="logout">退出登录</button></div>
      <slot />
    </main>
  </div>
</template>
