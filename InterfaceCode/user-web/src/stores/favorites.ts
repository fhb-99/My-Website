import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { EntityId } from '@shared/types'

const storageKey = 'blog-favorite-post-ids'

function readIds() {
  try {
    const value = JSON.parse(localStorage.getItem(storageKey) || '[]')
    return Array.isArray(value)
      ? value.filter((id): id is EntityId => typeof id === 'string' || Number.isInteger(id))
      : []
  } catch { return [] }
}

export const useFavoritesStore = defineStore('favorites', () => {
  const ids = ref<EntityId[]>(readIds())
  const has = (postId: EntityId) => ids.value.includes(postId)
  function toggle(postId: EntityId) {
    ids.value = has(postId) ? ids.value.filter((id) => id !== postId) : [...ids.value, postId]
    localStorage.setItem(storageKey, JSON.stringify(ids.value))
  }
  return { ids, has, toggle }
})
