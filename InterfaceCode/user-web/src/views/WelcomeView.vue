<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";

const slides = [
  {
    title: "Morning Clouds",
    description: "",
    image: "/clouds/cloud-morning.webp",
    pageTone:
      "radial-gradient(circle at 18% 18%, rgba(255, 224, 178, .42), transparent 32%), radial-gradient(circle at 78% 18%, rgba(155, 207, 255, .36), transparent 34%), linear-gradient(135deg, #fff9ef 0%, #eef7ff 55%, #ffffff 100%)",
    accent: "#e79345",
    titleGradient:
      "linear-gradient(110deg, #1f2937 0%, #d78238 45%, #4f91d9 100%)",
  },
  {
    title: "Daylight Clouds",
    description: "",
    image: "/clouds/cloud-day.webp",
    pageTone:
      "radial-gradient(circle at 16% 20%, rgba(126, 199, 255, .42), transparent 32%), radial-gradient(circle at 78% 18%, rgba(224, 244, 255, .66), transparent 34%), linear-gradient(135deg, #f5fbff 0%, #ecf7ff 52%, #ffffff 100%)",
    accent: "#2f7cf6",
    titleGradient:
      "linear-gradient(110deg, #18243a 0%, #276fd8 48%, #21a7c8 100%)",
  },
  {
    title: "Sunset Clouds",
    description: "",
    image: "/clouds/cloud-evening.webp",
    pageTone:
      "radial-gradient(circle at 18% 16%, rgba(255, 184, 132, .44), transparent 32%), radial-gradient(circle at 80% 20%, rgba(255, 139, 178, .28), transparent 34%), linear-gradient(135deg, #fff5ec 0%, #fff0f4 48%, #ffffff 100%)",
    accent: "#d86b57",
    titleGradient:
      "linear-gradient(110deg, #2b2430 0%, #d86b57 48%, #e7a34d 100%)",
  },
];

const activeIndex = ref(0);
const activeSlide = computed(() => slides[activeIndex.value]);
let timer: number | undefined;

const pageStyle = computed(() => ({
  "--welcome-tone": activeSlide.value.pageTone,
  "--accent": activeSlide.value.accent,
  "--welcome-title-gradient": activeSlide.value.titleGradient,
}));

onMounted(() => {
  timer = window.setInterval(() => {
    activeIndex.value = (activeIndex.value + 1) % slides.length;
  }, 6500);
});

onBeforeUnmount(() => {
  window.clearInterval(timer);
});
</script>

<template>
  <main class="welcome-page" :style="pageStyle">
    <nav class="welcome-nav">
      <div class="shell nav-inner">
        <RouterLink class="brand" to="/">My Blog</RouterLink>
        <div class="nav-links">
          <RouterLink class="nav-link" to="/home">首页</RouterLink>
          <RouterLink class="nav-link" to="/posts">文章</RouterLink>
          <RouterLink class="nav-link" to="/projects">项目</RouterLink>
          <RouterLink class="nav-link" to="/about">关于</RouterLink>
        </div>
      </div>
    </nav>

    <section class="shell welcome-hero">
      <div class="welcome-copy">
        <div class="eyebrow">Welcome to my corner</div>
        <h1 class="title">
          记录成长，也记录<span class="gradient-text">生活的光</span>
        </h1>
        <p class="subtitle">行胜于言 ikun</p>
        <div class="welcome-actions">
          <RouterLink class="btn primary" to="/home">进入博客主页</RouterLink>
        </div>
      </div>

      <section class="visual cloud-visual" aria-label="云主题风景轮播">
        <img
          v-for="(slide, index) in slides"
          :key="slide.title"
          class="cloud-image"
          :class="{ active: index === activeIndex }"
          :src="slide.image"
          :alt="slide.description"
        />
        <div class="cloud-shade"></div>
        <div class="cloud-caption">
          <p>Cloud Gallery</p>
          <h2>{{ activeSlide.title }}</h2>
          <span>{{ activeSlide.description }}</span>
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.welcome-page {
  background: var(--welcome-tone);
  transition: background 900ms ease;
}

.welcome-nav {
  background: rgba(255, 255, 255, 0.62);
}

.welcome-copy {
  position: relative;
  z-index: 1;
}

.gradient-text {
  background: var(--welcome-title-gradient);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
  transition: background 900ms ease;
}

.welcome-actions {
  display: flex;
  gap: 14px;
  flex-wrap: wrap;
  margin-top: 32px;
}

.cloud-visual {
  position: relative;
  min-height: 500px;
  overflow: hidden;
  border: 1px solid rgba(255, 255, 255, 0.82);
  border-radius: 38px;
  background: rgba(255, 255, 255, 0.38);
  box-shadow: 0 34px 90px rgba(59, 93, 132, 0.16);
}

.cloud-image {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
  opacity: 0;
  transform: scale(1.035);
  transition:
    opacity 1100ms ease,
    transform 6500ms ease;
}

.cloud-image.active {
  opacity: 1;
  transform: scale(1);
}

.cloud-shade {
  position: absolute;
  inset: 0;
  background:
    linear-gradient(180deg, rgba(14, 24, 42, 0.02), rgba(14, 24, 42, 0.28)),
    radial-gradient(
      circle at 18% 18%,
      rgba(255, 255, 255, 0.34),
      transparent 36%
    );
}

.cloud-caption {
  position: absolute;
  left: 28px;
  right: 28px;
  bottom: 26px;
  color: #fff;
  text-shadow: 0 12px 28px rgba(0, 0, 0, 0.32);
}

.cloud-caption p {
  margin: 0 0 8px;
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  opacity: 0.86;
}

.cloud-caption h2 {
  margin: 0;
  font-size: clamp(34px, 5vw, 58px);
  line-height: 1;
}

.cloud-caption span {
  display: block;
  max-width: 420px;
  margin-top: 12px;
  line-height: 1.7;
  opacity: 0.9;
}

:global(.app[data-theme="night"]) .welcome-page {
  color: var(--text);
  background:
    radial-gradient(
      circle at 18% 16%,
      rgba(69, 99, 143, 0.24),
      transparent 32%
    ),
    radial-gradient(
      circle at 82% 18%,
      rgba(216, 107, 87, 0.14),
      transparent 34%
    ),
    linear-gradient(135deg, #101827 0%, #172033 58%, #111827 100%);
}

@media (max-width: 900px) {
  .cloud-visual {
    min-height: 420px;
  }
}

@media (max-width: 560px) {
  .cloud-visual {
    min-height: 360px;
    border-radius: 28px;
  }

  .cloud-caption {
    left: 20px;
    right: 20px;
    bottom: 20px;
  }
}
</style>
