import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

import { siteConfigApi } from '@/api'
import type { SiteConfig } from '@shared/types'

export type SiteLoadState = 'idle' | 'loading' | 'ready' | 'error'

const emptyConfig = (): SiteConfig => ({
  title: '',
  subtitle: '',
  announcement: '',
})

export const useSiteStore = defineStore('site', () => {
  const config = ref<SiteConfig>(emptyConfig())
  const state = ref<SiteLoadState>('idle')
  const error = ref('')
  const isLoading = computed(() => state.value === 'loading')

  async function load() {
    state.value = 'loading'
    error.value = ''
    try {
      config.value = await siteConfigApi.getConfig()
      state.value = 'ready'
    } catch (reason) {
      state.value = 'error'
      error.value = reason instanceof Error ? reason.message : '站点配置加载失败'
    }
  }

  return { config, state, error, isLoading, load, retry: load }
})
