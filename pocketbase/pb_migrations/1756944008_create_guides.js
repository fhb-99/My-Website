/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  let guides
  try {
    guides = app.findCollectionByNameOrId("guides")
  } catch {
    guides = new Collection({
      type: "base",
      name: "guides",
      listRule: "is_published = true",
      viewRule: "is_published = true",
      createRule: null,
      updateRule: null,
      deleteRule: null,
      fields: [
        { type: "text", name: "title", required: true, max: 200, presentable: true },
        { type: "text", name: "slug", required: true, max: 200 },
        { type: "text", name: "summary", max: 1000 },
        { type: "text", name: "source_url", max: 1000 },
        { type: "text", name: "license_url", max: 1000 },
        { type: "text", name: "source_version", max: 100 },
        { type: "bool", name: "is_published" },
        { type: "autodate", name: "created", onCreate: true },
        { type: "autodate", name: "updated", onCreate: true, onUpdate: true },
      ],
      indexes: ["CREATE UNIQUE INDEX idx_guides_slug ON guides (slug)"],
    })
    app.save(guides)
  }

  try {
    app.findCollectionByNameOrId("guide_chapters")
  } catch {
    const chapters = new Collection({
      type: "base",
      name: "guide_chapters",
      listRule: "is_published = true && guide.is_published = true",
      viewRule: "is_published = true && guide.is_published = true",
      createRule: null,
      updateRule: null,
      deleteRule: null,
      fields: [
        { type: "relation", name: "guide", required: true, collectionId: guides.id, maxSelect: 1, cascadeDelete: true },
        { type: "number", name: "chapter_no", required: true, min: 1 },
        { type: "text", name: "title", required: true, max: 200, presentable: true },
        { type: "text", name: "slug", required: true, max: 200 },
        { type: "editor", name: "content_md", required: true, convertURLs: false },
        { type: "bool", name: "is_published" },
        { type: "autodate", name: "created", onCreate: true },
        { type: "autodate", name: "updated", onCreate: true, onUpdate: true },
      ],
      indexes: [
        "CREATE UNIQUE INDEX idx_guide_chapters_number ON guide_chapters (guide, chapter_no)",
        "CREATE UNIQUE INDEX idx_guide_chapters_slug ON guide_chapters (guide, slug)",
      ],
    })
    app.save(chapters)
  }
}, (app) => {
  // 先删除从集合，避免关系字段阻止指南集合回滚。
  for (const name of ["guide_chapters", "guides"]) {
    try {
      app.delete(app.findCollectionByNameOrId(name))
    } catch {
      // 回滚时允许集合已经被手动删除。
    }
  }
})
