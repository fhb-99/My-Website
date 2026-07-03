<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'
import AdminLayout from '../components/AdminLayout.vue'
import { moderationApi } from '../api'
import type { ModerationConfig, ModerationLog, ModerationTestResult } from '@shared/types'

const config = reactive<ModerationConfig>({
  agent_enabled: false,
  provider: 'openai-compatible',
  api_base_url: '',
  model: '',
  blocked_words: [],
  strictness: 'normal',
  max_links: 2,
  confidence_threshold: 0.8,
  system_prompt: '你是博客评论审核助手，请判断内容是否适合公开展示，并给出简短原因。',
  auto_reject_enabled: true,
  auto_approve_enabled: false
})

const blockedWordsText = ref('')
const logs = ref<ModerationLog[]>([])
const notice = ref('')
const loading = ref(false)
const testContent = ref('')
const testResult = ref<ModerationTestResult | null>(null)
const manualTargetType = ref<'comment' | 'guestbook'>('comment')
const manualTargetId = ref('')

function syncBlockedWordsFromText() {
  config.blocked_words = blockedWordsText.value
    .split('\n')
    .map((item) => item.trim())
    .filter(Boolean)
}

function syncBlockedWordsToText() {
  blockedWordsText.value = config.blocked_words.join('\n')
}

async function loadConfig() {
  try {
    loading.value = true
    const data = await moderationApi.getConfig()
    Object.assign(config, data)
    syncBlockedWordsToText()
    notice.value = ''
  } catch (error) {
    console.warn('[ai-moderation] config unavailable:', error)
    notice.value = 'AI 审核配置接口已预留，后端持久化实现后这里会自动接入。'
  } finally {
    loading.value = false
  }
}

async function saveConfig() {
  try {
    loading.value = true
    syncBlockedWordsFromText()
    await moderationApi.saveConfig(config)
    notice.value = 'AI 审核配置已保存'
  } catch (error) {
    console.warn('[ai-moderation] save failed:', error)
    notice.value = error instanceof Error ? error.message : 'AI 审核配置保存失败'
  } finally {
    loading.value = false
  }
}

async function runTest() {
  if (!testContent.value.trim()) {
    notice.value = '请输入要测试的文本'
    return
  }

  try {
    loading.value = true
    testResult.value = await moderationApi.testContent({
      content: testContent.value.trim(),
      target_type: 'comment'
    })
    notice.value = '测试完成'
  } catch (error) {
    console.warn('[ai-moderation] test failed:', error)
    testResult.value = null
    notice.value = error instanceof Error ? error.message : 'AI 审核测试接口暂不可用'
  } finally {
    loading.value = false
  }
}

async function runManualModeration() {
  const id = Number(manualTargetId.value)
  if (!Number.isInteger(id) || id <= 0) {
    notice.value = '请输入有效的内容 ID'
    return
  }

  try {
    loading.value = true
    testResult.value = manualTargetType.value === 'comment'
      ? await moderationApi.moderateComment(id)
      : await moderationApi.moderateGuestbook(id)
    notice.value = '已触发一次 AI 辅助审核'
  } catch (error) {
    console.warn('[ai-moderation] manual moderate failed:', error)
    notice.value = error instanceof Error ? error.message : '手动 AI 审核接口暂不可用'
  } finally {
    loading.value = false
  }
}

async function loadLogs() {
  try {
    loading.value = true
    const result = await moderationApi.listLogs({ page: 1, limit: 30 })
    logs.value = result.data
    notice.value = ''
  } catch (error) {
    console.warn('[ai-moderation] logs unavailable:', error)
    logs.value = []
    notice.value = '审核日志接口已预留，后端落库后这里会展示规则、AI 和人工审核记录。'
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  syncBlockedWordsToText()
  loadConfig()
  loadLogs()
})
</script>

