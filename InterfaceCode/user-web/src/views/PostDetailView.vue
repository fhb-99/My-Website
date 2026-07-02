<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
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
const content = ref('')
const comments = ref<Comment[]>([
  { id: 1, post_id: fallbackPost.id, nickname: '示例读者', content: '这个评论区会优先读取后端评论接口。', created_at: '2026-06-22 21:30:00' }
])
const status = ref('')
const commentStatus = ref('')
const postId = computed(() => post.value.id)
const viewReported = ref(false)
let viewTimer: number | undefined

function formatMinute(value: string) {
  const matched = value.match(/^(\d{4}-\d{2}-\d{2})(?:[ T](\d{2}:\d{2}))?/)
  if (matched) return `${matched[1]} ${matched[2] || '00:00'}`

  const date = new Date(value)
  if (Number.isNaN(date.getTime())) return value.slice(0, 16).replace('T', ' ')

  const pad = (num: number) => String(num).padStart(2, '0')
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}`
}

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
  if (!visitor.hasProfile || !content.value.trim()) return

  try {
    commentStatus.value = '正在提交评论...'
    await commentsApi.createComment(postId.value, {
      nickname: visitor.nickname,
      email: visitor.email,
      content: content.value.trim()
    })
    content.value = ''
    await loadComments()
  } catch (error) {
    console.warn('[comments] submit failed:', error)
    commentStatus.value = '评论提交失败，请稍后再试。'
  }
}

async function reportPostView() {
  if (viewReported.value || !postId.value) return

  try {
    const result = await postsApi.recordPostView(postId.value)
    viewReported.value = true
    if (result.counted) {
      post.value = { ...post.value, views: post.value.views + 1 }
    }
  } catch (error) {
    console.warn('[post-view] report failed:', error)
  }
}

function scheduleViewReport() {
  window.clearTimeout(viewTimer)
  viewReported.value = false
  // 用户停留一小段时间后再上报，避免刷新或误点立即增加阅读量。
  viewTimer = window.setTimeout(() => {
    void reportPostView()
  }, 10000)
}

onMounted(async () => {
  try {
    status.value = '正在加载文章...'
    post.value = await postsApi.getPostDetail(String(route.params.id))
    status.value = ''
    scheduleViewReport()
  } catch (error) {
    console.warn('[post-detail] use placeholder post:', error)
    status.value = '文章接口暂不可用，当前显示占位内容。'
  } finally {
    await loadComments()
  }
})

onBeforeUnmount(() => {
  window.clearTimeout(viewTimer)
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

    <section class="panel comment-panel">
      <p class="section-kicker">Comments</p>
      <h2>评论</h2>
      <p v-if="commentStatus" class="notice">{{ commentStatus }}</p>

      <div class="comment-list">
        <article v-for="comment in comments" :key="comment.id" class="message-card">
          <div class="message-body">
            <div class="avatar">{{ comment.nickname.slice(0, 1) }}</div>
            <div>
              <strong>{{ comment.nickname }}</strong>
              <p>{{ comment.content }}</p>
            </div>
          </div>
          <time class="message-time">{{ formatMinute(comment.created_at) }}</time>
        </article>
      </div>

      <div class="composer-block">
        <EmailGate scene="评论" />
        <form v-if="visitor.hasProfile" class="composer" @submit.prevent="submitComment">
          <label class="field">
            <span>{{ visitor.nickname }}，写下你的评论</span>
            <textarea v-model="content" maxlength="800" required placeholder="认真读完后的想法，可以从这里开始。" />
          </label>
          <button class="btn primary" type="submit">提交评论</button>
        </form>
      </div>
    </section>
  </PublicLayout>
</template>

<style scoped>
.comment-panel { margin-top: 18px; }
.comment-panel h2 { margin: 8px 0 16px; }
.comment-list { display: grid; gap: 12px; margin-top: 14px; }
.message-card {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 16px;
  align-items: start;
  border: 1px solid var(--line);
  border-radius: 18px;
  padding: 16px 18px;
  background: linear-gradient(135deg, var(--surface-strong), var(--surface));
}
.message-body { display: flex; gap: 12px; min-width: 0; }
.avatar { width: 38px; height: 38px; flex: 0 0 auto; display: grid; place-items: center; border-radius: 14px; background: var(--accent-soft); color: var(--accent); font-weight: 900; }
.message-body strong { display: block; margin-top: 1px; }
.message-body p { margin: 6px 0 0; color: var(--text-dim); line-height: 1.75; overflow-wrap: anywhere; }
.message-time { justify-self: end; color: var(--muted); font-size: 12px; white-space: nowrap; }
.composer-block { margin-top: 18px; }
.composer { display: grid; gap: 14px; margin-top: 14px; }
@media (max-width: 640px) { .message-card { grid-template-columns: 1fr; } .message-time { justify-self: start; } }
</style>
