import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

import { musicApi } from '@/api'
import type { MusicConfig, MusicTrack } from '@shared/types'

export type MusicLoadState = 'idle' | 'loading' | 'ready' | 'error'

const emptyConfig = (): MusicConfig => ({
  enabled: false,
  volume: 0.35,
  tracks: []
})

/**
 * 音乐状态只保存页面无关的数据。
 * 真正的 audio 元素挂在 App 根组件中，因此切换路由时不会中断正在播放的背景音乐。
 */
export const useMusicStore = defineStore('music', () => {
  const config = ref<MusicConfig>(emptyConfig())
  const state = ref<MusicLoadState>('idle')
  const error = ref('')
  const currentIndex = ref(0)
  const isPlaying = ref(false)
  const volume = ref(0.35)

  const currentTrack = computed<MusicTrack | null>(() => config.value.tracks[currentIndex.value] || null)
  const hasTracks = computed(() => config.value.enabled && config.value.tracks.length > 0)

  async function load() {
    state.value = 'loading'
    error.value = ''
    try {
      const musicConfig = await musicApi.getConfig()
      config.value = musicConfig
      volume.value = Math.min(1, Math.max(0, musicConfig.volume))
      currentIndex.value = 0
      state.value = 'ready'
    } catch (reason) {
      config.value = emptyConfig()
      state.value = 'error'
      error.value = reason instanceof Error ? reason.message : '音乐配置加载失败'
    }
  }

  function previous() {
    if (!config.value.tracks.length) return
    currentIndex.value = (currentIndex.value - 1 + config.value.tracks.length) % config.value.tracks.length
  }

  function next() {
    if (!config.value.tracks.length) return
    currentIndex.value = (currentIndex.value + 1) % config.value.tracks.length
  }

  return { config, state, error, currentTrack, hasTracks, isPlaying, volume, load, previous, next }
})
