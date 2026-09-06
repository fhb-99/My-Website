<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import PublicLayout from '../components/PublicLayout.vue'
import { postsApi } from '../api'
import type { PageResult, PostSummary, TagSummary } from '@shared/types'

const route = useRoute()
const router = useRouter()
const result = ref<PageResult<PostSummary>>({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
const tags = ref<TagSummary[]>([])
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')
const keyword = ref('')
const selectedTag = ref('')
const fallbackCovers = ['/clouds/cloud-day.webp', '/clouds/cloud-morning.webp', '/clouds/cloud-evening.webp']

const posts = computed(() => result.value.data)

function readQuery() {
  keyword.value = typeof route.query.q === 'string' ? route.query.q : ''
  selectedTag.value = typeof route.query.tag === 'string' ? route.query.tag : ''
}

async function loadPosts() {
  readQuery()
  state.value = 'loading'
  error.value = ''
  const page = Math.max(1, Number(route.query.page) || 1)
  try {
    result.value = await postsApi.listPosts({ page, limit: 10, q: keyword.value, tag: selectedTag.value, contentType: 'article' })
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '文章加载失败，请稍后重试。'
    result.value = { data: [], page, limit: 10, total: 0, total_pages: 0, has_more: false }
  }
}

async function loadTags() {
  try {
    tags.value = (await postsApi.listTags('article')).data
  } catch {
    tags.value = []
  }
}

function updateQuery(page = 1) {
  void router.push({
    query: {
      ...(keyword.value ? { q: keyword.value } : {}),
      ...(selectedTag.value ? { tag: selectedTag.value } : {}),
      ...(page > 1 ? { page: String(page) } : {}),
    },
  })
}

function clearFilters() {
  keyword.value = ''
  selectedTag.value = ''
  updateQuery()
}

function coverFor(post: PostSummary, index: number) {
  // 未配置封面时使用站内云景兜底，保证卡片布局完整且不依赖外部图片服务。
  return post.cover_url || fallbackCovers[index % fallbackCovers.length]
}

onMounted(() => { void loadTags() })
watch(() => route.fullPath, () => { void loadPosts() }, { immediate: true })
</script>

<template>
  <PublicLayout class="article-library-page" :show-hero="false" wide>
    <div class="article-library-content">
      <section class="article-library-toolbar" aria-label="文章筛选">
        <label class="article-category-select">
          <span>博客分类</span>
          <select v-model="selectedTag" aria-label="选择博客分类" @change="updateQuery()">
            <option value="">全部分类</option>
            <option v-for="tag in tags" :key="tag.name" :value="tag.name">{{ tag.name }}</option>
          </select>
        </label>

        <form class="article-title-search" @submit.prevent="updateQuery()">
          <label for="article-keyword">博客标题</label>
          <div class="article-search-field">
            <svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="11" cy="11" r="6.5"/><path d="m16 16 4 4"/></svg>
            <input id="article-keyword" v-model="keyword" type="search" placeholder="请输入博客标题进行模糊搜索" />
          </div>
        </form>

        <div class="article-toolbar-actions">
          <RouterLink class="btn" to="/home">
            <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m3 11 9-8 9 8"/><path d="M5 10v10h14V10M9 20v-6h6v6"/></svg>
            首页
          </RouterLink>
          <button class="btn" type="button" @click="clearFilters">重置</button>
          <button class="btn primary" type="button" @click="updateQuery()">
            <svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="11" cy="11" r="6.5"/><path d="m16 16 4 4"/></svg>
            查询
          </button>
        </div>
      </section>

      <section class="article-library-board" aria-live="polite">
        <p v-if="state === 'loading'" class="content-state">正在加载文章…</p>
        <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" @click="loadPosts">重试</button></div>
        <p v-else-if="!posts.length" class="content-state">暂无发布文章</p>
        <div v-else class="article-card-grid">
          <RouterLink v-for="(post, index) in posts" :key="post.id" class="article-card" :to="`/posts/${post.id}`">
            <div class="article-card-cover">
              <img :src="coverFor(post, index)" :alt="`${post.title}封面`" loading="lazy" />
              <span class="article-category">{{ post.tags[0] || '文章' }}</span>
              <h2>{{ post.title }}</h2>
            </div>
            <div class="article-card-body">
              <p>{{ post.summary || '打开文章，继续阅读完整内容。' }}</p>
              <footer>
                <span>
                  <svg viewBox="0 0 24 24" aria-hidden="true"><rect x="3" y="5" width="18" height="16" rx="2"/><path d="M8 3v4M16 3v4M3 10h18"/></svg>
                  {{ post.created_at }}
                </span>
                <span>
                  <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M2.5 12s3.5-6 9.5-6 9.5 6 9.5 6-3.5 6-9.5 6-9.5-6-9.5-6Z"/><circle cx="12" cy="12" r="2.5"/></svg>
                  {{ post.views }}
                </span>
              </footer>
            </div>
          </RouterLink>
        </div>

        <nav v-if="state === 'ready' && result.total_pages > 1" class="article-pagination" aria-label="文章分页">
          <span>共 {{ result.total }} 条</span>
          <button class="btn" :disabled="result.page <= 1" aria-label="上一页" @click="updateQuery(result.page - 1)">‹</button>
          <strong>{{ result.page }}</strong>
          <span>/ {{ result.total_pages }}</span>
          <button class="btn" :disabled="!result.has_more" aria-label="下一页" @click="updateQuery(result.page + 1)">›</button>
        </nav>
      </section>
    </div>
  </PublicLayout>
</template>
