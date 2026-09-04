<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { moderationApi } from '../api'
import type { AdminComment, AdminGuestbookMessage, ModerationStatus } from '@shared/types'

type ModerationRow =
  | (AdminComment & { kind: 'comment' })
  | (AdminGuestbookMessage & { kind: 'guestbook' })

const comments = ref<AdminComment[]>([])
const guestbook = ref<AdminGuestbookMessage[]>([])
const notice = ref('')
const loading = ref(false)

const rows = computed<ModerationRow[]>(() => [
  ...comments.value.map((item) => ({ ...item, kind: 'comment' as const })),
  ...guestbook.value.map((item) => ({ ...item, kind: 'guestbook' as const }))
].sort((a, b) => String(b.created_at).localeCompare(String(a.created_at))))

function statusText(status: ModerationStatus) {
  if (status === 'approved') return '已通过'
  if (status === 'rejected') return '已拒绝'
  return '待审核'
}

function sourceTitle(row: ModerationRow) {
  if (row.kind === 'guestbook') return '留言板'
  return row.post_title || `文章 #${row.post_id}`
}

function sourceMeta(row: ModerationRow) {
  if (row.kind === 'guestbook') return '站点留言'
  const parts = [`#${row.post_id}`]
  if (row.post_slug) parts.push(row.post_slug)
  return parts.join(' / ')
}

async function loadModeration() {
  try {
    loading.value = true
    notice.value = '正在加载评论和留言...'
    const [commentResult, guestbookResult] = await Promise.all([
      moderationApi.listComments({ page: 1, limit: 50 }),
      moderationApi.listGuestbook({ page: 1, limit: 50 })
    ])
    comments.value = commentResult.data
    guestbook.value = guestbookResult.data
    notice.value = ''
  } catch (error) {
    console.warn('[admin-moderation] load failed:', error)
    notice.value = error instanceof Error ? error.message : '审核列表加载失败'
  } finally {
    loading.value = false
  }
}

async function approve(row: ModerationRow) {
  try {
    loading.value = true
    notice.value = '正在通过内容...'
    if (row.kind === 'comment') await moderationApi.approveComment(row.id)
    else await moderationApi.approveGuestbook(row.id)
    notice.value = '内容已通过'
    await loadModeration()
  } catch (error) {
    console.warn('[admin-moderation] approve failed:', error)
    notice.value = error instanceof Error ? error.message : '审核通过失败'
  } finally {
    loading.value = false
  }
}

async function reject(row: ModerationRow) {
  try {
    loading.value = true
    notice.value = '正在拒绝内容...'
    if (row.kind === 'comment') await moderationApi.rejectComment(row.id)
    else await moderationApi.rejectGuestbook(row.id)
    notice.value = '内容已拒绝，公开页面将不再展示'
    await loadModeration()
  } catch (error) {
    console.warn('[admin-moderation] reject failed:', error)
    notice.value = error instanceof Error ? error.message : '审核拒绝失败'
  } finally {
    loading.value = false
  }
}

async function remove(row: ModerationRow) {
  const typeName = row.kind === 'comment' ? '评论' : '留言'
  if (!window.confirm(`确定删除这条${typeName}吗？删除后不可恢复。`)) return

  try {
    loading.value = true
    notice.value = '正在删除内容...'
    if (row.kind === 'comment') await moderationApi.deleteComment(row.id)
    else await moderationApi.deleteGuestbook(row.id)
    notice.value = '内容已删除'
    await loadModeration()
  } catch (error) {
    console.warn('[admin-moderation] delete failed:', error)
    notice.value = error instanceof Error ? error.message : '删除失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadModeration)
</script>

<template>
  <AdminLayout>
    <template #title>
      <div>
        <h1>留言 / 评论管理</h1>
        <p class="muted">统一审核用户评论和留言。文章评论会标明来源文章，避免审核时看不清上下文。</p>
      </div>
    </template>

    <section class="stack">
      <p v-if="notice" class="notice">{{ notice }}</p>
      <section class="card">
        <div class="moderation-head">
          <div>
            <p class="muted">Moderation</p>
            <h2>互动内容</h2>
          </div>
          <button class="btn" type="button" :disabled="loading" @click="loadModeration">刷新</button>
        </div>

        <div class="table-wrap"><table class="table moderation-table">
          <thead>
            <tr>
              <th>类型</th>
              <th>来源</th>
              <th>昵称 / 邮箱</th>
              <th>内容</th>
              <th>状态</th>
              <th>时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="row in rows" :key="`${row.kind}-${row.id}`">
              <td>{{ row.kind === 'comment' ? '文章评论' : '留言板' }}</td>
              <td class="source-cell">
                <strong>{{ sourceTitle(row) }}</strong>
                <small>{{ sourceMeta(row) }}</small>
              </td>
              <td>
                <strong>{{ row.nickname }}</strong>
                <small v-if="row.email">{{ row.email }}</small>
              </td>
              <td class="content-cell">{{ row.content }}</td>
              <td>
                <span class="status-pill" :class="row.status">{{ statusText(row.status) }}</span>
              </td>
              <td>{{ row.updated_at || row.created_at }}</td>
              <td>
                <div class="table-actions">
                  <button class="btn" type="button" :disabled="loading || row.status === 'approved'" @click="approve(row)">通过</button>
                  <button class="btn warning" type="button" :disabled="loading || row.status === 'rejected'" @click="reject(row)">拒绝</button>
                  <button class="btn danger" type="button" :disabled="loading" @click="remove(row)">删除</button>
                </div>
              </td>
            </tr>
            <tr v-if="!rows.length">
              <td colspan="7" class="muted">暂无评论或留言。</td>
            </tr>
          </tbody>
        </table></div>
      </section>
    </section>
  </AdminLayout>
</template>

<style scoped>
.moderation-head { display: flex; justify-content: space-between; align-items: center; gap: 16px; margin-bottom: 14px; }
.moderation-head h2 { margin: 2px 0 0; }
.moderation-table small { display: block; margin-top: 4px; color: #858c96; }
.source-cell { min-width: 180px; }
.content-cell { max-width: 420px; line-height: 1.7; }
@media (max-width: 980px) { .moderation-head { align-items: flex-start; flex-direction: column; } .table { display: block; overflow-x: auto; } }
</style>
