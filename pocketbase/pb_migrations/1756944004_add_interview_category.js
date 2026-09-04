/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  const posts = app.findCollectionByNameOrId("posts")
  if (posts.fields.getByName("interview_category")) return

  // 八股文使用独立目录字段，不再把标签当成目录层级。
  posts.fields.add(new TextField({
    name: "interview_category",
    max: 80,
  }))
  app.save(posts)
}, () => {
  // 回滚时保留字段，避免已经归档的八股文失去目录信息。
})
