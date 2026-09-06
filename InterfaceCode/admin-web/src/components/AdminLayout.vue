<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useSessionStore } from '../stores/session'

const session = useSessionStore()
const router = useRouter()

// 生产环境通过 VITE_SITE_URL 指向公网用户端，本地开发则继续打开 5173。
const siteUrl = (import.meta.env.VITE_SITE_URL as string | undefined)?.trim()
  || `${window.location.protocol}//${window.location.hostname}:5173/#/home`

const navItems = [
  { to: '/dashboard', icon: '▦', label: '数据概览' },
  { to: '/posts', icon: '▤', label: '文章' },
  { to: '/interview-posts', icon: '▥', label: '八股文' },
  { to: '/learning-roadmap', icon: '⌁', label: 'C++ 学习路线' },
  { to: '/notes', icon: '◫', label: '碎碎念' },
  { to: '/projects', icon: '◇', label: '项目' },
  { to: '/moderation', icon: '◎', label: '评论与留言' },
  { to: '/uploads', icon: '⇧', label: 'Markdown 导入' },
]

function logout() {
  session.logout()
  router.push('/login')
}
</script>

<template>
  <div class="admin-layout">
    <header class="app-bar">
      <RouterLink class="app-brand" to="/dashboard">
        <span class="brand-mark">PB</span>
        <span>博客管理端</span>
      </RouterLink>
      <nav class="app-tabs" aria-label="管理端主导航">
        <RouterLink to="/posts">内容管理</RouterLink>
        <RouterLink to="/dashboard">数据概览</RouterLink>
        <RouterLink to="/settings">系统设置</RouterLink>
      </nav>
      <div class="account-area">
        <a class="site-link" :href="siteUrl" target="_blank" rel="noreferrer">查看网站 ↗</a>
        <span class="account-name">{{ session.email || '管理员' }}</span>
        <button class="top-button" type="button" @click="logout">退出</button>
      </div>
    </header>

    <aside class="sidebar">
      <div class="sidebar-search">⌕　搜索管理功能…</div>
      <p class="sidebar-title">内容集合</p>
      <nav class="nav-list">
        <RouterLink v-for="item in navItems" :key="item.to" :to="item.to">
          <span class="nav-icon">{{ item.icon }}</span>
          <span>{{ item.label }}</span>
        </RouterLink>
      </nav>
      <div class="sidebar-bottom">
        <p class="sidebar-title">系统</p>
        <RouterLink class="system-link" to="/settings"><span class="nav-icon">⚙</span><span>站点设置</span></RouterLink>
        <p class="server-state"><span /> PocketBase 已连接<br><small>{{ session.baseUrl }}</small></p>
      </div>
    </aside>

    <main class="main">
      <div class="top"><slot name="title" /></div>
      <slot />
    </main>
  </div>
</template>
