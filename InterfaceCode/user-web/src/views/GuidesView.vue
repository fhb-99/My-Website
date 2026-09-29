<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import PublicLayout from '../components/PublicLayout.vue'
import { guidesApi } from '../api'
import type { Guide, GuideChapter, GuideChapterSummary } from '@shared/types'

const GUIDE_SLUG = 'how-to-live-better'
const route = useRoute()
const router = useRouter()
const guide = ref<Guide | null>(null)
const chapters = ref<GuideChapterSummary[]>([])
const chapter = ref<GuideChapter | null>(null)
const state = ref<'loading' | 'ready' | 'empty' | 'error'>('loading')
const error = ref('')
let loadVersion = 0

const currentIndex = computed(() => chapters.value.findIndex((item) => item.id === chapter.value?.id))
const previousChapter = computed(() => currentIndex.value > 0 ? chapters.value[currentIndex.value - 1] : null)
const nextChapter = computed(() => currentIndex.value >= 0 && currentIndex.value < chapters.value.length - 1
  ? chapters.value[currentIndex.value + 1]
  : null)
const sourceVersion = computed(() => guide.value?.source_version.slice(0, 8) || '')

function chapterPath(item: GuideChapterSummary) {
  return { name: 'guides', params: { chapterSlug: item.slug } }
}

async function loadGuideReader(refresh = false) {
  const version = ++loadVersion
  state.value = 'loading'
  error.value = ''

  try {
    if (refresh || !guide.value) {
      const nextGuide = await guidesApi.getGuide(GUIDE_SLUG)
      const nextChapters = await guidesApi.listChapters(nextGuide.id)
      if (version !== loadVersion) return
      guide.value = nextGuide
      chapters.value = nextChapters
    }

    if (!chapters.value.length) {
      chapter.value = null
      state.value = 'empty'
      return
    }

    const requestedSlug = String(route.params.chapterSlug || '')
    if (!requestedSlug) {
      await router.replace(chapterPath(chapters.value[0]))
      return
    }

    const nextChapter = await guidesApi.getChapter(guide.value!.id, requestedSlug)
    if (version !== loadVersion) return
    chapter.value = nextChapter
    state.value = 'ready'
  } catch (reason) {
    if (version !== loadVersion) return
    chapter.value = null
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '指南加载失败，请稍后重试。'
  }
}

watch(() => route.params.chapterSlug, () => { void loadGuideReader() }, { immediate: true })
</script>

<template>
  <PublicLayout :show-hero="false" wide>
    <p v-if="state === 'loading'" class="content-state guide-reader-state">正在加载人生指南…</p>
    <div v-else-if="state === 'error'" class="content-state guide-reader-state">
      <p>{{ error }}</p>
      <div class="guide-state-actions">
        <button class="btn" type="button" @click="loadGuideReader(true)">重试</button>
        <RouterLink class="btn" to="/guides">打开目录首章</RouterLink>
        <RouterLink class="btn" to="/home">返回首页</RouterLink>
      </div>
    </div>
    <div v-else-if="state === 'empty'" class="content-state guide-reader-state">
      <h2>章节内容尚未发布</h2>
      <p>指南已经建立，管理员发布章节后会在这里显示。</p>
    </div>

    <div v-else-if="guide && chapter" class="guide-reader-layout">
      <aside class="guide-directory" aria-label="指南章节目录">
        <RouterLink class="guide-directory-back" to="/home">← 返回首页</RouterLink>
        <div class="guide-directory-heading">
          <span class="guide-directory-mark">⌖</span>
          <div><small>LIFE GUIDE</small><h2>{{ guide.title }}</h2></div>
        </div>
        <p>{{ guide.summary }}</p>
        <nav class="guide-directory-list">
          <RouterLink
            v-for="item in chapters"
            :key="item.id"
            class="guide-directory-link"
            :class="{ active: item.id === chapter.id }"
            :to="chapterPath(item)"
            :aria-current="item.id === chapter.id ? 'page' : undefined"
          >
            <span>{{ String(item.chapter_no).padStart(2, '0') }}</span>
            <strong>{{ item.title }}</strong>
          </RouterLink>
        </nav>
        <div class="guide-directory-source">
          <a v-if="guide.source_url" :href="guide.source_url" target="_blank" rel="noopener noreferrer">查看原项目 ↗</a>
          <a v-if="guide.license_url" :href="guide.license_url" target="_blank" rel="noopener noreferrer">CC BY 4.0 ↗</a>
        </div>
      </aside>

      <main class="guide-reader-main">
        <header class="guide-chapter-heading">
          <p class="reader-breadcrumb"><RouterLink to="/guides">人生指南</RouterLink><span>/</span><span>第 {{ chapter.chapter_no }} 章</span></p>
          <div class="guide-chapter-number">{{ String(chapter.chapter_no).padStart(2, '0') }}</div>
          <h1>{{ chapter.title }}</h1>
          <p>全书共 {{ chapters.length }} 章，当前为第 {{ chapter.chapter_no }} 章，内容按原项目章节顺序展示。</p>
        </header>

        <details class="guide-directory-mobile">
          <summary>章节目录 · 第 {{ chapter.chapter_no }}/{{ chapters.length }} 章</summary>
          <nav>
            <RouterLink
              v-for="item in chapters"
              :key="item.id"
              :class="{ active: item.id === chapter.id }"
              :to="chapterPath(item)"
            >{{ String(item.chapter_no).padStart(2, '0') }}　{{ item.title }}</RouterLink>
          </nav>
        </details>

        <article class="article-content guide-article" v-html="chapter.content_html" />

        <nav v-if="previousChapter || nextChapter" class="reader-pagination guide-pagination" aria-label="章节翻页">
          <RouterLink v-if="previousChapter" :to="chapterPath(previousChapter)"><small>上一章</small><strong>{{ previousChapter.title }}</strong></RouterLink>
          <span v-else />
          <RouterLink v-if="nextChapter" :to="chapterPath(nextChapter)"><small>下一章</small><strong>{{ nextChapter.title }}</strong></RouterLink>
        </nav>

        <footer class="guide-attribution">
          <p>本文来自 <a v-if="guide.source_url" :href="guide.source_url" target="_blank" rel="noopener noreferrer">《{{ guide.title }}》</a><span v-else>《{{ guide.title }}》</span>，依据 <a v-if="guide.license_url" :href="guide.license_url" target="_blank" rel="noopener noreferrer">CC BY 4.0</a><span v-else>CC BY 4.0</span> 转载。</p>
          <p v-if="sourceVersion">来源版本：<code>{{ sourceVersion }}</code></p>
        </footer>
      </main>
    </div>
  </PublicLayout>
</template>
