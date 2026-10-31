(function () {
  "use strict";

  // Arms the file buttons a `p.demo(..., files=[...])` block rendered and the
  // ones a `[label](file:path)` link became in a line of text, and shows the
  // file one of them names in a panel down the right-hand side.
  //
  // The button carries `data-lk-source="<id>"` and nothing else: the id is a
  // hash of the path, and the server maps it back to the file the author named
  // (see lecturekit/source.py). So this file can ask for a source file the deck
  // already lists, and cannot say which file that is.
  //
  // The panel holds one file at a time — a second press replaces what is in
  // it, and pressing the button of the file already showing puts it away. The
  // content is fetched per press rather than kept, because a lecture that edits
  // a file and runs it again should see the edit.
  //
  // It is also where a file is written in front of the room. What is typed
  // stays in the browser, in a draft per file in localStorage (`lk-file-draft:`
  // and the id): putting the panel away, turning the page, and the reload the
  // dev server does on every rebuild all leave it as it was. Only Save presses
  // it onto the disk, and Reset reads the disk again and drops the draft. A
  // file the panel could not hand back byte for byte is shown read-only.
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
  var edit = null; // the editor holding `showing`, or null when it is read-only
  var DRAFT_KEY = "lk-file-draft:";
  var TOKEN_DELAY_MS = 250; // quiet time after a keystroke before it is re-coloured
  var INDENT = "    ";

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
      '<span class="lk-file-acts" hidden>' +
      '<button class="lk-file-act" type="button" data-act="save" ' +
      'title="Write to the file (Ctrl+S)">Save</button>' +
      '<button class="lk-file-act" type="button" data-act="reset" ' +
      'title="Read the file again and drop the unsaved text">Reset</button>' +
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
    var acts = el.querySelectorAll(".lk-file-act");
    for (var j = 0; j < acts.length; j++) {
      acts[j].addEventListener("click", function (event) {
        var button = event.currentTarget;
        button.blur();
        if (button.getAttribute("data-act") === "save") save();
        else reset();
      });
    }
    grip(el.querySelector(".lk-file-grip"));
    document.body.appendChild(el);
    return {
      root: el,
      zooms: zooms,
      acts: el.querySelector(".lk-file-acts"),
      save: el.querySelector('[data-act="save"]'),
      reset: el.querySelector('[data-act="reset"]'),
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
    panel.acts.hidden = true;
    edit = null;
    panel.root.setAttribute("data-open", "1");
    watch();

    var mine = ++token;
    read(id)
      .then(function (payload) {
        if (mine !== token) return; // a later press already took the panel
        if (payload.error) {
          panel.status.textContent = payload.error;
          return;
        }
        panel.path.textContent = payload.path;
        if (payload.editable === true) {
          begin(id, payload);
        } else {
          panel.status.textContent = fill(payload.text, payload.tokens) + " lines · read-only";
        }
      })
      .catch(function () {
        if (mine === token) panel.status.textContent = "unreachable";
      });
  }

  function json(response) {
    return response.json().catch(function () {
      return { error: "HTTP " + response.status };
    });
  }

  function read(id) {
    return fetch(ENDPOINT + "?id=" + encodeURIComponent(id)).then(json);
  }

  // A POST carries a file's text, so the server asks for JSON and the page's
  // own origin (see dev_server._demo_request); `fetch` supplies both.
  function post(route, body) {
    return fetch(ENDPOINT + route, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body)
    }).then(function (response) {
      return json(response).then(function (payload) {
        return { ok: response.ok, status: response.status, body: payload };
      });
    });
  }

  function hide() {
    if (!panel) return;
    panel.root.setAttribute("data-open", "0");
    showing = null;
    if (edit) {
      clearTimeout(edit.timer);
      edit = null; // the draft is already in localStorage; opening again reads it back
    }
    token++; // whatever is in flight is no longer wanted on screen
    if (observer) {
      observer.disconnect();
      observer = null;
    }
  }

  function isOpen() {
    return !!panel && panel.root.getAttribute("data-open") === "1";
  }

  // ---- the editor ---------------------------------------------------------
  // A textarea laid over a copy of its own text that carries the colours: the
  // textarea keeps the caret, selection, undo and input methods, and its text is
  // transparent so the copy shows through. Both are sized by the copy, so the
  // panel's body scrolls them together and nothing has to be kept in step.
  //
  // `base` is the file as the server last handed it over (line endings as they
  // are on disk) and travels with every save, so a file changed by something
  // else in between is noticed. `disk` is the same text as the textarea holds
  // it. The draft is kept exactly while the text differs from `disk`.

  function plain(text) {
    return text.replace(/\r\n/g, "\n");
  }

  function recallDraft(id) {
    try {
      var draft = JSON.parse(window.localStorage.getItem(DRAFT_KEY + id));
      return draft && typeof draft.base === "string" && typeof draft.text === "string"
        ? draft : null;
    } catch (err) {
      return null;
    }
  }

  function keepDraft(id, draft) {
    try {
      if (draft) window.localStorage.setItem(DRAFT_KEY + id, JSON.stringify(draft));
      else window.localStorage.removeItem(DRAFT_KEY + id);
    } catch (err) {
      /* nothing to keep it in: the text lives as long as this page does */
    }
  }

  function begin(id, payload) {
    var draft = recallDraft(id);
    var disk = plain(payload.text);
    if (draft && plain(draft.text) === disk) draft = null;
    var stale = !!draft && draft.base !== payload.text;
    var body = panel.body;
    body.textContent = "";
    var root = document.createElement("div");
    root.className = "lk-file-edit";
    var gutter = document.createElement("div");
    gutter.className = "lk-file-gutter";
    gutter.setAttribute("aria-hidden", "true");
    var stack = document.createElement("div");
    stack.className = "lk-file-stack";
    var under = document.createElement("pre");
    under.className = "lk-file-under";
    under.setAttribute("aria-hidden", "true");
    var input = document.createElement("textarea");
    input.className = "lk-file-input";
    input.setAttribute("wrap", "off");
    input.setAttribute("spellcheck", "false");
    input.setAttribute("autocomplete", "off");
    input.setAttribute("autocapitalize", "off");
    input.setAttribute("aria-label", payload.path);
    input.value = draft ? plain(draft.text) : disk;
    stack.appendChild(under);
    stack.appendChild(input);
    root.appendChild(gutter);
    root.appendChild(stack);
    body.appendChild(root);

    var mine = (edit = {
      id: id,
      path: payload.path,
      base: draft ? draft.base : payload.text,
      disk: draft && stale ? plain(draft.base) : disk,
      input: input,
      under: under,
      gutter: gutter,
      lines: null, // the text the colours below were made for, split into lines
      runs: null,
      serial: 0,
      timer: 0,
      busy: false,
      force: false,
      note: stale ? "file changed on disk since this draft began" : ""
    });
    input.addEventListener("input", function () {
      if (edit !== mine) return;
      changed(true);
    });
    input.addEventListener("keydown", function (event) {
      if (edit === mine) typed(event);
    });
    // The deck listens for keys on the document; what is typed here is the
    // file's.
    input.addEventListener("keyup", function (event) { event.stopPropagation(); });
    input.addEventListener("keypress", function (event) { event.stopPropagation(); });
    panel.acts.hidden = false;
    body.scrollTop = 0;
    mine.lines = tokenLines(input.value);
    mine.runs = Array.isArray(payload.tokens) &&
      payload.tokens.length === mine.lines.length && !draft ? payload.tokens : null;
    changed(!!draft && !mine.runs);
  }

  // The lines the lexer is given are the text without its last newline, as in
  // `fill`; the empty line after that newline is the cursor's, and has no colour.
  function tokenLines(text) {
    return text.replace(/\n$/, "").split("\n");
  }

  function dirty() {
    return edit.input.value !== edit.disk;
  }

  function changed(recolour) {
    persist();
    draw();
    if (recolour) {
      clearTimeout(edit.timer);
      var mine = edit;
      mine.timer = setTimeout(function () { colour(mine); }, TOKEN_DELAY_MS);
    }
    report();
  }

  function persist() {
    keepDraft(edit.id, dirty() ? { base: edit.base, text: edit.input.value } : null);
  }

  function report() {
    var count = edit.input.value.split("\n").length;
    panel.status.textContent = edit.note ||
      count + " lines" + (dirty() ? " · modified" : "");
    panel.save.disabled = edit.busy || !dirty();
  }

  // The copy under the textarea: the colours of the last answer for every line
  // that still reads as it did then — counted from the top and from the bottom,
  // which is where typing happens in between — and plain text for the rest,
  // until the next answer comes.
  function draw() {
    var text = edit.input.value;
    var lines = tokenLines(text);
    var known = edit.lines;
    var runs = edit.runs;
    var head = 0;
    var tail = 0;
    if (runs) {
      var most = Math.min(lines.length, known.length);
      while (head < most && lines[head] === known[head]) head++;
      while (tail < most - head &&
        lines[lines.length - 1 - tail] === known[known.length - 1 - tail]) tail++;
    }
    var frag = document.createDocumentFragment();
    for (var i = 0; i < lines.length; i++) {
      var code = document.createElement("span");
      if (runs && i < head) paint(code, runs[i]);
      else if (runs && i >= lines.length - tail) paint(code, runs[known.length - (lines.length - i)]);
      else code.textContent = lines[i];
      frag.appendChild(code);
      frag.appendChild(document.createTextNode("\n"));
    }
    // The textarea has a line for the cursor after a final newline; so does the copy.
    frag.appendChild(document.createTextNode(/\n$/.test(text) ? "\n" : ""));
    edit.under.textContent = "";
    edit.under.appendChild(frag);
    var count = text.split("\n").length;
    var numbers = [];
    for (var n = 1; n <= count; n++) numbers.push(n);
    edit.gutter.textContent = numbers.join("\n");
  }

  function colour(mine) {
    if (edit !== mine) return;
    var sent = mine.input.value;
    var which = ++mine.serial;
    post("/tokens", { id: mine.id, text: sent })
      .then(function (answer) {
        if (edit !== mine || which !== mine.serial || mine.input.value !== sent) return;
        var lines = tokenLines(sent);
        var runs = answer.body && answer.body.tokens;
        if (!answer.ok || !Array.isArray(runs) || runs.length !== lines.length) return;
        mine.lines = lines;
        mine.runs = runs;
        draw();
      })
      .catch(function () { /* the plain text stays; the next keystroke asks again */ });
  }

  function save() {
    if (!edit || edit.busy || !dirty()) return;
    var mine = edit;
    var sent = mine.input.value;
    mine.busy = true;
    report();
    post("/save", { id: mine.id, text: sent, base: mine.base, force: mine.force })
      .then(function (answer) {
        mine.busy = false;
        if (edit !== mine) return;
        if (answer.ok) {
          mine.base = answer.body.text;
          mine.disk = plain(answer.body.text);
          mine.force = false;
          mine.note = "";
          persist();
        } else if (answer.status === 409 && answer.body.error === "file changed on disk") {
          mine.force = true;
          mine.note = "file changed on disk · Save again to overwrite it";
        } else {
          mine.note = answer.body.error || "HTTP " + answer.status;
        }
        report();
      })
      .catch(function () {
        mine.busy = false;
        if (edit !== mine) return;
        mine.note = "unreachable";
        report();
      });
  }

  // What is on the disk now, and the unsaved text gone. No question asked: the
  // button says what it does.
  function reset() {
    if (!edit) return;
    var mine = edit;
    clearTimeout(mine.timer);
    read(mine.id)
      .then(function (payload) {
        if (edit !== mine) return;
        if (payload.error || payload.editable !== true) {
          mine.note = payload.error || "file is not editable";
          report();
          return;
        }
        mine.base = payload.text;
        mine.disk = plain(payload.text);
        mine.input.value = mine.disk;
        mine.force = false;
        mine.note = "";
        mine.lines = tokenLines(mine.disk);
        mine.runs = Array.isArray(payload.tokens) &&
          payload.tokens.length === mine.lines.length ? payload.tokens : null;
        changed(false);
      })
      .catch(function () {
        if (edit !== mine) return;
        mine.note = "unreachable";
        report();
      });
  }

  function insert(text) {
    var input = edit.input;
    if (document.execCommand("insertText", false, text)) return; // keeps undo
    input.setRangeText(text, input.selectionStart, input.selectionEnd, "end");
    input.dispatchEvent(new Event("input"));
  }

  // A Makefile recipe line is a tab, whatever the other files indent with.
  function indent() {
    return /(^|\/)(Makefile|GNUmakefile)$|\.mk$/.test(edit.path) ? "\t" : INDENT;
  }

  function typed(event) {
    event.stopPropagation();
    if (event.isComposing) return;
    var input = edit.input;
    var mod = event.ctrlKey || event.metaKey;
    if (mod && !event.shiftKey && !event.altKey && (event.key || "").toLowerCase() === "s") {
      event.preventDefault();
      save();
    } else if (event.key === "Escape") {
      // The first Escape leaves the text box, which gives the keys back to the
      // deck; the next puts the panel away.
      event.preventDefault();
      input.blur();
    } else if (event.key === "Tab" && !mod && !event.altKey) {
      event.preventDefault();
      if (!event.shiftKey) insert(indent());
    } else if (event.key === "Enter" && !mod && !event.shiftKey && !event.altKey) {
      // The new line starts where the one above did, one level deeper after an
      // opening brace or colon.
      event.preventDefault();
      var before = input.value.slice(0, input.selectionStart);
      var line = before.slice(before.lastIndexOf("\n") + 1);
      var lead = /^[ \t]*/.exec(line)[0];
      if (/[{:(]\s*$/.test(line)) lead += indent();
      insert("\n" + lead);
    }
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
    var buttons = document.querySelectorAll("button[data-lk-source]");
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

  // A file button in a line of text stands outside any `.lk-demo`, and a press
  // on it is a request for its file — the panel changes hands or goes away in
  // `open`, so the press must reach the button.
  function elsewhere(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !node || !node.closest("[data-lk-chrome], .lk-demo, [data-lk-source]");
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
