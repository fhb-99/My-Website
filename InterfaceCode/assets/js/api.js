(function (window) {
  "use strict";

  var TOKEN_KEY = "blog-admin-token";
  var API_BASE_KEY = "blog-api-base";

  function trimTrailingSlash(value) {
    return String(value || "").replace(/\/+$/, "");
  }

  function getDefaultBaseUrl() {
    var saved = localStorage.getItem(API_BASE_KEY);
    if (saved) return trimTrailingSlash(saved);

    // 直接双击打开 HTML 时没有同源后端，默认连本机开发服务。
    if (window.location.protocol === "file:") {
      return "http://127.0.0.1:8080";
    }

    return "";
  }

  var baseUrl = getDefaultBaseUrl();

  function buildUrl(path, query) {
    var url = trimTrailingSlash(baseUrl) + path;
    var params = new URLSearchParams();

    Object.keys(query || {}).forEach(function (key) {
      var value = query[key];
      if (value !== undefined && value !== null && value !== "") {
        params.set(key, value);
      }
    });

    var qs = params.toString();
    return qs ? url + "?" + qs : url;
  }

  function getToken() {
    return localStorage.getItem(TOKEN_KEY) || "";
  }

  function setToken(token) {
    if (token) {
      localStorage.setItem(TOKEN_KEY, token);
    } else {
      localStorage.removeItem(TOKEN_KEY);
    }
  }

  function toJsonSafe(response) {
    return response.text().then(function (text) {
      if (!text) return null;
      try {
        return JSON.parse(text);
      } catch (error) {
        return { message: text };
      }
    });
  }

  function request(path, options) {
    options = options || {};
    var headers = options.headers ? Object.assign({}, options.headers) : {};
    var method = options.method || "GET";
    var body = options.body;

    if (options.auth !== false && getToken()) {
      headers.Authorization = "Bearer " + getToken();
    }

    if (options.json !== undefined) {
      headers["Content-Type"] = "application/json; charset=utf-8";
      body = JSON.stringify(options.json);
    }

    return fetch(buildUrl(path, options.query), {
      method: method,
      headers: headers,
      body: body
    }).then(function (response) {
      return toJsonSafe(response).then(function (data) {
        if (!response.ok) {
          var message = data && (data.error || data.message) ? data.error || data.message : "请求失败";
          var error = new Error(message);
          error.status = response.status;
          error.data = data;
          throw error;
        }
        return data;
      });
    });
  }

  function setBaseUrl(nextBaseUrl) {
    baseUrl = trimTrailingSlash(nextBaseUrl);
    localStorage.setItem(API_BASE_KEY, baseUrl);
  }

  // 所有后端接口都集中在这里，后续后端路径变化时只改这一处。
  window.BlogApi = {
    getBaseUrl: function () {
      return baseUrl;
    },
    setBaseUrl: setBaseUrl,
    getToken: getToken,
    setToken: setToken,
    clearToken: function () {
      setToken("");
    },
    request: request,

    posts: {
      list: function (params) {
        return request("/api/posts", { query: params || {}, auth: false });
      },
      getById: function (id) {
        return request("/api/posts/" + encodeURIComponent(id), { auth: false });
      },
      getBySlug: function (slug) {
        return request("/api/posts/slug/" + encodeURIComponent(slug), { auth: false });
      },
      search: function (keyword, limit) {
        return request("/api/search", { query: { q: keyword, limit: limit || 10 }, auth: false });
      }
    },

    guestbook: {
      list: function (params) {
        return request("/api/guestbook", { query: params || {}, auth: false });
      },
      create: function (payload) {
        return request("/api/guestbook", { method: "POST", json: payload, auth: false });
      }
    },

    comments: {
      list: function (postId, params) {
        return request("/api/posts/" + encodeURIComponent(postId) + "/comments", { query: params || {}, auth: false });
      },
      create: function (postId, payload) {
        return request("/api/posts/" + encodeURIComponent(postId) + "/comments", { method: "POST", json: payload, auth: false });
      }
    },

    admin: {
      login: function (payload) {
        return request("/api/auth/login", { method: "POST", json: payload, auth: false }).then(function (data) {
          if (data && data.token) setToken(data.token);
          return data;
        });
      },
      listPosts: function (params) {
        return request("/api/admin/posts", { query: params || {} });
      },
      getPost: function (id) {
        return request("/api/admin/posts/" + encodeURIComponent(id));
      },
      createPost: function (payload) {
        return request("/api/admin/posts", { method: "POST", json: payload });
      },
      updatePost: function (id, payload) {
        return request("/api/admin/posts/" + encodeURIComponent(id), { method: "PUT", json: payload });
      },
      deletePost: function (id) {
        return request("/api/admin/posts/" + encodeURIComponent(id), { method: "DELETE" });
      },
      upload: function (file) {
        var form = new FormData();
        form.append("file", file);
        return request("/api/admin/uploads", { method: "POST", body: form });
      },
      listGuestbook: function (params) {
        return request("/api/admin/guestbook", { query: params || {} });
      },
      approveGuestbook: function (id) {
        return request("/api/admin/guestbook/" + encodeURIComponent(id) + "/approve", { method: "PUT" });
      },
      deleteGuestbook: function (id) {
        return request("/api/admin/guestbook/" + encodeURIComponent(id), { method: "DELETE" });
      },
      saveConfig: function (key, value) {
        return request("/api/admin/config/" + encodeURIComponent(key), { method: "PUT", json: { value: value } });
      }
    },

    notes: {
      list: function (params) {
        return request("/api/notes", { query: params || {}, auth: false });
      }
    },

    projects: {
      list: function (params) {
        return request("/api/projects", { query: params || {}, auth: false });
      }
    },

    config: {
      get: function () {
        return request("/api/config", { auth: false });
      }
    }
  };
})(window);
