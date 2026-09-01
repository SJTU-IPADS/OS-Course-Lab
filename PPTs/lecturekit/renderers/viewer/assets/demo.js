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
  // Injected only by the dev server, and only under `view --watch --demo`. Its
  // presence *is* the arming signal — the buttons ship `disabled` and a
  // rendered bundle, having no server behind it, never loads this.
  var ENDPOINT = "/__demo";
  var drawer = null;
  var runs = []; // every run started on the slide now showing, oldest first
  var active = null; // the run whose output the body is showing
  var frame = 0;
  var timer = 0;

  function build() {
    var el = document.createElement("div");
    el.className = "lk-drawer";
    el.setAttribute("data-open", "0");
    el.innerHTML =
      '<div class="lk-drawer-tabs"></div>' +
      '<div class="lk-drawer-head">' +
      '<span class="lk-drawer-cmd"></span>' +
      '<span class="lk-drawer-status"></span>' +
      '<button class="lk-drawer-stop" type="button" ' +
      'aria-label="Stop this run">■ stop</button>' +
      '<button class="lk-drawer-close" type="button" ' +
      'aria-label="Hide output; runs keep going">▾</button>' +
      "</div>" +
      '<pre class="lk-drawer-body"></pre>';
    el.querySelector(".lk-drawer-close").addEventListener("click", hide);
    el.querySelector(".lk-drawer-stop").addEventListener("click", stopActive);
    document.body.appendChild(el);

    // The way back to a drawer that was put away while its runs kept going.
    var peek = document.createElement("button");
    peek.className = "lk-drawer-peek";
    peek.type = "button";
    peek.hidden = true;
    peek.addEventListener("click", show);
    document.body.appendChild(peek);

    var body = el.querySelector(".lk-drawer-body");
    var doneNode = document.createTextNode("");
    var lineNode = document.createTextNode("");
    body.appendChild(doneNode);
    body.appendChild(lineNode);
    return {
      root: el,
      tabs: el.querySelector(".lk-drawer-tabs"),
      cmd: el.querySelector(".lk-drawer-cmd"),
      status: el.querySelector(".lk-drawer-status"),
      peek: peek,
      body: body,
      done: doneNode,
      line: lineNode
    };
  }

  // ---- the body: a terminal's last-line rule, and nothing more -------------
  // Output arrives in whatever slices the pipe handed over, so the tail of the
  // last line is kept apart and rewritten in place. A carriage return means
  // "back to column 0": that is the one control code worth honouring, because
  // it is the difference between a download's progress bar reading as one line
  // and as four hundred.
  //
  // Every run keeps its own three fields — `done`, `line`, `painted` — so a
  // background run's output accumulates while another tab is on screen. Two
  // text nodes rather than one string reassigned: finished lines are only ever
  // appended to, and a model that streams a token at a time would otherwise
  // rewrite the whole transcript per token. Painting is deferred to the next
  // frame for the same reason — the arrival rate is the process's, the redraw
  // rate is the screen's.

  function append(run, text) {
    var pieces = text.split("\n");
    for (var i = 0; i < pieces.length; i++) {
      var piece = pieces[i];
      var back = piece.lastIndexOf("\r");
      run.line = back === -1 ? run.line + piece : piece.slice(back + 1);
      if (i < pieces.length - 1) {
        run.done += run.line + "\n";
        run.line = "";
      }
    }
    if (run === active) schedule();
  }

  function schedule() {
    if (!drawer || frame || timer) return;
    frame = requestAnimationFrame(flush);
    // A window that is not being drawn — backgrounded, throttled, on a second
    // display nobody is looking at — gets no animation frames, and the output
    // would simply stop appearing. The timer stands behind the frame: whichever
    // arrives first paints, and cancels the other.
    timer = setTimeout(flush, 80);
  }

  function flush() {
    if (frame) cancelAnimationFrame(frame);
    if (timer) clearTimeout(timer);
    frame = 0;
    timer = 0;
    paint();
  }

  function paint() {
    if (!drawer || !active) return;
    var body = drawer.body;
    // Follow the output unless the reader has scrolled up to read something.
    var pinned = body.scrollHeight - body.scrollTop - body.clientHeight < 24;
    // Text nodes throughout: the body is whatever a process wrote to a pipe, so
    // it is text and only text — never markup to be parsed.
    if (active.done.length > active.painted) {
      drawer.done.appendData(active.done.slice(active.painted));
      active.painted = active.done.length;
    }
    if (drawer.line.data !== active.line) drawer.line.data = active.line;
    if (pinned) body.scrollTop = body.scrollHeight;
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
    if (!drawer) return;
    drawer.root.setAttribute("data-tabs", runs.length ? "1" : "0");
    if (run === active) {
      active = null;
      var next = runs[Math.min(at, runs.length - 1)];
      if (next) select(next);
      else blank();
    }
    updatePeek();
  }

  // A tab says two things at a glance: which command it is, and whether that
  // command is still going. The exit code is a number for the head row, where
  // there is room to read it.
  function mark(run) {
    if (!run.tab) return;
    var state = !run.ended ? "running" : run.ok ? "ok" : "fail";
    run.tab.setAttribute("data-state", state);
    run.tab.setAttribute("data-active", run === active ? "1" : "0");
  }

  function select(run) {
    active = run;
    // The body belongs to whichever run is on screen, so it is repainted whole
    // on a switch and then goes back to appending.
    drawer.done.data = run.done;
    drawer.line.data = run.line;
    run.painted = run.done.length;
    drawer.body.scrollTop = drawer.body.scrollHeight;
    drawer.cmd.textContent = run.label;
    head(run);
    for (var i = 0; i < runs.length; i++) mark(runs[i]);
    if (run.tab) run.tab.scrollIntoView({ block: "nearest", inline: "nearest" });
  }

  function head(run) {
    if (run !== active) return;
    drawer.status.textContent = run.status;
    if (run.ok === null) drawer.status.removeAttribute("data-ok");
    else drawer.status.setAttribute("data-ok", run.ok ? "1" : "0");
    drawer.root.setAttribute("data-running", run.ended ? "0" : "1");
    drawer.root.setAttribute(
      "data-empty",
      run.ended && run.done === "" && run.line === "" ? "1" : "0"
    );
  }

  // ---- the drawer ---------------------------------------------------------

  function show() {
    if (!drawer) return;
    drawer.root.setAttribute("data-open", "1");
    updatePeek();
  }

  // ▾ and Escape put the rack away. Nothing is stopped: a service started from
  // this slide is expected to outlive the drawer that shows its log. Closing a
  // run is the tab's own ✕ — see dismiss().
  function hide() {
    if (!drawer) return;
    drawer.root.setAttribute("data-open", "0");
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
    }
    runs = [];
    active = null;
    blank();
  }

  // An empty rack shows nothing and is put away.
  function blank() {
    if (!drawer) return;
    drawer.tabs.textContent = "";
    drawer.done.data = "";
    drawer.line.data = "";
    drawer.cmd.textContent = "";
    drawer.status.textContent = "";
    drawer.root.setAttribute("data-open", "0");
    drawer.root.setAttribute("data-tabs", "0");
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
      append(run, event.d);
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
      done: "",
      line: "",
      painted: 0,
      tab: null
    };
    hold(chip);
    runs.push(run);
    addTab(run);
    drawer.root.setAttribute("data-tabs", "1");
    select(run);
    show();
    watch();

    fetch(ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ id: chip.getAttribute("data-lk-demo") }),
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
        if (!aborted) append(run, String(err) + "\n");
      });
  }

  // What a tab and the head row call this run. The block shows every line of a
  // multi-line command already; there is one line to spend here, so it goes on
  // the first and says there is more. A second run of the same command is
  // numbered, because two identical tabs are two tabs nobody can tell apart.
  function label(chip) {
    var code = chip.querySelector(".lk-demo-cmd");
    var text = "";
    if (code) {
      var lines = code.textContent.split("\n");
      text = lines[0].replace(/^\$ /, "");
      if (lines.length > 1) text += " …";
    }
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
          if (!lines[i]) continue;
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
          start(chip);
        });
      })(chips[i]);
    }
  }

  // Escape is already Marp's own: it toggles the slide-grid overview. Taken in
  // the capture phase and only while the drawer is open, so the first press
  // puts the output away and every later one still reaches the deck.
  document.addEventListener(
    "keydown",
    function (event) {
      if (event.key !== "Escape") return;
      if (!drawer || drawer.root.getAttribute("data-open") !== "1") return;
      event.preventDefault();
      event.stopPropagation();
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
