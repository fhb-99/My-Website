<script setup lang="ts">
import { onMounted, ref } from 'vue'

import PublicLayout from '../components/PublicLayout.vue'
import { notesApi } from '../api'
import type { PageResult, NoteItem } from '@shared/types'

const result = ref<PageResult<NoteItem>>({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')

async function loadNotes(page = 1) {
  state.value = 'loading'
  error.value = ''
  try {
    result.value = await notesApi.listNotes({ page, limit: 10 })
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '随记加载失败，请稍后重试。'
    result.value = { data: [], page, limit: 10, total: 0, total_pages: 0, has_more: false }
  }
}

onMounted(() => { void loadNotes() })
</script>

<template>
  <PublicLayout><template #title>碎碎念</template><template #subtitle>不一定完整，但足够真实。这里会放一些短想法和状态记录。</template>
    <p v-if="state === 'loading'" class="content-state">正在加载随记…</p>
    <div v-else-if="state === 'error'" class="content-state"><p>{{ error }}</p><button class="btn" @click="loadNotes()">重试</button></div>
    <p v-else-if="!result.data.length" class="content-state">暂无公开随记</p>
    <section v-else class="list"><article v-for="note in result.data" :key="note.id" class="card"><p>{{ note.content }}</p><div class="mini-meta">{{ note.created_at }}<span v-if="note.mood"> · {{ note.mood }}</span></div></article></section>
    <nav v-if="state === 'ready' && result.total_pages > 1" class="pagination"><button class="btn" :disabled="result.page <= 1" @click="loadNotes(result.page - 1)">上一页</button><span>{{ result.page }} / {{ result.total_pages }}</span><button class="btn" :disabled="!result.has_more" @click="loadNotes(result.page + 1)">下一页</button></nav>
  </PublicLayout>
</template>
