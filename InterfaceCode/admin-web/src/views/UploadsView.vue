<script setup lang="ts">
import { onMounted, ref, watch } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminInterviewCategoriesApi, uploadApi } from '../api'
import { DEFAULT_INTERVIEW_CATEGORIES } from '@shared/types'
import type { InterviewCategory, PostContentType } from '@shared/types'

type MarkdownImportType = PostContentType | 'guide'

const markdownFiles = ref<File[]>([])
const fileInput = ref<HTMLInputElement | null>(null)
const notice = ref('')
const loading = ref(false)
const contentType = ref<MarkdownImportType>('article')
const interviewCategory = ref<string>(DEFAULT_INTERVIEW_CATEGORIES[0])
const categories = ref<InterviewCategory[]>(DEFAULT_INTERVIEW_CATEGORIES.map((name, index) => ({ id: `default-${index}`, name, sort_order: index * 10 })))

function pickFile(event: Event) {
  const input = event.target as HTMLInputElement
  markdownFiles.value = Array.from(input.files || [])
}

async function uploadMarkdown() {
  if (!markdownFiles.value.length) { notice.value = '请先选择 Markdown 文件'; return }
  if (contentType.value === 'interview' && !interviewCategory.value.trim()) { notice.value = '请填写八股文所属子目录'; return }
  try {
    loading.value = true
    notice.value = '正在导入 Markdown...'
    if (contentType.value === 'guide') {
      const result = await uploadApi.uploadGuideChapters(markdownFiles.value)
      notice.value = `章节导入完成：新建 ${result.created} 章，更新 ${result.updated} 章。新章节均为草稿，可前往人生指南管理检查并发布。`
    } else {
      const result = await uploadApi.uploadMarkdown(markdownFiles.value[0], contentType.value, interviewCategory.value.trim())
      notice.value = `已创建草稿《${result.title}》，可前往${contentType.value === 'interview' ? '八股文' : '文章'}管理继续编辑和发布。`
    }
  } catch (error) {
    console.warn('[upload-markdown] failed:', error)
    notice.value = error instanceof Error ? error.message : 'Markdown 导入失败'
  } finally {
    loading.value = false
  }
}

watch(contentType, () => {
  markdownFiles.value = []
  if (fileInput.value) fileInput.value.value = ''
  notice.value = ''
})

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
    <template #title><div><h1>Markdown 导入</h1><p class="muted">选择本地 Markdown 文件，将它导入为文章、八股文或人生指南章节草稿。</p></div></template>
    <p v-if="notice" class="notice">{{ notice }}</p>
    <section class="card import-card stack">
      <div><p class="muted">内容导入</p><h2>选择 Markdown 文件</h2></div>
      <label class="field"><span>内容类型</span><select v-model="contentType"><option value="article">普通文章</option><option value="interview">八股文</option><option value="guide">人生指南章节</option></select></label>
      <label v-if="contentType === 'interview'" class="field">
        <span>八股文子目录</span>
        <select v-model="interviewCategory"><option v-for="category in categories" :key="category.id" :value="category.name">{{ category.name }}</option></select>
        <small>需要新增目录时，请先前往八股文管理中的“管理子目录”。</small>
      </label>
      <label class="field">
        <span>文件</span>
        <input ref="fileInput" type="file" accept=".md,.markdown,text/markdown" :multiple="contentType === 'guide'" @change="pickFile" />
        <small v-if="contentType === 'guide'">可一次选择多个文件；文件名须类似 01-不要早死.md，章节号和一级标题会自动识别。请先在人生指南管理中保存基本信息。</small>
        <small v-else>一级标题会作为文章标题；导入后默认不发布。</small>
      </label>
      <button class="btn primary" type="button" :disabled="loading" @click="uploadMarkdown">{{ loading ? '导入中...' : contentType === 'guide' ? '导入章节草稿' : '导入为草稿' }}</button>
    </section>
  </AdminLayout>
</template>

<style scoped>
.import-card { max-width: 720px; }
.import-card h2, .import-card p { margin: 0; }
.import-card h2 { margin-top: 4px; }
.import-card .btn { justify-self: start; }
</style>
