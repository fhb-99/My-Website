/// <reference path="../pb_data/types.d.ts" />

routerAdd("POST", "/api/posts/{id}/view", (e) => {
  const postId = e.request.pathValue("id")
  const visitorId = String(e.requestInfo().body.visitor_id || "").trim()
  if (!/^[a-zA-Z0-9-]{8,128}$/.test(visitorId)) {
    throw new BadRequestError("访客标识无效")
  }

  let counted = false
  let views = 0
  e.app.runInTransaction((txApp) => {
    const post = txApp.findRecordById("posts", postId)
    if (!post.getBool("is_published")) throw new NotFoundError("文章不存在")

    const viewedDate = new Date().toISOString().slice(0, 10)
    const existing = txApp.findRecordsByFilter(
      "post_view_events",
      "post = {:post} && visitor_id = {:visitor} && viewed_date = {:date}",
      "",
      1,
      0,
      { post: postId, visitor: visitorId, date: viewedDate },
    )

    if (!existing.length) {
      const event = new Record(txApp.findCollectionByNameOrId("post_view_events"))
      event.set("post", postId)
      event.set("visitor_id", visitorId)
      event.set("viewed_date", viewedDate)
      txApp.save(event)

      post.set("views", post.getInt("views") + 1)
      txApp.save(post)
      counted = true
    }
    views = post.getInt("views")
  })

  return e.json(200, {
    counted,
    views,
    message: counted ? "阅读量已记录" : "今日已记录过该文章",
  })
})
