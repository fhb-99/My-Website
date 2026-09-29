<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminGuidesApi } from '../api'
import type { AdminGuideChapterDraft, AdminGuideChapterSummary, AdminGuideDraft } from '@shared/types'

const guideDraft = reactive<AdminGuideDraft>({
  title: '高性价比人生指南',
  slug: 'how-to-live-better',
  summary: '',
  source_url: '',
  license_url: '',
  source_version: '',
  is_published: false,
})
const chapters = ref<AdminGuideChapterSummary[]>([])
const chapterDraft = reactive<AdminGuideChapterDraft>({
  guide_id: '',
  chapter_no: 1,
  title: '',
  slug: '',
  content_md: '',
  is_published: false,
})
const notice = ref('')
const loading = ref(false)
const chapterEditorOpen = ref(false)
const updatingChapterId = ref<AdminGuideChapterSummary['id'] | null>(null)

function resetChapterDraft() {
  chapterDraft.id = undefined
  chapterDraft.guide_id = guideDraft.id || ''
  chapterDraft.chapter_no = chapters.value.length
    ? Math.max(...chapters.value.map((chapter) => chapter.chapter_no)) + 1
    : 1
  chapterDraft.title = ''
  chapterDraft.slug = ''
  chapterDraft.content_md = ''
  chapterDraft.is_published = false
}

function openCreateChapter() {
  if (!guideDraft.id) {
    notice.value = '请先保存指南基本信息，再新建章节。'
    return
  }
  resetChapterDraft()
  chapterEditorOpen.value = true
  notice.value = ''
}

function closeChapterEditor() {
  chapterEditorOpen.value = false
  resetChapterDraft()
}

function formatDateTime(value: string) {
  return value ? new Date(value).toLocaleString('zh-CN', { hour12: false }) : '-'
}

async function loadChapters() {
  if (!guideDraft.id) {
    chapters.value = []
    return
  }
  chapters.value = await adminGuidesApi.listChapters(guideDraft.id)
}

async function loadGuide() {
  try {
    loading.value = true
    const guide = await adminGuidesApi.getGuide()
    if (guide) Object.assign(guideDraft, guide)
    await loadChapters()
    notice.value = ''
  } catch (error) {
    console.warn('[admin-guides] load failed:', error)
    notice.value = error instanceof Error ? error.message : '指南加载失败'
  } finally {
    loading.value = false
  }
}

async function saveGuide() {
  if (!guideDraft.title.trim() || !guideDraft.slug.trim()) {
    notice.value = '指南名称和访问标识不能为空。'
    return
  }

  try {
    loading.value = true
    Object.assign(guideDraft, await adminGuidesApi.saveGuide(guideDraft))
    notice.value = '指南基本信息已保存。'
  } catch (error) {
    console.warn('[admin-guides] save guide failed:', error)
    notice.value = error instanceof Error ? error.message : '指南保存失败'
  } finally {
    loading.value = false
  }
}

async function editChapter(chapter: AdminGuideChapterSummary) {
  try {
    loading.value = true
    Object.assign(chapterDraft, await adminGuidesApi.getChapter(chapter.id))
    chapterEditorOpen.value = true
    notice.value = ''
  } catch (error) {
    console.warn('[admin-guides] load chapter failed:', error)
    notice.value = error instanceof Error ? error.message : '章节加载失败'
  } finally {
    loading.value = false
  }
}

async function saveChapter() {
  if (!Number.isInteger(chapterDraft.chapter_no) || chapterDraft.chapter_no < 1) {
    notice.value = '章节序号必须是大于 0 的整数。'
    return
  }
  if (!chapterDraft.title.trim() || !chapterDraft.slug.trim() || !chapterDraft.content_md.trim()) {
    notice.value = '章节标题、访问标识和 Markdown 正文不能为空。'
    return
  }

  try {
    loading.value = true
    const isEditing = Boolean(chapterDraft.id)
    await adminGuidesApi.saveChapter(chapterDraft)
    closeChapterEditor()
    await loadChapters()
    notice.value = isEditing ? '章节已更新。' : '章节已创建。'
  } catch (error) {
    console.warn('[admin-guides] save chapter failed:', error)
    notice.value = error instanceof Error ? error.message : '章节保存失败'
  } finally {
    loading.value = false
  }
}

