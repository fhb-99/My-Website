<script setup lang="ts">
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import AdminLayout from '../components/AdminLayout.vue'
import { adminInterviewCategoriesApi, adminPostsApi } from '../api'
import { DEFAULT_INTERVIEW_CATEGORIES } from '@shared/types'
import type { AdminPostDraft, AdminPostSummary, EntityId, InterviewCategory, PostContentType } from '@shared/types'

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
  is_published: true,
  content_type: 'article',
  interview_category: ''
})

const route = useRoute()
const contentType = computed<PostContentType>(() => route.meta.contentType === 'interview' ? 'interview' : 'article')
const contentName = computed(() => contentType.value === 'interview' ? '八股文' : '文章')
const contentNameEn = computed(() => contentType.value === 'interview' ? 'Interview Notes' : 'Posts')

const rows = ref<AdminPostSummary[]>([])
const categories = ref<InterviewCategory[]>([])
const selectedCategory = ref('')
const notice = ref('')
const loading = ref(false)
const editorOpen = ref(false)
const categoryManagerOpen = ref(false)
const editingId = ref<EntityId | null>(null)
const coverFile = ref<File | null>(null)
const newCategoryName = ref('')

const categoryNames = computed(() => {
  const names = categories.value.length ? categories.value.map((category) => category.name) : [...DEFAULT_INTERVIEW_CATEGORIES]
  if (draft.interview_category && !names.includes(draft.interview_category)) names.push(draft.interview_category)
  return names
})

function parseTags(value: string) {
  return value.split(',').map((item) => item.trim()).filter(Boolean)
}

function formatDateTime(value: string) {
  // PocketBase 返回 ISO 时间，这里只保留到秒，避免表格显示毫秒和 Z 后缀。
  return value.replace('T', ' ').replace(/\.\d+Z?$/, '').replace(/Z$/, '').slice(0, 19)
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
  draft.content_type = contentType.value
  draft.interview_category = contentType.value === 'interview' ? categoryNames.value[0] || '' : ''
  editingId.value = null
  coverFile.value = null
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
    notice.value = `正在加载${contentName.value}...`
    const result = await adminPostsApi.listPosts({ page: 1, limit: 50, contentType: contentType.value, category: selectedCategory.value })
    rows.value = result.data
    notice.value = ''
  } catch (error) {
    console.warn('[admin-posts] list failed:', error)
    notice.value = error instanceof Error ? error.message : `${contentName.value}列表加载失败`
  } finally {
    loading.value = false
  }
}

async function loadCategories() {
  if (contentType.value !== 'interview') return
  try {
    categories.value = await adminInterviewCategoriesApi.list()
  } catch (error) {
    console.warn('[admin-interview-categories] list failed:', error)
    notice.value = error instanceof Error ? error.message : '八股文子目录加载失败'
  }
}

async function createCategory() {
  if (!newCategoryName.value.trim()) { notice.value = '请输入子目录名称'; return }
  try {
    loading.value = true
    const category = await adminInterviewCategoriesApi.create(newCategoryName.value)
    newCategoryName.value = ''
    await loadCategories()
    selectedCategory.value = category.name
    notice.value = `子目录“${category.name}”已创建`
    await loadPosts()
  } catch (error) {
    notice.value = error instanceof Error ? error.message : '子目录创建失败'
  } finally {
    loading.value = false
  }
}

async function deleteCategory(category: InterviewCategory) {
  if (!window.confirm(`确定删除子目录“${category.name}”吗？`)) return
  try {
    loading.value = true
    await adminInterviewCategoriesApi.delete(category)
    if (selectedCategory.value === category.name) selectedCategory.value = ''
    notice.value = `子目录“${category.name}”已删除`
    await Promise.all([loadCategories(), loadPosts()])
  } catch (error) {
    notice.value = error instanceof Error ? error.message : '子目录删除失败'
  } finally {
    loading.value = false
  }
}

function manageCategory(name: string) {
  selectedCategory.value = name
  categoryManagerOpen.value = false
  void loadPosts()
}

