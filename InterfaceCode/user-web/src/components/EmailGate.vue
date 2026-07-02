<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useVisitorStore } from '../stores/visitor'

const props = defineProps<{ scene: '评论' | '留言' }>()
const visitor = useVisitorStore()
const draftEmail = ref(visitor.email)
const draftNickname = ref(visitor.nickname)
const error = ref('')

const title = computed(() => {
  if (!visitor.hasEmail) return `先登记邮箱，再${props.scene}`
  return `补充昵称后再${props.scene}`
})

watch(
  () => visitor.email,
  (value) => {
    draftEmail.value = value
  }
)

watch(
  () => visitor.nickname,
  (value) => {
    draftNickname.value = value
  }
)

function submit() {
  try {
    visitor.registerProfile(draftEmail.value.trim(), draftNickname.value.trim())
    error.value = ''
  } catch (err) {
    error.value = err instanceof Error ? err.message : '登记信息不正确'
  }
}
</script>

<template>
  <section v-if="!visitor.hasProfile" class="card visitor-card">
    <p class="kicker">Before {{ scene }}</p>
    <h3>{{ title }}</h3>
    <p class="subtitle">邮箱只用于基础识别，不会在页面公开展示；昵称登记一次后评论和留言都会复用。</p>
    <form class="visitor-form" @submit.prevent="submit">
      <label class="mini-field">
        <span>邮箱</span>
        <input v-model="draftEmail" type="email" placeholder="you@example.com" autocomplete="email" />
      </label>
      <label class="mini-field">
        <span>昵称</span>
        <input v-model="draftNickname" maxlength="32" placeholder="怎么称呼你" autocomplete="nickname" />
      </label>
      <button class="btn primary" type="submit">登记后{{ scene }}</button>
    </form>
    <p v-if="error" class="error">{{ error }}</p>
  </section>
</template>

<style scoped>
.visitor-card {
  margin-top: 18px;
  background:
    radial-gradient(circle at 14% 0%, rgba(47, 124, 246, 0.12), transparent 30%),
    var(--surface-strong);
}
.visitor-card h3 { margin: 6px 0; }
.visitor-form { display: grid; grid-template-columns: minmax(0, 1fr) minmax(180px, 0.72fr) auto; gap: 12px; align-items: end; margin-top: 16px; }
.mini-field { display: grid; gap: 7px; color: var(--muted); font-size: 13px; }
.mini-field input { border: 1px solid var(--line); border-radius: 999px; padding: 12px 14px; background: var(--surface-strong); color: var(--text); outline: none; }
.error { color: #d64545; margin-bottom: 0; }
@media (max-width: 760px) { .visitor-form { grid-template-columns: 1fr; } }
</style>
