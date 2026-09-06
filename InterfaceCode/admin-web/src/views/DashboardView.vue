<script setup lang="ts">
import { onMounted, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminContentApi, adminPostsApi, moderationApi } from '../api'

const stats = ref([
  { label: '文章', value: '...' },
  { label: '八股文', value: '...' },
  { label: '碎碎念', value: '...' },
  { label: '项目', value: '...' },
  { label: '待审核', value: '...' }
])
const notice = ref('')

async function loadDashboard() {
  try {
    const [posts, interviews, notes, projects, comments, guestbook] = await Promise.all([
      adminPostsApi.listPosts({ page: 1, limit: 1, contentType: 'article' }),
      adminPostsApi.listPosts({ page: 1, limit: 1, contentType: 'interview' }),
      adminContentApi.listNotes({ page: 1, limit: 1 }),
      adminContentApi.listProjects({ page: 1, limit: 1 }),
      moderationApi.listComments({ page: 1, limit: 50 }),
      moderationApi.listGuestbook({ page: 1, limit: 50 })
    ])
    stats.value[0].value = String(posts.total)
    stats.value[1].value = String(interviews.total)
    stats.value[2].value = String(notes.total)
    stats.value[3].value = String(projects.total)
    stats.value[4].value = String([
      ...comments.data,
      ...guestbook.data
    ].filter((item) => item.status === 'pending').length)
    notice.value = ''
  } catch (error) {
    console.warn('[dashboard] load failed:', error)
    stats.value.forEach((item) => { item.value = '未知' })
    notice.value = error instanceof Error ? error.message : '概览数据加载失败'
  }
}

onMounted(loadDashboard)
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>数据概览</h1><p class="muted">博客内容与互动数据的快速概览。</p></div></template>
    <p v-if="notice" class="notice">{{ notice }}</p>
    <section class="grid dashboard-grid">
      <article v-for="(item, index) in stats" :key="item.label" class="card stat-card">
        <span class="stat-icon">{{ ['▤', '▥', '◫', '◇', '◎'][index] }}</span>
        <div><p class="muted">{{ item.label }}</p><h2>{{ item.value }}</h2></div>
      </article>
    </section>
  </AdminLayout>
</template>

<style scoped>
.stat-card { display: flex; align-items: center; gap: 16px; }
.stat-card p, .stat-card h2 { margin: 0; }
.stat-card h2 { margin-top: 5px; font-size: 27px; }
.stat-icon { display: grid; place-items: center; width: 44px; height: 44px; border-radius: 7px; color: #69a9f6; background: rgba(39,119,220,.13); font-size: 22px; }
</style>
