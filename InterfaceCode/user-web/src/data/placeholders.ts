import type { NoteItem, PostSummary, ProjectItem } from '@shared/types'
export const featuredPosts: PostSummary[] = [
  { id: 1, title: '把博客重新整理成一个安静的角落', slug: 'quiet-blog-corner', summary: '记录搭建博客时的取舍：少一点装饰，多一点可读性。', tags: ['建站', '随笔'], views: 128, created_at: '2026-06-20' },
  { id: 2, title: 'C++ 后端接口的阶段性闭环', slug: 'cpp-backend-loop', summary: '从文章上传到用户访问，再到评论留言的下一步规划。', tags: ['C++', '后端'], views: 96, created_at: '2026-06-18' },
  { id: 3, title: '前端从静态页走向工程化', slug: 'frontend-vue-refactor', summary: '用 Vue 管理页面、状态和后续迭代，让博客慢慢长成产品。', tags: ['Vue', '前端'], views: 76, created_at: '2026-06-16' }
]
export const notes: NoteItem[] = [
  { id: 1, content: '今天把界面改得更亮了一点，心情也像被擦干净的玻璃。', mood: 'calm', created_at: '2026-06-22' },
  { id: 2, content: '先把结构搭稳，再慢慢追求细节。', mood: 'focus', created_at: '2026-06-19' }
]
export const projects: ProjectItem[] = [
  { id: 1, name: '个人博客平台', summary: 'Vue 前端 + C++ 后端的长期维护项目。', tags: ['Vue', 'C++', 'SQLite'] },
  { id: 2, name: 'Markdown 发布流', summary: '围绕 Markdown 上传、渲染和文章访问的内容管线。', tags: ['Markdown', 'Tooling'] }
]
