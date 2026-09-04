/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  const posts = app.findCollectionByNameOrId("posts")
  if (posts.fields.getByName("content_type")) return

  // 空值兼容迁移前的文章；新的管理端会始终写入明确的栏目类型。
  posts.fields.add(new SelectField({
    name: "content_type",
    maxSelect: 1,
    values: ["article", "interview"],
  }))
  app.save(posts)
}, () => {
  // 回滚时保留字段，避免已经分类的文章失去栏目信息。
})
