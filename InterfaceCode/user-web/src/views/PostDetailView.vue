<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import PublicLayout from '../components/PublicLayout.vue'
import EmailGate from '../components/EmailGate.vue'
import { commentsApi, postsApi } from '../api'
import { useVisitorStore } from '../stores/visitor'
import { featuredPosts } from '../data/placeholders'
import type { Comment, PostDetail } from '@shared/types'

const route = useRoute()
const visitor = useVisitorStore()
const fallbackPost = featuredPosts.find((item) => String(item.id) === String(route.params.id)) || featuredPosts[0]
const post = ref<PostDetail>({ ...fallbackPost, content_html: '', updated_at: fallbackPost.created_at })
const nickname = ref('')
const content = ref('')
const comments = ref<Comment[]>([{ id: 1, post_id: fallbackPost.id, nickname: '示例读者', content: '这个评论区会优先读取后端评论接口。', created_at: '2026-06-22' }])
const status = ref('')
const commentStatus = ref('')
const postId = computed(() => post.value.id)

async function loadComments() {
  try {
    commentStatus.value = '正在加载评论...'
    const result = await commentsApi.listComments(postId.value, { page: 1, limit: 20 })
    comments.value = result.data
    commentStatus.value = ''
  } catch (error) {
    console.warn('[comments] use placeholder comments:', error)
    commentStatus.value = '评论接口暂不可用，当前显示占位评论。'
  }
}

async function submitComment() {
  if (!visitor.hasEmail || !nickname.value.trim() || !content.value.trim()) return

  try {
    commentStatus.value = '正在提交评论...'
    await commentsApi.createComment(postId.value, { nickname: nickname.value.trim(), email: visitor.email, content: content.value.trim() })
    nickname.value = ''
    content.value = ''
    await loadComments()
  } catch (error) {
    console.warn('[comments] submit failed:', error)
    commentStatus.value = '评论提交失败，请稍后再试。'
  }
}

onMounted(async () => {
  try {
    status.value = '正在加载文章...'
    post.value = await postsApi.getPostDetail(String(route.params.id))
    status.value = ''
  } catch (error) {
    console.warn('[post-detail] use placeholder post:', error)
    status.value = '文章接口暂不可用，当前显示占位内容。'
  } finally {
    await loadComments()
  }
})
</script>

<template>
  <PublicLayout>
    <template #title>{{ post.title }}</template>
    <template #subtitle>{{ post.created_at }} · 阅读 {{ post.views }} · {{ post.tags.join(' / ') }}</template>
    <p v-if="status" class="notice" style="margin-bottom: 14px">{{ status }}</p>
    <article class="panel">
      <div v-if="post.content_html" v-html="post.content_html"></div>
      <template v-else>
        <p>{{ post.summary }}</p>
        <p style="margin-top:16px">这是文章详情页的 Vue 版本结构。真实正文会在后续由 postsApi.getPostDetail 接入后端返回。</p>
        <p style="margin-top:12px">当前阶段优先请求后端，失败时保留占位内容，避免页面白屏。</p>
      </template>
    </article>

    <section class="panel" style="margin-top:18px">
      <p class="section-kicker">Comments</p>
      <h2>评论</h2>
      <EmailGate scene="评论" />
      <form v-if="visitor.hasEmail" class="list" style="margin:18px 0" @submit.prevent="submitComment">
        <label class="field"><span>昵称</span><input v-model="nickname" maxlength="32" required /></label>
        <label class="field"><span>评论内容</span><textarea v-model="content" maxlength="800" required /></label>
        <button class="btn primary" type="submit">提交评论</button>
      </form>
      <p v-if="commentStatus" class="notice" style="margin-bottom: 14px">{{ commentStatus }}</p>
      <div class="list">
        <article v-for="comment in comments" :key="comment.id" class="notice">
          <strong>{{ comment.nickname }}</strong>
          <p>{{ comment.content }}</p>
        </article>
      </div>
    </section>
  </PublicLayout>
</template>
