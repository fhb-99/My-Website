import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { guidesApi } from '@/api'
import GuidesView from './GuidesView.vue'

vi.mock('@/api', () => ({
  guidesApi: {
    getGuide: vi.fn(),
    listChapters: vi.fn(),
    getChapter: vi.fn(),
  },
}))

const guide = {
  id: 'guide-1',
  title: '高性价比人生指南',
  slug: 'how-to-live-better',
  summary: '按章节查阅生活中的重要选择。',
  source_url: 'https://github.com/example/guide',
  license_url: 'https://creativecommons.org/licenses/by/4.0/',
  source_version: '1234567890abcdef',
  updated_at: '',
}
const chapters = [
  { id: 'chapter-1', guide_id: guide.id, chapter_no: 1, title: '不要早死', slug: '01-不要早死', updated_at: '' },
  { id: 'chapter-2', guide_id: guide.id, chapter_no: 2, title: '不要慢慢死', slug: '02-不要慢慢死', updated_at: '' },
]

async function mountGuides(path: string) {
  const router = createRouter({
    history: createMemoryHistory(),
    routes: [
      { path: '/home', component: { template: '<div />' } },
      { path: '/guides/:chapterSlug?', name: 'guides', component: GuidesView },
    ],
  })
  await router.push(path)
  await router.isReady()
  const wrapper = mount(GuidesView, {
    global: {
      plugins: [router],
      stubs: { PublicLayout: { template: '<div><slot /></div>' } },
    },
  })
  await flushPromises()
  return { router, wrapper }
}

describe('GuidesView', () => {
  beforeEach(() => {
    vi.mocked(guidesApi.getGuide).mockResolvedValue(guide)
    vi.mocked(guidesApi.listChapters).mockResolvedValue(chapters)
    vi.mocked(guidesApi.getChapter).mockImplementation(async (_guideId, slug) => {
      const summary = chapters.find((item) => item.slug === slug) || chapters[0]
      return { ...summary, content_html: `<h3>${summary.title}正文</h3><p>章节内容</p>` }
    })
  })

  it('loads the directory and selected chapter', async () => {
    const { wrapper } = await mountGuides('/guides/02-不要慢慢死')

    expect(guidesApi.getGuide).toHaveBeenCalledWith('how-to-live-better')
    expect(guidesApi.listChapters).toHaveBeenCalledWith(guide.id)
    expect(guidesApi.getChapter).toHaveBeenCalledWith(guide.id, '02-不要慢慢死')
    expect(wrapper.get('.guide-chapter-heading h1').text()).toBe('不要慢慢死')
    expect(wrapper.findAll('.guide-directory-link')).toHaveLength(2)
    expect(wrapper.get('.guide-directory-link.active').text()).toContain('不要慢慢死')
    expect(wrapper.get('.guide-attribution').text()).toContain('CC BY 4.0')
  })

  it('opens the first chapter when no chapter is specified', async () => {
    const { router, wrapper } = await mountGuides('/guides')

    expect(router.currentRoute.value.params.chapterSlug).toBe('01-不要早死')
    expect(wrapper.get('.guide-chapter-heading h1').text()).toBe('不要早死')
  })
})
