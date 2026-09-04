<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'

import SiteHeader from '../components/SiteHeader.vue'
import { notesApi, postsApi } from '../api'
import { useSiteStore } from '../stores/site'
import type { NoteItem, PostSummary } from '@shared/types'

const site = useSiteStore()
const router = useRouter()
const posts = ref<PostSummary[]>([])
const notes = ref<NoteItem[]>([])
const postsState = ref<'loading' | 'ready' | 'error'>('loading')
const notesState = ref<'loading' | 'ready' | 'error'>('loading')
const keyword = ref('')
const now = ref(new Date())
const announcementOpen = ref(false)
const suppressAiWheelHover = ref(false)
let clockTimer: number | undefined

const currentTime = computed(() => new Intl.DateTimeFormat('zh-CN', {
  hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false,
}).format(now.value))
const currentDate = computed(() => new Intl.DateTimeFormat('zh-CN', {
  year: 'numeric', month: '2-digit', day: '2-digit', weekday: 'long',
}).format(now.value))
const serviceStatus = computed(() => {
  if (postsState.value === 'error' && notesState.value === 'error') return { title: '前端界面已就绪', detail: '内容服务暂时不可用' }
  if (site.state === 'ready') return { title: '内容服务已连接', detail: '文章与站点配置加载正常' }
  if (site.state === 'loading') return { title: '正在连接内容服务', detail: '前端界面已经就绪' }
  return { title: '前端界面已就绪', detail: '内容服务暂时不可用' }
})

async function loadPosts() { postsState.value = 'loading'; try { posts.value = (await postsApi.listPosts({ page: 1, limit: 6, contentType: 'article' })).data; postsState.value = 'ready' } catch { posts.value = []; postsState.value = 'error' } }
async function loadNotes() { notesState.value = 'loading'; try { notes.value = (await notesApi.listNotes({ page: 1, limit: 5 })).data; notesState.value = 'ready' } catch { notes.value = []; notesState.value = 'error' } }

function searchPosts() {
  const q = keyword.value.trim()
  void router.push({ path: '/posts', query: q ? { q } : {} })
}

function closeAnnouncement() {
  announcementOpen.value = false
  if (site.config.announcement) sessionStorage.setItem('blog-announcement-seen', site.config.announcement)
}

function releaseAiSectorFocus(event: MouseEvent) {
  const link = event.currentTarget as EventTarget & { blur?: () => void }
  // 新标签页打开后同时清除焦点与残留悬停态，返回主页时扇区立即复原。
  suppressAiWheelHover.value = true
  window.setTimeout(() => link.blur?.(), 0)
}

function restoreAiWheelHover() {
  suppressAiWheelHover.value = false
}

onMounted(async () => {
  if (site.state === 'idle') await site.load()
  announcementOpen.value = Boolean(site.config.announcement && sessionStorage.getItem('blog-announcement-seen') !== site.config.announcement)
  void loadPosts()
  void loadNotes()
  clockTimer = window.setInterval(() => { now.value = new Date() }, 1000)
})

onBeforeUnmount(() => window.clearInterval(clockTimer))
</script>

