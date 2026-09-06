<script setup lang="ts">
import { onMounted, ref } from 'vue'

import PublicLayout from '../components/PublicLayout.vue'
import { projectsApi } from '../api'
import type { PageResult, ProjectItem } from '@shared/types'

const result = ref<PageResult<ProjectItem>>({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')

async function loadProjects(page = 1) {
  state.value = 'loading'
  error.value = ''
  try {
    result.value = await projectsApi.listProjects({ page, limit: 10 })
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '项目加载失败，请稍后重试。'
    result.value = { data: [], page, limit: 10, total: 0, total_pages: 0, has_more: false }
  }
}

onMounted(() => { void loadProjects() })
</script>

<template>
  <PublicLayout><template #title>项目</template><template #subtitle>把做过的东西整理出来，给未来的自己留一个清晰入口。</template>
    <p v-if="state === 'loading'" class="content-state">正在加载项目…</p>
    <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" @click="loadProjects()">重试</button></div>
    <p v-else-if="!result.data.length" class="content-state">暂无公开项目</p>
    <section v-else class="grid"><article v-for="project in result.data" :key="project.id" class="card"><p class="kicker">Project</p><h2>{{ project.name }}</h2><p>{{ project.summary }}</p><div class="tags"><span v-for="tag in project.tags" :key="tag" class="tag">{{ tag }}</span></div><a v-if="project.url" class="btn project-link" :href="project.url" target="_blank" rel="noreferrer">访问项目</a></article></section>
    <nav v-if="state === 'ready' && result.total_pages > 1" class="pagination"><button class="btn" :disabled="result.page <= 1" @click="loadProjects(result.page - 1)">上一页</button><span>{{ result.page }} / {{ result.total_pages }}</span><button class="btn" :disabled="!result.has_more" @click="loadProjects(result.page + 1)">下一页</button></nav>
  </PublicLayout>
</template>
