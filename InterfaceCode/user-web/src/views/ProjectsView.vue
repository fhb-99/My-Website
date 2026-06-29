<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import { projects as placeholderProjects } from '../data/placeholders'
import { projectsApi } from '../api'
import type { ProjectItem } from '@shared/types'

const projects = ref<ProjectItem[]>(placeholderProjects)
const status = ref('')

onMounted(async () => {
  try {
    status.value = '正在加载项目...'
    const result = await projectsApi.listProjects()
    projects.value = result.length ? result : placeholderProjects
    status.value = ''
  } catch (error) {
    console.warn('[projects] use placeholders:', error)
    status.value = '项目接口暂不可用，当前显示占位内容。'
  }
})
</script>

<template>
  <PublicLayout>
    <template #title>项目</template>
    <template #subtitle>把做过的东西整理出来，给未来的自己留一个清晰入口。</template>
    <p v-if="status" class="notice" style="margin-bottom: 14px">{{ status }}</p>
    <section class="grid"><article v-for="project in projects" :key="project.id" class="card"><p class="kicker">Project</p><h2>{{ project.name }}</h2><p>{{ project.summary }}</p><div class="tags"><span v-for="tag in project.tags" :key="tag" class="tag">{{ tag }}</span></div></article></section>
  </PublicLayout>
</template>
