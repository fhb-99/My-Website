<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'

import AdminLayout from '../components/AdminLayout.vue'
import { adminLearningRoadmapApi } from '../api'
import type { AdminLearningRoadmapDraft } from '@shared/types'

const form = reactive<AdminLearningRoadmapDraft>({
  title: 'C++学习路线 - 从C++入门到入土',
  subtitle: '',
  content_md: '',
  cover_url: '',
  is_published: true,
})
const coverFile = ref<File>()
const loading = ref(false)
const notice = ref('')

async function loadRoadmap() {
  try {
    loading.value = true
    const roadmap = await adminLearningRoadmapApi.get()
    if (roadmap) Object.assign(form, roadmap)
    notice.value = ''
  } catch (error) {
    console.warn('[learning-roadmap] load failed:', error)
    notice.value = error instanceof Error ? error.message : '学习路线加载失败'
  } finally {
    loading.value = false
  }
}

function pickCover(event: Event) {
  coverFile.value = (event.target as HTMLInputElement).files?.[0]
}

async function saveRoadmap() {
  if (!form.title.trim()) {
    notice.value = '请填写学习路线标题'
    return
  }
  if (!form.content_md.trim()) {
    notice.value = '请填写学习路线正文'
    return
  }

  try {
    loading.value = true
    notice.value = '正在保存学习路线...'
    const saved = await adminLearningRoadmapApi.save(form, coverFile.value)
    Object.assign(form, saved)
    coverFile.value = undefined
    notice.value = '学习路线已保存'
  } catch (error) {
    console.warn('[learning-roadmap] save failed:', error)
    notice.value = error instanceof Error ? error.message : '学习路线保存失败'
  } finally {
    loading.value = false
  }
}

onMounted(() => { void loadRoadmap() })
</script>

<template>
  <AdminLayout>
    <template #title>
      <div><h1>C++ 学习路线</h1><p class="muted">独立维护八股文首页入口、封面与学习路线正文。</p></div>
    </template>

    <p v-if="notice" class="notice">{{ notice }}</p>
    <form class="card stack roadmap-editor" @submit.prevent="saveRoadmap">
      <label class="field"><span>入口标题</span><input v-model="form.title" placeholder="C++学习路线 - 从C++入门到入土" /></label>
      <label class="field"><span>入口说明</span><textarea v-model="form.subtitle" placeholder="简要说明这份路线适合谁" /></label>
      <label class="field">
        <span>展示图片</span>
        <input type="file" accept="image/jpeg,image/png,image/webp,image/gif" :disabled="loading" @change="pickCover" />
        <small v-if="coverFile">已选择：{{ coverFile.name }}</small>
        <small v-else-if="!form.cover_url">未上传时使用用户端默认图片。</small>
        <img v-if="form.cover_url" class="roadmap-cover-preview" :src="form.cover_url" alt="当前学习路线封面" />
      </label>
      <label class="field inline-field"><span>在用户端发布</span><input v-model="form.is_published" type="checkbox" /></label>
      <label class="field markdown-field"><span>学习路线 Markdown 正文</span><textarea v-model="form.content_md" placeholder="# C++ 学习路线" /></label>
      <div class="editor-actions">
        <button class="btn primary" type="submit" :disabled="loading">{{ loading ? '保存中...' : '保存学习路线' }}</button>
      </div>
    </form>
  </AdminLayout>
</template>

<style scoped>
.roadmap-editor{max-width:980px}.inline-field{display:flex;align-items:center;justify-content:space-between;border:1px solid #3c3c3c;border-radius:5px;padding:10px 12px;background:#292929}.inline-field input{width:18px;height:18px}.markdown-field textarea{min-height:480px;font-family:"Cascadia Code",Consolas,monospace;font-size:13px;line-height:1.7}.roadmap-cover-preview{display:block;width:min(520px,100%);max-height:260px;margin-top:8px;border-radius:6px;object-fit:cover}.editor-actions{display:flex;justify-content:flex-end}
</style>
