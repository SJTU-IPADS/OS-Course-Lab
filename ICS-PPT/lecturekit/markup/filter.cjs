// A filter: Marp markdown in, the HTML of the deck out, formulas as MathML.
//
//   echo '"cost is $O(n^2)$"' | node lecturekit/markup/filter.cjs
//
// One document per line, as a JSON string, and one JSON string back for each,
// in order, until the input ends. The parser is the deck's: the engine
// marp-cli runs, with `--html` as lecturekit passes it. Two output rules
// differ, the two that draw pictures for a browser: a formula is written as
// the MathML MathJax typesets it from (the deck has its SVG), an emoji as its
// character (the deck has a twemoji image). A formula MathJax cannot read
// keeps what the deck shows for it and is named on stderr.
const readline = require('node:readline')
const { Marp } = require('@marp-team/marp-core')
const { mathjax } = require('mathjax-full/js/mathjax.js')
const { TeX } = require('mathjax-full/js/input/tex.js')
const { AllPackages } = require('mathjax-full/js/input/tex/AllPackages.js')
const { SVG } = require('mathjax-full/js/output/svg.js')
const { liteAdaptor } = require('mathjax-full/js/adaptors/liteAdaptor.js')
const { RegisterHTMLHandler } = require('mathjax-full/js/handlers/html.js')
const { SerializedMmlVisitor } = require('mathjax-full/js/core/MmlTree/SerializedMmlVisitor.js')
const { STATE } = require('mathjax-full/js/core/MathItem.js')
const deckEngine = require('../renderers/viewer/marp-engine.cjs')

RegisterHTMLHandler(liteAdaptor())
// Set up as marp-core sets it up. Nothing is drawn with the output jax, but
// some of the packages (bussproofs) measure with it.
const math = mathjax.document('', {
  InputJax: new TeX({ packages: AllPackages }),
  OutputJax: new SVG({ fontCache: 'none' })
})
const serializer = new SerializedMmlVisitor()

function warn (problem, tex) {
  process.stderr.write(`lecturekit: math: ${problem}: $${tex.replace(/\s+/g, ' ').trim()}$\n`)
}

// Limits that move beside their operator outside display style. MathJax's
// output jax decides that as it draws, so it is decided here the same way.
const MOVED = { munder: 'msub', mover: 'msup', munderover: 'msubsup' }

function mathml (tex, display) {
  const root = math.convert(tex, { display, end: STATE.CONVERT })
  const moved = []
  root.walkTree(node => {
    if (MOVED[node.kind] && !node.attributes.get('displaystyle') &&
        node.childNodes[node.base].coreMO().attributes.get('movablelimits')) {
      moved.push(node)
    } else if (node.isKind('merror')) {
      warn(node.attributes.get('data-mjx-error'), tex)
    } else if (node.isKind('mtext') && node.attributes.getExplicit('mathcolor') === 'red' &&
               node.getText().startsWith('\\')) {
      warn(`undefined ${node.getText()}`, tex) // what the noundefined package leaves
    } else if (node.isKind('mo')) {
      // An operator's properties come from MathJax's dictionary. Written out,
      // a reader of the MathML needs no dictionary of its own.
      for (const name of ['largeop', 'stretchy', 'accent']) {
        if (node.attributes.get(name)) node.attributes.set(name, true)
      }
      if (node.texClass >= 0) node.setProperty('texClass', node.texClass)
    }
  })
  for (const node of moved) {
    const scripts = node.factory.create(MOVED[node.kind])
    node.parent.replaceChild(scripts, node)
    scripts.setChildren(node.childNodes)
  }
  return serializer.visitTree(root)
}

const marp = deckEngine({
  // `script: false` leaves out the browser script Marp appends to every deck.
  marp: new Marp({ html: true, script: false, emoji: { shortcode: true, unicode: true } })
})
const { rules } = marp.markdown.renderer
const { escapeHtml } = marp.markdown.utils

function formula (display) {
  return (tokens, i) => {
    const tex = tokens[i].content
    let html
    try {
      html = mathml(tex, display)
    } catch (error) { // the deck prints the source then
      warn(String(error.message || error), tex)
      html = escapeHtml(tex)
    }
    return display ? `<p>${html}</p>\n` : html
  }
}
rules.marp_math_inline = formula(false)
rules.marp_math_block = formula(true)

readline.createInterface({ input: process.stdin, crlfDelay: Infinity }).on('line', line => {
  // A document is a part of a deck and never its first line, the one place
  // where Marp reads dashes as front matter (it takes "- `--watch`" for that).
  const markdown = `\n${JSON.parse(line)}`
  process.stdout.write(`${JSON.stringify(marp.markdown.render(markdown))}\n`)
})
