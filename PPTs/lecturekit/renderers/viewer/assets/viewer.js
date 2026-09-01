(function () {
  "use strict";

  var dataEl = document.getElementById("lecture-data");
  var data = JSON.parse(dataEl.textContent);
  var app = document.getElementById("app");

  // lecturekit's own chrome, resolved by the renderer from the deck's language
  // (see i18n.UI_STRINGS). A bundle rendered before this existed has no table,
  // so each string carries the Chinese it used to be hardcoded as.
  function ui(name) {
    return (data.ui && data.ui[name]) || { outline: "大纲" }[name] || name;
  }

  // Two numberings, deliberately distinct: slideIndexById addresses a slide
  // (1-based deck position, what navigation and the deck anchors use), while
  // shownNumberById is the number the deck *prints* — an animation's frames all
  // show their group's number. They diverge once a deck holds an animation.
  var pageOrder = data.pages.map(function (p) { return p.id; });
  var pageById = {};
  var slideIndexById = {};
  var shownNumberById = {};
  data.pages.forEach(function (p, i) {
    pageById[p.id] = p;
    slideIndexById[p.id] = i + 1;
    shownNumberById[p.id] = p.number || i + 1;
  });

  // An outline row can stand for several slides: a page node with "frames": n
  // is an animation, and the n slides from it onward all belong to that row.
  // rowIdByPageId maps any slide back to the row that owns it, so paging inside
  // an animation still highlights the right entry.
  var rowIdByPageId = {};
  (function collectRows(nodes) {
    nodes.forEach(function (node) {
      if (node.type === "section") { collectRows(node.children || []); return; }
      var start = slideIndexById[node.id];
      for (var i = 0; i < (node.frames || 1); i++) {
        rowIdByPageId[pageOrder[start - 1 + i]] = node.id;
      }
    });
  })(data.tree);

  var expanded = {};
  (function collectSections(nodes) {
    nodes.forEach(function (node) {
      if (node.type === "section") {
        expanded[node.id] = !node.collapsed;
        collectSections(node.children || []);
      }
    });
  })(data.tree);

  var state = {
    mode: "outline",
    currentPageId: null,
    expanded: expanded,
  };

  // A live-reload reloads the whole shell, which would otherwise drop us back
  // on the outline. Persist the reader's position per-tab and restore it, so
  // editing the source keeps them on the page (and exact slide) they're viewing.
  var STORAGE_KEY = "lecturekit:viewer:" + data.lecture.id;
  restoreState();

  function currentSlideNumber() {
    var frame = document.querySelector(".slide-frame");
    if (!frame) { return null; }
    try {
      var n = parseInt(frame.contentWindow.location.hash.replace(/^#/, ""), 10);
      return isNaN(n) ? null : n;
    } catch (e) {
      return null; // iframe not ready
    }
  }

  function persistState() {
    // Pages map 1:1 onto deck slides; page index i is deck slide i + 1. The live
    // iframe hash tells us which page the reader paged to inside the deck.
    if (state.mode === "slide") {
      var slide = currentSlideNumber();
      if (slide && pageOrder[slide - 1]) { state.currentPageId = pageOrder[slide - 1]; }
    }
    try {
      sessionStorage.setItem(STORAGE_KEY, JSON.stringify({
        mode: state.mode,
        currentPageId: state.currentPageId,
        expanded: state.expanded,
      }));
    } catch (e) { /* storage unavailable */ }
  }

  function restoreState() {
    var saved;
    try {
      saved = JSON.parse(sessionStorage.getItem(STORAGE_KEY) || "null");
    } catch (e) { saved = null; }
    if (!saved) { return; }
    if (saved.mode === "slide" && pageById[saved.currentPageId]) {
      state.mode = "slide";
      state.currentPageId = saved.currentPageId;
    }
    if (saved.expanded) {
      Object.keys(state.expanded).forEach(function (id) {
        if (id in saved.expanded) { state.expanded[id] = saved.expanded[id]; }
      });
    }
  }

  window.addEventListener("beforeunload", persistState);

  function showPage(pageId) {
    state.mode = "slide";
    state.currentPageId = pageId;
    render();
  }

  function showOutline() {
    state.mode = "outline";
    render();
  }

  function renderOutline() {
    var stage = document.createElement("div");
    stage.className = "outline-stage";

    var root = document.createElement("div");
    root.className = "outline";

    if (data.lecture.subtitle) {
      var sub = document.createElement("p");
      sub.className = "outline-subtitle";
      sub.textContent = data.lecture.subtitle;
      root.appendChild(sub);
    }

    root.appendChild(makeRootRow());
    var rows = [];
    walkTree(data.tree, [], rows);
    rows.forEach(function (r) { root.appendChild(r); });

    stage.appendChild(root);
    return stage;
  }

  // Build the org-mode style connector prefix (│, ├──, └──).
  function buildPrefix(ancestorsLast, isLast) {
    var s = "";
    ancestorsLast.forEach(function (last) { s += last ? "    " : "│   "; });
    return s + (isLast ? "└── " : "├── ");
  }

  function walkTree(nodes, ancestorsLast, rows) {
    nodes.forEach(function (node, i) {
      var isLast = i === nodes.length - 1;
      rows.push(makeRow(node, buildPrefix(ancestorsLast, isLast)));
      if (
        node.type === "section" &&
        state.expanded[node.id] &&
        node.children &&
        node.children.length
      ) {
        walkTree(node.children, ancestorsLast.concat(isLast), rows);
      }
    });
  }

  function makeLabel(markerText, markerClass, title) {
    var label = document.createElement("span");
    label.className = "label";

    var marker = document.createElement("span");
    marker.className = markerClass;
    marker.textContent = markerText;
    label.appendChild(marker);

    var titleEl = document.createElement("span");
    titleEl.className = "node-title";
    titleEl.appendChild(document.createTextNode(" "));
    appendTitleText(titleEl, title);
    label.appendChild(titleEl);
    return label;
  }

  function appendTitleText(parent, title) {
    var re = /\\emph\{([^{}]+)\}/g;
    var last = 0;
    var match;
    while ((match = re.exec(title)) !== null) {
      parent.appendChild(document.createTextNode(title.slice(last, match.index)));
      var span = document.createElement("span");
      span.className = "title-parenthetical";
      span.textContent = match[1];
      parent.appendChild(span);
      last = re.lastIndex;
    }
    parent.appendChild(document.createTextNode(title.slice(last)));
  }

  function makeRootRow() {
    var row = document.createElement("div");
    row.className = "row root-row";
    row.appendChild(makeLabel("▼", "marker", data.lecture.title));
    return row;
  }

  function makeRow(node, prefix) {
    var row = document.createElement("div");
    row.className = "row " + (node.type === "section" ? "section-row" : "page-row");

    var connector = document.createElement("span");
    connector.className = "connector";
    connector.textContent = prefix;
    row.appendChild(connector);

    var label;
    if (node.type === "section") {
      label = makeLabel(state.expanded[node.id] ? "▼" : "▶", "marker", node.title);
      row.addEventListener("click", function () {
        state.expanded[node.id] = !state.expanded[node.id];
        render();
      });
    } else {
      label = makeLabel("✦", "icon", node.title);
      if (rowIdByPageId[state.currentPageId] === node.id) {
        label.className += " selected";
      }
      row.addEventListener("click", function () { showPage(node.id); });
    }
    if (node.type === "page") {
      var text = String(shownNumberById[node.id]);
      var pageNumber = document.createElement("span");
      pageNumber.className = "page-number";
      pageNumber.textContent = text;
      pageNumber.setAttribute("aria-label", "Page " + text);
      row.appendChild(pageNumber);
    }
    row.appendChild(label);
    return row;
  }

  function renderSlide() {
    var idx = pageOrder.indexOf(state.currentPageId);

    var stage = document.createElement("div");
    stage.className = "slide-stage";

    var frame = document.createElement("iframe");
    frame.className = "slide-frame";
    // Pages map 1:1 onto deck slides; page index i is deck slide i + 1.
    frame.src = "slides.html#" + (idx + 1);
    stage.appendChild(frame);

    var back = document.createElement("button");
    back.type = "button";
    back.className = "slide-back";
    back.textContent = "≡ " + ui("outline");
    back.addEventListener("click", showOutline);
    stage.appendChild(back);

    return stage;
  }

  function render() {
    app.innerHTML = "";
    if (state.mode === "slide" && pageById[state.currentPageId]) {
      app.appendChild(renderSlide());
    } else {
      state.mode = "outline";
      app.appendChild(renderOutline());
    }
  }

  document.addEventListener("keydown", function (event) {
    // Marp owns paging inside the focused iframe; this only fires when the
    // shell itself has focus, as a best-effort return to the outline.
    if (
      state.mode === "slide" &&
      (event.key === "Escape" || event.key === "o" || event.key === "O")
    ) {
      showOutline();
    }
  });

  render();
})();
