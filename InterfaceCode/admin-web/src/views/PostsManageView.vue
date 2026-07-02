<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminPostsApi } from '../api'
import type { AdminPostDraft, AdminPostSummary } from '@shared/types'

type PostDraftForm = Omit<AdminPostDraft, 'cover_url'> & {
  cover_url: string
  tagsText: string
}

const draft = reactive<PostDraftForm>({
  id: undefined,
  title: '',
  slug: '',
  summary: '',
  content_md: '',
  cover_url: '',
  tags: [],
  tagsText: '',
  is_published: true
})

const rows = ref<AdminPostSummary[]>([])
const notice = ref('')
const loading = ref(false)
const editorOpen = ref(false)
const editingId = ref<number | null>(null)

function parseTags(value: string) {
  return value.split(',').map((item) => item.trim()).filter(Boolean)
}

function resetDraft() {
  draft.id = undefined
  draft.title = ''
  draft.slug = ''
  draft.summary = ''
  draft.content_md = ''
  draft.cover_url = ''
  draft.tags = []
  draft.tagsText = ''
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

async function loadPosts() {
  try {
    loading.value = true
    notice.value = '正在加载文章...'
    const result = await adminPostsApi.listPosts({ page: 1, limit: 50 })
    rows.value = result.data
    notice.value = ''
  } catch (error) {
    console.warn('[admin-posts] list failed:', error)
    notice.value = error instanceof Error ? error.message : '文章列表加载失败'
  } finally {
    loading.value = false
  }
}

async function editPost(id: number) {
  try {
    loading.value = true
    notice.value = '正在读取文章详情...'
    const post = await adminPostsApi.getPost(id)
    draft.id = post.id
    draft.title = post.title
    draft.slug = post.slug
    draft.summary = post.summary
    draft.content_md = post.content_md
    draft.cover_url = post.cover_url || ''
    draft.tags = post.tags
    draft.tagsText = post.tags.join(',')
    draft.is_published = post.is_published
    editingId.value = post.id
    editorOpen.value = true
    notice.value = '已载入文章，可以编辑后保存。'
  } catch (error) {
    console.warn('[admin-posts] get failed:', error)
    notice.value = error instanceof Error ? error.message : '文章详情读取失败'
  } finally {
    loading.value = false
  }
}

async function savePost() {
  if (!draft.title.trim() || !draft.slug.trim() || !draft.content_md.trim()) {
    notice.value = '标题、Slug 和 Markdown 正文必填'
    return
  }

  try {
    loading.value = true
    notice.value = '正在保存文章...'
    await adminPostsApi.savePost({
      id: draft.id,
      title: draft.title.trim(),
      slug: draft.slug.trim(),
      summary: draft.summary.trim(),
      content_md: draft.content_md,
      cover_url: draft.cover_url.trim(),
      tags: parseTags(draft.tagsText),
      is_published: draft.is_published
    })
    notice.value = draft.id ? '文章已更新' : '文章已创建'
    closeEditor()
    await loadPosts()
  } catch (error) {
    console.warn('[admin-posts] save failed:', error)
    notice.value = error instanceof Error ? error.message : '文章保存失败'
  } finally {
    loading.value = false
  }
}

async function deletePost(id: number) {
  const target = rows.value.find((row) => row.id === id)
  const title = target ? `《${target.title}》` : `ID ${id}`
  if (!window.confirm(`确定删除 ${title} 吗？删除后不可恢复。`)) return

  try {
    loading.value = true
    notice.value = '正在删除文章...'
    await adminPostsApi.deletePost(id)
    if (editingId.value === id) closeEditor()
    notice.value = '文章已删除'
    await loadPosts()
  } catch (error) {
    console.warn('[admin-posts] delete failed:', error)
    notice.value = error instanceof Error ? error.message : '文章删除失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadPosts)
</script>

<template>
  <AdminLayout>
    <template #title>
      <div>
        <h1>文章管理</h1>
        <p class="muted">默认展示全部文章。点击“新建文章”或某篇文章的“编辑”后，再展开编辑表单。</p>
      </div>
    </template>

    <section class="stack">
      <p v-if="notice" class="card muted">{{ notice }}</p>

      <section class="card">
        <div class="form-head">
          <div>
            <p class="muted">Posts</p>
            <h2>文章列表</h2>
          </div>
          <div class="table-actions">
            <button class="btn" type="button" :disabled="loading" @click="loadPosts">刷新</button>
            <button class="btn primary" type="button" :disabled="loading" @click="openCreateEditor">新建文章</button>
          </div>
        </div>

        <table class="table">
          <thead>
            <tr>
              <th>标题</th>
              <th>Slug</th>
              <th>状态</th>
              <th>阅读</th>
              <th>更新时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="row in rows" :key="row.id">
              <td>{{ row.title }}</td>
              <td>{{ row.slug }}</td>
              <td>
                <span class="status-pill" :class="{ draft: !row.is_published }">
                  {{ row.is_published ? '已发布' : '草稿' }}
                </span>
              </td>
              <td>{{ row.views }}</td>
              <td>{{ row.updated_at || row.created_at }}</td>
              <td>
                <div class="table-actions">
                  <button class="btn" type="button" :disabled="loading" @click="editPost(row.id)">编辑</button>
                  <button class="btn danger" type="button" :disabled="loading" @click="deletePost(row.id)">删除</button>
                </div>
              </td>
            </tr>
            <tr v-if="!rows.length">
              <td colspan="6" class="muted">暂无文章或接口暂不可用。</td>
            </tr>
          </tbody>
        </table>
      </section>

      <form v-if="editorOpen" class="card stack editor-card" @submit.prevent="savePost">
        <div class="form-head">
          <div>
            <p class="muted">Post Editor</p>
            <h2>{{ editingId ? '编辑文章' : '新建文章' }}</h2>
          </div>
          <button class="btn" type="button" @click="closeEditor">关闭编辑</button>
        </div>

        <label class="field">
          <span>标题</span>
          <input v-model="draft.title" />
        </label>
        <label class="field">
          <span>Slug</span>
          <input v-model="draft.slug" />
        </label>
        <label class="field">
          <span>摘要</span>
          <textarea v-model="draft.summary" />
        </label>
        <label class="field">
          <span>封面 URL</span>
          <input v-model="draft.cover_url" placeholder="/uploads/images/cover.jpg" />
        </label>
        <label class="field">
          <span>标签，用英文逗号分隔</span>
          <input v-model="draft.tagsText" placeholder="C++,Vue,随笔" />
        </label>
        <label class="field">
          <span>发布状态</span>
          <select v-model="draft.is_published">
            <option :value="true">发布</option>
            <option :value="false">草稿</option>
          </select>
        </label>
        <label class="field">
          <span>Markdown 正文</span>
          <textarea v-model="draft.content_md" />
        </label>

        <button class="btn primary" type="submit" :disabled="loading">
          {{ loading ? '处理中...' : editingId ? '保存修改' : '创建文章' }}
        </button>
      </form>
    </section>
  </AdminLayout>
</template>

<style scoped>
.form-head { display: flex; justify-content: space-between; align-items: center; gap: 16px; margin-bottom: 14px; }
.form-head h2 { margin: 2px 0 0; }
.editor-card { border-color: rgba(47, 124, 246, .26); box-shadow: 0 24px 70px rgba(47, 124, 246, .12); }
.status-pill { display: inline-flex; align-items: center; border-radius: 999px; padding: 4px 10px; background: rgba(28, 145, 90, .12); color: #158150; font-weight: 800; font-size: 12px; }
.status-pill.draft { background: rgba(104, 117, 141, .14); color: #68758d; }
.table-actions { display: flex; gap: 8px; flex-wrap: wrap; }
.btn.danger { color: #c2413b; border-color: rgba(194, 65, 59, .22); }
@media (max-width: 780px) { .form-head { align-items: flex-start; flex-direction: column; } .table { display: block; overflow-x: auto; } }
</style>
