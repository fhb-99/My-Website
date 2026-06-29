<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useThemeStore } from './stores/theme'
const themeStore = useThemeStore()
const { mode } = storeToRefs(themeStore)
</script>
<template><div class="app" :data-theme="mode"><RouterView /><button class="theme-fab" type="button" @click="themeStore.toggle">{{ mode === 'day' ? '夜' : '昼' }}</button></div></template>
