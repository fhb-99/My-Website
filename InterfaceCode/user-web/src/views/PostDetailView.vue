<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import PublicLayout from '../components/PublicLayout.vue'
import EmailGate from '../components/EmailGate.vue'
import { commentsApi, postsApi } from '../api'
import { useFavoritesStore } from '../stores/favorites'
import { useVisitorStore } from '../stores/visitor'
import {
  buildTableOfContents,
  clampReaderTocWidth,
  READER_TOC_DEFAULT_WIDTH,
  READER_TOC_MAX_WIDTH,
  READER_TOC_MIN_WIDTH,
  scrollToHeading,
  type TocItem,
} from '../utils/article-reading'
import type { Comment, EntityId, PostDetail, PostNavigation } from '@shared/types'

const route = useRoute()
const visitor = useVisitorStore()
const favorites = useFavoritesStore()
const post = ref<PostDetail | null>(null)
const comments = ref<Comment[]>([])
const navigation = ref<PostNavigation>({ previous: null, next: null })
const toc = ref<TocItem[]>([])
const articleRef = ref<HTMLElement | null>(null)
const readerLayoutRef = ref<HTMLElement | null>(null)
const activeHeading = ref('')
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')
const commentState = ref<'loading' | 'ready' | 'error'>('loading')
const commentMessage = ref('')
const actionMessage = ref('')
const content = ref('')
const progress = ref(0)
const tocWidth = ref(READER_TOC_DEFAULT_WIDTH)
const isResizingToc = ref(false)
const postId = computed<EntityId | null>(() => post.value?.id ?? null)
const modulePath = computed(() => route.path.startsWith('/interview') ? '/interview' : '/posts')
const moduleName = computed(() => route.path.startsWith('/interview') ? '八股文' : '知识库')
const readerLayoutStyle = computed(() => ({ '--reader-toc-width': `${tocWidth.value}px` }))
const READER_TOC_STORAGE_KEY = 'blog-reader-toc-width'
let viewTimer: number | undefined
let headingObserver: IntersectionObserver | undefined
let resizeStartX = 0
let resizeStartWidth = READER_TOC_DEFAULT_WIDTH

function detailPath(id: EntityId) {
  return `${modulePath.value}/${id}`
}

function updateProgress() {
  const element = articleRef.value
  if (!element) return
  const height = element.offsetHeight - window.innerHeight
  progress.value = height <= 0 ? 100 : Math.max(0, Math.min(100, Math.round((-element.getBoundingClientRect().top / height) * 100)))
}

function observeHeadings() {
  headingObserver?.disconnect()
  if (!articleRef.value || !('IntersectionObserver' in window)) return

  // 只观察正文标题，目录高亮不会干扰文章滚动位置。
  headingObserver = new IntersectionObserver((entries) => {
    const current = entries.find((entry) => entry.isIntersecting)
    if (current?.target.id) activeHeading.value = current.target.id
  }, { rootMargin: '-88px 0px -72% 0px', threshold: 0 })

  articleRef.value.querySelectorAll('h2, h3, h4').forEach((heading) => headingObserver?.observe(heading))
}

function goToHeading(id: string) {
  activeHeading.value = id
  scrollToHeading(id)
}

function fitTocWidth(width: number) {
  tocWidth.value = clampReaderTocWidth(width, readerLayoutRef.value?.clientWidth)
}

function stopTocResize() {
  if (!isResizingToc.value) return
  isResizingToc.value = false
  document.body.classList.remove('reader-is-resizing')
  window.removeEventListener('pointermove', resizeToc)
  window.removeEventListener('pointerup', stopTocResize)
  window.removeEventListener('pointercancel', stopTocResize)
  localStorage.setItem(READER_TOC_STORAGE_KEY, String(tocWidth.value))
}

function resizeToc(event: PointerEvent) {
  fitTocWidth(resizeStartWidth + event.clientX - resizeStartX)
}

function startTocResize(event: PointerEvent) {
  if (event.button !== 0) return
  event.preventDefault()
  resizeStartX = event.clientX
  resizeStartWidth = tocWidth.value
  isResizingToc.value = true
  document.body.classList.add('reader-is-resizing')
  window.addEventListener('pointermove', resizeToc)
  window.addEventListener('pointerup', stopTocResize)
  window.addEventListener('pointercancel', stopTocResize)
}

