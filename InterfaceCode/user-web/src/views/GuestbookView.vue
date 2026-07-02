<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import EmailGate from '../components/EmailGate.vue'
import { guestbookApi } from '../api'
import { useVisitorStore } from '../stores/visitor'
import type { GuestbookMessage } from '@shared/types'

const visitor = useVisitorStore()
const content = ref('')
const messages = ref<GuestbookMessage[]>([
  { id: 1, nickname: '示例访客', content: '留言板 UI 已准备好，后续接 guestbookApi。', created_at: '2026-06-22 21:30:00' }
])
const status = ref('')

function formatMinute(value: string) {
  const matched = value.match(/^(\d{4}-\d{2}-\d{2})(?:[ T](\d{2}:\d{2}))?/)
  if (matched) return `${matched[1]} ${matched[2] || '00:00'}`

  const date = new Date(value)
  if (Number.isNaN(date.getTime())) return value.slice(0, 16).replace('T', ' ')

  const pad = (num: number) => String(num).padStart(2, '0')
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}`
}

async function loadMessages() {
  try {
    status.value = '正在加载留言...'
    const result = await guestbookApi.listMessages({ page: 1, limit: 20 })
    messages.value = result.data
    status.value = ''
  } catch (error) {
    console.warn('[guestbook] use placeholders:', error)
    status.value = '留言接口暂不可用，当前显示占位内容。'
  }
}

async function submitMessage() {
  if (!visitor.hasProfile || !content.value.trim()) return

  try {
    status.value = '正在提交留言...'
    await guestbookApi.createMessage({
      nickname: visitor.nickname,
      email: visitor.email,
      content: content.value.trim()
    })
    content.value = ''
    await loadMessages()
  } catch (error) {
    console.warn('[guestbook] submit failed:', error)
    status.value = '留言提交失败，请稍后再试。'
  }
}

onMounted(loadMessages)
</script>

<template>
  <PublicLayout>
    <template #title>留言板</template>
    <template #subtitle>先登记邮箱和昵称，再留下想说的话；如果已在评论区登记过，这里会自动跳过。</template>

    <section class="layout">
      <div class="panel guestbook-panel">
        <p class="section-kicker">Guestbook</p>
        <h2>大家留下的话</h2>
        <p v-if="status" class="notice">{{ status }}</p>

        <section class="message-list">
          <article v-for="message in messages" :key="message.id" class="message-card">
            <div class="message-body">
              <div class="avatar">{{ message.nickname.slice(0, 1) }}</div>
              <div>
                <strong>{{ message.nickname }}</strong>
                <p>{{ message.content }}</p>
              </div>
            </div>
            <time class="message-time">{{ formatMinute(message.created_at) }}</time>
          </article>
        </section>

        <div class="composer-block">
          <EmailGate scene="留言" />
          <form v-if="visitor.hasProfile" class="composer" @submit.prevent="submitMessage">
            <label class="field">
              <span>{{ visitor.nickname }}，留下你的留言</span>
              <textarea v-model="content" maxlength="800" required placeholder="写一点想说的话，给之后路过这里的人。" />
            </label>
            <button class="btn primary" type="submit">提交留言</button>
          </form>
        </div>
      </div>

      <aside class="sidebar">
        <div class="widget">
          <h3>说明</h3>
          <p>邮箱只用于联系和基础识别，不会在公开页面展示。登记一次后，评论和留言都会复用同一个昵称。</p>
        </div>
      </aside>
    </section>
  </PublicLayout>
</template>

<style scoped>
.guestbook-panel h2 { margin: 8px 0 16px; }
.message-list { display: grid; gap: 12px; margin-top: 14px; }
.message-card {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 16px;
  align-items: start;
  border: 1px solid var(--line);
  border-radius: 20px;
  padding: 16px 18px;
  background:
    radial-gradient(circle at 0% 0%, rgba(47, 124, 246, 0.1), transparent 28%),
    linear-gradient(135deg, var(--surface-strong), var(--surface));
  box-shadow: 0 12px 30px rgba(47, 91, 140, 0.08);
}
.message-body { display: flex; gap: 12px; min-width: 0; }
.avatar { width: 40px; height: 40px; flex: 0 0 auto; display: grid; place-items: center; border-radius: 15px; background: var(--accent-soft); color: var(--accent); font-weight: 900; }
.message-body strong { display: block; margin-top: 1px; }
.message-body p { margin: 6px 0 0; color: var(--text-dim); line-height: 1.75; overflow-wrap: anywhere; }
.message-time { justify-self: end; color: var(--muted); font-size: 12px; white-space: nowrap; }
.composer-block { margin-top: 20px; }
.composer { display: grid; gap: 14px; margin-top: 14px; }
@media (max-width: 640px) { .message-card { grid-template-columns: 1fr; } .message-time { justify-self: start; } }
</style>
