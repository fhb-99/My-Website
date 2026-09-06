<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { adminMusicApi, settingsApi, uploadApi } from '../api'
import type { AdminMusicConfig, AdminMusicTrack } from '@shared/types'

const form = reactive({ title: '', subtitle: '', announcement: '' })
const notice = ref('')
const loading = ref(false)
const musicForm = reactive<AdminMusicConfig>({ enabled: false, volume: 0.35, tracks: [] })
const musicNotice = ref('')
const musicLoading = ref(false)

async function loadSettings() {
  try {
    loading.value = true
    const config = await settingsApi.getConfig()
    form.title = config.title
    form.subtitle = config.subtitle
    form.announcement = config.announcement || ''
    notice.value = ''
  } catch (error) {
    console.warn('[settings] load failed:', error)
    notice.value = error instanceof Error ? error.message : '设置加载失败'
  } finally {
    loading.value = false
  }
}

async function saveSettings() {
  try {
    loading.value = true
    notice.value = '正在保存设置...'
    await settingsApi.saveSetting('title', form.title)
    await settingsApi.saveSetting('subtitle', form.subtitle)
    await settingsApi.saveSetting('announcement', form.announcement)
    notice.value = '设置已保存'
  } catch (error) {
    console.warn('[settings] save failed:', error)
    notice.value = error instanceof Error ? error.message : '设置保存失败'
  } finally {
    loading.value = false
  }
}

async function loadMusicSettings() {
  try {
    musicLoading.value = true
    const config = await adminMusicApi.getConfig()
    musicForm.enabled = config.enabled
    musicForm.volume = config.volume
    musicForm.tracks = config.tracks.map((track) => ({ ...track }))
    musicNotice.value = ''
  } catch (error) {
    console.warn('[music-settings] load failed:', error)
    musicNotice.value = error instanceof Error ? error.message : '音乐设置加载失败'
  } finally {
    musicLoading.value = false
  }
}

function addTrack() {
  musicForm.tracks.push({
    title: '',
    artist: '',
    cover_url: '',
    audio_url: '',
    sort_order: musicForm.tracks.length,
    is_enabled: true
  })
}

function removeTrack(index: number) {
  musicForm.tracks.splice(index, 1)
}

/**
 * 从常见的“歌曲名 - 歌手.扩展名”文件名中预填曲目信息。
 * 只补全空字段，管理员先手动填写过的内容不会因重新选择文件而被覆盖。
 */
function fillTrackMetadataFromFileName(track: AdminMusicTrack, fileName: string) {
  const name = fileName
    .replace(/\.[^.]+$/, '')
    .replace(/^\s*\d{1,3}\s*[._-]\s*/, '')
    .trim()
  if (!name) return

  // 使用带空格的连字符作为分隔，避免误把歌曲名本身的短横线拆开。
  const separator = name.match(/\s[-–—]\s/)
  const title = separator ? name.slice(0, separator.index).trim() : name
  const artist = separator ? name.slice((separator.index || 0) + separator[0].length).trim() : ''

  if (!track.title.trim()) track.title = title || name
  if (!track.artist.trim() && artist) track.artist = artist
}

function isEmptyTrack(track: AdminMusicTrack) {
  return !track.title.trim() && !track.artist.trim() &&
    !track.cover_url.trim() && !track.audio_url.trim()
}

async function uploadTrackAudio(track: AdminMusicTrack, event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return

  // 选中文件时立即读取名称；例如“夜空中最亮的星 - 逃跑计划.mp3”。
  fillTrackMetadataFromFileName(track, file.name)

  try {
    musicLoading.value = true
    musicNotice.value = '正在上传音频...'
    const result = await uploadApi.uploadAudio(file)
    // 上传接口返回的相对路径可被同源 /uploads 静态目录直接访问。
    track.audio_url = result.url
    musicNotice.value = '音频已上传，保存音乐设置后会加入歌单'
  } catch (error) {
    console.warn('[music-upload] failed:', error)
    musicNotice.value = error instanceof Error ? error.message : '音频上传失败'
  } finally {
    musicLoading.value = false
  }
}

async function saveMusicSettings() {
  // 点击“添加歌曲”后尚未填写的空行不应阻止保存；真正填写过的曲目才做完整性校验。
  const tracks = musicForm.tracks.filter((track) => !isEmptyTrack(track))
  if (tracks.some((track) => !track.title.trim() || !track.audio_url.trim())) {
    musicNotice.value = '每首已填写的歌曲都需要歌曲名称和音频文件'
    return
  }

  try {
    musicLoading.value = true
    const payload: AdminMusicConfig = {
      enabled: musicForm.enabled,
      volume: Math.min(1, Math.max(0, Number(musicForm.volume) || 0)),
      // 保存前整理文本和排序值，后端拿到的就是可直接持久化的歌单数据。
      tracks: tracks.map((track, index): AdminMusicTrack => ({
        ...track,
        title: track.title.trim(),
        artist: track.artist.trim(),
        cover_url: track.cover_url.trim(),
        audio_url: track.audio_url.trim(),
        sort_order: Number(track.sort_order) || index
      }))
    }
    const saved = await adminMusicApi.saveConfig(payload)
    musicForm.enabled = saved.enabled
    musicForm.volume = saved.volume
    musicForm.tracks = saved.tracks.map((track) => ({ ...track }))
    musicNotice.value = '音乐设置已保存'
  } catch (error) {
    console.warn('[music-settings] save failed:', error)
    musicNotice.value = error instanceof Error ? error.message : '音乐设置保存失败'
  } finally {
    musicLoading.value = false
  }
}

