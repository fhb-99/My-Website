<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import PublicLayout from '../components/PublicLayout.vue'
import { learningRoadmapApi, postsApi } from '../api'
import { DEFAULT_INTERVIEW_CATEGORIES } from '@shared/types'
import type { InterviewDirectory, LearningRoadmap, PageResult, PostSummary } from '@shared/types'

const route = useRoute()
const router = useRouter()
const result = ref<PageResult<PostSummary>>({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
const directories = ref<InterviewDirectory[]>(DEFAULT_INTERVIEW_CATEGORIES.map((name) => ({ name, post_count: 0, posts: [] })))
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')
const keyword = ref('')
const selectedCategory = ref('')
const topicsCollapsed = ref(false)
const expandedCategories = ref<string[]>([])
const roadmap = ref<LearningRoadmap | null>(null)
const roadmapState = ref<'loading' | 'ready' | 'unavailable'>('loading')

const posts = computed(() => result.value.data)
const currentTopic = computed(() => selectedCategory.value || '搜索结果')
const hasSelection = computed(() => Boolean(
  selectedCategory.value || (typeof route.query.q === 'string' && route.query.q.trim()),
))

function readQuery() {
  keyword.value = typeof route.query.q === 'string' ? route.query.q : ''
  selectedCategory.value = typeof route.query.category === 'string' ? route.query.category : ''
  if (selectedCategory.value && !expandedCategories.value.includes(selectedCategory.value)) {
    expandedCategories.value.push(selectedCategory.value)
  }
}

async function loadPosts() {
  readQuery()
  state.value = 'loading'
  error.value = ''
  const page = Math.max(1, Number(route.query.page) || 1)
  try {
    result.value = await postsApi.listPosts({ page, limit: 10, q: keyword.value, category: selectedCategory.value, contentType: 'interview' })
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '八股文加载失败，请稍后重试。'
    result.value = { data: [], page, limit: 10, total: 0, total_pages: 0, has_more: false }
  }
}

async function loadDirectories() {
  try {
    directories.value = (await postsApi.listInterviewDirectory()).data
  } catch {
    // 接口暂时不可用时仍保留预设目录，让页面结构可见。
  }
}

async function loadRoadmap() {
  try {
    roadmap.value = await learningRoadmapApi.get()
    roadmapState.value = 'ready'
  } catch {
    // 独立内容未发布时不回退到八股文目录，避免两套数据重新耦合。
    roadmap.value = null
    roadmapState.value = 'unavailable'
  }
}

function updateQuery(page = 1) {
  void router.push({
    query: {
      ...(keyword.value ? { q: keyword.value } : {}),
      ...(selectedCategory.value ? { category: selectedCategory.value } : {}),
      ...(page > 1 ? { page: String(page) } : {}),
    },
  })
}

function setCategory(category: string) {
  selectedCategory.value = category
  keyword.value = ''
  if (!expandedCategories.value.includes(category)) expandedCategories.value.push(category)
  updateQuery()
}

function toggleCategory(category: string) {
  if (expandedCategories.value.includes(category)) {
    expandedCategories.value = expandedCategories.value.filter((item) => item !== category)
    if (selectedCategory.value === category) clearFilters()
    return
  }
  setCategory(category)
}

function clearFilters() {
  keyword.value = ''
  selectedCategory.value = ''
  updateQuery()
}

onMounted(() => { void loadDirectories(); void loadRoadmap() })
watch(() => route.fullPath, () => { void loadPosts() }, { immediate: true })
</script>

<template>
  <PublicLayout :show-hero="false" wide>
    <section class="knowledge-directory interview-directory" :class="{ 'topics-collapsed': topicsCollapsed }">
      <aside class="knowledge-topics" :class="{ collapsed: topicsCollapsed }" aria-label="八股文主题目录">
        <button class="knowledge-topics-heading" type="button" :aria-expanded="!topicsCollapsed" @click="topicsCollapsed = !topicsCollapsed">
          <span><strong>八股文目录</strong><small>TOPICS</small></span>
          <i aria-hidden="true">‹</i>
        </button>
        <nav>
          <button type="button" :class="{ active: !selectedCategory }" @click="clearFilters">
            <span>全部目录</span><small>{{ directories.length }}</small>
          </button>
          <div v-for="directory in directories" :key="directory.name" class="topic-tree-node">
            <button class="topic-directory-toggle" type="button" :class="{ active: selectedCategory === directory.name }" :aria-expanded="expandedCategories.includes(directory.name)" @click="toggleCategory(directory.name)">
              <span><i aria-hidden="true">›</i>{{ directory.name }}</span><small>{{ directory.post_count }}</small>
            </button>
            <Transition name="topic-files">
              <div v-if="expandedCategories.includes(directory.name)" class="topic-files expanded">
                <RouterLink v-for="post in directory.posts" :key="post.id" :to="`/interview/${post.id}`">{{ post.title }}</RouterLink>
                <small v-if="!directory.posts.length">暂无文件</small>
              </div>
            </Transition>
          </div>
        </nav>
      </aside>

      <div class="knowledge-results">
        <section class="knowledge-toolbar">
          <form class="knowledge-search" @submit.prevent="updateQuery()">
            <span>⌕</span>
            <input v-model="keyword" type="search" placeholder="搜索面试题或技术主题" aria-label="搜索八股文" />
            <button class="btn primary" type="submit">查询</button>
            <button v-if="keyword || selectedCategory" class="btn" type="button" @click="clearFilters">重置</button>
          </form>
        </section>

        <section v-if="!hasSelection" class="interview-welcome">
          <img class="interview-landing-image" :src="roadmap?.cover_url || '/backgrounds/knowledge-sky-v1.png'" alt="C++ 学习路线封面" />
          <RouterLink v-if="roadmap" class="learning-roadmap-card" to="/learning-roadmap">
            <span class="learning-roadmap-mark" aria-hidden="true">C++</span>
            <span><small>LEARNING ROADMAP</small><strong>{{ roadmap.title }}</strong><p>{{ roadmap.subtitle }}</p></span>
            <i aria-hidden="true">→</i>
          </RouterLink>
          <p v-else class="content-state">{{ roadmapState === 'loading' ? '正在加载学习路线…' : '学习路线暂未发布' }}</p>
        </section>

        <template v-else>
          <header v-if="state === 'ready'" class="knowledge-section-heading">
            <div><small>INTERVIEW NOTES</small><h2>{{ currentTopic }}</h2></div>
            <span>共 {{ result.total }} 篇</span>
          </header>
          <p v-if="state === 'loading'" class="content-state">正在加载八股文…</p>
          <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" @click="loadPosts">重试</button></div>
          <p v-else-if="!posts.length" class="content-state">暂无发布内容</p>
          <section v-else class="knowledge-list">
            <article v-for="(post, index) in posts" :key="post.id" class="knowledge-item">
              <div class="knowledge-index">{{ String((result.page - 1) * result.limit + index + 1).padStart(2, '0') }}</div>
              <div class="knowledge-copy">
                <div class="knowledge-meta"><span>{{ post.interview_category || '未分类' }}</span><span>{{ post.created_at }}</span><span>阅读 {{ post.views }}</span></div>
                <h2><RouterLink :to="`/interview/${post.id}`">{{ post.title }}</RouterLink></h2>
                <p>{{ post.summary }}</p>
              </div>
              <RouterLink class="knowledge-open" :to="`/interview/${post.id}`" aria-label="查看八股文">→</RouterLink>
            </article>
          </section>
          <nav v-if="state === 'ready' && result.total_pages > 1" class="pagination" aria-label="八股文分页">
            <button class="btn" :disabled="result.page <= 1" @click="updateQuery(result.page - 1)">上一页</button>
            <span>{{ result.page }} / {{ result.total_pages }}</span>
            <button class="btn" :disabled="!result.has_more" @click="updateQuery(result.page + 1)">下一页</button>
          </nav>
        </template>
      </div>
    </section>
  </PublicLayout>
</template>
