<script setup lang="ts">
import { onMounted, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminInterviewCategoriesApi, uploadApi } from '../api'
import { DEFAULT_INTERVIEW_CATEGORIES } from '@shared/types'
import type { InterviewCategory, PostContentType } from '@shared/types'

const markdownFile = ref<File | null>(null)
const notice = ref('')
const loading = ref(false)
const contentType = ref<PostContentType>('article')
const interviewCategory = ref<string>(DEFAULT_INTERVIEW_CATEGORIES[0])
const categories = ref<InterviewCategory[]>(DEFAULT_INTERVIEW_CATEGORIES.map((name, index) => ({ id: `default-${index}`, name, sort_order: index * 10 })))

function pickFile(event: Event) {
  const input = event.target as HTMLInputElement
  markdownFile.value = input.files?.[0] || null
}

async function uploadMarkdown() {
  if (!markdownFile.value) { notice.value = '请先选择 Markdown 文件'; return }
  if (contentType.value === 'interview' && !interviewCategory.value.trim()) { notice.value = '请填写八股文所属子目录'; return }
  try {
    loading.value = true
    notice.value = '正在导入 Markdown...'
    const result = await uploadApi.uploadMarkdown(markdownFile.value, contentType.value, interviewCategory.value.trim())
    notice.value = `已创建草稿《${result.title}》，可前往${contentType.value === 'interview' ? '八股文' : '文章'}管理继续编辑和发布。`
  } catch (error) {
    console.warn('[upload-markdown] failed:', error)
    notice.value = error instanceof Error ? error.message : 'Markdown 导入失败'
  } finally {
    loading.value = false
  }
}

onMounted(async () => {
  try {
    categories.value = await adminInterviewCategoriesApi.list()
    interviewCategory.value = categories.value[0]?.name || DEFAULT_INTERVIEW_CATEGORIES[0]
  } catch (error) {
    console.warn('[upload-categories] list failed:', error)
    notice.value = error instanceof Error ? error.message : '八股文子目录加载失败'
  }
})
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>Markdown 导入</h1><p class="muted">选择本地 Markdown 文件，管理端会将它保存为未发布的文章草稿。</p></div></template>
    <p v-if="notice" class="notice">{{ notice }}</p>
    <section class="card import-card stack">
      <div><p class="muted">文章导入</p><h2>选择 Markdown 文件</h2></div>
      <label class="field"><span>内容类型</span><select v-model="contentType"><option value="article">普通文章</option><option value="interview">八股文</option></select></label>
      <label v-if="contentType === 'interview'" class="field">
        <span>八股文子目录</span>
        <select v-model="interviewCategory"><option v-for="category in categories" :key="category.id" :value="category.name">{{ category.name }}</option></select>
        <small>需要新增目录时，请先前往八股文管理中的“管理子目录”。</small>
      </label>
      <label class="field"><span>文件</span><input type="file" accept=".md,.markdown,text/markdown" @change="pickFile" /><small>一级标题会作为文章标题；导入后默认不发布。</small></label>
      <button class="btn primary" type="button" :disabled="loading" @click="uploadMarkdown">{{ loading ? '导入中...' : '导入为草稿' }}</button>
    </section>
  </AdminLayout>
</template>

<style scoped>
.import-card { max-width: 720px; }
.import-card h2, .import-card p { margin: 0; }
.import-card h2 { margin-top: 4px; }
.import-card .btn { justify-self: start; }
</style>
