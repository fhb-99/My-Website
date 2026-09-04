<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminContentApi } from '../api'
import type { AdminProject, AdminProjectDraft, EntityId } from '@shared/types'

type ProjectForm = Omit<AdminProjectDraft, 'tags'> & { tagsText: string }

const rows = ref<AdminProject[]>([])
const notice = ref('')
const loading = ref(false)
const editorOpen = ref(false)
const editingId = ref<EntityId | null>(null)
const draft = reactive<ProjectForm>({ name: '', summary: '', url: '', tagsText: '', sort_order: 0, is_published: true })

function parseTags(value: string) {
  return value.split(',').map((item) => item.trim()).filter(Boolean)
}

function resetDraft() {
  draft.name = ''
  draft.summary = ''
  draft.url = ''
  draft.tagsText = ''
  draft.sort_order = 0
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

function editProject(project: AdminProject) {
  draft.name = project.name
  draft.summary = project.summary
  draft.url = project.url || ''
  draft.tagsText = project.tags.join(',')
  draft.sort_order = project.sort_order
  draft.is_published = project.is_published
  editingId.value = project.id
  editorOpen.value = true
  notice.value = ''
}

async function loadProjects() {
  try {
    loading.value = true
    const result = await adminContentApi.listProjects({ page: 1, limit: 50 })
    rows.value = result.data
    notice.value = ''
  } catch (error) {
    console.warn('[admin-projects] list failed:', error)
    notice.value = error instanceof Error ? error.message : '项目加载失败'
  } finally {
    loading.value = false
  }
}

async function saveProject() {
  if (!draft.name.trim()) {
    notice.value = '项目名称不能为空'
    return
  }

  try {
    loading.value = true
    const payload: AdminProjectDraft = {
      name: draft.name.trim(),
      summary: draft.summary.trim(),
      url: draft.url.trim(),
      tags: parseTags(draft.tagsText),
      sort_order: Number(draft.sort_order) || 0,
      is_published: draft.is_published
    }
    if (editingId.value) await adminContentApi.updateProject(editingId.value, payload)
    else await adminContentApi.createProject(payload)
    notice.value = editingId.value ? '项目已更新' : '项目已创建'
    closeEditor()
    await loadProjects()
  } catch (error) {
    console.warn('[admin-projects] save failed:', error)
    notice.value = error instanceof Error ? error.message : '项目保存失败'
  } finally {
    loading.value = false
  }
}

async function deleteProject(project: AdminProject) {
  if (!window.confirm(`确定删除项目《${project.name}》吗？删除后不可恢复。`)) return

  try {
    loading.value = true
    await adminContentApi.deleteProject(project.id)
    notice.value = '项目已删除'
    if (editingId.value === project.id) closeEditor()
    await loadProjects()
  } catch (error) {
    console.warn('[admin-projects] delete failed:', error)
    notice.value = error instanceof Error ? error.message : '项目删除失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadProjects)
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>项目管理</h1><p class="muted">排序值越小越靠前；仅已发布项目会展示给用户。</p></div></template>
    <section class="stack">
      <p v-if="notice" class="notice">{{ notice }}</p>
      <section class="card">
        <div class="form-head"><h2>项目列表</h2><div class="table-actions"><button class="btn" :disabled="loading" @click="loadProjects">刷新</button><button class="btn primary" :disabled="loading" @click="openCreateEditor">新建项目</button></div></div>
        <div class="table-wrap"><table class="table"><thead><tr><th>名称</th><th>标签</th><th>排序</th><th>状态</th><th>操作</th></tr></thead><tbody>
          <tr v-for="project in rows" :key="project.id"><td><strong>{{ project.name }}</strong><small>{{ project.summary }}</small></td><td>{{ project.tags.join(' / ') || '-' }}</td><td>{{ project.sort_order }}</td><td>{{ project.is_published ? '已发布' : '草稿' }}</td><td><div class="table-actions"><button class="btn" :disabled="loading" @click="editProject(project)">编辑</button><button class="btn danger" :disabled="loading" @click="deleteProject(project)">删除</button></div></td></tr>
          <tr v-if="!rows.length"><td colspan="5" class="muted">暂无项目。</td></tr>
        </tbody></table></div>
      </section>
      <div v-if="editorOpen" class="editor-overlay" @click.self="closeEditor">
        <form class="editor-panel stack" @submit.prevent="saveProject">
          <div class="form-head"><div><p class="muted">项目记录</p><h2>{{ editingId ? '编辑项目' : '新建项目' }}</h2></div><button class="btn" type="button" @click="closeEditor">关闭</button></div>
          <label class="field"><span>项目名称</span><input v-model="draft.name" required /></label>
          <label class="field"><span>简介</span><textarea v-model="draft.summary" /></label>
          <label class="field"><span>项目链接</span><input v-model="draft.url" placeholder="https://example.com" /></label>
          <label class="field"><span>标签，用英文逗号分隔</span><input v-model="draft.tagsText" placeholder="C++,Vue" /></label>
          <label class="field"><span>排序</span><input v-model.number="draft.sort_order" type="number" /></label>
          <label class="field"><span>发布状态</span><select v-model="draft.is_published"><option :value="true">发布</option><option :value="false">草稿</option></select></label>
          <div class="editor-actions"><button class="btn" type="button" @click="closeEditor">取消</button><button class="btn primary" :disabled="loading">{{ loading ? '处理中...' : '保存' }}</button></div>
        </form>
      </div>
    </section>
  </AdminLayout>
</template>

<style scoped>
small{display:block;color:#858c96;margin-top:4px}
</style>
