// The engine lecturekit runs marp-cli with: Marp itself, taught the dialects
// of lecturekit/dialects.json (see lecturekit/dialects.py for what one is).
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
  return marp
}
