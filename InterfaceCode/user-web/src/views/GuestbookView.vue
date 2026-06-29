<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import EmailGate from '../components/EmailGate.vue'
import { guestbookApi } from '../api'
import { useVisitorStore } from '../stores/visitor'
import type { GuestbookMessage } from '@shared/types'

const visitor = useVisitorStore()
const nickname = ref('')
const content = ref('')
const messages = ref<GuestbookMessage[]>([{ id: 1, nickname: '示例访客', content: '留言板 UI 已准备好，后续接 guestbookApi。', created_at: '2026-06-22' }])
const status = ref('')

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
  if (!visitor.hasEmail || !nickname.value.trim() || !content.value.trim()) return

  try {
    status.value = '正在提交留言...'
    await guestbookApi.createMessage({ nickname: nickname.value.trim(), email: visitor.email, content: content.value.trim() })
    nickname.value = ''
    content.value = ''
    await loadMessages()
  } catch (error) {
    console.warn('[guestbook] submit failed:', error)
    status.value = '留言提交失败，可能是后端留言接口还未实现。'
  }
}

onMounted(loadMessages)
</script>

<template>
  <PublicLayout>
    <template #title>留言板</template>
    <template #subtitle>先登记邮箱，再留下想说的话；如果已在评论区登记过邮箱，这里会自动跳过。</template>
    <section class="layout">
      <div class="panel">
        <EmailGate scene="留言" />
        <form v-if="visitor.hasEmail" class="list" style="margin-top:18px" @submit.prevent="submitMessage">
          <label class="field"><span>昵称</span><input v-model="nickname" required /></label>
          <label class="field"><span>留言</span><textarea v-model="content" required /></label>
          <button class="btn primary" type="submit">提交留言</button>
        </form>
      </div>
      <aside class="sidebar"><div class="widget"><h3>说明</h3><p>邮箱只用于联系和基础识别，不会在公开页面展示。</p></div></aside>
    </section>
    <p v-if="status" class="notice" style="margin-top: 18px">{{ status }}</p>
    <section class="list" style="margin-top:18px">
      <article v-for="message in messages" :key="message.id" class="card"><strong>{{ message.nickname }}</strong><p>{{ message.content }}</p></article>
    </section>
  </PublicLayout>
</template>
