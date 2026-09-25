(function () {
  "use strict";

  // Arms the `.lk-demo` blocks Marp rendered and shows what their commands
  // print, as they print it. The block carries `data-lk-demo="<id>"` and
  // nothing else: the id is a hash of the command, and the server maps it back
  // to the command the author wrote (see lecturekit/demo.py). So this file can
  // ask for a command to run, and cannot say what that command is.
  //
  // A press starts a run; a run gets a tab. Nothing is stopped to make room for
  // it, because a page that demonstrates a service demonstrates it running:
  // `ollama serve` holds its terminal, and the next two commands on the slide
  // are the ones that need it up. The drawer is therefore a rack of terminals,
  // not one terminal — ▾ puts the rack away and leaves everything in it
  // running, a tab's ✕ closes that one run, and leaving the slide takes the
  // whole rack down.
  //
  // Each run is an xterm.js terminal, so what a program draws — colours, a
  // progress bar redrawing its line, a chat prompt — is drawn as a terminal
  // would. The server announces each command line as bash reaches it, and the
  // terminal shows it in pale italics above what it printed: a transcript. A
  // demo the author marked interactive takes the keyboard; every other one is
  // display only, and keys pressed in it still reach the deck.
  //
  // Injected only by the dev server, and only under `view --watch`. Its
  // presence *is* the arming signal — the buttons ship `disabled` and a
  // rendered bundle, having no server behind it, never loads this.
  var ENDPOINT = "/__demo";
  var STDIN = "/__demo/stdin";
  var WINSIZE = "/__demo/winsize";
  var FONT =
    '"Cascadia Code", "SFMono-Regular", "JetBrains Mono", Consolas, ' +
    '"Liberation Mono", Menlo, monospace';
  var THEME = {
    background: "#10171d",
    foreground: "#dfe6ec",
    cursor: "#8fd3ff",
    cursorAccent: "#10171d",
    selectionBackground: "#2c4a63"
  };
  // A command line as the transcript shows it: the prompt dim, the command in
  // pale italics, so it reads as what was typed rather than what came back.
  var PROMPT = "\x1b[0;38;2;93;115;134m";
  var TYPED = "\x1b[0;3;38;2;159;179;194m";
  var RESET = "\x1b[0m";
  var HIDE_CURSOR = "\x1b[?25l";
  var SHOW_CURSOR = "\x1b[?25h";
  var DRAWER_SHARE = 0.55; // of the window's height; demo.css says the same
  var INPUT_PIECE = 4096; // characters per keystroke request, for a long paste

  var drawer = null;
  var runs = []; // every run started on the slide now showing, oldest first
  var active = null; // the run whose terminal is on screen

  function build() {
    var el = document.createElement("div");
    el.className = "lk-drawer";
    el.setAttribute("data-lk-chrome", "demo");
    el.setAttribute("data-open", "0");
    el.innerHTML =
      '<div class="lk-drawer-bar">' +
      '<div class="lk-drawer-tabs"></div>' +
      '<span class="lk-drawer-status"></span>' +
      '<button class="lk-drawer-stop" type="button" ' +
      'aria-label="Stop this run">■ stop</button>' +
      '<button class="lk-drawer-close" type="button" ' +
      'aria-label="Hide output; runs keep going">▾</button>' +
      "</div>" +
      '<div class="lk-drawer-screen"></div>';
    el.querySelector(".lk-drawer-close").addEventListener("click", hide);
    el.querySelector(".lk-drawer-stop").addEventListener("click", stopActive);
    // Keys typed into an interactive run are the program's. Marp pages on the
    // arrow keys and space from a listener on the document, so they stop here,
    // on the way up.
    el.addEventListener("keydown", function (event) {
      if (typing(event.target)) event.stopPropagation();
    });
    document.body.appendChild(el);

    // The way back to a drawer that was put away while its runs kept going.
    var peek = document.createElement("button");
    peek.className = "lk-drawer-peek";
    peek.setAttribute("data-lk-chrome", "peek");
    peek.type = "button";
    peek.hidden = true;
    peek.addEventListener("click", show);
    document.body.appendChild(peek);

    return {
      root: el,
      bar: el.querySelector(".lk-drawer-bar"),
      tabs: el.querySelector(".lk-drawer-tabs"),
      status: el.querySelector(".lk-drawer-status"),
      screen: el.querySelector(".lk-drawer-screen"),
      peek: peek
    };
  }

  // ---- the terminal -------------------------------------------------------

  function terminal(run) {
    run.box = document.createElement("div");
    run.box.className = "lk-drawer-term";
    run.box.setAttribute("data-input", "0");
    drawer.screen.appendChild(run.box);
    run.term = new window.Terminal({
      convertEol: true, // a pipe writes "\n" and means a new line
      disableStdin: true,
      cursorBlink: false,
      cursorInactiveStyle: "none",
      fontFamily: FONT,
      fontSize: 14,
      lineHeight: 1.2,
      scrollback: 5000,
      theme: THEME
    });
    run.fit = new window.FitAddon.FitAddon();
    run.term.loadAddon(run.fit);
    // Display only, until the server says otherwise: the terminal leaves every
    // key alone, so the arrows still page the deck with the output focused.
    run.term.attachCustomKeyEventHandler(function () {
      return run.interactive && !run.ended;
    });
    run.term.onData(function (data) {
      if (run.interactive && !run.ended) type(run, data);
    });
  }

  function write(run, text) {
    if (!text || !run.term) return;
    run.fresh = text.charAt(text.length - 1) === "\n";
    run.term.write(text, function () {
      if (run === active && !run.interactive) size(run);
    });
  }

  // A command bash is about to run, on a line of its own. The server puts a
  // two-character prompt on every line, "$ " or "> ", as bash would echo it.
  function command(run, text) {
    var out = run.fresh ? "" : RESET + "\r\n";
    var lines = text.split("\n");
    for (var i = 0; i < lines.length; i++) {
      out += PROMPT + lines[i].slice(0, 2) + TYPED + lines[i].slice(2) + RESET + "\r\n";
    }
    write(run, out);
  }

  // How big the terminal is. The width is the drawer's. The height is what
  // the output needs, up to the drawer's share of the window, for a run that
  // only prints — a one-line answer gets a one-line drawer. An interactive run
  // gets the whole share at once, and keeps it: a program drawing a screen
  // needs a screen that holds still, and the far end is told its size.
  function fit(run) {
    if (run !== active || !run.term || !run.term.element) return;
    var room =
      Math.floor(window.innerHeight * DRAWER_SHARE) -
      drawer.bar.offsetHeight -
      padding(drawer.screen);
    run.box.style.height = Math.max(room, 0) + "px";
    var dims = run.fit.proposeDimensions();
    run.box.style.height = "";
    if (!dims || !isFinite(dims.cols) || !isFinite(dims.rows)) return;
    run.cols = dims.cols;
    run.maxRows = dims.rows;
    size(run);
  }

  // Measuring is `fit`'s; this only counts rows, cheap enough for every write.
  function size(run) {
    if (!run.term || !run.cols) return;
    var rows = run.maxRows;
    if (!run.interactive) {
      var buffer = run.term.buffer.active;
      rows = Math.min(Math.max(buffer.baseY + buffer.cursorY + 1, 1), run.maxRows);
    }
    if (run.term.cols === run.cols && run.term.rows === rows) return;
    run.term.resize(run.cols, rows);
    if (run.interactive && run.token && !run.ended) {
      post(WINSIZE, { run: run.token, cols: run.cols, rows: rows });
    }
  }

  function padding(el) {
    var style = window.getComputedStyle(el);
    return parseFloat(style.paddingTop) + parseFloat(style.paddingBottom);
  }

  var refit = 0;
  window.addEventListener("resize", function () {
    if (refit) return;
    refit = requestAnimationFrame(function () {
      refit = 0;
      if (active) fit(active);
    });
  });

  // ---- input --------------------------------------------------------------
  // Keystrokes travel as requests of their own, one at a time and in order:
  // what piles up while one is out goes in the next. A long paste is split,
  // never inside a surrogate pair.

  function type(run, data) {
    run.input += data;
    if (!run.sending) send(run);
  }

  function send(run) {
    if (!run.input || run.ended || !run.token) {
      run.sending = false;
      run.input = "";
      return;
    }
    run.sending = true;
    var cut = Math.min(run.input.length, INPUT_PIECE);
    var code = run.input.charCodeAt(cut - 1);
    if (cut < run.input.length && code >= 0xd800 && code <= 0xdbff) cut--;
    var piece = run.input.slice(0, cut);
    run.input = run.input.slice(cut);
    post(STDIN, { run: run.token, data: piece }).then(
      function () {
        send(run);
      },
      function () {
        send(run);
      }
    );
  }

  function post(path, payload) {
    return fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
  }

  // The server's word that this run is on a terminal the drawer may type into.
  function interactive(run, token) {
    run.interactive = true;
    run.token = token;
    run.term.options.convertEol = false; // a terminal writes "\r\n" itself
    run.term.options.disableStdin = false;
    run.term.textarea.readOnly = false;
    run.box.setAttribute("data-input", "1");
    run.term.write(SHOW_CURSOR);
    fit(run);
    if (run === active && isOpen()) run.term.focus();
  }

  // Is this key on its way to a program?
  function typing(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !!node && !!node.closest('.lk-drawer-term[data-input="1"]');
  }

  // ---- tabs ---------------------------------------------------------------

  function addTab(run) {
    var tab = document.createElement("div");
    tab.className = "lk-drawer-tab";

    var pick = document.createElement("button");
    pick.className = "lk-drawer-tab-pick";
    pick.type = "button";
    var dot = document.createElement("span");
    dot.className = "lk-drawer-dot";
    var name = document.createElement("span");
    name.className = "lk-drawer-name";
    name.textContent = run.label;
    pick.appendChild(dot);
    pick.appendChild(name);
    pick.addEventListener("click", function () {
      select(run);
    });

    var shut = document.createElement("button");
    shut.className = "lk-drawer-tab-close";
    shut.type = "button";
    shut.textContent = "✕";
    shut.setAttribute("aria-label", "Close this run");
    shut.addEventListener("click", function (event) {
      event.stopPropagation();
      dismiss(run);
    });

    tab.appendChild(pick);
    tab.appendChild(shut);
    run.tab = tab;
    drawer.tabs.appendChild(tab);
    mark(run);
    return tab;
  }

  // ✕ on a tab is closing a terminal, not hiding one: the run goes with it, and
  // a run that is still going is stopped on the way out — there would be no
  // handle left to stop it with afterwards. The rack closes when its last tab
  // does, so a slide is never stuck showing a drawer nobody wants.
  function dismiss(run) {
    if (!run.ended) run.controller.abort(); // settle() follows, off the catch
    var at = runs.indexOf(run);
    if (at !== -1) runs.splice(at, 1);
    if (run.tab && run.tab.parentNode) run.tab.parentNode.removeChild(run.tab);
    discard(run);
    if (!drawer) return;
    if (run === active) {
      active = null;
      var next = runs[Math.min(at, runs.length - 1)];
      if (next) select(next);
      else blank();
    }
    updatePeek();
  }

  function discard(run) {
    if (run.term) run.term.dispose();
    run.term = null;
    if (run.box && run.box.parentNode) run.box.parentNode.removeChild(run.box);
  }

  // A tab says two things at a glance: which demo it is, and whether it is
  // still going. The exit code is a number for the bar, where there is room to
  // read it.
  function mark(run) {
    if (!run.tab) return;
    var state = !run.ended ? "running" : run.ok ? "ok" : "fail";
    run.tab.setAttribute("data-state", state);
    run.tab.setAttribute("data-active", run === active ? "1" : "0");
  }

  function select(run) {
    active = run;
    for (var i = 0; i < runs.length; i++) {
      runs[i].box.hidden = runs[i] !== run;
      mark(runs[i]);
    }
    fit(run); // the window may have changed while this one was out of sight
    head(run);
    if (run.tab) run.tab.scrollIntoView({ block: "nearest", inline: "nearest" });
    if (run.interactive && !run.ended && isOpen()) run.term.focus();
  }

  function head(run) {
    if (run !== active) return;
    drawer.status.textContent = run.status;
    if (run.ok === null) drawer.status.removeAttribute("data-ok");
    else drawer.status.setAttribute("data-ok", run.ok ? "1" : "0");
    drawer.root.setAttribute("data-running", run.ended ? "0" : "1");
  }

  // ---- the drawer ---------------------------------------------------------

  function show() {
    if (!drawer) return;
    drawer.root.setAttribute("data-open", "1");
    updatePeek();
    if (active && active.interactive && !active.ended) active.term.focus();
  }

  // ▾, Escape and a click on the slide put the rack away. Nothing is stopped:
  // a service started from this slide is expected to outlive the drawer that
  // shows its log. Closing a run is the tab's own ✕ — see dismiss().
  function hide() {
    if (!drawer) return;
    drawer.root.setAttribute("data-open", "0");
    if (active && active.term) active.term.blur(); // the keys are the deck's again
    updatePeek();
  }

  function updatePeek() {
    if (!drawer) return;
    var open = drawer.root.getAttribute("data-open") === "1";
    var live = 0;
    for (var i = 0; i < runs.length; i++) if (!runs[i].ended) live++;
    drawer.peek.textContent = live
      ? "▲ " + live + " running"
      : "▲ " + runs.length + " output";
    drawer.peek.hidden = open || runs.length === 0;
  }

  function stopActive() {
    // Aborting the fetch drops the connection, which is how the server is told
    // to kill the command — there is no second request to send.
    if (active && !active.ended) active.controller.abort();
  }

  // Leaving the slide is the one thing that ends a run the author did not stop.
  // The tabs go with it: they are the previous slide's terminals, and carrying
  // them forward would only make the next slide lie about what is running.
  function clear() {
    for (var i = 0; i < runs.length; i++) {
      var run = runs[i];
      if (!run.ended) run.controller.abort();
      release(run.chip);
      discard(run);
    }
    runs = [];
    active = null;
    blank();
  }

  // An empty rack shows nothing and is put away.
  function blank() {
    if (!drawer) return;
    drawer.tabs.textContent = "";
    drawer.status.textContent = "";
    drawer.root.setAttribute("data-open", "0");
    drawer.root.setAttribute("data-running", "0");
    updatePeek();
  }

  // A chip is "running" while any of its runs is: the same command can be going
  // twice, and the block should stop pulsing only when the last one is over.
  function hold(chip) {
    chip.lkDemoRuns = (chip.lkDemoRuns || 0) + 1;
    chip.setAttribute("data-lk-demo-state", "running");
  }

  function release(chip) {
    chip.lkDemoRuns = Math.max(0, (chip.lkDemoRuns || 0) - 1);
    if (!chip.lkDemoRuns) chip.removeAttribute("data-lk-demo-state");
  }

  function settle(run, label, ok) {
    if (run.ended) return;
    run.ended = true;
    run.status = label;
    run.ok = ok;
    release(run.chip);
    if (run.interactive && run.term) {
      // Over: nothing is listening for keys any more, and they go back to the
      // deck.
      run.term.options.disableStdin = true;
      run.term.textarea.readOnly = true;
      run.box.setAttribute("data-input", "0");
      run.term.write(HIDE_CURSOR);
      run.term.blur();
    }
    mark(run);
    head(run);
    updatePeek();
  }

  function endLabel(event) {
    if (event.timedOut) return "timed out · " + event.duration.toFixed(1) + "s";
    return "exit " + event.exit + " · " + event.duration.toFixed(2) + "s";
  }

  function handle(event, run) {
    if (event.t === "out") {
      write(run, event.d);
    } else if (event.t === "cmd") {
      command(run, event.d);
    } else if (event.t === "stdin") {
      interactive(run, event.run);
    } else if (event.t === "tick") {
      run.status = "running… " + event.elapsed.toFixed(0) + "s";
      head(run);
    } else if (event.t === "end") {
      settle(run, endLabel(event), event.exit === 0);
    }
  }

  // ---- running ------------------------------------------------------------

  function start(chip) {
    if (!drawer) drawer = build();
    var run = {
      chip: chip,
      label: label(chip),
      controller: new AbortController(),
      ended: false,
      status: "running…",
      ok: null,
      tab: null,
      box: null,
      term: null,
      fit: null,
      cols: 0,
      maxRows: 24,
      fresh: true, // the cursor is at the start of a line
      interactive: false,
      token: null,
      input: "",
      sending: false
    };
    hold(chip);
    runs.push(run);
    terminal(run);
    addTab(run);
    run.term.open(run.box);
    run.term.write(HIDE_CURSOR);
    // Read-only until the run turns out to take input: the deck's own keys
    // (Enter for the next step) pass a field that cannot be typed into.
    run.term.textarea.readOnly = true;
    select(run);
    show();
    watch();

    fetch(ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      // The size is for an interactive run's terminal; the server decides
      // whether this is one, and says so in the stream.
      body: JSON.stringify({
        id: chip.getAttribute("data-lk-demo"),
        cols: run.cols || run.term.cols,
        rows: run.maxRows
      }),
      signal: run.controller.signal
    })
      .then(function (response) {
        if (!response.ok || !response.body) return fail(response, run);
        return pump(response.body.getReader(), run);
      })
      .catch(function (err) {
        if (run.ended) return;
        var aborted = run.controller.signal.aborted;
        settle(run, aborted ? "stopped" : "unreachable", false);
        if (!aborted) write(run, String(err) + "\r\n");
      });
  }

  // What a tab calls this run: the demo's own name, which is short and is what
  // the slide calls it too. The commands themselves are in the terminal. A
  // second run of the same demo is numbered, because two identical tabs are
  // two tabs nobody can tell apart.
  function label(chip) {
    var name = chip.querySelector(".lk-demo-name");
    var code = chip.querySelector(".lk-demo-cmd");
    var text = name ? name.textContent.trim() : "";
    if (!text && code) text = code.textContent.split("\n")[0].replace(/^\$ /, "");
    var seen = 0;
    for (var i = 0; i < runs.length; i++) {
      if (runs[i].chip === chip) seen++;
    }
    return seen ? text + " (" + (seen + 1) + ")" : text;
  }

  function fail(response, run) {
    return response
      .json()
      .catch(function () {
        return {};
      })
      .then(function (payload) {
        settle(run, payload.error || "HTTP " + response.status, false);
      });
  }

  // One JSON object per line. A read can land mid-line, so the tail waits for
  // the next one.
  function pump(reader, run) {
    var decoder = new TextDecoder();
    var pending = "";
    function step() {
      return reader.read().then(function (chunk) {
        pending += decoder.decode(chunk.value || new Uint8Array(), {
          stream: !chunk.done
        });
        var lines = pending.split("\n");
        pending = lines.pop();
        for (var i = 0; i < lines.length; i++) {
          if (!lines[i] || !run.term) continue;
          try {
            handle(JSON.parse(lines[i]), run);
          } catch (err) {
            /* a half-written line at the very end: nothing to report */
          }
        }
        if (chunk.done) {
          // The stream stopped without saying so — the server went away.
          settle(run, "disconnected", false);
          return;
        }
        return step();
      });
    }
    return step();
  }

  // ---- which slide are we on ----------------------------------------------
  // Marp's controller does not touch the URL when it pages, so the deck's own
  // marker is the signal: exactly one section carries `bespoke-marp-active`.
  // Watched only while something is running — with an empty rack there is
  // nothing a page turn could interrupt.

  var observer = null;

  function watch() {
    if (observer || !window.MutationObserver) return;
    var slides = document.querySelectorAll(".bespoke-marp-slide");
    if (!slides.length) return;
    var here = current();
    observer = new MutationObserver(function () {
      var now = current();
      if (now === here) return;
      here = now;
      clear();
      observer.disconnect();
      observer = null;
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
    if (!window.Terminal || !window.FitAddon) return; // no terminal, no runs
    var chips = document.querySelectorAll(".lk-demo[data-lk-demo]");
    for (var i = 0; i < chips.length; i++) {
      (function (chip) {
        var button = chip.querySelector(".lk-demo-run");
        if (!button || button.getAttribute("data-lk-armed")) return;
        button.setAttribute("data-lk-armed", "1");
        button.disabled = false;
        button.addEventListener("click", function (event) {
          // The deck owns clicks for paging; a demo press is not a page turn.
          event.preventDefault();
          event.stopPropagation();
          // Nor does it keep the keys. Marp ignores every key aimed at a
          // button, so a focused ▶ would leave the arrows and a clicker's
          // PageDown dead until the next click on the slide. Marp's own
          // controls let go the same way.
          button.blur();
          start(chip);
        });
      })(chips[i]);
    }
  }

  // ---- putting it away ----------------------------------------------------

  function isOpen() {
    return !!drawer && drawer.root.getAttribute("data-open") === "1";
  }

  // The deck's own chrome — this drawer, its peek button, the file panel — and
  // the demo blocks themselves. A press on any of them is a press on something
  // that is already about the output; anything else is a press on the slide.
  function elsewhere(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !node || !node.closest("[data-lk-chrome], .lk-demo");
  }

  // A click on the slide while the rack is up means "let me see the slide", and
  // Marp would otherwise take it as a click on the slide itself — its own
  // handler jumps to that page and resets what the page has revealed. Taken in
  // the capture phase, and only while the drawer is open, so the deck keeps the
  // gesture the rest of the time. Nothing is stopped: this is ▾, by another
  // route.
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

  // Escape is already Marp's own: it toggles the slide-grid overview. Taken in
  // the capture phase and only while the drawer is open, so the first press
  // puts the output away and every later one still reaches the deck. Not while
  // typing into a run: there Escape is the program's (a REPL's, an editor's),
  // and the way out is a click on the slide or ▾.
  document.addEventListener(
    "keydown",
    function (event) {
      if (event.key !== "Escape" || !isOpen() || typing(event.target)) return;
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
