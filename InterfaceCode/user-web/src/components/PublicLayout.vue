<script setup lang="ts">
import { onMounted } from 'vue'
import { useSiteStore } from '@/stores/site'

const site = useSiteStore()
onMounted(() => { if (site.state === 'idle') void site.load() })
</script>

<template>
  <div>
    <header class="nav-blur"><div class="shell nav-inner"><div class="nav-left"><RouterLink class="nav-link brand" to="/home"><span v-if="site.config.title">{{ site.config.title }}</span></RouterLink><RouterLink class="nav-link" to="/posts">文章</RouterLink></div><nav class="nav-center" aria-label="主导航"><RouterLink class="nav-link" to="/notes">碎碎念</RouterLink><RouterLink class="nav-link" to="/projects">项目</RouterLink><RouterLink class="nav-link" to="/about">关于</RouterLink><RouterLink class="nav-link" to="/guestbook">留言板</RouterLink></nav><div class="nav-right"><RouterLink class="nav-link" to="/">开屏</RouterLink></div></div></header>
    <section class="shell hero-wrap"><h1 class="hero-title"><slot name="title"><span v-if="site.config.title">{{ site.config.title }}</span></slot></h1><p v-if="$slots.subtitle || site.config.subtitle" class="hero-sub"><slot name="subtitle">{{ site.config.subtitle }}</slot></p><p v-if="site.config.announcement" class="notice">{{ site.config.announcement }}</p></section>
    <main class="shell"><slot /></main>
    <footer class="shell footer">© {{ new Date().getFullYear() }}<span v-if="site.config.title"> {{ site.config.title }}</span></footer>
  </div>
</template>
