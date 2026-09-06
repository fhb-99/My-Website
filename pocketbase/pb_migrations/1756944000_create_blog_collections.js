/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  const timestampFields = () => [
    { type: "autodate", name: "created", onCreate: true },
    { type: "autodate", name: "updated", onCreate: true, onUpdate: true },
  ]

  const tags = new Collection({
    type: "base",
    name: "tags",
    listRule: "",
    viewRule: "",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "name", required: true, max: 64, presentable: true },
      { type: "text", name: "slug", required: true, max: 80 },
      ...timestampFields(),
    ],
    indexes: [
      "CREATE UNIQUE INDEX idx_tags_name ON tags (name)",
      "CREATE UNIQUE INDEX idx_tags_slug ON tags (slug)",
    ],
  })
  app.save(tags)

  const posts = new Collection({
    type: "base",
    name: "posts",
    listRule: "is_published = true",
    viewRule: "is_published = true",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "number", name: "legacy_id", min: 0 },
      { type: "text", name: "title", required: true, max: 200, presentable: true },
      { type: "text", name: "slug", required: true, max: 200 },
      { type: "text", name: "summary", max: 1000 },
      { type: "text", name: "content_md" },
      { type: "editor", name: "content_html" },
      {
        type: "file",
        name: "cover",
        maxSelect: 1,
        maxSize: 5242880,
        mimeTypes: ["image/jpeg", "image/png", "image/webp", "image/gif"],
      },
      {
        type: "relation",
        name: "tags",
        collectionId: tags.id,
        maxSelect: 20,
      },
      { type: "bool", name: "is_published" },
      { type: "number", name: "views", min: 0 },
      ...timestampFields(),
    ],
    indexes: [
      "CREATE UNIQUE INDEX idx_posts_slug ON posts (slug)",
      "CREATE UNIQUE INDEX idx_posts_legacy_id ON posts (legacy_id) WHERE legacy_id > 0",
    ],
  })
  app.save(posts)

  const comments = new Collection({
    type: "base",
    name: "comments",
    listRule: "status = 'approved'",
    viewRule: "status = 'approved'",
    createRule: "@request.body.status = 'pending' && @request.body.post != '' && @request.body.nickname != '' && @request.body.content != ''",
    updateRule: null,
    deleteRule: null,
    fields: [
      {
        type: "relation",
        name: "post",
        required: true,
        collectionId: posts.id,
        maxSelect: 1,
        cascadeDelete: true,
      },
      { type: "text", name: "nickname", required: true, max: 32, presentable: true },
      { type: "text", name: "content", required: true, max: 800 },
      {
        type: "select",
        name: "status",
        required: true,
        maxSelect: 1,
        values: ["pending", "approved", "rejected"],
      },
      ...timestampFields(),
    ],
    indexes: ["CREATE INDEX idx_comments_post_status ON comments (post, status)"],
  })
  app.save(comments)

  const guestbook = new Collection({
    type: "base",
    name: "guestbook_messages",
    listRule: "status = 'approved'",
    viewRule: "status = 'approved'",
    createRule: "@request.body.status = 'pending' && @request.body.nickname != '' && @request.body.content != ''",
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "nickname", required: true, max: 32, presentable: true },
      { type: "text", name: "content", required: true, max: 800 },
      {
        type: "select",
        name: "status",
        required: true,
        maxSelect: 1,
        values: ["pending", "approved", "rejected"],
      },
      ...timestampFields(),
    ],
    indexes: ["CREATE INDEX idx_guestbook_status ON guestbook_messages (status)"],
  })
  app.save(guestbook)

  const notes = new Collection({
    type: "base",
    name: "notes",
    listRule: "is_published = true",
    viewRule: "is_published = true",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "content", required: true, max: 4000, presentable: true },
      { type: "text", name: "mood", max: 32 },
      { type: "bool", name: "is_published" },
      ...timestampFields(),
    ],
    indexes: ["CREATE INDEX idx_notes_public ON notes (is_published)"],
  })
  app.save(notes)

  const projects = new Collection({
    type: "base",
    name: "projects",
    listRule: "is_published = true",
    viewRule: "is_published = true",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "name", required: true, max: 120, presentable: true },
      { type: "text", name: "summary", max: 1000 },
      { type: "url", name: "url" },
      {
        type: "relation",
        name: "tags",
        collectionId: tags.id,
        maxSelect: 20,
      },
      { type: "number", name: "sort_order", min: 0 },
      { type: "bool", name: "is_published" },
      ...timestampFields(),
    ],
    indexes: ["CREATE INDEX idx_projects_public_order ON projects (is_published, sort_order)"],
  })
  app.save(projects)

  const siteConfig = new Collection({
    type: "base",
    name: "site_config",
    listRule: "",
    viewRule: "",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "title", required: true, max: 120, presentable: true },
      { type: "text", name: "subtitle", max: 240 },
      { type: "text", name: "announcement", max: 1000 },
      { type: "bool", name: "music_enabled" },
      { type: "number", name: "music_volume", min: 0, max: 1 },
      ...timestampFields(),
    ],
  })
  app.save(siteConfig)

  const initialConfig = new Record(siteConfig)
  initialConfig.set("title", "个人博客")
  initialConfig.set("subtitle", "记录技术与生活")
  initialConfig.set("announcement", "")
  initialConfig.set("music_enabled", false)
  initialConfig.set("music_volume", 0.35)
  app.save(initialConfig)

  const musicTracks = new Collection({
    type: "base",
    name: "music_tracks",
    listRule: "is_enabled = true",
    viewRule: "is_enabled = true",
    createRule: null,
    updateRule: null,
    deleteRule: null,
    fields: [
      { type: "text", name: "title", required: true, max: 120, presentable: true },
      { type: "text", name: "artist", max: 120 },
      {
        type: "file",
        name: "cover",
        maxSelect: 1,
        maxSize: 5242880,
        mimeTypes: ["image/jpeg", "image/png", "image/webp", "image/gif"],
      },
      {
        type: "file",
        name: "audio",
        required: true,
        maxSelect: 1,
        maxSize: 52428800,
        mimeTypes: ["audio/mpeg", "audio/mp4", "audio/aac", "audio/ogg", "audio/wav"],
      },
      { type: "number", name: "sort_order", min: 0 },
      { type: "bool", name: "is_enabled" },
      ...timestampFields(),
    ],
    indexes: ["CREATE INDEX idx_music_enabled_order ON music_tracks (is_enabled, sort_order)"],
  })
  app.save(musicTracks)
}, (app) => {
  const names = [
    "music_tracks",
    "site_config",
    "projects",
    "notes",
    "guestbook_messages",
    "comments",
    "posts",
    "tags",
  ]

  for (const name of names) {
    try {
      app.delete(app.findCollectionByNameOrId(name))
    } catch {
      // 回滚时允许集合已经被手动删除。
    }
  }
})
