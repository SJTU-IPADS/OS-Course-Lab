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
  // Two things are the presenter's to set: the size of the text (A− and A+ in
  // the head) and the width of the panel (its left edge drags; a double click
  // on the edge gives the width back to the stylesheet). Both are kept in
  // localStorage, because the dev server reloads this page on every rebuild.
  //
  // Injected only by the dev server, and only under `view --watch`; the
  // buttons ship `disabled` and a rendered bundle never loads this.
  var ENDPOINT = "/__source";
  var FONT_SIZES = { least: 10, most: 40, step: 2, usual: 14 }; // px
  var FONT_KEY = "lk-file-font";
  var WIDTH_KEY = "lk-file-width";
  var LEAST_WIDTH = 280; // px: a gutter and a few words of a line
  var SLIDE_LEFT = 80; // px of slide a dragged panel always leaves beside itself
  var fontSize = clamp(recall(FONT_KEY) || FONT_SIZES.usual, FONT_SIZES.least, FONT_SIZES.most);
  var pinned = recall(WIDTH_KEY); // the width the edge was dragged to, or null
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
    // A− and A+ follow it, for the same reason: on the left, clear of that
    // corner.
    el.innerHTML =
      '<div class="lk-file-grip" title="Drag to resize; double-click to reset"></div>' +
      '<div class="lk-file-head">' +
      '<button class="lk-file-close" type="button" ' +
      'aria-label="Hide this file">▸</button>' +
      '<span class="lk-file-zooms">' +
      '<button class="lk-file-zoom" type="button" data-zoom="-1" ' +
      'aria-label="Smaller text" title="Smaller text">A−</button>' +
      '<button class="lk-file-zoom" type="button" data-zoom="1" ' +
      'aria-label="Larger text" title="Larger text">A+</button>' +
      "</span>" +
      '<span class="lk-file-path"></span>' +
      '<span class="lk-file-status"></span>' +
      "</div>" +
      '<div class="lk-file-body"></div>';
    el.querySelector(".lk-file-close").addEventListener("click", hide);
    var zooms = el.querySelectorAll(".lk-file-zoom");
    for (var i = 0; i < zooms.length; i++) {
      zooms[i].addEventListener("click", function (event) {
        zoom(parseInt(event.currentTarget.getAttribute("data-zoom"), 10));
        // Marp ignores every key aimed at a button (see demo.js).
        event.currentTarget.blur();
      });
    }
    grip(el.querySelector(".lk-file-grip"));
    document.body.appendChild(el);
    return {
      root: el,
      zooms: zooms,
      path: el.querySelector(".lk-file-path"),
      status: el.querySelector(".lk-file-status"),
      body: el.querySelector(".lk-file-body")
    };
  }

  // ---- the presenter's settings -------------------------------------------

  function clamp(value, least, most) {
    return Math.min(Math.max(value, least), most);
  }

  // A number the presenter set on an earlier load, or null. Storage can be
  // off (a private window); the panel then simply forgets between reloads.
  function recall(key) {
    try {
      var value = parseFloat(window.localStorage.getItem(key));
      return isFinite(value) ? value : null;
    } catch (err) {
      return null;
    }
  }

  function keep(key, value) {
    try {
      if (value === null) window.localStorage.removeItem(key);
      else window.localStorage.setItem(key, String(value));
    } catch (err) {
      /* nothing to keep it in */
    }
  }

  // The file's text, and only that: the head stays the size it is, so the
  // buttons are where they were after a press.
  function zoom(direction) {
    fontSize = clamp(
      fontSize + direction * FONT_SIZES.step, FONT_SIZES.least, FONT_SIZES.most
    );
    keep(FONT_KEY, fontSize);
    dress();
  }

  // The panel as the presenter left it: the text at their size, the width
  // their drag gave it — kept inside the window — or the stylesheet's own.
  function dress() {
    if (!panel) return;
    panel.body.style.fontSize = fontSize + "px";
    panel.zooms[0].disabled = fontSize <= FONT_SIZES.least;
    panel.zooms[1].disabled = fontSize >= FONT_SIZES.most;
    panel.root.style.width = pinned === null ? "" : span() + "px";
  }

  function span() {
    return clamp(pinned, LEAST_WIDTH, Math.max(window.innerWidth - SLIDE_LEFT, LEAST_WIDTH));
  }

  window.addEventListener("resize", dress);

  // The left edge: drag it and the panel is as wide as the pointer says, double
  // click it and the width is the stylesheet's again. The pointer is captured,
  // so the drag goes on over the slide.
  function grip(el) {
    var dragging = false;
    el.addEventListener("pointerdown", function (event) {
      if (event.button !== 0) return;
      dragging = true;
      el.setPointerCapture(event.pointerId);
      el.setAttribute("data-dragging", "1");
      event.preventDefault(); // no text selection trailing the pointer
    });
    el.addEventListener("pointermove", function (event) {
      if (!dragging) return;
      pinned = window.innerWidth - event.clientX;
      dress();
    });
    function done() {
      if (!dragging) return;
      dragging = false;
      el.removeAttribute("data-dragging");
      if (pinned !== null) {
        pinned = span(); // what it was held to, not where the pointer went
        keep(WIDTH_KEY, pinned);
      }
      swallowClick();
    }
    el.addEventListener("pointerup", done);
    el.addEventListener("pointercancel", done);
    el.addEventListener("dblclick", function () {
      pinned = null;
      keep(WIDTH_KEY, null);
      dress();
    });
  }

  // A drag is not a click, but a browser may report one when the button comes
  // up — on the slide, if the pointer ended there, where it would put this
  // panel away and turn Marp's page. The one click that follows a drag is taken
  // before anything else hears it; a drag that produced none leaves nothing
  // behind.
  function swallowClick() {
    function swallow(event) {
      event.preventDefault();
      event.stopImmediatePropagation();
    }
    window.addEventListener("click", swallow, true);
    setTimeout(function () {
      window.removeEventListener("click", swallow, true);
    }, 0);
  }

  // ---- the body -----------------------------------------------------------
  // One row per line, numbered: the number is what a lecture points at when it
  // says where to look. Text nodes throughout — the body is a file's bytes, so
  // it is text and only text, never markup to be parsed.
  //
  // The colours come from the server: `tokens` is the same text cut into lines
  // of [class, text] runs (see source.highlight), and a run becomes a span
  // around a text node. There is no lexer here. When the server sent none, or
  // sent lines that do not match the text's own, the text is shown plain.

  function paint(code, runs) {
    for (var i = 0; i < runs.length; i++) {
      var kind = runs[i][0];
      var piece = document.createTextNode(String(runs[i][1]));
      if (typeof kind === "string" && /^[a-z]+$/.test(kind)) {
        var span = document.createElement("span");
        span.className = "lk-tok-" + kind;
        span.appendChild(piece);
        code.appendChild(span);
      } else {
        code.appendChild(piece);
      }
    }
  }

  function fill(text, tokens) {
    var body = panel.body;
    body.textContent = "";
    var lines = text.replace(/\n$/, "").split("\n");
    var coloured = Array.isArray(tokens) && tokens.length === lines.length;
    var frag = document.createDocumentFragment();
    for (var i = 0; i < lines.length; i++) {
      var row = document.createElement("div");
      row.className = "lk-file-row";
      var no = document.createElement("span");
      no.className = "lk-file-no";
      no.textContent = String(i + 1);
      var code = document.createElement("span");
      code.className = "lk-file-code";
      if (coloured && Array.isArray(tokens[i])) paint(code, tokens[i]);
      else code.textContent = lines[i];
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
    if (!panel) {
      panel = build();
      dress();
    }
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
        panel.status.textContent = fill(payload.text, payload.tokens) + " lines";
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
          // And it keeps no focus: Marp ignores every key aimed at a button,
          // so the page would stop turning (see demo.js).
          button.blur();
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

  // Escape typed into a field — an interactive demo's terminal — is that
  // program's own key, so it is left alone. A read-only one takes no typing.
  function editable(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !!node && (node.isContentEditable ||
      (/^(INPUT|TEXTAREA|SELECT)$/.test(node.tagName) && !node.readOnly));
  }

  document.addEventListener(
    "keydown",
    function (event) {
      if (event.key !== "Escape" || !isOpen() || editable(event.target)) return;
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
