<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import EmailGate from '../components/EmailGate.vue'
import { guestbookApi } from '../api'
import { useVisitorStore } from '../stores/visitor'
import type { GuestbookMessage } from '@shared/types'

const visitor = useVisitorStore()
const content = ref('')
const messages = ref<GuestbookMessage[]>([])
const state = ref<'loading' | 'ready' | 'error'>('loading')
const message = ref('')

async function loadMessages() { state.value = 'loading'; message.value = ''; try { messages.value = (await guestbookApi.listMessages({ page: 1, limit: 20 })).data; state.value = 'ready' } catch (error) { messages.value = []; state.value = 'error'; message.value = error instanceof Error ? error.message : '留言加载失败，请稍后重试。' } }
async function submitMessage() { if (!visitor.hasProfile || !content.value.trim()) return; message.value = '正在提交留言…'; try { const result = await guestbookApi.createMessage({ nickname: visitor.nickname, email: visitor.email, content: content.value.trim() }); content.value = ''; message.value = result.is_approved ? '留言已发布。' : '已提交，等待审核。'; await loadMessages() } catch { message.value = '留言提交失败，请稍后再试。' } }
onMounted(() => { void loadMessages() })
</script>

<template><PublicLayout><template #title>留言板</template><template #subtitle>先登记邮箱和昵称，再留下想说的话；公开展示只会读取已通过审核的留言。</template><section class="layout"><div class="panel guestbook-panel"><p class="section-kicker">Guestbook</p><h2>大家留下的话</h2><p v-if="state === 'loading'" class="content-state">正在加载留言…</p><div v-else-if="state === 'error'" class="content-state"><p>{{ message }}</p><button class="btn" @click="loadMessages">重试</button></div><p v-else-if="!messages.length" class="content-state">暂无留言</p><section v-else class="message-list"><article v-for="item in messages" :key="item.id" class="message-card"><div class="message-body"><div class="avatar">{{ item.nickname.slice(0, 1) }}</div><div><strong>{{ item.nickname }}</strong><p>{{ item.content }}</p></div></div><time class="message-time">{{ item.created_at }}</time></article></section><p v-if="message && state === 'ready'" class="notice">{{ message }}</p><div class="composer-block"><EmailGate scene="留言" /><form v-if="visitor.hasProfile" class="composer" @submit.prevent="submitMessage"><label class="field"><span>{{ visitor.nickname }}，留下你的留言</span><textarea v-model="content" maxlength="800" required placeholder="写一点想说的话，给之后路过这里的人。" /></label><button class="btn primary" type="submit">提交留言</button></form></div></div><aside class="sidebar"><div class="widget"><h3>说明</h3><p>邮箱只用于联系和基础识别，不会在公开页面展示。</p></div></aside></section></PublicLayout></template>
