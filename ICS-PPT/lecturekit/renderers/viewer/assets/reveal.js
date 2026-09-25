(function () {
  "use strict";

  // Each .reveal-block on a slide carries data-reveal="i". A slide starts
  // dimmed; Enter reveals the next step; when a slide's steps run out, Enter
  // pages forward. Paging backward shows the whole slide.
  //
  // A block that also carries data-reveal-items splits further: its own list
  // items become one step each (`p.slide(..., reveal="items")`). The split can
  // only happen here — the <li>s exist once Marp has rendered the markdown, so
  // the build side ships the flag and the browser finds the items.
  var DIM = "reveal-dim";
  var ITEM = "reveal-item"; // a sub-step element, dimmed on its own
  var lastSlide = null; // previous slide number, to detect direction
  var cursor = {};      // slide number -> next step index to reveal

  // A reload (live-reload after an edit, or the reader pressing F5) rebuilds the
  // deck and would dim the slide being read. What the slide shows is recorded
  // per tab as the page unloads and put back on load. It is read off the DOM,
  // since bespoke's arrow paging uses replaceState and fires no hashchange, so
  // the cursor alone does not know which slide is on screen. The viewer shell
  // drops the record when the reader opens a page from the outline, so that
  // entry starts dimmed.
  var STORAGE_KEY = "lecturekit:reveal:" + location.pathname;

  function shownSteps(seq) {
    var n = 0;
    while (n < seq.length && !seq[n].some(function (el) { return el.classList.contains(DIM); })) { n++; }
    return n;
  }

  function saveProgress() {
    var slide = activeSlide();
    if (!slide) { return; }
    try {
      sessionStorage.setItem(STORAGE_KEY, JSON.stringify({ slide: slideNumber(), step: shownSteps(steps(slide)) }));
    } catch (e) { /* storage unavailable */ }
  }

  function savedProgress(num) {
    try {
      var saved = JSON.parse(sessionStorage.getItem(STORAGE_KEY) || "null");
      return saved && saved.slide === num ? saved.step : null;
    } catch (e) {
      return null;
    }
  }

  function slideNumber() {
    var n = parseInt((location.hash || "").replace(/^#/, ""), 10);
    return isNaN(n) ? 1 : n;
  }

  function totalSlides() {
    return document.querySelectorAll("section").length;
  }

  function activeSlide() {
    // Marp's bespoke template marks the active slide on the <svg> wrapper
    // (svg.bespoke-marp-slide.bespoke-marp-active), not the inner <section>.
    // The .reveal-block divs are descendants of it either way. Fall back to the
    // nth slide by hash if the class is ever absent.
    return (
      document.querySelector(".bespoke-marp-active") ||
      document.querySelectorAll("svg.bespoke-marp-slide")[slideNumber() - 1] ||
      document.querySelectorAll("section")[slideNumber() - 1] ||
      null
    );
  }

  // The units an item-splitting block steps through, in document order: every
  // top-level list item, and every other element around them (a headline
  // paragraph, a floated image) as one unit apiece. A container that holds a
  // list is descended into rather than taken whole — otherwise the wrapper of a
  // slide with a floated image would be one unit and nothing would split.
  function units(el, out) {
    var children = el.children;
    for (var i = 0; i < children.length; i++) {
      var child = children[i];
      var tag = child.tagName;
      if (tag === "UL" || tag === "OL") {
        // The <li> is taken whole, so a nested list rides its parent item.
        for (var j = 0; j < child.children.length; j++) {
          out.push(child.children[j]);
        }
      } else if (child.querySelector("ul, ol")) {
        units(child, out);
      } else {
        out.push(child);
      }
    }
    return out;
  }

  // The slide's reveal steps, each an array of elements to light together.
  function steps(slide) {
    var byIndex = {};
    slide.querySelectorAll("[data-reveal]").forEach(function (el) {
      var i = parseInt(el.getAttribute("data-reveal"), 10);
      (byIndex[i] = byIndex[i] || []).push(el);
    });
    var out = [];
    Object.keys(byIndex)
      .map(Number)
      .sort(function (a, b) { return a - b; })
      .forEach(function (i) {
        var group = byIndex[i];
        var split = null;
        group.forEach(function (el) {
          if (el.hasAttribute("data-reveal-items")) { split = el; }
        });
        var parts = split ? units(split, []) : [];
        if (!parts.length) {
          out.push(group); // a plain block — or one whose split found nothing
          return;
        }
        parts.forEach(function (el) { el.classList.add(ITEM); });
        // Whatever else shares the index (the block's annotation bubbles)
        // rides the last item: a bubble comments on the finished block.
        var rest = group.filter(function (el) { return el !== split; });
        parts.forEach(function (el, k) {
          out.push(k === parts.length - 1 ? [el].concat(rest) : [el]);
        });
      });
    return out;
  }

  function setDim(slide, on) {
    steps(slide).forEach(function (group) {
      group.forEach(function (el) { el.classList.toggle(DIM, on); });
    });
  }

  function revealStep(group) {
    group.forEach(function (el) { el.classList.remove(DIM); });
  }

  function enterSlide(num, forward, restored) {
    var slide = activeSlide();
    if (!slide) { return; }
    var seq = steps(slide);
    if (restored !== null) {
      setDim(slide, true);
      cursor[num] = Math.min(restored, seq.length);
      seq.slice(0, cursor[num]).forEach(revealStep);
    } else if (forward) {
      setDim(slide, true);
      cursor[num] = 0;
    } else {
      setDim(slide, false);
      cursor[num] = seq.length;
    }
  }

  function onHashChange() {
    var num = slideNumber();
    var restored = lastSlide === null ? savedProgress(num) : null;
    var forward = lastSlide === null || num >= lastSlide;
    lastSlide = num;
    // Defer a tick so bespoke has applied the active-slide class.
    setTimeout(function () { enterSlide(num, forward, restored); }, 0);
  }

  function editable(target) {
    var node = target && target.nodeType === 1 ? target : null;
    return !!node && (node.isContentEditable ||
      (/^(INPUT|TEXTAREA|SELECT)$/.test(node.tagName) && !node.readOnly));
  }

  document.addEventListener(
    "keydown",
    function (e) {
      if (e.key !== "Enter") { return; }
      // Enter typed into a field — an interactive demo's terminal is one —
      // is that field's, not a step. A read-only one takes no typing.
      if (editable(e.target)) { return; }
      var slide = activeSlide();
      if (!slide) { return; }
      var num = slideNumber();
      var seq = steps(slide);
      var c = cursor[num] || 0;
      e.preventDefault();
      e.stopImmediatePropagation();
      if (c < seq.length) {
        revealStep(seq[c]);
        cursor[num] = c + 1;
      } else if (num < totalSlides()) {
        location.hash = String(num + 1); // bespoke navigates on hash change
      }
    },
    true // capture: run before bespoke's own key handling
  );

  window.addEventListener("hashchange", onHashChange);
  window.addEventListener("pagehide", saveProgress);
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", onHashChange);
  } else {
    onHashChange();
  }
})();
