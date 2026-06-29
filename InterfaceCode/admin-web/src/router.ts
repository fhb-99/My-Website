import { createRouter, createWebHistory } from "vue-router";
import LoginView from "./views/LoginView.vue";
import DashboardView from "./views/DashboardView.vue";
import PostsManageView from "./views/PostsManageView.vue";
import UploadsView from "./views/UploadsView.vue";
import ModerationView from "./views/ModerationView.vue";
import SettingsView from "./views/SettingsView.vue";
export const router = createRouter({
  history: createWebHistory("/admin/"),
  routes: [
    { path: "/", redirect: "/dashboard" },
    { path: "/login", component: LoginView },
    { path: "/dashboard", component: DashboardView },
    { path: "/posts", component: PostsManageView },
    { path: "/uploads", component: UploadsView },
    { path: "/moderation", component: ModerationView },
    { path: "/settings", component: SettingsView },
  ],
});
