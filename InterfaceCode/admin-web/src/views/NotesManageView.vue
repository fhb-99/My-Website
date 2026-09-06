<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminContentApi } from '../api'
import type { AdminNote, AdminNoteDraft, EntityId } from '@shared/types'

const rows = ref<AdminNote[]>([])
const notice = ref('')
const loading = ref(false)
const editorOpen = ref(false)
const editingId = ref<EntityId | null>(null)
const draft = reactive<AdminNoteDraft>({ content: '', mood: '', is_published: true })

function resetDraft() {
  draft.content = ''
  draft.mood = ''
  draft.is_published = true
  editingId.value = null
}

function openCreateEditor() {
  resetDraft()
  editorOpen.value = true
  notice.value = ''
}

function closeEditor() {
  resetDraft()
  editorOpen.value = false
}

function editNote(note: AdminNote) {
  draft.content = note.content
  draft.mood = note.mood || ''
  draft.is_published = note.is_published
  editingId.value = note.id
  editorOpen.value = true
  notice.value = ''
}

async function loadNotes() {
  try {
    loading.value = true
    const result = await adminContentApi.listNotes({ page: 1, limit: 50 })
    rows.value = result.data
    notice.value = ''
  } catch (error) {
    console.warn('[admin-notes] list failed:', error)
    notice.value = error instanceof Error ? error.message : '碎碎念加载失败'
  } finally {
    loading.value = false
  }
}

async function saveNote() {
  if (!draft.content.trim()) {
    notice.value = '内容不能为空'
    return
  }

  try {
    loading.value = true
    const payload: AdminNoteDraft = {
      content: draft.content.trim(),
      mood: draft.mood.trim(),
      is_published: draft.is_published
    }
    if (editingId.value) await adminContentApi.updateNote(editingId.value, payload)
    else await adminContentApi.createNote(payload)
    notice.value = editingId.value ? '碎碎念已更新' : '碎碎念已创建'
    closeEditor()
    await loadNotes()
  } catch (error) {
    console.warn('[admin-notes] save failed:', error)
    notice.value = error instanceof Error ? error.message : '碎碎念保存失败'
  } finally {
    loading.value = false
  }
}

async function deleteNote(note: AdminNote) {
  if (!window.confirm('确定删除这条碎碎念吗？删除后不可恢复。')) return

  try {
    loading.value = true
    await adminContentApi.deleteNote(note.id)
    notice.value = '碎碎念已删除'
    if (editingId.value === note.id) closeEditor()
    await loadNotes()
  } catch (error) {
    console.warn('[admin-notes] delete failed:', error)
    notice.value = error instanceof Error ? error.message : '碎碎念删除失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadNotes)
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>碎碎念管理</h1><p class="muted">草稿不会出现在用户端，发布后会进入首页和碎碎念页面。</p></div></template>
    <section class="stack">
      <p v-if="notice" class="notice">{{ notice }}</p>
      <section class="card">
        <div class="form-head"><h2>内容列表</h2><div class="table-actions"><button class="btn" :disabled="loading" @click="loadNotes">刷新</button><button class="btn primary" :disabled="loading" @click="openCreateEditor">新建碎碎念</button></div></div>
        <div class="table-wrap"><table class="table"><thead><tr><th>内容</th><th>心情</th><th>状态</th><th>时间</th><th>操作</th></tr></thead><tbody>
          <tr v-for="note in rows" :key="note.id"><td class="content-cell">{{ note.content }}</td><td>{{ note.mood || '-' }}</td><td>{{ note.is_published ? '已发布' : '草稿' }}</td><td>{{ note.created_at }}</td><td><div class="table-actions"><button class="btn" :disabled="loading" @click="editNote(note)">编辑</button><button class="btn danger" :disabled="loading" @click="deleteNote(note)">删除</button></div></td></tr>
          <tr v-if="!rows.length"><td colspan="5" class="muted">暂无碎碎念。</td></tr>
        </tbody></table></div>
      </section>
      <div v-if="editorOpen" class="editor-overlay" @click.self="closeEditor">
        <form class="editor-panel stack" @submit.prevent="saveNote">
          <div class="form-head"><div><p class="muted">碎碎念记录</p><h2>{{ editingId ? '编辑碎碎念' : '新建碎碎念' }}</h2></div><button class="btn" type="button" @click="closeEditor">关闭</button></div>
          <label class="field"><span>内容</span><textarea v-model="draft.content" required /></label>
          <label class="field"><span>心情</span><input v-model="draft.mood" placeholder="平静、开心……" /></label>
          <label class="field"><span>发布状态</span><select v-model="draft.is_published"><option :value="true">发布</option><option :value="false">草稿</option></select></label>
          <div class="editor-actions"><button class="btn" type="button" @click="closeEditor">取消</button><button class="btn primary" :disabled="loading">{{ loading ? '处理中...' : '保存' }}</button></div>
        </form>
      </div>
    </section>
  </AdminLayout>
</template>

<style scoped>
.content-cell{max-width:440px;line-height:1.7}
</style>
