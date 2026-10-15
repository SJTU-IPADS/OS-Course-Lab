// The engine lecturekit runs marp-cli with: Marp itself, taught the dialects
// of lecturekit/dialects.json (see lecturekit/dialects.py for what one is),
// and opening a web link of the slide text in a new tab.
//
// Marp highlights a fenced block with highlight.js and leaves a language it
// does not know uncoloured. A dialect is registered as its base language with
// the dialect's words added to the base's keyword lists, so ```cuda is C++
// that also knows __global__ and threadIdx.
const dialects = require('../../dialects.json')

const ROLES = ['keyword', 'built_in', 'type']

module.exports = ({ marp }) => {
  const hljs = marp.highlightjs
  for (const [name, dialect] of Object.entries(dialects)) {
    hljs.registerLanguage(name, () => {
      // rawDefinition() builds the base afresh, so the base itself is untouched.
      const language = hljs.getLanguage(dialect.hljs).rawDefinition()
      for (const role of ROLES) language.keywords[role].push(...(dialect[role] || []))
      language.name = name
      // The base's aliases (c++, hpp, ...) stay the base's.
      language.aliases = []
      return language
    })
  }

  // [label](https://...) in slide text: followed in place, the link would
  // take the deck away, so it opens in a new tab as p.link's does. Other
  // addresses (relative ones, #anchors) keep Marp's rendering.
  const rules = marp.markdown.renderer.rules
  const linkOpen = rules.link_open || ((tokens, i, options, env, self) => self.renderToken(tokens, i, options))
  rules.link_open = (tokens, i, options, env, self) => {
    if (/^https?:\/\//i.test(tokens[i].attrGet('href') || '')) {
      tokens[i].attrSet('target', '_blank')
      tokens[i].attrSet('rel', 'noopener noreferrer')
    }
    return linkOpen(tokens, i, options, env, self)
  }
  return marp
}
