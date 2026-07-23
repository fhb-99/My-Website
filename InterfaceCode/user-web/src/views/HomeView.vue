<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import PublicLayout from '../components/PublicLayout.vue'
import { notesApi, postsApi } from '../api'
import { useSiteStore } from '../stores/site'
import type { NoteItem, PostSummary } from '@shared/types'

const site = useSiteStore()
const posts = ref<PostSummary[]>([])
const notes = ref<NoteItem[]>([])
const postsState = ref<'loading' | 'ready' | 'error'>('loading')
const notesState = ref<'loading' | 'ready' | 'error'>('loading')
const featured = computed(() => posts.value[0])

async function loadPosts() { postsState.value = 'loading'; try { posts.value = (await postsApi.listPosts({ page: 1, limit: 6 })).data; postsState.value = 'ready' } catch { posts.value = []; postsState.value = 'error' } }
async function loadNotes() { notesState.value = 'loading'; try { notes.value = (await notesApi.listNotes({ page: 1, limit: 5 })).data; notesState.value = 'ready' } catch { notes.value = []; notesState.value = 'error' } }
onMounted(() => { void site.load(); void loadPosts(); void loadNotes() })
</script>

<template><PublicLayout>
  <article v-if="featured" class="featured"><div class="featured-cover"><p class="featured-mark">{{ featured.title }}</p></div><div class="featured-body"><h2>{{ featured.title }}</h2><div class="meta-row"><span>{{ featured.created_at }}</span><span>阅读 {{ featured.views }}</span><span>{{ featured.tags.join(' · ') }}</span></div></div></article>
  <p v-else-if="postsState === 'loading'" class="content-state">正在加载文章…</p><p v-else-if="postsState === 'error'" class="content-state">文章加载失败，请稍后重试。</p><p v-else class="content-state">暂无发布文章</p>
  <div class="layout"><section class="home-fill"><div class="fill-card"><h3>快捷入口</h3><div class="quick-grid"><RouterLink class="quick" to="/posts"><strong>文章</strong><span>→</span></RouterLink><RouterLink class="quick" to="/notes"><strong>碎碎念</strong><span>→</span></RouterLink><RouterLink class="quick" to="/projects"><strong>项目</strong><span>→</span></RouterLink><RouterLink class="quick" to="/guestbook"><strong>留言板</strong><span>→</span></RouterLink></div></div><section><div class="section-head"><h2>最新文章</h2><RouterLink class="nav-link" to="/posts">全部文章 →</RouterLink></div><div class="post-list"><article v-for="post in posts" :key="post.id" class="card post-item"><div><p class="kicker">{{ post.created_at }}</p><h3>{{ post.title }}</h3><p>{{ post.summary }}</p></div><RouterLink class="btn" :to="`/posts/${post.id}`">阅读</RouterLink></article></div></section></section><aside class="sidebar"><div v-if="site.config.announcement" class="widget"><h3>公告</h3><p>{{ site.config.announcement }}</p></div><div class="widget"><h3>最近碎碎念</h3><p v-if="notesState === 'loading'">正在加载…</p><p v-else-if="notesState === 'error'">随记加载失败。</p><p v-else-if="!notes.length">暂无公开随记</p><ul v-else class="widget-list"><li v-for="note in notes" :key="note.id">{{ note.content }}</li></ul></div></aside></div>
</PublicLayout></template>