onMounted(() => {
  void loadSettings()
  void loadMusicSettings()
})
</script>

<template>
  <AdminLayout>
    <template #title><div><h1>基础设置</h1><p class="muted">站点标题、公告等配置入口。</p></div></template>
    <p v-if="notice" class="notice">{{ notice }}</p>
    <form class="card stack" @submit.prevent="saveSettings">
      <label class="field"><span>站点标题</span><input v-model="form.title" /></label>
      <label class="field"><span>站点副标题</span><input v-model="form.subtitle" /></label>
      <label class="field"><span>公告</span><textarea v-model="form.announcement" placeholder="写一点给访客看的话" /></label>
      <button class="btn primary" type="submit" :disabled="loading">{{ loading ? '保存中...' : '保存设置' }}</button>
    </form>

    <section class="card stack music-settings">
      <div class="section-head">
        <div><h2>背景音乐</h2><p class="muted">用户首次点击播放后，歌曲会在用户端切换页面时持续播放。</p></div>
        <button class="btn primary" type="button" :disabled="musicLoading" @click="saveMusicSettings">{{ musicLoading ? '保存中...' : '保存音乐设置' }}</button>
      </div>
      <p v-if="musicNotice" class="notice">{{ musicNotice }}</p>
      <label class="field inline-field"><span>启用用户端背景音乐</span><input v-model="musicForm.enabled" type="checkbox" /></label>
      <label class="field"><span>默认音量</span><input v-model.number="musicForm.volume" type="range" min="0" max="1" step="0.05" /><small>{{ Math.round(musicForm.volume * 100) }}%</small></label>
      <div class="track-head"><h3>歌单</h3><button class="btn" type="button" :disabled="musicLoading" @click="addTrack">添加歌曲</button></div>
      <article v-for="(track, index) in musicForm.tracks" :key="track.id || index" class="track-editor stack">
        <div class="track-head"><strong>歌曲 {{ index + 1 }}</strong><button class="btn danger" type="button" :disabled="musicLoading" @click="removeTrack(index)">删除</button></div>
        <div class="track-grid">
          <label class="field"><span>歌曲名称</span><input v-model="track.title" placeholder="例如：Morning Clouds" /></label>
          <label class="field"><span>歌手/说明</span><input v-model="track.artist" placeholder="可选" /></label>
          <label class="field"><span>排序</span><input v-model.number="track.sort_order" type="number" /></label>
          <label class="field inline-field"><span>在用户端展示</span><input v-model="track.is_enabled" type="checkbox" /></label>
        </div>
        <label class="field"><span>上传音频</span><input type="file" accept="audio/mpeg,audio/mp4,audio/aac,audio/ogg,audio/wav,.mp3,.m4a,.aac,.ogg,.wav" :disabled="musicLoading" @change="uploadTrackAudio(track, $event)" /><small>文件名按“歌曲名 - 歌手”自动预填；手动填写的内容不会被覆盖。</small></label>
        <label class="field"><span>PocketBase 音频地址</span><input v-model="track.audio_url" placeholder="上传成功后自动生成" disabled /></label>
        <label class="field"><span>封面地址</span><input v-model="track.cover_url" placeholder="当前版本只展示已有封面" disabled /></label>
        <audio v-if="track.audio_url" class="track-preview" :src="track.audio_url" controls preload="none" />
      </article>
      <p v-if="!musicForm.tracks.length" class="muted">暂未添加歌曲。</p>
    </section>
  </AdminLayout>
</template>

<style scoped>
.music-settings{margin-top:16px}.section-head,.track-head{display:flex;align-items:center;justify-content:space-between;gap:14px;flex-wrap:wrap}.section-head h2,.section-head p,.track-head h3{margin:0}.section-head p{margin-top:6px}.inline-field{display:flex;align-items:center;justify-content:space-between;border:1px solid #3c3c3c;border-radius:5px;padding:10px 12px;background:#292929}.inline-field input{width:18px;height:18px}.track-editor{padding:16px;border:1px solid #383838;border-radius:6px;background:#252525}.track-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px}.track-preview{width:100%;height:36px}@media(max-width:900px){.track-grid{grid-template-columns:1fr}}
</style>
