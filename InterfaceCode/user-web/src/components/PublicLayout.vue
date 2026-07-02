<template>
  <div>
    <header class="nav-blur">
      <div class="shell nav-inner">
        <div class="nav-left">
          <span class="music-pill">♪ TODO <span>&gt;</span></span>
          <RouterLink class="nav-link brand" to="/home">My Blog</RouterLink>
          <RouterLink class="nav-link" to="/posts">文章</RouterLink>
        </div>
        <nav class="nav-center" aria-label="主导航">
          <RouterLink class="nav-link" to="/notes">碎碎念</RouterLink>
          <RouterLink class="nav-link" to="/projects">项目</RouterLink>
          <RouterLink class="nav-link" to="/about">关于</RouterLink>
          <RouterLink class="nav-link" to="/guestbook">留言板</RouterLink>
        </nav>
        <div class="nav-right">
          <RouterLink class="nav-link" to="/">开屏</RouterLink>
        </div>
      </div>
    </header>

    <section class="shell hero-wrap">
      <h1 class="hero-title"><slot name="title">My Blog</slot></h1>
      <p class="hero-sub">
        <slot name="subtitle">Life is a coding, I will debug it.</slot>
      </p>
    </section>

    <main class="shell"><slot /></main>
    <footer class="shell footer">
      © {{ new Date().getFullYear() }} My Blog · Built with curiosity.
    </footer>
  </div>
</template>
