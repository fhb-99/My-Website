import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'

import { siteConfigApi } from '@/api'
import { useSiteStore } from './site'

vi.mock('@/api', () => ({
  siteConfigApi: {
    getConfig: vi.fn(),
  },
}))

const mockedGetConfig = vi.mocked(siteConfigApi.getConfig)

describe('site store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    mockedGetConfig.mockReset()
  })

  it('keeps an empty real config instead of substituting display content', async () => {
    mockedGetConfig.mockResolvedValue({ title: '', subtitle: '', announcement: '' })

    const store = useSiteStore()
    await store.load()

    expect(store.state).toBe('ready')
    expect(store.config.title).toBe('')
    expect(store.config.subtitle).toBe('')
    expect(store.config.announcement).toBe('')
  })
})
