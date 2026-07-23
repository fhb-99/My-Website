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
    result.value = await postsApi.listPosts({ page, limit: 10, q: keyword.value, tag: selectedTag.value })
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '文章加载失败，请稍后重试。'
    result.value = { data: [], page, limit: 10, total: 0, total_pages: 0, has_more: false }
  }
}

async function loadTags() {
  try {
    tags.value = (await postsApi.listTags()).data
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

function setTag(tag: string) {
  selectedTag.value = selectedTag.value === tag ? '' : tag
  updateQuery()
}

function clearFilters() {
  keyword.value = ''
  selectedTag.value = ''
  updateQuery()
}

onMounted(() => { void loadTags() })
watch(() => route.fullPath, () => { void loadPosts() }, { immediate: true })
</script>

<template>
  <PublicLayout>
    <template #title>文章</template>
    <template #subtitle>把阶段性的学习、项目和想法整理成更容易回看的文字。</template>

    <form class="filter-bar" @submit.prevent="updateQuery()">
      <input v-model="keyword" type="search" placeholder="搜索文章" aria-label="搜索文章" />
      <button class="btn" type="submit">搜索</button>
      <button v-if="keyword || selectedTag" class="btn" type="button" @click="clearFilters">清除筛选</button>
    </form>
    <div v-if="tags.length" class="tag-filter" aria-label="标签筛选">
      <button v-for="tag in tags" :key="tag.name" class="tag" :class="{ active: selectedTag === tag.name }" type="button" @click="setTag(tag.name)">
        {{ tag.name }} ({{ tag.post_count }})
      </button>
    </div>

    <p v-if="state === 'loading'" class="content-state">正在加载文章…</p>
    <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" @click="loadPosts">重试</button></div>
    <p v-else-if="!posts.length" class="content-state">暂无发布文章</p>
    <section v-else class="post-list">
      <article v-for="post in posts" :key="post.id" class="card post-item">
        <div><p class="kicker">{{ post.created_at }}</p><h2>{{ post.title }}</h2><p>{{ post.summary }}</p><div class="tags"><span v-for="tag in post.tags" :key="tag" class="tag">{{ tag }}</span></div></div>
        <RouterLink class="btn" :to="`/posts/${post.id}`">查看详情</RouterLink>
      </article>
    </section>
    <nav v-if="state === 'ready' && result.total_pages > 1" class="pagination" aria-label="文章分页">
      <button class="btn" :disabled="result.page <= 1" @click="updateQuery(result.page - 1)">上一页</button>
      <span>{{ result.page }} / {{ result.total_pages }}</span>
      <button class="btn" :disabled="!result.has_more" @click="updateQuery(result.page + 1)">下一页</button>
    </nav>
  </PublicLayout>
</template>
