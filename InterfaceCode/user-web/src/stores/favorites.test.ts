import { beforeEach, describe, expect, it } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'

import { useFavoritesStore } from './favorites'

describe('favorites store', () => {
  beforeEach(() => {
    localStorage.clear()
    setActivePinia(createPinia())
  })

  it('persists a toggled favourite id', () => {
    const store = useFavoritesStore()
    store.toggle(42)
    expect(store.has(42)).toBe(true)
    expect(localStorage.getItem('blog-favorite-post-ids')).toContain('42')
  })
})
