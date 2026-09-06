(function (window, document) {
  "use strict";

  function text(value, fallback) {
    if (value === undefined || value === null || value === "") return fallback || "";
    return String(value);
  }

  function escapeHtml(value) {
    return text(value).replace(/[&<>"']/g, function (char) {
      return {
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
      }[char];
    });
  }

  function formatDate(value) {
    if (!value) return "";
    var normalized = String(value).replace(" ", "T");
    var date = new Date(normalized);
    if (Number.isNaN(date.getTime())) return String(value).slice(0, 10);
    return date.toLocaleDateString("zh-CN", { year: "numeric", month: "2-digit", day: "2-digit" });
  }

  function getList(payload) {
    if (Array.isArray(payload)) return payload;
    if (payload && Array.isArray(payload.data)) return payload.data;
    if (payload && Array.isArray(payload.items)) return payload.items;
    return [];
  }

  function setStatus(node, message, type) {
    if (!node) return;
    node.textContent = message || "";
    node.dataset.type = type || "";
    node.hidden = !message;
  }

  function getParam(name) {
    return new URLSearchParams(window.location.search).get(name);
  }

  function isValidEmail(value) {
    var email = text(value).trim();
    var at = email.indexOf("@");
    var dot = email.lastIndexOf(".");
    return email.length > 0 && email.length <= 120 && at > 0 && dot > at + 1 && dot + 1 < email.length;
  }

  var VISITOR_EMAIL_KEY = "blog-visitor-email";

  function getVisitorEmail() {
    return localStorage.getItem(VISITOR_EMAIL_KEY) || "";
  }

  function setVisitorEmail(email) {
    localStorage.setItem(VISITOR_EMAIL_KEY, email);
  }

  function clearVisitorEmail() {
    localStorage.removeItem(VISITOR_EMAIL_KEY);
  }

  function maskEmail(email) {
    var parts = text(email).split("@");
    if (parts.length !== 2) return email;
    var name = parts[0];
    var visible = name.length <= 2 ? name.charAt(0) : name.slice(0, 2);
    return visible + "***@" + parts[1];
  }

  function initEmailGate(kind, form, statusNode) {
    var gate = document.querySelector('[data-email-register="' + kind + '"]');
    var summary = document.querySelector('[data-email-summary="' + kind + '"]');
    var current = document.querySelector('[data-email-current="' + kind + '"]');
    var changeButton = document.querySelector('[data-email-change="' + kind + '"]');

    function refresh() {
      var email = getVisitorEmail();
      var ready = isValidEmail(email);

      if (form) {
        form.hidden = !ready;
        if (form.email) form.email.value = ready ? email : "";
      }
      if (gate) gate.hidden = ready;
      if (summary) summary.hidden = !ready;
      if (current) current.textContent = ready ? maskEmail(email) : "";
    }

    if (gate && gate.dataset.emailGateBound !== "true") {
      gate.dataset.emailGateBound = "true";
      gate.addEventListener("submit", function (event) {
        event.preventDefault();
        var input = gate.querySelector("input[name='email']");
        var email = input ? input.value.trim() : "";

        if (!isValidEmail(email)) {
          setStatus(statusNode, "请先登记有效邮箱，邮箱不会公开展示。", "error");
          if (input) input.focus();
          return;
        }

        setVisitorEmail(email);
        setStatus(statusNode, "邮箱已登记，现在可以继续操作。", "success");
        refresh();
      });
    }

    if (changeButton && changeButton.dataset.emailChangeBound !== "true") {
      changeButton.dataset.emailChangeBound = "true";
      changeButton.addEventListener("click", function () {
        clearVisitorEmail();
        setStatus(statusNode, "已清除邮箱，请重新登记。", "warning");
        refresh();
        var input = gate ? gate.querySelector("input[name='email']") : null;
        if (input) input.focus();
      });
    }

    refresh();
  }

  function postUrl(post) {
    if (post.slug) return "example-post.html?slug=" + encodeURIComponent(post.slug);
    return "example-post.html?id=" + encodeURIComponent(post.id);
  }

  function renderTags(tags) {
    if (!Array.isArray(tags) || tags.length === 0) return "";
    return '<div class="tags">' + tags.map(function (tag) {
      return '<span class="tag">' + escapeHtml(tag) + "</span>";
    }).join("") + "</div>";
  }

  function renderPostListItem(post) {
    var date = formatDate(post.created_at);
    return [
      '<li class="post-item">',
      "<article>",
      '<p class="post-meta"><time datetime="' + escapeHtml(String(post.created_at || "").slice(0, 10)) + '">' + escapeHtml(date || "未标注日期") + "</time></p>",
      '<h3><a href="' + escapeHtml(postUrl(post)) + '">' + escapeHtml(post.title || "未命名文章") + "</a></h3>",
      '<p class="post-excerpt">' + escapeHtml(post.summary || "暂无摘要。") + "</p>",
      renderTags(post.tags),
      "</article>",
      "</li>"
    ].join("");
  }

  function initPostsPage(options) {
    options = options || {};
    var listNode = document.querySelector(options.listSelector || "[data-post-list]");
    var statusNode = document.querySelector(options.statusSelector || "[data-post-status]");
    var searchForm = document.querySelector(options.searchSelector || "[data-post-search]");
    if (!listNode || !window.BlogApi) return;

    var staticHtml = listNode.innerHTML;

    function loadPosts(keyword) {
      setStatus(statusNode, keyword ? "正在搜索文章..." : "正在加载文章...", "loading");
      var action = keyword
        ? window.BlogApi.posts.search(keyword, 20)
        : window.BlogApi.posts.list({ page: 1, limit: options.limit || 10 });

      action.then(function (payload) {
        var posts = getList(payload);
        if (posts.length === 0) {
          listNode.innerHTML = '<li class="post-item"><article><p class="post-excerpt">暂时没有文章。</p></article></li>';
          setStatus(statusNode, "", "");
          return;
        }
        listNode.innerHTML = posts.map(renderPostListItem).join("");
        setStatus(statusNode, "", "");
      }).catch(function (error) {
        listNode.innerHTML = staticHtml;
        setStatus(statusNode, "后端暂未连接，当前显示静态占位内容。", "warning");
        console.warn("[posts] load failed:", error);
      });
    }

    if (searchForm) {
      searchForm.addEventListener("submit", function (event) {
        event.preventDefault();
        var input = searchForm.querySelector("input[name='q']");
        loadPosts(input ? input.value.trim() : "");
      });
    }

    loadPosts("");
  }

  function initPostDetailPage(options) {
    options = options || {};
    if (!window.BlogApi) return;

    var id = getParam("id");
    var slug = getParam("slug");
    var titleNode = document.querySelector(options.titleSelector || "[data-post-title]");
    var metaNode = document.querySelector(options.metaSelector || "[data-post-meta]");
    var bodyNode = document.querySelector(options.bodySelector || "[data-post-body]");
    var statusNode = document.querySelector(options.statusSelector || "[data-post-status]");
    if ((!id && !slug) || !titleNode || !bodyNode) return;

    if (id) {
      initPostComments(id, options.comments || {});
    }

    setStatus(statusNode, "正在加载文章...", "loading");
    var action = slug ? window.BlogApi.posts.getBySlug(slug) : window.BlogApi.posts.getById(id);

    action.then(function (post) {
      titleNode.textContent = post.title || "未命名文章";
      document.title = (post.title || "文章") + " · My Blog";
      if (metaNode) {
        metaNode.innerHTML = '<time datetime="' + escapeHtml(String(post.created_at || "").slice(0, 10)) + '">' + escapeHtml(formatDate(post.created_at)) + "</time>"
          + (post.views !== undefined ? " · 阅读 " + escapeHtml(post.views) : "");
      }

      // content_html 需要由后端在导入或保存时完成 Markdown 渲染和安全处理。
      if (post.content_html) {
        bodyNode.innerHTML = post.content_html;
      } else if (post.content_md) {
        bodyNode.innerHTML = "<pre>" + escapeHtml(post.content_md) + "</pre>";
      } else {
        bodyNode.innerHTML = "<p>暂无正文内容。</p>";
      }
      setStatus(statusNode, "", "");
      if (!id) {
        initPostComments(post.id, options.comments || {});
      }
    }).catch(function (error) {
      setStatus(statusNode, "文章接口暂不可用，当前保留静态示例内容。", "warning");
      console.warn("[post-detail] load failed:", error);
    });
  }

  function renderGuestbookItem(item) {
    return [
      '<li class="guestbook-item">',
      '<div class="guestbook-author">' + escapeHtml(item.nickname || "匿名访客") + "</div>",
      '<p class="guestbook-content">' + escapeHtml(item.content || "") + "</p>",
      '<time class="guestbook-time" datetime="' + escapeHtml(item.created_at || "") + '">' + escapeHtml(formatDate(item.created_at)) + "</time>",
      "</li>"
    ].join("");
  }

  function renderCommentItem(item) {
    return [
      '<li class="comment-item">',
      '<div class="comment-head">',
      '<strong>' + escapeHtml(item.nickname || "匿名读者") + "</strong>",
      '<time datetime="' + escapeHtml(item.created_at || "") + '">' + escapeHtml(formatDate(item.created_at)) + "</time>",
      "</div>",
      '<p class="comment-content">' + escapeHtml(item.content || "") + "</p>",
      "</li>"
    ].join("");
  }

  function initPostComments(postId, options) {
    options = options || {};
    if (!postId || !window.BlogApi || !window.BlogApi.comments) return;

    var form = document.querySelector(options.formSelector || "[data-comment-form]");
    var listNode = document.querySelector(options.listSelector || "[data-comment-list]");
    var statusNode = document.querySelector(options.statusSelector || "[data-comment-status]");
    initEmailGate("comment", form, statusNode);

    function loadComments() {
      if (!listNode) return;
      setStatus(statusNode, "正在加载评论...", "loading");
      window.BlogApi.comments.list(postId, { page: 1, limit: 20 }).then(function (payload) {
        var items = getList(payload);
        listNode.innerHTML = items.length
          ? items.map(renderCommentItem).join("")
          : '<li class="comment-item"><p class="comment-content">还没有评论，来写第一条吧。</p></li>';
        setStatus(statusNode, "", "");
      }).catch(function (error) {
        setStatus(statusNode, "评论接口暂未连接，后端完成后这里会显示真实评论。", "warning");
        console.warn("[comments] load failed:", error);
      });
    }

    if (form && form.dataset.commentBound !== "true") {
      form.dataset.commentBound = "true";
      form.addEventListener("submit", function (event) {
        event.preventDefault();
        var payload = {
          nickname: form.nickname.value.trim(),
          email: getVisitorEmail(),
          content: form.content.value.trim()
        };

        if (!payload.nickname || !payload.content) {
          setStatus(statusNode, "昵称和评论内容不能为空。", "error");
          return;
        }
        if (!isValidEmail(payload.email)) {
          setStatus(statusNode, "请填写有效邮箱，邮箱不会公开展示。", "error");
          return;
        }

        setStatus(statusNode, "正在提交评论...", "loading");
        window.BlogApi.comments.create(postId, payload).then(function () {
          form.reset();
          setStatus(statusNode, "评论已提交。", "success");
          loadComments();
        }).catch(function (error) {
          setStatus(statusNode, error.message || "评论提交失败。", "error");
        });
      });
    }

    loadComments();
  }

  function initGuestbookPage(options) {
    options = options || {};
    if (!window.BlogApi) return;

    var form = document.querySelector(options.formSelector || "[data-guestbook-form]");
    var listNode = document.querySelector(options.listSelector || "[data-guestbook-list]");
    var statusNode = document.querySelector(options.statusSelector || "[data-guestbook-status]");
    initEmailGate("guestbook", form, statusNode);

    function loadMessages() {
      if (!listNode) return;
      setStatus(statusNode, "正在加载留言...", "loading");
      window.BlogApi.guestbook.list({ page: 1, limit: 20 }).then(function (payload) {
        var items = getList(payload);
        listNode.innerHTML = items.length
          ? items.map(renderGuestbookItem).join("")
          : '<li class="guestbook-item"><p class="guestbook-content">还没有留言，写下第一条吧。</p></li>';
        setStatus(statusNode, "", "");
      }).catch(function (error) {
        setStatus(statusNode, "留言接口暂未连接，表单会在后端完成后生效。", "warning");
        console.warn("[guestbook] load failed:", error);
      });
    }

    if (form) {
      form.addEventListener("submit", function (event) {
        event.preventDefault();
        var payload = {
          nickname: form.nickname.value.trim(),
          email: getVisitorEmail(),
          content: form.content.value.trim()
        };

        if (!payload.nickname || !payload.content) {
          setStatus(statusNode, "昵称和留言内容不能为空。", "error");
          return;
        }
        if (!isValidEmail(payload.email)) {
          setStatus(statusNode, "请填写有效邮箱，邮箱不会公开展示。", "error");
          return;
        }

        setStatus(statusNode, "正在提交留言...", "loading");
        window.BlogApi.guestbook.create(payload).then(function () {
          form.reset();
          setStatus(statusNode, "留言已提交，等待显示。", "success");
          loadMessages();
        }).catch(function (error) {
          setStatus(statusNode, error.message || "留言提交失败。", "error");
        });
      });
    }

    loadMessages();
  }

  function initHiddenAdminEntrance(options) {
    options = options || {};
    if (!window.BlogApi) return;

    var trigger = document.querySelector(options.triggerSelector || "[data-admin-trigger]");
    var link = document.querySelector(options.linkSelector || "[data-admin-link]");
    var statusNode = document.querySelector(options.statusSelector || "[data-post-status]");
    if (!trigger || !link) return;

    function revealAdminLink(message) {
      link.hidden = false;
      link.setAttribute("aria-hidden", "false");
      setStatus(statusNode, message || "管理员入口已显示。", "success");
    }

    trigger.addEventListener("click", function () {
      // 前端只负责隐藏入口和发起登录；真正的权限校验必须在后端接口中完成。
      var username = window.prompt("请输入管理员用户名");
      if (!username) return;

      var password = window.prompt("请输入管理员密码");
      if (!password) return;

      setStatus(statusNode, "正在验证管理员身份...", "loading");
      window.BlogApi.admin.login({
        username: username.trim(),
        password: password
      }).then(function () {
        revealAdminLink("验证通过，管理入口已显示。");
      }).catch(function (error) {
        link.hidden = true;
        link.setAttribute("aria-hidden", "true");
        setStatus(statusNode, error.message || "身份验证失败。", "error");
      });
    });
  }

  window.BlogPages = {
    initPostsPage: initPostsPage,
    initPostDetailPage: initPostDetailPage,
    initPostComments: initPostComments,
    initGuestbookPage: initGuestbookPage,
    initHiddenAdminEntrance: initHiddenAdminEntrance,
    formatDate: formatDate,
    escapeHtml: escapeHtml
  };
})(window, document);
