<script setup lang="ts">
import { ref } from 'vue'
import { storeToRefs } from 'pinia'
import { useVisitorStore } from '../stores/visitor'
defineProps<{ scene: '评论' | '留言' }>()
const visitor = useVisitorStore(); const { hasEmail, maskedEmail } = storeToRefs(visitor); const draftEmail = ref(''); const error = ref('')
function submit(){ try{ visitor.registerEmail(draftEmail.value.trim()); draftEmail.value=''; error.value='' }catch(err){ error.value = err instanceof Error ? err.message : '邮箱格式不正确' } }
</script>
<template><section v-if="!hasEmail" class="card email-card"><p class="kicker">Email first</p><h3>先登记邮箱，再{{ scene }}</h3><p class="subtitle">登记一次即可在评论和留言之间复用，邮箱不会在页面公开展示。</p><form class="email-row" @submit.prevent="submit"><input v-model="draftEmail" type="email" placeholder="you@example.com" autocomplete="email" /><button class="btn primary" type="submit">登记邮箱</button></form><p v-if="error" class="error">{{ error }}</p></section><section v-else class="notice">已登记邮箱：<strong>{{ maskedEmail }}</strong><button class="link-button" type="button" @click="visitor.clearEmail">更换邮箱</button></section></template>
<style scoped>.email-card{margin:18px 0}.email-card h3{margin:6px 0}.email-row{display:flex;gap:10px;margin-top:16px}.email-row input{flex:1;border:1px solid var(--line);border-radius:999px;padding:12px 14px;background:var(--surface-strong);color:var(--text)}.link-button{border:none;background:transparent;color:var(--accent);font-weight:900;cursor:pointer;margin-left:8px}.error{color:#d64545}@media(max-width:640px){.email-row{flex-direction:column}}</style>