<template>
  <AdminLayout>
    <template #title>
      <div>
        <h1>AI 辅助审核</h1>
        <p class="muted">先搭建审核 agent 的管理界面：配置开关、屏蔽词、模型参数、测试文本和查看审核日志。</p>
      </div>
    </template>

    <section class="stack">
      <p v-if="notice" class="card muted">{{ notice }}</p>

      <section class="card stack">
        <div class="section-head">
          <div>
            <p class="muted">Agent Config</p>
            <h2>审核策略配置</h2>
          </div>
          <button class="btn primary" type="button" :disabled="loading" @click="saveConfig">保存配置</button>
        </div>

        <div class="config-grid">
          <label class="field inline-field">
            <span>启用 AI 审核</span>
            <input v-model="config.agent_enabled" type="checkbox" />
          </label>
          <label class="field inline-field">
            <span>自动拒绝高风险内容</span>
            <input v-model="config.auto_reject_enabled" type="checkbox" />
          </label>
          <label class="field inline-field">
            <span>允许自动通过低风险内容</span>
            <input v-model="config.auto_approve_enabled" type="checkbox" />
          </label>
          <label class="field">
            <span>审核严格度</span>
            <select v-model="config.strictness">
              <option value="loose">宽松</option>
              <option value="normal">普通</option>
              <option value="strict">严格</option>
            </select>
          </label>
          <label class="field">
            <span>AI 平台类型</span>
            <input v-model="config.provider" placeholder="openai-compatible / siliconflow / deepseek" />
          </label>
          <label class="field">
            <span>API Base URL</span>
            <input v-model="config.api_base_url" placeholder="https://api.example.com/v1" />
          </label>
          <label class="field">
            <span>模型名称</span>
            <input v-model="config.model" placeholder="moderation-model-name" />
          </label>
          <label class="field">
            <span>自动判定置信度阈值</span>
            <input v-model.number="config.confidence_threshold" type="number" min="0" max="1" step="0.05" />
          </label>
          <label class="field">
            <span>最大链接数量</span>
            <input v-model.number="config.max_links" type="number" min="0" />
          </label>
        </div>

        <label class="field">
          <span>屏蔽词，每行一个</span>
          <textarea v-model="blockedWordsText" placeholder="广告&#10;辱骂词&#10;敏感词" />
        </label>
        <label class="field">
          <span>系统提示词</span>
          <textarea v-model="config.system_prompt" />
        </label>
      </section>

      <section class="grid two-columns">
        <article class="card stack">
          <div class="section-head">
            <div>
              <p class="muted">Dry Run</p>
              <h2>测试一段内容</h2>
            </div>
            <button class="btn" type="button" :disabled="loading" @click="runTest">运行测试</button>
          </div>
          <label class="field">
            <span>测试文本</span>
            <textarea v-model="testContent" placeholder="输入一段评论或留言，后续会调用 AI 审核接口给出判断。" />
          </label>
          <p v-if="testResult" class="result-box">
            <strong>{{ testResult.decision }}</strong>
            <span>{{ testResult.reason }}</span>
          </p>
        </article>

        <article class="card stack">
          <div class="section-head">
            <div>
              <p class="muted">Manual Trigger</p>
              <h2>手动重新审核</h2>
            </div>
            <button class="btn" type="button" :disabled="loading" @click="runManualModeration">触发审核</button>
          </div>
          <label class="field">
            <span>内容类型</span>
            <select v-model="manualTargetType">
              <option value="comment">文章评论</option>
              <option value="guestbook">留言板</option>
            </select>
          </label>
          <label class="field">
            <span>内容 ID</span>
            <input v-model="manualTargetId" placeholder="例如：2" />
          </label>
          <p class="muted">这个入口后续会让管理员对某条评论或留言重新跑一次 AI 辅助审核。</p>
        </article>
      </section>

      <section class="card">
        <div class="section-head">
          <div>
            <p class="muted">Audit Logs</p>
            <h2>审核日志</h2>
          </div>
          <button class="btn" type="button" :disabled="loading" @click="loadLogs">刷新日志</button>
        </div>

        <table class="table">
          <thead>
            <tr>
              <th>对象</th>
              <th>决策</th>
              <th>来源</th>
              <th>原因</th>
              <th>时间</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in logs" :key="item.id">
              <td>{{ item.target_type }} #{{ item.target_id }}</td>
              <td>{{ item.decision }}</td>
              <td>{{ item.source }}</td>
              <td>{{ item.reason }}</td>
              <td>{{ item.created_at }}</td>
            </tr>
            <tr v-if="!logs.length">
              <td colspan="5" class="muted">暂无审核日志。后端日志表实现后会在这里显示。</td>
            </tr>
          </tbody>
        </table>
      </section>
    </section>
  </AdminLayout>
</template>

<style scoped>
.section-head { display: flex; justify-content: space-between; align-items: center; gap: 16px; margin-bottom: 10px; }
.section-head h2 { margin: 2px 0 0; }
.config-grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 14px; }
.two-columns { grid-template-columns: repeat(2, minmax(0, 1fr)); }
.inline-field { display: flex; align-items: center; justify-content: space-between; border: 1px solid rgba(26,58,96,.12); border-radius: 16px; padding: 12px 14px; background: #fff; }
.inline-field input { width: 18px; height: 18px; }
.result-box { display: grid; gap: 6px; border-radius: 18px; padding: 14px 16px; background: rgba(47,124,246,.08); color: #172033; }
@media (max-width: 980px) { .config-grid, .two-columns { grid-template-columns: 1fr; } .section-head { align-items: flex-start; flex-direction: column; } .table { display: block; overflow-x: auto; } }
</style>
