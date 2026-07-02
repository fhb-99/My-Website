<script setup lang="ts">
import { onMounted, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminPostsApi } from '../api'

const stats = ref([
  { label: '文章', value: '...' },
  { label: '待审核', value: '待接入' },
  { label: '上传资源', value: '待接入' }
])
const notice = ref('')

async function loadDashboard() {
  try {
    const result = await adminPostsApi.listPosts({ page: 1, limit: 1 })
    stats.value[0].value = String(result.total)
    notice.value = ''
  } catch (error) {
    console.warn('[dashboard] load failed:', error)
    stats.value[0].value = '未知'
    notice.value = error instanceof Error ? error.message : '概览数据加载失败'
  }
}

onMounted(loadDashboard)
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>管理概览</h1><p class="muted">这里展示管理端的关键数据。</p></div></template>
    <p v-if="notice" class="card muted">{{ notice }}</p>
    <section class="grid"><article v-for="item in stats" :key="item.label" class="card"><p class="muted">{{ item.label }}</p><h2>{{ item.value }}</h2></article></section>
  </AdminLayout>
</template>