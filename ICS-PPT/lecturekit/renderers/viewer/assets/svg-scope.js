(function () {
  "use strict";

  // Keep marpit-svg-polyfill off the slides nobody is looking at.
  //
  // Marpit's inline SVG mode wraps every slide in <svg data-marpit-svg>, and
  // WebKit cannot scale the HTML inside a <foreignObject> (webkit.org bug
  // 23113, open since 2009). marp-cli therefore ships marpit-svg-polyfill,
  // which runs on *every animation frame*: for each svg[data-marpit-svg] in the
  // document it calls getScreenCTM() and writes a transform onto the <section>
  // inside. In WebKit getScreenCTM() forces a synchronous full-document layout,
  // and the transform it then writes dirties layout again — so the pass is one
  // forced relayout per slide, sixty times a second.
  //
  // A deck of a dozen slides never notices. A lecture is 240+ slides in one
  // document, and measured in Safari 26 that pass costs 10.1 ms — 61% of a core
  // burned for as long as the tab is on screen, whether or not anything is being
  // edited. Only one slide is ever visible, so we park the polyfill's own
  // selector on the rest: the same pass then costs 0.05 ms.
  //
  // Nothing here is WebKit-specific. Other engines drop the polyfill after the
  // first frame (its list of applicable polyfills comes back empty), so parking
  // the attribute costs them nothing and spares us a UA sniff.

  var ATTR = "data-marpit-svg";
  var PARKED = "data-lk-marpit-svg";
  var ACTIVE = "bespoke-marp-active";

  // Views that genuinely show every slide at once. There the polyfill has to
  // run on all of them, so the attribute goes back everywhere.
  var MULTI_SLIDE_VIEWS = { overview: true, presenter: true };

  var printing = false;

  function slides() {
    return document.querySelectorAll("svg[" + ATTR + "], svg[" + PARKED + "]");
  }

  function park(el) {
    if (el.hasAttribute(ATTR)) {
      el.setAttribute(PARKED, el.getAttribute(ATTR));
      el.removeAttribute(ATTR);
    }
  }

  function unpark(el) {
    if (el.hasAttribute(PARKED)) {
      el.setAttribute(ATTR, el.getAttribute(PARKED));
      el.removeAttribute(PARKED);
    }
  }

  function sync() {
    var list = slides();
    var i;

    // Before bespoke has marked a slide active — and while printing, where the
    // deck's own @media print rules select on [data-marpit-svg] — every slide
    // stays unparked. Parking is an optimization; it must never be the reason a
    // slide fails to render.
    var keepAll = printing || MULTI_SLIDE_VIEWS[document.body.getAttribute("data-bespoke-view")];
    if (!keepAll) {
      keepAll = true;
      for (i = 0; i < list.length; i++) {
        if (list[i].classList.contains(ACTIVE)) { keepAll = false; break; }
      }
    }

    for (i = 0; i < list.length; i++) {
      if (keepAll || list[i].classList.contains(ACTIVE)) unpark(list[i]);
      else park(list[i]);
    }
  }

  // bespoke moves the active class as you page, and sets data-bespoke-view when
  // you open overview or presenter view. Both are attribute writes, so one
  // observer covers navigation without polling. sync() only ever writes ATTR /
  // PARKED, which the filter excludes, so it cannot retrigger itself.
  new MutationObserver(sync).observe(document.documentElement, {
    subtree: true,
    attributes: true,
    attributeFilter: ["class", "data-bespoke-view"],
  });

  window.addEventListener("beforeprint", function () { printing = true; sync(); });
  window.addEventListener("afterprint", function () { printing = false; sync(); });

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", sync);
  } else {
    sync();
  }
})();
