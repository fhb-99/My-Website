<script setup lang="ts">
import { onMounted, ref } from 'vue'

import PublicLayout from '../components/PublicLayout.vue'
import { learningRoadmapApi } from '../api'
import type { LearningRoadmap } from '@shared/types'

const roadmap = ref<LearningRoadmap | null>(null)
const state = ref<'loading' | 'ready' | 'error'>('loading')
const error = ref('')

async function loadRoadmap() {
  state.value = 'loading'
  error.value = ''
  try {
    roadmap.value = await learningRoadmapApi.get()
    state.value = 'ready'
  } catch (reason) {
    state.value = 'error'
    error.value = reason instanceof Error ? reason.message : '学习路线加载失败，请稍后重试。'
  }
}

onMounted(() => { void loadRoadmap() })
</script>

<template>
  <PublicLayout :show-hero="false" wide>
    <article v-if="roadmap" class="roadmap-reader">
      <RouterLink class="reader-back" to="/interview">← 返回八股文</RouterLink>
      <img class="roadmap-cover" :src="roadmap.cover_url || '/backgrounds/knowledge-sky-v1.png'" alt="C++ 学习路线封面" />
      <header class="roadmap-heading">
        <p class="page-eyebrow">C++ LEARNING ROADMAP</p>
        <h1>{{ roadmap.title }}</h1>
        <p>{{ roadmap.subtitle }}</p>
      </header>
      <section class="article-content roadmap-content" v-html="roadmap.content_html"></section>
    </article>
    <section v-else class="roadmap-reader">
      <p v-if="state === 'loading'" class="content-state">正在加载学习路线…</p>
      <div v-else class="content-state"><p>{{ error }}</p><button class="btn" type="button" @click="loadRoadmap">重试</button></div>
    </section>
  </PublicLayout>
</template>
