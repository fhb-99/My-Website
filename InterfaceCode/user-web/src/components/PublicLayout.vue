<script setup lang="ts">
import { onMounted } from 'vue'
import { useSiteStore } from '@/stores/site'
import SiteHeader from './SiteHeader.vue'

withDefaults(defineProps<{ showHero?: boolean; wide?: boolean }>(), {
  showHero: true,
  wide: false,
})

const site = useSiteStore()
onMounted(() => { if (site.state === 'idle') void site.load() })
</script>

<template>
  <div class="public-page">
    <SiteHeader />
    <section v-if="showHero" class="shell page-hero">
      <p class="page-eyebrow">IKUN KNOWLEDGE SPACE</p>
      <h1 class="page-title"><slot name="title">{{ site.config.title || 'IKUN Blog' }}</slot></h1>
      <p v-if="$slots.subtitle || site.config.subtitle" class="page-subtitle"><slot name="subtitle">{{ site.config.subtitle }}</slot></p>
    </section>
    <main :class="wide ? 'wide-shell' : 'shell'"><slot /></main>
    <footer class="wide-shell footer">
      <div class="footer-main">
        <span>© {{ new Date().getFullYear() }} {{ site.config.title || 'IKUN Blog' }}</span>
        <span>在代码与生活之间，持续记录。</span>
      </div>
      <div class="beian-row">
        <a href="https://beian.miit.gov.cn/" target="_blank" rel="noopener noreferrer">赣ICP备2026009462号-1</a>
        <a
          class="beian-gongan"
          href="http://www.beian.gov.cn/portal/registerSystemInfo?recordcode=36010602000401"
          target="_blank"
          rel="noopener noreferrer"
        >
          <img :src="'/ghs.png'" width="16" height="16" alt="" />
          赣公网安备36010602000401号
        </a>
      </div>
    </footer>
  </div>
</template>
