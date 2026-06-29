<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import { featuredPosts } from '../data/placeholders'
import { postsApi } from '../api'
import type { PostSummary } from '@shared/types'

const posts = ref<PostSummary[]>(featuredPosts)
const status = ref('')

onMounted(async () => {
  try {
    status.value = '正在加载文章...'
    const result = await postsApi.listPosts({ page: 1, limit: 20 })
    posts.value = result.data.length ? result.data : featuredPosts
    status.value = ''
  } catch (error) {
    console.warn('[posts] fallback to placeholders:', error)
    status.value = '后端文章接口暂不可用，当前显示占位内容。'
  }
})
</script>

<template>
  <PublicLayout>
    <template #title>文章</template>
    <template #subtitle>把阶段性的学习、项目和想法整理成更容易回看的文字。</template>
    <p v-if="status" class="notice" style="margin-bottom: 14px">{{ status }}</p>
    <section class="post-list">
      <article v-for="post in posts" :key="post.id" class="card post-item">
        <div>
          <p class="kicker">{{ post.created_at }}</p>
          <h2>{{ post.title }}</h2>
          <p>{{ post.summary }}</p>
          <div class="tags"><span v-for="tag in post.tags" :key="tag" class="tag">{{ tag }}</span></div>
        </div>
        <RouterLink class="btn" :to="`/posts/${post.id}`">查看详情</RouterLink>
      </article>
    </section>
  </PublicLayout>
</template>
