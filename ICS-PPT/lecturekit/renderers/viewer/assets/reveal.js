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

  function enterSlide(num, forward) {
    var slide = activeSlide();
    if (!slide) { return; }
    if (forward) {
      setDim(slide, true);
      cursor[num] = 0;
    } else {
      setDim(slide, false);
      cursor[num] = steps(slide).length;
    }
  }

  function onHashChange() {
    var num = slideNumber();
    var forward = lastSlide === null || num >= lastSlide;
    lastSlide = num;
    // Defer a tick so bespoke has applied the active-slide class.
    setTimeout(function () { enterSlide(num, forward); }, 0);
  }

  document.addEventListener(
    "keydown",
    function (e) {
      if (e.key !== "Enter") { return; }
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
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", onHashChange);
  } else {
    onHashChange();
  }
})();
