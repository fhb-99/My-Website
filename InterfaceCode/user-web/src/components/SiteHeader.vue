<script setup lang="ts">
import { ref } from 'vue'

import { useSiteStore } from '@/stores/site'

const site = useSiteStore()
const mobileOpen = ref(false)
</script>

<template>
  <header class="site-header">
    <div class="wide-shell site-header-inner">
      <RouterLink class="site-brand" to="/home" @click="mobileOpen = false">
        <span class="brand-mark">I</span>
        <span class="brand-copy">
          <strong>{{ site.config.title || 'IKUN Blog' }}</strong>
          <small>Keep learning · Keep building</small>
        </span>
      </RouterLink>

      <button
        class="mobile-nav-button"
        type="button"
        :aria-expanded="mobileOpen"
        aria-label="切换导航菜单"
        @click="mobileOpen = !mobileOpen"
      >
        <span></span><span></span><span></span>
      </button>

      <nav class="site-nav" :class="{ open: mobileOpen }" aria-label="主导航">
        <RouterLink to="/home" @click="mobileOpen = false">首页</RouterLink>
        <RouterLink to="/posts" @click="mobileOpen = false">知识库</RouterLink>
        <RouterLink to="/interview" @click="mobileOpen = false">八股文</RouterLink>
        <RouterLink to="/notes" @click="mobileOpen = false">随记</RouterLink>
        <RouterLink to="/projects" @click="mobileOpen = false">项目</RouterLink>
        <RouterLink to="/about" @click="mobileOpen = false">关于</RouterLink>
      </nav>

      <RouterLink class="header-action" to="/guestbook">留言板</RouterLink>
    </div>
  </header>
</template>
