import { createRouter, createWebHistory } from 'vue-router'
import WelcomeView from './views/WelcomeView.vue'
import HomeView from './views/HomeView.vue'
import PostsView from './views/PostsView.vue'
import PostDetailView from './views/PostDetailView.vue'
import NotesView from './views/NotesView.vue'
import ProjectsView from './views/ProjectsView.vue'
import GuestbookView from './views/GuestbookView.vue'
import AboutView from './views/AboutView.vue'
export const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/', name: 'welcome', component: WelcomeView },
    { path: '/home', name: 'home', component: HomeView },
    { path: '/posts', name: 'posts', component: PostsView },
    { path: '/posts/:id', name: 'post-detail', component: PostDetailView },
    { path: '/notes', name: 'notes', component: NotesView },
    { path: '/projects', name: 'projects', component: ProjectsView },
    { path: '/guestbook', name: 'guestbook', component: GuestbookView },
    { path: '/about', name: 'about', component: AboutView }
  ],
  scrollBehavior: () => ({ top: 0 })
})
