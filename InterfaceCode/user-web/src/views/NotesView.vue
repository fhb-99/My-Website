<script setup lang="ts">
import { onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import { notes as placeholderNotes } from '../data/placeholders'
import { notesApi } from '../api'
import type { NoteItem } from '@shared/types'

const notes = ref<NoteItem[]>(placeholderNotes)
const status = ref('')

onMounted(async () => {
  try {
    status.value = '正在加载碎碎念...'
    const result = await notesApi.listNotes()
    notes.value = result.length ? result : placeholderNotes
    status.value = ''
  } catch (error) {
    console.warn('[notes] use placeholders:', error)
    status.value = '碎碎念接口暂不可用，当前显示占位内容。'
  }
})
</script>

<template>
  <PublicLayout>
    <template #title>碎碎念</template>
    <template #subtitle>不一定完整，但足够真实。这里会放一些短想法和状态记录。</template>
    <p v-if="status" class="notice" style="margin-bottom: 14px">{{ status }}</p>
    <section class="list"><article v-for="note in notes" :key="note.id" class="card"><p>{{ note.content }}</p><div class="mini-meta">{{ note.created_at }} · {{ note.mood }}</div></article></section>
  </PublicLayout>
</template>