function resizeTocWithKeyboard(event: KeyboardEvent) {
  const step = event.shiftKey ? 32 : 12
  if (event.key === 'ArrowLeft') fitTocWidth(tocWidth.value - step)
  else if (event.key === 'ArrowRight') fitTocWidth(tocWidth.value + step)
  else if (event.key === 'Home') fitTocWidth(READER_TOC_DEFAULT_WIDTH)
  else return
  event.preventDefault()
  localStorage.setItem(READER_TOC_STORAGE_KEY, String(tocWidth.value))
}

function resetTocWidth() {
  fitTocWidth(READER_TOC_DEFAULT_WIDTH)
  localStorage.setItem(READER_TOC_STORAGE_KEY, String(tocWidth.value))
}

function handleWindowResize() {
  fitTocWidth(tocWidth.value)
}

async function loadComments() {
  if (!postId.value) return
  commentState.value = 'loading'
  try {
    comments.value = (await commentsApi.listComments(postId.value, { page: 1, limit: 20 })).data
    commentState.value = 'ready'
  } catch {
    comments.value = []
    commentState.value = 'error'
  }
}

async function recordPostView(id: EntityId) {
  try {
    const result = await postsApi.recordPostView(id)
    if (result.counted && post.value?.id === id) post.value = { ...post.value, views: post.value.views + 1 }
  } catch (reason) {
    // 阅读统计失败不影响正文阅读，下一次符合条件的阅读仍会再次尝试。
    console.warn('阅读量上报失败', reason)
  }
}

function schedulePostView(id: EntityId) {
  window.clearTimeout(viewTimer)
  viewTimer = window.setTimeout(() => { void recordPostView(id) }, 10000)
}

async function loadPost() {
  state.value = 'loading'
  error.value = ''
  actionMessage.value = ''
  toc.value = []
  activeHeading.value = ''
  navigation.value = { previous: null, next: null }
  headingObserver?.disconnect()

  try {
    const detail = await postsApi.getPostDetail(String(route.params.id))
    post.value = detail
    state.value = 'ready'
    await nextTick()
    if (articleRef.value) {
      fitTocWidth(tocWidth.value)
      toc.value = buildTableOfContents(articleRef.value)
      activeHeading.value = toc.value[0]?.id || ''
      observeHeadings()
      updateProgress()
    }
    void loadComments()
    schedulePostView(detail.id)

    try {
      navigation.value = await postsApi.getNavigation(detail.id, modulePath.value === '/interview' ? 'interview' : 'article')
    } catch (reason) {
      // 上一篇、下一篇加载失败时仍保留当前文章和阅读上报。
      console.warn('文章导航加载失败', reason)
    }
  } catch (reason) {
    post.value = null
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '文章不存在或暂未发布。'
    commentState.value = 'ready'
  }
}

async function submitComment() {
  if (!visitor.hasProfile || !postId.value || !content.value.trim()) return
  try {
    const result = await commentsApi.createComment(postId.value, { nickname: visitor.nickname, email: visitor.email, content: content.value.trim() })
    content.value = ''
    commentMessage.value = result.is_approved ? '评论已发布。' : '已提交，等待审核。'
    await loadComments()
  } catch {
    commentMessage.value = '评论提交失败，请稍后再试。'
  }
}

async function copyLink() {
  try {
    await navigator.clipboard.writeText(window.location.href)
    actionMessage.value = '链接已复制'
  } catch {
    actionMessage.value = '复制失败，请手动复制地址栏链接'
  }
}

onMounted(() => {
  const storedValue = localStorage.getItem(READER_TOC_STORAGE_KEY)
  const storedWidth = storedValue === null ? Number.NaN : Number(storedValue)
  if (Number.isFinite(storedWidth)) fitTocWidth(storedWidth)
  window.addEventListener('scroll', updateProgress, { passive: true })
  window.addEventListener('resize', handleWindowResize, { passive: true })
})
watch(() => route.params.id, () => { void loadPost() }, { immediate: true })
onBeforeUnmount(() => {
  window.clearTimeout(viewTimer)
  headingObserver?.disconnect()
  stopTocResize()
  window.removeEventListener('scroll', updateProgress)
  window.removeEventListener('resize', handleWindowResize)
})
</script>

