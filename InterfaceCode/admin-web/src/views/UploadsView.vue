<script setup lang="ts">
import { ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { uploadApi } from '../api'

const imageFile = ref<File | null>(null)
const markdownFile = ref<File | null>(null)
const notice = ref('')
const imageUrl = ref('')
const loading = ref(false)

function pickFile(event: Event, target: 'image' | 'markdown') {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0] || null
  if (target === 'image') imageFile.value = file
  else markdownFile.value = file
}

async function uploadImage() {
  if (!imageFile.value) { notice.value = '请先选择图片'; return }
  try {
    loading.value = true
    notice.value = '正在上传图片...'
    const result = await uploadApi.uploadImage(imageFile.value)
    imageUrl.value = result.url
    notice.value = `图片已上传：${result.url}`
  } catch (error) {
    console.warn('[upload-image] failed:', error)
    notice.value = error instanceof Error ? error.message : '图片上传失败'
  } finally {
    loading.value = false
  }
}

async function uploadMarkdown() {
  if (!markdownFile.value) { notice.value = '请先选择 Markdown 文件'; return }
  try {
    loading.value = true
    notice.value = '正在导入 Markdown...'
    const result = await uploadApi.uploadMarkdown(markdownFile.value)
    notice.value = `Markdown 已导入：${result.title}`
  } catch (error) {
    console.warn('[upload-markdown] failed:', error)
    notice.value = error instanceof Error ? error.message : 'Markdown 导入失败'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>上传管理</h1><p class="muted">上传图片和导入 Markdown 文章。</p></div></template>
    <p v-if="notice" class="card muted">{{ notice }}</p>
    <section class="grid">
      <article class="card stack">
        <h2>图片上传</h2>
        <input type="file" accept="image/*" @change="pickFile($event, 'image')" />
        <button class="btn primary" type="button" :disabled="loading" @click="uploadImage">上传图片</button>
        <p v-if="imageUrl" class="muted">可用于文章封面：{{ imageUrl }}</p>
      </article>
      <article class="card stack">
        <h2>Markdown 上传</h2>
        <input type="file" accept=".md,.markdown" @change="pickFile($event, 'markdown')" />
        <button class="btn primary" type="button" :disabled="loading" @click="uploadMarkdown">导入 Markdown</button>
      </article>
    </section>
  </AdminLayout>
</template>