<template>
  <main class="welcome-page">
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
      <div>
        <div class="eyebrow">Welcome to my corner</div>
        <h1 class="title">记录技术，也记录<span class="gradient-text">生活的光</span></h1>
        <p class="subtitle">这里是我的个人博客，放一些 C++ 后端、前端界面、项目复盘和日常想法。希望它像一张干净的书桌，打开时轻松，读起来舒服。</p>
        <div style="display:flex;gap:14px;flex-wrap:wrap;margin-top:32px">
          <RouterLink class="btn primary" to="/home">进入博客主页</RouterLink>
          <RouterLink class="btn" to="/posts">先看文章</RouterLink>
        </div>
      </div>

      <section class="visual" aria-hidden="true">
        <div class="soft-orb orb-a"></div>
        <div class="soft-orb orb-b"></div>
        <div class="float-card reading"><div class="float-label">Now writing</div><div class="float-value">Blog</div></div>
        <div class="desk-card">
          <div class="window-dots"><span></span><span></span><span></span></div>
          <div class="note-title">Today's notes</div>
          <div class="note-line long"></div><div class="note-line mid"></div><div class="note-line short"></div>
          <div class="mini-grid">
            <div class="card"><strong>Backend</strong><p>接口、数据、部署和安全边界。</p></div>
            <div class="card"><strong>Frontend</strong><p>页面、交互和一点审美实验。</p></div>
          </div>
        </div>
        <div class="float-card idea"><div class="float-label">Mood</div><div class="float-value">Clean</div></div>
      </section>
    </section>
  </main>
</template>