<template>
  <PublicLayout :show-hero="false" wide>
    <div class="reading-progress" :style="{ width: `${progress}%` }" />
    <p v-if="state === 'loading'" class="content-state reader-state">正在加载文章…</p>
    <div v-else-if="state === 'error'" class="content-state reader-state">
      <p>{{ error }}</p>
      <RouterLink class="btn" :to="modulePath">返回{{ moduleName }}</RouterLink>
    </div>

    <div v-else-if="post" ref="readerLayoutRef" class="reader-layout" :class="{ resizing: isResizingToc }" :style="readerLayoutStyle">
      <aside class="reader-toc toc" aria-label="文章目录">
        <div class="reader-toc-title"><span>目录</span><small>CONTENTS</small></div>
        <div v-if="toc.length" class="reader-toc-list">
          <button
            v-for="item in toc"
            :key="item.id"
            type="button"
            :title="item.text"
            :class="[`toc-level-${item.level}`, { active: activeHeading === item.id }]"
            :aria-current="activeHeading === item.id ? 'location' : undefined"
            @click="goToHeading(item.id)"
          >{{ item.text }}</button>
        </div>
        <p v-else class="reader-toc-empty">正文暂无章节标题</p>
        <RouterLink class="reader-back" :to="modulePath">← 返回{{ moduleName }}</RouterLink>
      </aside>

      <div
        class="reader-resizer"
        role="separator"
        aria-label="调整目录和正文宽度"
        aria-orientation="vertical"
        :aria-valuemin="READER_TOC_MIN_WIDTH"
        :aria-valuemax="READER_TOC_MAX_WIDTH"
        :aria-valuenow="tocWidth"
        tabindex="0"
        title="左右拖动调整目录宽度，双击恢复默认"
        @pointerdown="startTocResize"
        @keydown="resizeTocWithKeyboard"
        @dblclick="resetTocWidth"
      ><span /></div>

      <main class="reader-main">
        <header class="reader-heading">
          <p class="reader-breadcrumb"><RouterLink :to="modulePath">{{ moduleName }}</RouterLink><span>/</span><span>{{ post.tags[0] || '文章' }}</span></p>
          <h1>{{ post.title }}</h1>
          <p v-if="post.summary" class="reader-summary">{{ post.summary }}</p>
          <div class="reader-meta">
            <span>{{ post.created_at }}</span>
            <span>阅读 {{ post.views }}</span>
            <span v-for="tag in post.tags" :key="tag"># {{ tag }}</span>
          </div>
          <div class="reader-actions">
            <button class="btn" type="button" @click="copyLink">复制链接</button>
            <button class="btn" type="button" :aria-pressed="favorites.has(post.id)" @click="favorites.toggle(post.id)">{{ favorites.has(post.id) ? '已收藏' : '收藏' }}</button>
            <span v-if="actionMessage">{{ actionMessage }}</span>
          </div>
        </header>

        <details v-if="toc.length" class="reader-toc-mobile">
          <summary>文章目录 · {{ toc.length }} 节</summary>
          <button v-for="item in toc" :key="item.id" type="button" :title="item.text" :class="`toc-level-${item.level}`" @click="goToHeading(item.id)">{{ item.text }}</button>
        </details>

        <img v-if="post.cover_url" class="post-cover" :src="post.cover_url" alt="" />
        <article ref="articleRef" class="article-content" v-html="post.content_html" />

        <nav v-if="navigation.previous || navigation.next" class="reader-pagination">
          <RouterLink v-if="navigation.previous" :to="detailPath(navigation.previous.id)"><small>上一篇</small><strong>{{ navigation.previous.title }}</strong></RouterLink>
          <RouterLink v-if="navigation.next" :to="detailPath(navigation.next.id)"><small>下一篇</small><strong>{{ navigation.next.title }}</strong></RouterLink>
        </nav>

        <section class="comment-panel">
          <p class="page-eyebrow">DISCUSSION</p>
          <h2>评论</h2>
          <p v-if="commentState === 'loading'" class="content-state">正在加载评论…</p>
          <p v-else-if="commentState === 'error'" class="content-state">评论加载失败，请稍后再试。</p>
          <p v-else-if="!comments.length" class="content-state">暂无评论</p>
          <div v-else class="comment-list">
            <article v-for="item in comments" :key="item.id" class="message-card"><strong>{{ item.nickname }}</strong><p>{{ item.content }}</p><time>{{ item.created_at }}</time></article>
          </div>
          <p v-if="commentMessage" class="notice">{{ commentMessage }}</p>
          <EmailGate scene="评论" />
          <form v-if="visitor.hasProfile" class="composer" @submit.prevent="submitComment">
            <textarea v-model="content" required maxlength="800" placeholder="认真读完后的想法，可以从这里开始。" />
            <button class="btn primary">提交评论</button>
          </form>
        </section>
      </main>
    </div>
  </PublicLayout>
</template>
