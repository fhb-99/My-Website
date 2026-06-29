<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import { featuredPosts, notes } from '../data/placeholders'
import { postsApi } from '../api'
import type { PostSummary } from '@shared/types'

const posts = ref<PostSummary[]>(featuredPosts)
const featured = computed(() => posts.value[0] || featuredPosts[0])

onMounted(async () => {
  try {
    const result = await postsApi.listPosts({ page: 1, limit: 6 })
    if (result.data.length) posts.value = result.data
  } catch (error) {
    console.warn('[home] use placeholder posts:', error)
  }
})
</script>

<template>
  <PublicLayout>
    <template #title>My Blog</template>
    <template #subtitle>Life is a coding, I will debug it.</template>

    <article class="featured">
      <div class="featured-cover"><p class="featured-mark">{{ featured.title }}</p></div>
      <div class="featured-body">
        <h2>{{ featured.title }}</h2>
        <div class="meta-row"><span>{{ featured.created_at }}</span><span>阅读 {{ featured.views }}</span><span>{{ featured.tags.join(' · ') }}</span></div>
      </div>
    </article>

    <div class="layout">
      <section class="home-fill">
        <div class="fill-card">
          <h3>快捷入口</h3>
          <div class="quick-grid">
            <RouterLink class="quick" to="/posts"><strong>文章</strong><span>→</span></RouterLink>
            <RouterLink class="quick" to="/notes"><strong>碎碎念</strong><span>→</span></RouterLink>
            <RouterLink class="quick" to="/projects"><strong>项目</strong><span>→</span></RouterLink>
            <RouterLink class="quick" to="/guestbook"><strong>留言板</strong><span>→</span></RouterLink>
          </div>
        </div>

        <div class="fill-card">
          <h3>最近更新</h3>
          <ul class="mini-list">
            <li v-for="post in posts" :key="post.id">
              <RouterLink :to="`/posts/${post.id}`">{{ post.title }}</RouterLink>
              <div class="mini-meta">{{ post.created_at }} · {{ post.tags.join(' / ') }}</div>
            </li>
          </ul>
        </div>

        <section>
          <div class="section-head"><div><p class="section-kicker">Articles</p><h2>最新文章</h2></div><RouterLink class="nav-link" to="/posts">全部文章 →</RouterLink></div>
          <div class="post-list">
            <article v-for="post in posts" :key="post.id" class="card post-item">
              <div>
                <p class="kicker">{{ post.created_at }}</p>
                <h3>{{ post.title }}</h3>
                <p>{{ post.summary }}</p>
                <div class="tags"><span v-for="tag in post.tags" :key="tag" class="tag">{{ tag }}</span></div>
              </div>
              <RouterLink class="btn" :to="`/posts/${post.id}`">阅读</RouterLink>
            </article>
          </div>
        </section>
      </section>

      <aside class="sidebar">
        <div class="widget"><h3>公告</h3><p>欢迎光临这个小角落：记录学习、项目复盘，偶尔堆一点排版练习。</p></div>
        <div class="widget"><h3>站长寄语</h3><p>慢慢来，先把版面收拾顺眼，再一点点把文字填满。</p></div>
        <div class="widget"><h3>最近碎碎念</h3><ul class="widget-list"><li v-for="note in notes" :key="note.id">{{ note.content }}</li></ul></div>
        <div class="widget"><h3>导航</h3><ul class="widget-list"><li><RouterLink to="/posts">文章</RouterLink></li><li><RouterLink to="/about">关于</RouterLink></li><li><RouterLink to="/guestbook">留言板</RouterLink></li></ul></div>
      </aside>
    </div>
  </PublicLayout>
</template>
