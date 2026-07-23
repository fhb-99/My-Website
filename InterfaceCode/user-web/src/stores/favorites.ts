import { ref } from 'vue'
import { defineStore } from 'pinia'

const storageKey = 'blog-favorite-post-ids'

function readIds() {
  try {
    const value = JSON.parse(localStorage.getItem(storageKey) || '[]')
    return Array.isArray(value) ? value.filter((id): id is number => Number.isInteger(id)) : []
  } catch { return [] }
}

export const useFavoritesStore = defineStore('favorites', () => {
  const ids = ref<number[]>(readIds())
  const has = (postId: number) => ids.value.includes(postId)
  function toggle(postId: number) {
    ids.value = has(postId) ? ids.value.filter((id) => id !== postId) : [...ids.value, postId]
    localStorage.setItem(storageKey, JSON.stringify(ids.value))
  }
  return { ids, has, toggle }
})
