/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  try {
    app.findCollectionByNameOrId("post_view_events")
    return
  } catch {
    // 同一访客每天只为同一篇文章增加一次阅读量。
  }

  const posts = app.findCollectionByNameOrId("posts")
  const events = new Collection({
    type: "base",
    name: "post_view_events",
    listRule: null,
    viewRule: null,
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "relation", name: "post", required: true, collectionId: posts.id, maxSelect: 1, cascadeDelete: true },
      { type: "text", name: "visitor_id", required: true, max: 128 },
      { type: "text", name: "viewed_date", required: true, max: 10 },
      { type: "autodate", name: "created", onCreate: true },
    ],
    indexes: ["CREATE UNIQUE INDEX idx_post_view_daily ON post_view_events (post, visitor_id, viewed_date)"],
  })
  app.save(events)
}, () => {
  // 阅读事件属于统计数据，回滚时保留，避免计数去重记录丢失。
})