<template>
  <div class="portal-page">
    <div class="portal-backdrop" aria-hidden="true"></div>
    <SiteHeader />

    <main class="wide-shell portal-content">
      <aside class="portal-left" aria-label="快捷入口">
        <div class="left-ambient" aria-hidden="true">
          <span class="ambient-aura"></span>
          <span class="ambient-ring ambient-ring-outer"><i></i></span>
          <span class="ambient-ring ambient-ring-middle"><i></i></span>
          <span class="ambient-ring ambient-ring-inner"><i></i></span>
          <span class="ambient-core"><i></i></span>
        </div>

        <nav class="ai-wheel" :class="{ 'is-click-reset': suppressAiWheelHover }" aria-label="AI 与开发工具" @pointerleave="restoreAiWheelHover">
          <svg class="ai-wheel-svg" viewBox="-18 -18 236 236">
            <defs><linearGradient id="gemini-gradient" x1="8" y1="40" x2="40" y2="8"><stop stop-color="#4f7cff"/><stop offset=".52" stop-color="#8b5cf6"/><stop offset="1" stop-color="#ef6bb7"/></linearGradient></defs>
            <a class="ai-sector ai-sector-chatgpt" href="https://chatgpt.com/" target="_blank" rel="noopener noreferrer" aria-label="打开 ChatGPT 官网" @click="releaseAiSectorFocus">
              <path class="ai-sector-shape" d="M100 100 13.4 50A100 100 0 0 1 186.6 50Z" />
              <svg class="ai-sector-icon ai-sector-icon-chatgpt" x="76" y="16" width="48" height="48" viewBox="0 0 721 721" aria-hidden="true">
                <!-- 使用 OpenAI 官方单色 Blossom 图形，缩小后仍能保持清晰辨识度。 -->
                <path fill="currentColor" d="M304.246 294.611V249.028C304.246 245.189 305.687 242.309 309.044 240.392L400.692 187.612C413.167 180.415 428.042 177.058 443.394 177.058C500.971 177.058 537.44 221.682 537.44 269.182C537.44 272.54 537.44 276.379 536.959 280.218L441.954 224.558C436.197 221.201 430.437 221.201 424.68 224.558L304.246 294.611ZM518.245 472.145V363.224C518.245 356.505 515.364 351.707 509.608 348.349L389.174 278.296L428.519 255.743C431.877 253.826 434.757 253.826 438.115 255.743L529.762 308.523C556.154 323.879 573.905 356.505 573.905 388.171C573.905 424.636 552.315 458.225 518.245 472.141V472.145ZM275.937 376.182L236.592 353.152C233.235 351.235 231.794 348.354 231.794 344.515V238.956C231.794 187.617 271.139 148.749 324.4 148.749C344.555 148.749 363.264 155.468 379.102 167.463L284.578 222.164C278.822 225.521 275.942 230.319 275.942 237.039V376.186L275.937 376.182ZM360.626 425.122L304.246 393.455V326.283L360.626 294.616L417.002 326.283V393.455L360.626 425.122ZM396.852 570.989C376.698 570.989 357.989 564.27 342.151 552.276L436.674 497.574C442.431 494.217 445.311 489.419 445.311 482.699V343.552L485.138 366.582C488.495 368.499 489.936 371.379 489.936 375.219V480.778C489.936 532.117 450.109 570.985 396.852 570.985V570.989ZM283.134 463.99L191.486 411.211C165.094 395.854 147.343 363.229 147.343 331.562C147.343 294.616 169.415 261.509 203.48 247.593V356.991C203.48 363.71 206.361 368.508 212.117 371.866L332.074 441.437L292.729 463.99C289.372 465.907 286.491 465.907 283.134 463.99ZM277.859 542.68C223.639 542.68 183.813 501.895 183.813 451.514C183.813 447.675 184.294 443.836 184.771 439.997L279.295 494.698C285.051 498.056 290.812 498.056 296.568 494.698L417.002 425.127V470.71C417.002 474.549 415.562 477.429 412.204 479.346L320.557 532.126C308.081 539.323 293.206 542.68 277.854 542.68H277.859ZM396.852 599.776C454.911 599.776 503.37 558.513 514.41 503.812C568.149 489.896 602.696 439.515 602.696 388.176C602.696 354.587 588.303 321.962 562.392 298.45C564.791 288.373 566.231 278.296 566.231 268.224C566.231 199.611 510.571 148.267 446.274 148.267C433.322 148.267 420.846 150.184 408.37 154.505C386.775 133.392 357.026 119.958 324.4 119.958C266.342 119.958 217.883 161.22 206.843 215.921C153.104 229.837 118.557 280.218 118.557 331.557C118.557 365.146 132.95 397.771 158.861 421.283C156.462 431.36 155.022 441.437 155.022 451.51C155.022 520.123 210.682 571.466 274.978 571.466C287.931 571.466 300.407 569.549 312.883 565.228C334.473 586.341 364.222 599.776 396.852 599.776Z"/>
              </svg>
            </a>
            <a class="ai-sector ai-sector-gemini" href="https://gemini.google.com/" target="_blank" rel="noopener noreferrer" aria-label="打开 Gemini 官网" @click="releaseAiSectorFocus">
              <path class="ai-sector-shape" d="M100 100 186.6 50A100 100 0 0 1 100 200Z" />
              <svg class="ai-sector-icon" x="133" y="108" width="43" height="43" viewBox="0 0 48 48"><path fill="url(#gemini-gradient)" d="M24 4c1.5 11.6 7.3 17.4 20 20-12.7 2.6-18.5 8.4-20 20-1.5-11.6-7.3-17.4-20-20C16.7 21.4 22.5 15.6 24 4Z"/></svg>
            </a>
            <a class="ai-sector ai-sector-github" href="https://github.com/" target="_blank" rel="noopener noreferrer" aria-label="打开 GitHub 官网" @click="releaseAiSectorFocus">
              <path class="ai-sector-shape" d="M100 100 100 200A100 100 0 0 1 13.4 50Z" />
              <svg class="ai-sector-icon" x="32" y="118" width="38" height="38" viewBox="0 0 24 24"><path fill="currentColor" d="M12 .7a11.5 11.5 0 0 0-3.64 22.4c.58.1.79-.25.79-.56v-2.02c-3.22.7-3.9-1.37-3.9-1.37-.53-1.34-1.29-1.7-1.29-1.7-1.05-.72.08-.71.08-.71 1.17.08 1.78 1.2 1.78 1.2 1.03 1.77 2.71 1.26 3.37.96.1-.75.4-1.26.74-1.55-2.57-.29-5.28-1.29-5.28-5.69 0-1.26.45-2.28 1.2-3.09-.12-.29-.52-1.47.11-3.05 0 0 .98-.31 3.16 1.18a10.9 10.9 0 0 1 5.76 0c2.18-1.49 3.16-1.18 3.16-1.18.63 1.58.23 2.76.11 3.05.75.81 1.2 1.83 1.2 3.09 0 4.41-2.71 5.39-5.29 5.68.42.36.79 1.06.79 2.14v3.17c0 .31.21.67.79.56A11.5 11.5 0 0 0 12 .7Z"/></svg>
            </a>
          </svg>
        </nav>

        <nav class="community-dock" aria-label="程序员社区">
          <a class="community-link community-csdn" href="https://www.csdn.net/" target="_blank" rel="noopener noreferrer" aria-label="打开 CSDN" data-label="CSDN 社区"><span class="community-icon">CSDN</span></a>
          <a class="community-link community-juejin" href="https://juejin.cn/" target="_blank" rel="noopener noreferrer" aria-label="打开稀土掘金" data-label="稀土掘金"><span class="community-icon community-juejin-mark"><i></i></span></a>
          <a class="community-link community-gitee" href="https://gitee.com/" target="_blank" rel="noopener noreferrer" aria-label="打开 Gitee" data-label="Gitee 社区"><span class="community-icon">G</span></a>
        </nav>
      </aside>

      <section class="portal-main">
        <form class="portal-search" @submit.prevent="searchPosts">
          <span class="search-engine">站内</span>
          <input v-model="keyword" type="search" placeholder="搜索文章、技术笔记与项目复盘" aria-label="搜索知识库" />
          <button type="submit" aria-label="开始搜索">⌕</button>
        </form>

        <div class="portal-grid">
          <RouterLink class="portal-card portal-card-3d knowledge-card knowledge-flip-card" to="/posts">
            <span class="knowledge-flip-inner">
              <span class="portal-card-face portal-card-front">
                <span class="portal-card-icon">▤</span>
                <span class="portal-card-copy"><small>KNOWLEDGE</small><strong>知识库</strong><p>分类浏览文章与技术笔记</p></span>
                <span class="portal-card-arrow">↗</span>
              </span>
              <span class="portal-card-face knowledge-card-back" aria-hidden="true">
                <span class="card-back-mark">▤</span>
                <span class="card-placeholder-lines"><i></i><i></i><i></i></span>
              </span>
            </span>
          </RouterLink>
          <RouterLink class="portal-card portal-card-3d feature-card interview-book-card" to="/interview">
            <span class="portal-book-page" aria-hidden="true">
              <span class="book-page-mark">✦</span>
              <span class="card-placeholder-lines"><i></i><i></i><i></i></span>
            </span>
            <span class="portal-book-cover">
              <span class="portal-book-cover-face portal-book-cover-front">
                <span class="portal-card-icon">✦</span>
                <span class="portal-card-copy"><small>INTERVIEW</small><strong>八股文</strong><p>按主题整理面试知识与常见问题</p></span>
                <span class="portal-card-arrow">↗</span>
              </span>
              <span class="portal-book-cover-face portal-book-cover-back" aria-hidden="true">
                <span class="card-placeholder-lines"><i></i><i></i><i></i></span>
              </span>
            </span>
          </RouterLink>
          <RouterLink class="portal-card project-card" to="/projects">
            <span class="portal-card-icon">⌘</span>
            <div><small>PROJECTS</small><strong>项目实践</strong><p>从需求到落地的完整记录</p></div>
            <span class="portal-card-arrow">↗</span>
          </RouterLink>
          <RouterLink class="portal-card note-card" to="/notes">
            <span class="portal-card-icon">☁</span>
            <div><small>NOTES</small><strong>最近随记</strong><p>{{ notes[0]?.content || (notesState === 'loading' ? '正在加载最近随记' : '记录灵感与生活碎片') }}</p></div>
            <span class="portal-card-arrow">↗</span>
          </RouterLink>
        </div>
      </section>

      <aside class="portal-side">
        <section class="clock-card">
          <p>LOCAL TIME</p>
          <strong>{{ currentTime }}</strong>
          <span>{{ currentDate }}</span>
          <div class="clock-orbit" aria-hidden="true">
            <div class="clock-orbit-disc">
              <svg class="clock-hud" viewBox="0 0 220 220">
                <circle class="hud-track hud-track-outer" cx="110" cy="110" r="94" />
                <circle class="hud-ring hud-ring-a" cx="110" cy="110" r="88" pathLength="100" />
                <circle class="hud-ring hud-ring-b" cx="110" cy="110" r="72" pathLength="100" />
                <circle class="hud-ring hud-ring-warm" cx="110" cy="110" r="59" pathLength="100" />
                <circle class="hud-track hud-track-inner" cx="110" cy="110" r="45" />
                <circle class="hud-ring hud-ring-c" cx="110" cy="110" r="38" pathLength="100" />
                <circle class="hud-ring hud-ring-core" cx="110" cy="110" r="18" pathLength="100" />
                <circle class="hud-center" cx="110" cy="110" r="4" />
              </svg>
            </div>
          </div>
        </section>
        <section class="status-card">
          <span><i class="status-dot"></i> {{ serviceStatus.title }}</span>
          <span>{{ serviceStatus.detail }}</span>
        </section>
        <RouterLink class="feedback-card" to="/guestbook">
          <small>FEEDBACK</small>
          <strong>意见与留言</strong>
          <span>告诉我下一次应该更新什么 →</span>
        </RouterLink>
      </aside>
    </main>

    <footer class="wide-shell footer portal-footer">
      <div class="footer-main">
        <span>© {{ new Date().getFullYear() }} {{ site.config.title }}</span>
        <span>在代码与生活之间，持续记录。</span>
      </div>
      <div class="beian-row" aria-label="网站备案信息">
        <a href="https://beian.miit.gov.cn/" target="_blank" rel="noopener noreferrer">赣ICP备2026009462号-1</a>
        <a class="beian-gongan" href="http://www.beian.gov.cn/portal/registerSystemInfo?recordcode=36010602000401" target="_blank" rel="noopener noreferrer">
          <img :src="'/ghs.png'" alt="" aria-hidden="true" />
          <span>赣公网安备36010602000401号</span>
        </a>
      </div>
    </footer>

    <div v-if="announcementOpen" class="announcement-mask" role="dialog" aria-modal="true" aria-label="站点公告" @click.self="closeAnnouncement">
      <section class="announcement-dialog">
        <button class="announcement-close" type="button" aria-label="关闭公告" @click="closeAnnouncement">×</button>
        <p class="page-eyebrow">LATEST NEWS</p>
        <h2>站点公告</h2>
        <p>{{ site.config.announcement }}</p>
        <button class="btn primary" type="button" @click="closeAnnouncement">知道了</button>
      </section>
    </div>
  </div>
</template>
