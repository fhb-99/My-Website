<script setup lang="ts">
import { nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useMusicStore } from '@/stores/music'

const music = useMusicStore()
const audio = ref<HTMLAudioElement | null>(null)
const notice = ref('')

async function playCurrentTrack() {
  if (!audio.value || !music.currentTrack) return

  try {
    await audio.value.play()
    notice.value = ''
  } catch (error) {
    // 浏览器会拦截非用户操作触发的播放，也会在音频地址无效时拒绝播放。
    console.warn('[music] play failed:', error)
    notice.value = '无法播放当前音乐'
  }
}

function pause() {
  audio.value?.pause()
}

async function togglePlayback() {
  if (music.isPlaying) pause()
  else await playCurrentTrack()
}

async function switchTrack(direction: 'previous' | 'next') {
  const shouldContinue = music.isPlaying
  if (direction === 'previous') music.previous()
  else music.next()

  // src 变更后等待 DOM 更新，再继续播放，避免仍然播放上一首。
  await nextTick()
  if (shouldContinue) await playCurrentTrack()
}

function updateVolume() {
  if (audio.value) audio.value.volume = music.volume
}

async function handleEnded() {
  // ended 事件会先让 audio 进入暂停状态，不能复用“是否正在播放”的判断。
  music.next()
  await nextTick()
  await playCurrentTrack()
}

onMounted(() => {
  if (music.state === 'idle') void music.load()
})

watch(() => music.currentTrack?.audio_url, () => {
  if (audio.value) audio.value.load()
})

watch(() => music.volume, updateVolume)

onBeforeUnmount(() => {
  // App 根组件通常不会卸载；保留暂停逻辑以避免应用被销毁后仍有声音。
  pause()
})
</script>

<template>
  <section class="music-player" aria-label="背景音乐播放器">
    <audio
      ref="audio"
      :src="music.currentTrack?.audio_url || ''"
      :volume="music.volume"
      preload="metadata"
      @play="music.isPlaying = true"
      @pause="music.isPlaying = false"
      @ended="handleEnded"
    />

    <template v-if="music.hasTracks && music.currentTrack">
      <img v-if="music.currentTrack.cover_url" class="music-cover" :src="music.currentTrack.cover_url" alt="" />
      <div v-else class="music-cover placeholder" aria-hidden="true">♪</div>
      <div class="music-actions">
        <button type="button" title="上一首" @click="switchTrack('previous')">◀</button>
        <button type="button" class="play-button" :title="music.isPlaying ? '暂停' : '播放'" @click="togglePlayback">{{ music.isPlaying ? 'Ⅱ' : '▶' }}</button>
        <button type="button" title="下一首" @click="switchTrack('next')">▶</button>
      </div>
      <label class="volume-control" title="音量">
        <span>♬</span>
        <input v-model.number="music.volume" type="range" min="0" max="1" step="0.05" />
      </label>
      <span v-if="notice" class="music-notice">{{ notice }}</span>
    </template>

    <span v-else class="music-empty">{{ music.state === 'loading' ? '正在加载音乐…' : '背景音乐未配置' }}</span>
  </section>
</template>

<style scoped>
/* 播放器放在左下角，避免遮挡新的顶部导航。 */
.music-player{position:fixed;bottom:18px;left:18px;z-index:80;display:flex;align-items:center;gap:9px;max-width:calc(100vw - 92px);padding:7px 10px;border:1px solid var(--line);border-radius:16px;background:var(--surface-strong);box-shadow:var(--shadow);backdrop-filter:blur(14px)}.music-cover{width:34px;height:34px;border-radius:10px;object-fit:cover}.music-cover.placeholder{display:grid;place-items:center;background:var(--accent-soft);color:var(--accent);font-weight:900}.music-empty,.music-notice{font-size:11px;color:var(--muted)}.music-actions{display:flex;gap:3px}.music-actions button{width:27px;height:27px;padding:0;border:0;border-radius:8px;background:transparent;color:var(--text);cursor:pointer}.music-actions button:hover{background:var(--accent-soft);color:var(--accent)}.music-actions .play-button{background:var(--accent);color:#fff}.volume-control{display:flex;align-items:center;gap:4px;color:var(--muted);font-size:12px}.volume-control input{width:56px;accent-color:var(--accent)}.music-notice{max-width:82px}@media(max-width:720px){.music-player{left:10px;bottom:10px}.volume-control{display:none}.music-actions button{width:25px}}
</style>
