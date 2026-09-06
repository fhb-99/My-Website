/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  const names = [
    "tags",
    "posts",
    "comments",
    "guestbook_messages",
    "notes",
    "projects",
    "site_config",
    "music_tracks",
  ]

  for (const name of names) {
    const collection = app.findCollectionByNameOrId(name)
    let changed = false

    if (!collection.fields.getByName("created")) {
      collection.fields.add(new AutodateField({ name: "created", onCreate: true }))
      changed = true
    }
    if (!collection.fields.getByName("updated")) {
      collection.fields.add(new AutodateField({ name: "updated", onCreate: true, onUpdate: true }))
      changed = true
    }

    if (changed) app.save(collection)
  }
}, () => {
  // 兼容迁移可能没有实际新增字段，回滚时保留时间字段更安全。
})