async function updateChapterStatus(chapter: AdminGuideChapterSummary, event: Event) {
  const select = event.currentTarget as HTMLSelectElement
  const isPublished = select.value === 'published'
  if (isPublished === chapter.is_published) return

  try {
    updatingChapterId.value = chapter.id
    Object.assign(chapter, await adminGuidesApi.updateChapterStatus(chapter.id, isPublished))
    notice.value = `第 ${chapter.chapter_no} 章已设为${isPublished ? '发布' : '草稿'}。`
  } catch (error) {
    select.value = chapter.is_published ? 'published' : 'draft'
    console.warn('[admin-guides] update chapter status failed:', error)
    notice.value = error instanceof Error ? error.message : '章节发布状态更新失败'
  } finally {
    updatingChapterId.value = null
  }
}

async function deleteChapter(chapter: AdminGuideChapterSummary) {
  if (!window.confirm(`确定删除第 ${chapter.chapter_no} 章《${chapter.title}》吗？删除后不可恢复。`)) return

  try {
    loading.value = true
    await adminGuidesApi.deleteChapter(chapter.id)
    await loadChapters()
    notice.value = '章节已删除。'
  } catch (error) {
    console.warn('[admin-guides] delete chapter failed:', error)
    notice.value = error instanceof Error ? error.message : '章节删除失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadGuide)
</script>

<template>
  <AdminLayout>
    <template #title>
      <div>
        <h1>人生指南管理</h1>
        <p class="muted">维护指南基本信息与章节目录；只有指南和章节都已发布时，用户端才可读取。</p>
      </div>
    </template>

    <section class="stack">
      <p v-if="notice" class="notice">{{ notice }}</p>

      <form class="card guide-form" @submit.prevent="saveGuide">
        <div class="form-head wide">
          <div>
            <p class="muted">指南设置</p>
            <h2>{{ guideDraft.title || '高性价比人生指南' }}</h2>
          </div>
          <div class="table-actions">
            <span class="status-pill" :class="{ draft: !guideDraft.is_published }">{{ guideDraft.is_published ? '已发布' : '草稿' }}</span>
            <button class="btn primary" :disabled="loading">{{ loading ? '处理中...' : '保存基本信息' }}</button>
          </div>
        </div>
        <label class="field"><span>指南名称</span><input v-model="guideDraft.title" required /></label>
        <label class="field"><span>访问标识</span><input v-model="guideDraft.slug" required placeholder="how-to-live-better" /></label>
        <label class="field wide"><span>简介</span><textarea v-model="guideDraft.summary" placeholder="显示在指南首页的简短说明" /></label>
        <label class="field"><span>原项目地址</span><input v-model="guideDraft.source_url" placeholder="https://github.com/..." /></label>
        <label class="field"><span>授权协议地址</span><input v-model="guideDraft.license_url" placeholder="https://creativecommons.org/..." /></label>
        <label class="field"><span>来源版本</span><input v-model="guideDraft.source_version" placeholder="提交哈希或版本号" /></label>
        <label class="field"><span>发布状态</span><select v-model="guideDraft.is_published"><option :value="false">草稿</option><option :value="true">发布</option></select></label>
      </form>

      <section class="card">
        <div class="form-head">
          <div>
            <p class="muted">章节目录</p>
            <h2>{{ chapters.length }} 个章节</h2>
            <small class="muted">可直接在列表中切换发布状态。</small>
          </div>
          <div class="table-actions">
            <button class="btn" :disabled="loading || !guideDraft.id" @click="loadChapters">刷新</button>
            <button class="btn primary" :disabled="loading || !guideDraft.id" @click="openCreateChapter">新建章节</button>
          </div>
        </div>
        <div class="table-wrap">
          <table class="table">
            <thead><tr><th>序号</th><th>章节</th><th>访问标识</th><th>状态</th><th>更新时间</th><th>操作</th></tr></thead>
            <tbody>
              <tr v-for="chapter in chapters" :key="chapter.id">
                <td><strong>{{ String(chapter.chapter_no).padStart(2, '0') }}</strong></td>
                <td><strong>{{ chapter.title }}</strong></td>
                <td>{{ chapter.slug }}</td>
                <td>
                  <select
                    class="chapter-status-select"
                    :class="{ draft: !chapter.is_published }"
                    :value="chapter.is_published ? 'published' : 'draft'"
                    :disabled="updatingChapterId !== null"
                    :aria-label="`设置第 ${chapter.chapter_no} 章发布状态`"
                    @change="updateChapterStatus(chapter, $event)"
                  >
                    <option value="draft">草稿</option>
                    <option value="published">已发布</option>
                  </select>
                </td>
                <td>{{ formatDateTime(chapter.updated_at) }}</td>
                <td><div class="table-actions"><button class="btn" :disabled="loading" @click="editChapter(chapter)">编辑</button><button class="btn danger" :disabled="loading" @click="deleteChapter(chapter)">删除</button></div></td>
              </tr>
              <tr v-if="!chapters.length"><td colspan="6" class="muted">{{ guideDraft.id ? '暂无章节。' : '保存指南基本信息后即可创建章节。' }}</td></tr>
            </tbody>
          </table>
        </div>
      </section>

      <div v-if="chapterEditorOpen" class="editor-overlay" @click.self="closeChapterEditor">
        <form class="editor-panel chapter-editor stack" @submit.prevent="saveChapter">
          <div class="form-head">
            <div><p class="muted">章节正文</p><h2>{{ chapterDraft.id ? '编辑章节' : '新建章节' }}</h2></div>
            <button class="btn" type="button" @click="closeChapterEditor">关闭</button>
          </div>
          <div class="chapter-fields">
            <label class="field"><span>章节序号</span><input v-model.number="chapterDraft.chapter_no" type="number" min="1" step="1" required /></label>
            <label class="field"><span>访问标识</span><input v-model="chapterDraft.slug" required placeholder="01-dont-die-early" /></label>
          </div>
          <label class="field"><span>章节标题</span><input v-model="chapterDraft.title" required /></label>
          <label class="field markdown-field"><span>Markdown 正文</span><textarea v-model="chapterDraft.content_md" required spellcheck="false" /></label>
          <div class="editor-actions"><button class="btn" type="button" @click="closeChapterEditor">取消</button><button class="btn primary" :disabled="loading">{{ loading ? '处理中...' : '保存章节' }}</button></div>
        </form>
      </div>
    </section>
  </AdminLayout>
</template>

<style scoped>
.guide-form { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; }
.wide { grid-column: 1 / -1; }
.chapter-editor { width: min(780px, 100%); }
.chapter-fields { display: grid; grid-template-columns: 150px minmax(0, 1fr); gap: 16px; }
.markdown-field textarea { min-height: 420px; font-family: Consolas, "SFMono-Regular", monospace; }
.chapter-status-select {
  min-width: 92px;
  border: 1px solid rgba(52, 168, 112, .34);
  border-radius: 999px;
  padding: 6px 28px 6px 10px;
  color: #62d49b;
  background-color: rgba(52, 168, 112, .14);
  outline: none;
  font-weight: 700;
  cursor: pointer;
}
.chapter-status-select.draft { border-color: rgba(217, 137, 37, .34); color: #f1b86b; background-color: rgba(217, 137, 37, .14); }
.chapter-status-select:focus { border-color: #438eea; box-shadow: 0 0 0 2px rgba(67, 142, 234, .12); }
.chapter-status-select:disabled { cursor: wait; opacity: .65; }
@media (max-width: 760px) {
  .guide-form, .chapter-fields { grid-template-columns: 1fr; }
  .wide { grid-column: auto; }
}
</style>
