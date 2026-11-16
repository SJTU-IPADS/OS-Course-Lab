(function () {
  "use strict";

  // Deck variables: `lecture.variable(...)`, the controls bound to them
  // (`p.slider`, `p.input`) and every place a page uses one.
  //
  // The deck is built at each variable's default, so a page without this
  // script (the PDF, the PNG) is complete. Here a value moves three things:
  //
  //   - the custom property --lkv-<name> on the root element, which every size
  //     written as "$name" reads (`calc(var(--lkv-size, 480) * 1px)`);
  //   - every bound figure that reads the variable. The page carries the
  //     figure's SVG (script[data-lk-figure]); its bindings are evaluated
  //     with bind.js, the same language as lecturekit/bind.py, and the result
  //     is shown in place of the file through the same <img>, so its box,
  //     fonts and look are the file's. At the values the file is drawn at,
  //     the <img> shows the file itself;
  //   - every control bound to it, on any slide.
  //
  // The value is kept in localStorage under the lecture's id. All slides are
  // one document, so the controls of one deck agree directly; another window
  // of the same deck (Marp's presenter view and its next-slide preview, a
  // second tab) hears of a change through the storage event, as Marp's own
  // presenter view hears of a page turn. Only a value that differs from the
  // default is stored, so a default changed in the source is not hidden by an
  // old stored copy of the previous one. Where storage is refused (some file://
  // pages, a private window) the values live as long as the page.
  //
  // The definitions come from the JSON table, script[data-lk-vars], that each
  // page using a variable carries (`variables.page_table`).

  var tables = document.querySelectorAll("script[data-lk-vars]");
  if (!tables.length) return;

  var B = window.lkBind;
  var defs = {};
  var lectureId = "";
  for (var t = 0; t < tables.length; t++) {
    var table;
    try { table = JSON.parse(tables[t].textContent); } catch (e) { continue; }
    lectureId = table.lecture || lectureId;
    for (var name in table.vars) defs[name] = table.vars[name];
  }
  var KEY = "lecturekit:vars:" + lectureId;
  var values = {};

  // ------------------------------------------------------------ values

  function decimalsOf(step) {
    var text = B ? B.positional(step) : String(step);
    var dot = text.indexOf(".");
    return dot < 0 ? 0 : text.length - dot - 1;
  }

  // A value within the variable's range, on its grid when it has a step, or
  // null for something that is not a number.
  function snap(def, raw) {
    if (raw === "" || raw === null || raw === undefined) return null;
    var v = Number(raw);
    if (!isFinite(v)) return null;
    v = Math.min(def.max, Math.max(def.min, v));
    if (def.step === null || def.step === undefined) return v;
    var k = Math.round((v - def.min) / def.step);
    var places = Math.max(decimalsOf(def.step), decimalsOf(def.min));
    return Number((def.min + k * def.step).toFixed(places));
  }

  function defaultOf(name) {
    return snap(defs[name], defs[name]["default"]);
  }

  // `value` with as few decimals as it needs and at least `places`; the same
  // text as variables.number_text.
  function text(value, places) {
    var out = B ? B.positional(value) : String(value);
    if (out.charAt(0) === "-" && Number(out) === 0) out = out.slice(1);
    var dot = out.indexOf(".");
    var have = dot < 0 ? 0 : out.length - dot - 1;
    if (have < places) out += (dot < 0 ? "." : "") + new Array(places - have + 1).join("0");
    return out;
  }

  function stored() {
    try { return JSON.parse(localStorage.getItem(KEY) || "{}") || {}; } catch (e) { return {}; }
  }

  function load() {
    var saved = stored();
    for (var name in defs) {
      var v = name in saved ? snap(defs[name], saved[name]) : null;
      values[name] = v === null ? defaultOf(name) : v;
    }
  }

  function save(name) {
    try {
      var saved = stored();
      if (values[name] === defaultOf(name)) delete saved[name];
      else saved[name] = values[name];
      if (Object.keys(saved).length) localStorage.setItem(KEY, JSON.stringify(saved));
      else localStorage.removeItem(KEY);
    } catch (e) { /* storage refused: the value lives as long as the page */ }
  }

  // ------------------------------------------------------------ figures

  var figures = [];

  function warn(figure, error) {
    if (figure.warned) return;
    figure.warned = true;
    if (window.console) console.warn("lecturekit: " + figure.key + ": " + error.message);
  }

  function Figure(key, source) {
    var doc = new DOMParser().parseFromString(source, "image/svg+xml");
    if (doc.getElementsByTagName("parsererror").length) throw new Error("not well-formed SVG");
    var declaration = doc.querySelector("[data-lk-vars]");
    if (!declaration) throw new Error("no data-lk-vars");
    this.key = key;
    this.doc = doc;
    this.drawn = {};
    var inputs = B.parseProgram(declaration.getAttribute("data-lk-vars"));
    for (var i = 0; i < inputs.length; i++) this.drawn[inputs[i][0]] = B.evaluate(inputs[i][1], {});
    this.lets = B.parseProgram(declaration.getAttribute("data-lk-let") || "");
    this.binds = [];
    var bound = doc.querySelectorAll("[data-lk-attr], [data-lk-text]");
    for (var j = 0; j < bound.length; j++) {
      var el = bound[j];
      this.binds.push({
        el: el,
        attrs: B.parseAttrs(el.getAttribute("data-lk-attr") || ""),
        text: el.hasAttribute("data-lk-text") ? B.parse(el.getAttribute("data-lk-text")) : null
      });
    }
    this.url = null;
    this.wanted = null;
    this.pending = false;
  }

  Figure.prototype.reads = function (name) {
    return Object.prototype.hasOwnProperty.call(this.drawn, name);
  };

  Figure.prototype.images = function () {
    var all = document.querySelectorAll("img[data-lk-figure]");
    var out = [];
    for (var i = 0; i < all.length; i++) {
      if (all[i].getAttribute("data-lk-figure") === this.key) out.push(all[i]);
    }
    return out;
  };

  Figure.prototype.show = function (src) {
    var images = this.images();
    for (var i = 0; i < images.length; i++) {
      var img = images[i];
      if (img.lkFile === undefined) img.lkFile = img.getAttribute("src");
      img.setAttribute("src", src === null ? img.lkFile : src);
    }
    if (this.url && this.url !== src) URL.revokeObjectURL(this.url);
    this.url = src;
  };

  // The figure at the current values. At the values the file is drawn at, the
  // file itself; otherwise the bindings evaluated into the SVG, which replaces
  // the picture once it has loaded, so the figure never blanks.
  Figure.prototype.draw = function () {
    this.pending = false;
    var env = {};
    var atFile = true;
    for (var name in this.drawn) {
      env[name] = name in values ? values[name] : this.drawn[name];
      if (env[name] !== this.drawn[name]) atFile = false;
    }
    if (atFile) {
      this.wanted = null;
      this.show(null);
      return;
    }
    try {
      env = B.run(this.lets, env);
    } catch (e) {
      warn(this, e);
      return;
    }
    for (var i = 0; i < this.binds.length; i++) {
      var bind = this.binds[i];
      try {
        for (var j = 0; j < bind.attrs.length; j++) {
          var attr = bind.attrs[j];
          bind.el.setAttribute(attr[0], B.attrValue(B.evaluate(attr[1], env), attr[0]));
        }
        if (bind.text) bind.el.textContent = B.textValue(B.evaluate(bind.text, env), "text");
      } catch (e) {
        warn(this, e);
      }
    }
    var svg = new XMLSerializer().serializeToString(this.doc);
    var url = URL.createObjectURL(new Blob([svg], { type: "image/svg+xml" }));
    var figure = this;
    this.wanted = url;
    var next = new Image();
    next.onload = function () {
      if (figure.wanted === url) figure.show(url);
      else URL.revokeObjectURL(url);
    };
    next.onerror = function () { URL.revokeObjectURL(url); };
    next.src = url;
  };

  // Many changes in one frame (a drag) draw once. A window in the background
  // runs no frames; the timer draws it there.
  Figure.prototype.schedule = function () {
    if (this.pending) return;
    this.pending = true;
    var figure = this;
    var run = function () { if (figure.pending) figure.draw(); };
    if (window.requestAnimationFrame) window.requestAnimationFrame(run);
    setTimeout(run, 120);
  };

  function loadFigures() {
    if (!B) return;
    var sources = document.querySelectorAll("script[data-lk-figure]");
    var seen = {};
    for (var i = 0; i < sources.length; i++) {
      var key = sources[i].getAttribute("data-lk-figure");
      if (seen[key]) continue;
      seen[key] = true;
      try {
        figures.push(new Figure(key, sources[i].textContent));
      } catch (e) {
        warn({ key: key }, e);
      }
    }
  }

  // ------------------------------------------------------------ controls

  function places(control) {
    return Number(control.getAttribute("data-lk-places")) || 0;
  }

  // Show the value of `name` everywhere it is used. `typing` is an input box
  // the reader is typing into: its text is theirs until they leave it.
  function show(name, typing) {
    var value = values[name];
    var atDefault = value === defaultOf(name);
    document.documentElement.style.setProperty("--lkv-" + name, text(value, 0));
    var controls = document.querySelectorAll('.lk-control[data-lk-var="' + name + '"]');
    for (var i = 0; i < controls.length; i++) {
      var control = controls[i];
      var shown = text(value, places(control));
      var box = control.querySelector('input[type="number"]');
      if (box && box !== typing) box.value = shown;
      var range = control.querySelector('input[type="range"]');
      if (range) {
        range.value = String(value);
        range.setAttribute("aria-valuetext", shown);
      }
      var reset = control.querySelector(".lk-control-reset");
      if (reset) reset.disabled = atDefault;
    }
    for (var j = 0; j < figures.length; j++) {
      if (figures[j].reads(name)) figures[j].schedule();
    }
  }

  function set(name, raw, typing) {
    var v = snap(defs[name], raw);
    if (v === null) return;
    var changed = v !== values[name];
    values[name] = v;
    show(name, typing);
    if (changed) save(name);
  }

  // Focus left on a control would take the arrow keys the deck pages with,
  // so a pointer gesture gives it back. A reader who reached the control with
  // Tab keeps it, and moves the slider with the arrows.
  function release(el) {
    if (document.activeElement === el) el.blur();
  }

  function arm(control) {
    var name = control.getAttribute("data-lk-var");
    if (!defs[name]) return;
    control.setAttribute("data-lk-live", "");

    // A drag along the slider is not a swipe to the next slide.
    ["touchstart", "touchmove", "touchend"].forEach(function (type) {
      control.addEventListener(type, function (event) { event.stopPropagation(); });
    });

    var range = control.querySelector('input[type="range"]');
    if (range) {
      range.addEventListener("input", function () {
        set(name, Number(Number(range.value).toFixed(places(control))), null);
      });
      range.addEventListener("pointerup", function () { release(range); });
    }

    var box = control.querySelector('input[type="number"]');
    if (box) {
      // Each keystroke that leaves a number in the range sets it; the text
      // stays as typed. Enter or leaving the box sets what is there, a value
      // outside the range brought inside it.
      box.addEventListener("input", function () {
        var v = Number(box.value);
        var def = defs[name];
        if (box.value !== "" && isFinite(v) && v >= def.min && v <= def.max) set(name, v, box);
      });
      box.addEventListener("change", function () {
        set(name, box.value, null);
        show(name, null);
      });
      box.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
          set(name, box.value, null);
          show(name, null);
          box.blur();
        } else if (event.key === "Escape") {
          show(name, null);
          box.blur();
        }
      });
    }

    var marks = control.querySelectorAll("[data-lk-value]");
    for (var i = 0; i < marks.length; i++) {
      marks[i].addEventListener("click", function (event) {
        set(name, event.currentTarget.getAttribute("data-lk-value"), null);
      });
    }

    var reset = control.querySelector(".lk-control-reset");
    if (reset) {
      reset.addEventListener("click", function () {
        set(name, defs[name]["default"], null);
        release(reset);
      });
    }
  }

  // Another window of this deck changed a value (or cleared them all).
  window.addEventListener("storage", function (event) {
    if (event.key !== KEY && event.key !== null) return;
    var before = {};
    for (var name in values) before[name] = values[name];
    load();
    for (var n in values) if (values[n] !== before[n]) show(n, null);
  });

  function start() {
    load();
    loadFigures();
    var controls = document.querySelectorAll(".lk-control[data-lk-var]");
    for (var i = 0; i < controls.length; i++) arm(controls[i]);
    for (var name in defs) show(name, null);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start);
  } else {
    start();
  }
})();
