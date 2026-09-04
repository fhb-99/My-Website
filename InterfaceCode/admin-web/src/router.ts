import { createRouter, createWebHashHistory } from "vue-router";
import { useSessionStore } from "./stores/session";
import LoginView from "./views/LoginView.vue";
import DashboardView from "./views/DashboardView.vue";
import PostsManageView from "./views/PostsManageView.vue";
import NotesManageView from "./views/NotesManageView.vue";
import ProjectsManageView from "./views/ProjectsManageView.vue";
import UploadsView from "./views/UploadsView.vue";
import ModerationView from "./views/ModerationView.vue";
import SettingsView from "./views/SettingsView.vue";
import LearningRoadmapManageView from "./views/LearningRoadmapManageView.vue";

export const router = createRouter({
  // 管理端独立部署时不要求服务端配置 /admin/* 回退到 index.html。
  history: createWebHashHistory(),
  routes: [
    { path: "/", redirect: "/dashboard" },
    { path: "/login", component: LoginView, meta: { public: true } },
    { path: "/dashboard", component: DashboardView },
    { path: "/posts", component: PostsManageView, meta: { contentType: "article" } },
    { path: "/interview-posts", component: PostsManageView, meta: { contentType: "interview" } },
    { path: "/learning-roadmap", component: LearningRoadmapManageView },
    { path: "/notes", component: NotesManageView },
    { path: "/projects", component: ProjectsManageView },
    { path: "/uploads", component: UploadsView },
    { path: "/moderation", component: ModerationView },
    { path: "/settings", component: SettingsView },
  ],
});

router.beforeEach((to) => {
  const session = useSessionStore();
  if (to.meta.public) return true;
  if (!session.isLoggedIn) return "/login";
  return true;
});
