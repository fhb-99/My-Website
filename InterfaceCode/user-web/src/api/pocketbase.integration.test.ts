import { describe, expect, it } from 'vitest'

import { pocketBasePostsApi, pocketBaseSiteConfigApi } from './pocketbase'

// 只有显式配置本地 PocketBase 时才运行，避免普通单元测试依赖外部进程。
const describeWithPocketBase = import.meta.env.VITE_RUN_POCKETBASE_INTEGRATION === 'true' ? describe : describe.skip

describeWithPocketBase('PocketBase 本地集成', () => {
  it('读取初始化的站点配置', async () => {
    await expect(pocketBaseSiteConfigApi.getConfig()).resolves.toMatchObject({
      title: '个人博客',
      subtitle: '记录技术与生活',
    })
  })

  it('以旧分页结构返回文章列表', async () => {
    const result = await pocketBasePostsApi.listPosts({ page: 1, limit: 10 })

    expect(result).toMatchObject({
      data: [],
      page: 1,
      limit: 10,
      total: 0,
      total_pages: 0,
      has_more: false,
    })
  })
})
