/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  const posts = app.findCollectionByNameOrId("posts")
  if (!posts.fields.getByName("content_html")) {
    posts.fields.add(new EditorField({ name: "content_html" }))
    app.save(posts)
  }
}, () => {
  // 兼容迁移在新数据库上可能没有实际新增字段，因此不主动删除。
})
