<script setup lang="ts">
import { reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { settingsApi } from '../api'

const form = reactive({ title: 'My Blog', announcement: '' })
const notice = ref('')
const loading = ref(false)

async function saveSettings() {
  try {
    loading.value = true
    notice.value = '正在保存设置...'
    await settingsApi.saveSetting('title', form.title)
    await settingsApi.saveSetting('announcement', form.announcement)
    notice.value = '设置已保存'
  } catch (error) {
    console.warn('[settings] save failed:', error)
    notice.value = error instanceof Error ? `${error.message}。后端配置接口补齐后即可保存。` : '设置保存失败，后端接口可能尚未实现。'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>基础设置</h1><p class="muted">站点标题、公告等配置入口。</p></div></template>
    <p v-if="notice" class="card muted">{{ notice }}</p>
    <form class="card stack" @submit.prevent="saveSettings">
      <label class="field"><span>站点标题</span><input v-model="form.title" /></label>
      <label class="field"><span>公告</span><textarea v-model="form.announcement" placeholder="写一点给访客看的话" /></label>
      <button class="btn primary" type="submit" :disabled="loading">{{ loading ? '保存中...' : '保存设置' }}</button>
    </form>
  </AdminLayout>
</template>