/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  let categories
  try {
    categories = app.findCollectionByNameOrId("interview_categories")
  } catch {
    categories = new Collection({
      type: "base",
      name: "interview_categories",
      listRule: "",
      viewRule: "",
      createRule: null,
      updateRule: null,
      deleteRule: null,
      fields: [
        { type: "text", name: "name", required: true, max: 80, presentable: true },
        { type: "number", name: "sort_order", min: 0 },
        { type: "autodate", name: "created", onCreate: true },
        { type: "autodate", name: "updated", onCreate: true, onUpdate: true },
      ],
      indexes: ["CREATE UNIQUE INDEX idx_interview_categories_name ON interview_categories (name)"],
    })
    app.save(categories)
  }

  const names = ["C++ 基础语法", "C++ 进阶", "Linux", "计算机网络", "Qt", "音视频"]
  for (let index = 0; index < names.length; index += 1) {
    try {
      app.findFirstRecordByFilter(categories, "name = {:name}", { name: names[index] })
    } catch {
      const record = new Record(categories)
      record.set("name", names[index])
      record.set("sort_order", index * 10)
      app.save(record)
    }
  }
}, () => {
  // 已有文章通过目录名称关联，回滚时保留集合避免管理入口丢失。
})