function pickCover(event: Event) {
  coverFile.value = (event.target as HTMLInputElement).files?.[0] || null
}

async function editPost(id: EntityId) {
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
    draft.content_type = post.content_type || 'article'
    draft.interview_category = post.interview_category || ''
    editingId.value = post.id
    editorOpen.value = true
    notice.value = `已载入${contentName.value}，可以编辑后保存。`
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
  if (draft.content_type === 'interview' && !draft.interview_category.trim()) {
    notice.value = '八股文必须选择或填写所属子目录'
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
      is_published: draft.is_published,
      content_type: draft.content_type,
      interview_category: draft.interview_category.trim()
    }, coverFile.value || undefined)
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

async function deletePost(id: EntityId) {
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

onMounted(async () => {
  await loadCategories()
  await loadPosts()
})
watch(contentType, async () => {
  closeEditor()
  selectedCategory.value = ''
  categories.value = []
  await loadCategories()
  await loadPosts()
})
</script>

<template>
  <AdminLayout>
    <template #title>
      <div>
        <h1>{{ contentName }}管理</h1>
        <p class="muted">这里只展示{{ contentName }}栏目内容，新建时会自动保存到对应栏目。</p>
      </div>
    </template>

    <section class="stack">
      <p v-if="notice" class="notice">{{ notice }}</p>

      <section class="card">
        <div class="form-head">
          <div>
            <p class="muted">{{ contentNameEn }}</p>
            <h2>{{ contentName }}列表</h2>
          </div>
          <div class="table-actions">
            <select v-if="contentType === 'interview'" v-model="selectedCategory" class="table-filter" aria-label="按八股文子目录筛选" @change="loadPosts">
              <option value="">全部子目录</option>
              <option v-for="category in categories" :key="category.id" :value="category.name">{{ category.name }}</option>
              <option value="未分类">未分类</option>
            </select>
            <button v-if="contentType === 'interview'" class="btn" type="button" @click="categoryManagerOpen = true">管理子目录</button>
            <button class="btn" type="button" :disabled="loading" @click="loadPosts">刷新</button>
            <button class="btn primary" type="button" :disabled="loading" @click="openCreateEditor">新建{{ contentName }}</button>
          </div>
        </div>

        <div class="table-wrap"><table class="table">
          <thead>
            <tr>
              <th>标题</th>
              <th>Slug</th>
              <th v-if="contentType === 'interview'">子目录</th>
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
              <td v-if="contentType === 'interview'">{{ row.interview_category || '未分类' }}</td>
              <td>
                <span class="status-pill" :class="{ draft: !row.is_published }">
                  {{ row.is_published ? '已发布' : '草稿' }}
                </span>
              </td>
              <td>{{ row.views }}</td>
              <td>{{ formatDateTime(row.updated_at || row.created_at) }}</td>
              <td>
                <div class="table-actions">
                  <button class="btn" type="button" :disabled="loading" @click="editPost(row.id)">编辑</button>
                  <button class="btn danger" type="button" :disabled="loading" @click="deletePost(row.id)">删除</button>
                </div>
              </td>
            </tr>
            <tr v-if="!rows.length">
              <td :colspan="contentType === 'interview' ? 7 : 6" class="muted">暂无{{ contentName }}。</td>
            </tr>
          </tbody>
        </table></div>
      </section>

      <div v-if="editorOpen" class="editor-overlay" @click.self="closeEditor">
        <form class="editor-panel stack" @submit.prevent="savePost">
          <div class="form-head">
            <div>
              <p class="muted">{{ contentName }}记录</p>
              <h2>{{ editingId ? `编辑${contentName}` : `新建${contentName}` }}</h2>
            </div>
            <button class="btn" type="button" @click="closeEditor">关闭</button>
          </div>

          <label class="field"><span>标题</span><input v-model="draft.title" /></label>
          <label class="field"><span>Slug</span><input v-model="draft.slug" placeholder="例如：my-first-post" /></label>
          <label class="field"><span>摘要</span><textarea v-model="draft.summary" /></label>
          <label class="field">
            <span>封面图片</span>
            <input type="file" accept="image/jpeg,image/png,image/webp,image/gif" @change="pickCover" />
            <small v-if="coverFile">已选择：{{ coverFile.name }}</small>
            <img v-else-if="draft.cover_url" class="cover-preview" :src="draft.cover_url" alt="当前文章封面" />
          </label>
          <label class="field"><span>标签，用英文逗号分隔</span><input v-model="draft.tagsText" placeholder="C++,Vue,随笔" /></label>
          <label class="field"><span>所属栏目</span><select v-model="draft.content_type"><option value="article">普通文章</option><option value="interview">八股文</option></select><small>修改栏目后，保存内容会移动到对应管理列表。</small></label>
          <label v-if="draft.content_type === 'interview'" class="field">
            <span>八股文子目录</span>
            <select v-model="draft.interview_category"><option value="" disabled>请选择子目录</option><option v-for="category in categoryNames" :key="category" :value="category">{{ category }}</option></select>
            <small>新目录请先通过“管理子目录”创建。</small>
          </label>
          <label class="field"><span>发布状态</span><select v-model="draft.is_published"><option :value="true">发布</option><option :value="false">草稿</option></select></label>
          <label class="field markdown-field"><span>Markdown 正文</span><textarea v-model="draft.content_md" /></label>

          <div class="editor-actions">
            <button class="btn" type="button" @click="closeEditor">取消</button>
            <button class="btn primary" type="submit" :disabled="loading">{{ loading ? '处理中...' : editingId ? '保存修改' : `创建${contentName}` }}</button>
          </div>
        </form>
      </div>

      <div v-if="categoryManagerOpen" class="editor-overlay" @click.self="categoryManagerOpen = false">
        <section class="editor-panel category-panel stack">
          <div class="form-head">
            <div><p class="muted">Interview Directories</p><h2>八股文子目录管理</h2></div>
            <button class="btn" type="button" @click="categoryManagerOpen = false">关闭</button>
          </div>
          <form class="category-create" @submit.prevent="createCategory">
            <label class="field"><span>新子目录名称</span><input v-model="newCategoryName" placeholder="例如：操作系统" /></label>
            <button class="btn primary" type="submit" :disabled="loading">添加子目录</button>
          </form>
          <div class="category-list">
            <article v-for="category in categories" :key="category.id">
              <button type="button" @click="manageCategory(category.name)"><strong>{{ category.name }}</strong><small>查看该目录文章</small></button>
              <button class="btn danger" type="button" :disabled="loading" @click="deleteCategory(category)">删除</button>
            </article>
          </div>
        </section>
      </div>
    </section>
  </AdminLayout>
</template>

<style scoped>
.form-head { display: flex; justify-content: space-between; align-items: center; gap: 16px; margin-bottom: 14px; }
.form-head h2 { margin: 2px 0 0; }
.markdown-field textarea { min-height: 360px; font-family: "Cascadia Code", Consolas, monospace; font-size: 13px; }
.cover-preview { display: block; width: 180px; max-height: 110px; border-radius: 5px; object-fit: cover; }
.table-filter { min-height: 36px; border: 1px solid #444; border-radius: 5px; padding: 6px 32px 6px 10px; color: #d7d9dd; background: #292929; }
.category-create { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 10px; align-items: end; }
.category-list { display: grid; gap: 8px; }
.category-list article { display: flex; align-items: center; gap: 10px; padding: 10px; border: 1px solid #383838; border-radius: 6px; background: #252525; }
.category-list article > button:first-child { display: grid; min-width: 0; flex: 1; gap: 4px; border: 0; color: #e8eaed; background: transparent; cursor: pointer; text-align: left; }
.category-list article small { color: #858c96; }
@media (max-width: 780px) { .form-head { align-items: flex-start; flex-direction: column; } .table { display: block; overflow-x: auto; } }
</style>
