(function () {
  "use strict";

  // Tell the viewer shell which slide is on screen.
  //
  // The shell (index.html) frames this deck and needs the slide number twice:
  // to highlight the page you paged to when you go back to the outline, and to
  // come back to the same slide after a reload. It cannot look for itself. Marp
  // keeps the number in location.hash, but reading another document's location
  // is a same-origin read, and Chrome gives every file:// document an origin of
  // its own — so a bundle opened by double-click could never answer. A message
  // crosses that line.
  //
  // The number is the 1-based position of bespoke's active slide, the same one
  // the deck's #N anchors use. It is said once per change, not per mutation.

  if (window.parent === window) return; // not framed, so nobody to tell

  var ACTIVE = "bespoke-marp-active";
  var said = 0;

  function report() {
    var slides = document.querySelectorAll(".bespoke-marp-slide");
    for (var i = 0; i < slides.length; i++) {
      if (!slides[i].classList.contains(ACTIVE)) continue;
      if (i + 1 !== said) {
        said = i + 1;
        window.parent.postMessage({ lecturekit: "slide", number: said }, "*");
      }
      return;
    }
  }

  // bespoke marks the slides and moves the active class with class writes,
  // so one observer sees the deck start and every page turn after it.
  new MutationObserver(report).observe(document.documentElement, {
    subtree: true,
    attributes: true,
    attributeFilter: ["class"],
  });

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", report);
  } else {
    report();
  }
})();
