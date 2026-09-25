<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import PublicLayout from '../components/PublicLayout.vue'
import { notesApi, postsApi, projectsApi } from '../api'
import type { NoteItem, PageResult, PostSummary, ProjectItem } from '@shared/types'

const route = useRoute()
const router = useRouter()
const keyword = ref('')
const posts = ref<PageResult<PostSummary>>({ data: [], page: 1, limit: 50, total: 0, total_pages: 0, has_more: false })
const projects = ref<PageResult<ProjectItem>>({ data: [], page: 1, limit: 50, total: 0, total_pages: 0, has_more: false })
const notes = ref<PageResult<NoteItem>>({ data: [], page: 1, limit: 50, total: 0, total_pages: 0, has_more: false })
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')
const total = computed(() => posts.value.total + projects.value.total + notes.value.total)

function postPath(post: PostSummary) {
  return post.content_type === 'interview' ? `/interview/${post.id}` : `/posts/${post.id}`
}

function noteExcerpt(content: string) {
  return content.length > 160 ? `${content.slice(0, 160)}…` : content
}

async function loadResults() {
  const q = typeof route.query.q === 'string' ? route.query.q.trim() : ''
  keyword.value = q
  if (!q) {
    state.value = 'ready'
    return
  }

  state.value = 'loading'
  error.value = ''
  try {
    const [postResult, projectResult, noteResult] = await Promise.all([
      postsApi.listPosts({ page: 1, limit: 50, q }),
      projectsApi.listProjects({ page: 1, limit: 50, q }),
      notesApi.listNotes({ page: 1, limit: 50, q }),
    ])
    posts.value = postResult
    projects.value = projectResult
    notes.value = noteResult
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '搜索失败，请稍后重试。'
  }
}

function search() {
  const q = keyword.value.trim()
  if (q) void router.push({ path: '/search', query: { q } })
}

watch(() => route.query.q, () => { void loadResults() }, { immediate: true })
</script>

<template>
  <PublicLayout>
    <template #title>站内搜索</template>
    <template #subtitle>搜索已公开的文章、八股文、项目和随记。</template>

    <section class="knowledge-toolbar">
      <form class="knowledge-search" @submit.prevent="search">
        <span aria-hidden="true">⌕</span>
        <input v-model="keyword" type="search" required placeholder="输入站内搜索词" aria-label="输入站内搜索词" />
        <button class="btn primary" type="submit">搜索</button>
      </form>
    </section>

    <p v-if="!keyword && state === 'ready'" class="content-state">请输入搜索词</p>
    <p v-else-if="state === 'loading'" class="content-state">正在搜索站内内容…</p>
    <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" type="button" @click="loadResults">重试</button></div>
    <p v-else-if="!total" class="content-state">没有找到与“{{ keyword }}”相关的公开内容</p>

    <template v-else>
      <p class="site-search-summary">找到 {{ total }} 条与“{{ keyword }}”相关的公开内容</p>
      <div class="site-search-groups">
        <section v-if="posts.data.length">
          <div class="site-search-heading"><h2>文章与八股文</h2><span>{{ posts.total }} 条</span></div>
          <div class="list">
            <RouterLink v-for="post in posts.data" :key="post.id" class="card site-search-result" :to="postPath(post)">
              <div class="mini-meta"><span class="tag">{{ post.content_type === 'interview' ? '八股文' : '文章' }}</span><span>{{ post.created_at }}</span></div>
              <h3>{{ post.title }}</h3>
              <p>{{ post.summary || '打开查看正文' }}</p>
            </RouterLink>
          </div>
        </section>

        <section v-if="projects.data.length">
          <div class="site-search-heading"><h2>项目</h2><span>{{ projects.total }} 条</span></div>
          <div class="grid">
            <article v-for="project in projects.data" :key="project.id" class="card site-search-result">
              <p class="kicker">Project</p>
              <h3>{{ project.name }}</h3>
              <p>{{ project.summary }}</p>
              <a v-if="project.url" class="btn project-link" :href="project.url" target="_blank" rel="noopener noreferrer">访问项目</a>
            </article>
          </div>
        </section>

        <section v-if="notes.data.length">
          <div class="site-search-heading"><h2>随记</h2><span>{{ notes.total }} 条</span></div>
          <div class="list">
            <article v-for="note in notes.data" :key="note.id" class="card site-search-result">
              <p>{{ noteExcerpt(note.content) }}</p>
              <div class="mini-meta"><span>{{ note.created_at }}</span><span v-if="note.mood">{{ note.mood }}</span></div>
            </article>
          </div>
        </section>
      </div>
    </template>
  </PublicLayout>
</template>
