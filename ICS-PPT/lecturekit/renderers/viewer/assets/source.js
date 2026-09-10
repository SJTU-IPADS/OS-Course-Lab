(function () {
  "use strict";

  // Arms the file buttons a `p.demo(..., files=[...])` block rendered, and
  // shows the file one of them names in a panel down the right-hand side.
  //
  // The button carries `data-lk-source="<id>"` and nothing else: the id is a
  // hash of the path, and the server maps it back to the file the author named
  // (see lecturekit/source.py). So this file can ask for a source file the deck
  // already lists, and cannot say which file that is.
  //
  // The panel is a reading surface and holds one file at a time — a second
  // press replaces what is in it, and pressing the button of the file already
  // showing puts it away. The content is fetched per press rather than kept,
  // because a lecture that edits a file and runs it again should see the edit.
  //
  // Injected only by the dev server, and only under `view --watch`; the
  // buttons ship `disabled` and a rendered bundle never loads this.
  var ENDPOINT = "/__source";
  var panel = null;
  var showing = null; // the id the panel is holding, or null when it is away
  var token = 0; // which fetch the body belongs to; a later press wins
  var observer = null;

  function build() {
    var el = document.createElement("aside");
    el.className = "lk-file";
    el.setAttribute("data-lk-chrome", "file");
    el.setAttribute("data-open", "0");
    // ▸ leads the row rather than ending it. The viewer shell floats its own
    // "≡ 大纲" control over the top right of the deck — outside this document,
    // so nothing here can be drawn above it — and a close button under that is
    // a close button that sends the reader back to the outline instead.
    el.innerHTML =
      '<div class="lk-file-head">' +
      '<button class="lk-file-close" type="button" ' +
      'aria-label="Hide this file">▸</button>' +
      '<span class="lk-file-path"></span>' +
      '<span class="lk-file-status"></span>' +
      "</div>" +
      '<div class="lk-file-body"></div>';
    el.querySelector(".lk-file-close").addEventListener("click", hide);
    document.body.appendChild(el);
    return {
      root: el,
      path: el.querySelector(".lk-file-path"),
      status: el.querySelector(".lk-file-status"),
      body: el.querySelector(".lk-file-body")
    };
  }

  // ---- the body -----------------------------------------------------------
  // One row per line, numbered: the number is what a lecture points at when it
  // says where to look. Text nodes throughout — the body is a file's bytes, so
  // it is text and only text, never markup to be parsed.

  function fill(text) {
    var body = panel.body;
    body.textContent = "";
    var lines = text.replace(/\n$/, "").split("\n");
    var frag = document.createDocumentFragment();
    for (var i = 0; i < lines.length; i++) {
      var row = document.createElement("div");
      row.className = "lk-file-row";
      var no = document.createElement("span");
      no.className = "lk-file-no";
      no.textContent = String(i + 1);
      var code = document.createElement("span");
      code.className = "lk-file-code";
      code.textContent = lines[i];
      row.appendChild(no);
      row.appendChild(code);
      frag.appendChild(row);
    }
    body.appendChild(frag);
    body.scrollTop = 0;
    return lines.length;
  }

  // ---- the panel ----------------------------------------------------------

  function open(button) {
    if (!panel) panel = build();
    var id = button.getAttribute("data-lk-source");
    if (showing === id) {
      hide();
      return;
    }
    showing = id;
    // The button's own label until the server says the path in full: something
    // has to name what is being read while it is being read.
    panel.path.textContent = button.textContent;
    panel.status.textContent = "reading…";
    panel.body.textContent = "";
    panel.root.setAttribute("data-open", "1");
    watch();

    var mine = ++token;
    fetch(ENDPOINT + "?id=" + encodeURIComponent(id))
      .then(function (response) {
        return response
          .json()
          .catch(function () {
            return { error: "HTTP " + response.status };
          });
      })
      .then(function (payload) {
        if (mine !== token) return; // a later press already took the panel
        if (payload.error) {
          panel.status.textContent = payload.error;
          return;
        }
        panel.path.textContent = payload.path;
        panel.status.textContent = fill(payload.text) + " lines";
      })
      .catch(function () {
        if (mine === token) panel.status.textContent = "unreachable";
      });
  }

  function hide() {
    if (!panel) return;
    panel.root.setAttribute("data-open", "0");
    showing = null;
    token++; // whatever is in flight is no longer wanted on screen
    if (observer) {
      observer.disconnect();
      observer = null;
    }
  }

  function isOpen() {
    return !!panel && panel.root.getAttribute("data-open") === "1";
  }

  // ---- which slide are we on ----------------------------------------------
  // The panel belongs to the slide whose button opened it. Paging away with it
  // still up would leave the next slide half covered by the previous slide's
  // source, so the page turn takes it down. Marp's controller does not touch
  // the URL when it pages; the deck's own marker is the signal.

  function watch() {
    if (observer || !window.MutationObserver) return;
    var slides = document.querySelectorAll(".bespoke-marp-slide");
    if (!slides.length) return;
    var here = current();
    observer = new MutationObserver(function () {
      if (current() === here) return;
      hide();
    });
    for (var i = 0; i < slides.length; i++) {
      observer.observe(slides[i], { attributes: true, attributeFilter: ["class"] });
    }
  }

  function current() {
    var slides = document.querySelectorAll(".bespoke-marp-slide");
    for (var i = 0; i < slides.length; i++) {
      if (slides[i].classList.contains("bespoke-marp-active")) return i;
    }
    return -1;
  }

  // ---- arming -------------------------------------------------------------

  function arm() {
    var buttons = document.querySelectorAll(".lk-demo-file[data-lk-source]");
    for (var i = 0; i < buttons.length; i++) {
      (function (button) {
        if (button.getAttribute("data-lk-armed")) return;
        button.setAttribute("data-lk-armed", "1");
        button.disabled = false;
        button.addEventListener("click", function (event) {
          // Marp gives every slide a click handler of its own, which resets the
          // page's revealed items. A press on a button is not a press on the
          // slide behind it.
          event.preventDefault();
          event.stopPropagation();
          open(button);
        });
      })(buttons[i]);
    }
  }

  // ---- putting it away ----------------------------------------------------
  // A click anywhere but on the deck's own chrome means "back to the slide",
  // and Escape means the same. Both are taken in the capture phase and only
  // while the panel is up, so the deck keeps them the rest of the time.
  //
  // `stopImmediatePropagation`, not `stopPropagation`: the demo drawer's
  // controller listens on this same node, and one gesture should put away one
  // panel — the one on top, which is this one. It is loaded first for exactly
  // that reason (see dev_server.inject_source).

  function elsewhere(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !node || !node.closest("[data-lk-chrome], .lk-demo");
  }

  document.addEventListener(
    "click",
    function (event) {
      if (!isOpen() || !elsewhere(event.target)) return;
      event.preventDefault();
      event.stopImmediatePropagation();
      hide();
    },
    true
  );

  document.addEventListener(
    "keydown",
    function (event) {
      if (event.key !== "Escape" || !isOpen()) return;
      event.preventDefault();
      event.stopImmediatePropagation();
      hide();
    },
    true
  );

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", arm);
  } else {
    arm();
  }
})();
