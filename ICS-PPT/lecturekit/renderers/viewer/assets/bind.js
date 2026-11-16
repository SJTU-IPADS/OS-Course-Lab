(function (root) {
  "use strict";

  // The expression language of bound figures, as lecturekit/bind.py defines
  // it: the same grammar, functions and number formatting, so the figure the
  // viewer computes at the defaults is the file the figure script wrote.
  // `vars.js` uses it; nothing here touches the page. Errors are thrown as
  // BindError and leave the binding where it was.
  //
  // Values: numbers, strings, booleans, arrays. Arithmetic is IEEE, as in
  // Python's bind.py, which follows JavaScript here (1 / 0 is Infinity).

  var MINUS = "−";

  function BindError(message) {
    this.name = "BindError";
    this.message = message;
  }
  BindError.prototype = Object.create(Error.prototype);

  // ---------------------------------------------------------------- numbers

  function zeros(count) {
    var out = "";
    for (var i = 0; i < count; i++) out += "0";
    return out;
  }

  // The shortest digits that read back as `value`, without an exponent.
  function positional(value) {
    var text = String(value);
    if (text.indexOf("e") < 0) return text;
    var negative = text.charAt(0) === "-";
    if (negative) text = text.slice(1);
    var parts = text.split("e");
    var exponent = Number(parts[1]);
    var dot = parts[0].indexOf(".");
    var digits = parts[0].replace(".", "");
    var point = (dot < 0 ? parts[0].length : dot) + exponent;
    var out;
    if (point <= 0) out = "0." + zeros(-point) + digits;
    else if (point >= digits.length) out = digits + zeros(point - digits.length);
    else out = digits.slice(0, point) + "." + digits.slice(point);
    return (negative ? "-" : "") + out;
  }

  // `value` with `nd` decimals, a half rounded away from zero (toFixed).
  function fixed(value, nd) {
    var text = value.toFixed(nd);
    return text.indexOf("e") < 0 ? text : positional(value);
  }

  function unsignedZero(text) {
    return text.charAt(0) === "-" && Number(text) === 0 ? text.slice(1) : text;
  }

  function decimals(nd) {
    nd = number(nd, "fmt");
    if (nd !== Math.floor(nd) || nd < 0 || nd > 20) {
      throw new BindError("a number of decimals is a whole number from 0 to 20, got " + nd);
    }
    return nd;
  }

  function fmt(value, nd) {
    var n = number(value, "fmt");
    if (!isFinite(n)) return isNaN(n) ? "NaN" : (n > 0 ? "∞" : MINUS + "∞");
    var text = nd === undefined ? positional(n) : fixed(n, decimals(nd));
    return unsignedZero(text).replace("-", MINUS);
  }

  function attrNumber(value) {
    var text = fixed(value, 2);
    if (text.indexOf(".") >= 0) text = text.replace(/0+$/, "").replace(/\.$/, "");
    return unsignedZero(text);
  }

  // ---------------------------------------------------------------- parsing

  var TOKEN = /\s*(?:((?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)|('[^']*')|([A-Za-z_][A-Za-z0-9_]*)|(&&|\|\||==|!=|<=|>=|[-+*\/%^()\[\],?:<>!]))/y;

  function tokens(source) {
    var out = [];
    var pos = 0;
    while (true) {
      while (pos < source.length && /\s/.test(source.charAt(pos))) pos++;
      if (pos === source.length) break;
      TOKEN.lastIndex = pos;
      var m = TOKEN.exec(source);
      if (!m) throw new BindError("unexpected '" + source.charAt(pos) + "' at " + pos + " in '" + source + "'");
      if (m[1] !== undefined) out.push(["num", Number(m[1])]);
      else if (m[2] !== undefined) out.push(["str", m[2].slice(1, -1)]);
      else if (m[3] !== undefined) out.push(["name", m[3]]);
      else out.push(["op", m[4]]);
      pos = TOKEN.lastIndex;
    }
    out.push(["end", null]);
    return out;
  }

  var LEVELS = [["||"], ["&&"], ["==", "!=", "<", "<=", ">", ">="], ["+", "-"], ["*", "/", "%"]];

  function Parser(source) {
    this.source = source;
    this.tokens = tokens(source);
    this.at = 0;
  }

  Parser.prototype.peek = function () { return this.tokens[this.at]; };

  Parser.prototype.fail = function (what) {
    var token = this.peek();
    var found = token[0] === "end" ? "the end" : "'" + token[1] + "'";
    throw new BindError(what + found + " in '" + this.source + "'");
  };

  Parser.prototype.take = function (op) {
    var token = this.peek();
    if (op !== undefined && !(token[0] === "op" && token[1] === op)) this.fail("expected '" + op + "', found ");
    this.at++;
    return token;
  };

  Parser.prototype.accept = function (ops) {
    var token = this.peek();
    if (token[0] === "op" && ops.indexOf(token[1]) >= 0) {
      this.at++;
      return token[1];
    }
    return null;
  };

  Parser.prototype.whole = function () {
    var tree = this.expr();
    if (this.peek()[0] !== "end") this.fail("unexpected ");
    return tree;
  };

  Parser.prototype.expr = function () {
    var cond = this.binary(0);
    if (this.accept(["?"])) {
      var yes = this.expr();
      this.take(":");
      return ["cond", cond, yes, this.expr()];
    }
    return cond;
  };

  Parser.prototype.binary = function (level) {
    if (level === LEVELS.length) return this.unary();
    var left = this.binary(level + 1);
    while (true) {
      var op = this.accept(LEVELS[level]);
      if (op === null) return left;
      left = ["bin", op, left, this.binary(level + 1)];
      if (level === 2) {
        if (this.accept(LEVELS[level])) throw new BindError("comparisons do not chain in '" + this.source + "'");
        return left;
      }
    }
  };

  Parser.prototype.unary = function () {
    var op = this.accept(["-", "!"]);
    if (op) return [op === "-" ? "neg" : "not", this.unary()];
    var base = this.postfix();
    if (this.accept(["^"])) return ["bin", "^", base, this.unary()];
    return base;
  };

  Parser.prototype.postfix = function () {
    var tree = this.primary();
    while (this.accept(["["])) {
      var index = this.expr();
      this.take("]");
      tree = ["index", tree, index];
    }
    return tree;
  };

  Parser.prototype.primary = function () {
    var token = this.peek();
    if (token[0] === "num" || token[0] === "str") { this.at++; return [token[0], token[1]]; }
    if (token[0] === "name") {
      this.at++;
      if (this.accept(["("])) return ["call", token[1], this.items(")")];
      return ["name", token[1]];
    }
    if (this.accept(["("])) {
      var tree = this.expr();
      this.take(")");
      return tree;
    }
    if (this.accept(["["])) return ["list", this.items("]")];
    this.fail("unexpected ");
  };

  Parser.prototype.items = function (close) {
    var out = [];
    if (this.accept([close])) return out;
    while (true) {
      out.push(this.expr());
      if (this.accept([close])) return out;
      this.take(",");
    }
  };

  function parse(source) {
    return new Parser(source).whole();
  }

  // `source` cut at each ";" outside a string, empty pieces dropped.
  function split(source) {
    var pieces = [];
    var current = "";
    var quoted = false;
    for (var i = 0; i < source.length; i++) {
      var ch = source.charAt(i);
      if (ch === "'") quoted = !quoted;
      if (ch === ";" && !quoted) { pieces.push(current); current = ""; } else current += ch;
    }
    pieces.push(current);
    return pieces.filter(function (piece) { return /\S/.test(piece); });
  }

  function parseProgram(source) {
    return split(source || "").map(function (piece) {
      var m = /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)([\s\S]*)$/.exec(piece);
      if (!m) throw new BindError("expected 'name = expression', got '" + piece.trim() + "'");
      return [m[1], parse(m[2])];
    });
  }

  function parseAttrs(source) {
    return split(source || "").map(function (piece) {
      var m = /^\s*([A-Za-z_][-A-Za-z0-9_.]*)\s*:([\s\S]*)$/.exec(piece);
      if (!m) throw new BindError("expected 'attribute: expression', got '" + piece.trim() + "'");
      return [m[1], parse(m[2])];
    });
  }

  // ---------------------------------------------------------------- values

  function kind(value) {
    if (typeof value === "boolean") return "a truth value";
    if (typeof value === "string") return "the string '" + value + "'";
    if (Array.isArray(value)) return "a list";
    return String(value);
  }

  function number(value, what) {
    if (typeof value !== "number") throw new BindError(what + " takes a number, got " + kind(value));
    return value;
  }

  function list(value, what) {
    if (!Array.isArray(value)) throw new BindError(what + " takes a list, got " + kind(value));
    return value;
  }

  function truth(value) {
    if (typeof value === "boolean") return value;
    var n = number(value, "a condition");
    return n !== 0 && !isNaN(n);
  }

  var ARITH = {
    "+": function (a, b) { return a + b; },
    "-": function (a, b) { return a - b; },
    "*": function (a, b) { return a * b; },
    "/": function (a, b) { return a / b; },
    "%": function (a, b) { return a % b; },
    "^": function (a, b) { return Math.pow(a, b); }
  };

  var COMPARE = {
    "<": function (a, b) { return a < b; },
    "<=": function (a, b) { return a <= b; },
    ">": function (a, b) { return a > b; },
    ">=": function (a, b) { return a >= b; }
  };

  function arith(op, a, b) {
    var aList = Array.isArray(a);
    var bList = Array.isArray(b);
    if (aList || bList) {
      if (aList && bList) {
        if (a.length !== b.length) throw new BindError("'" + op + "' over lists of " + a.length + " and " + b.length + " items");
        return a.map(function (x, i) { return arith(op, x, b[i]); });
      }
      if (aList) return a.map(function (x) { return arith(op, x, b); });
      return b.map(function (y) { return arith(op, a, y); });
    }
    if (op === "+" && typeof a === "string" && typeof b === "string") return a + b;
    return ARITH[op](number(a, "'" + op + "'"), number(b, "'" + op + "'"));
  }

  function map(fn, name) {
    return function apply(value) {
      if (Array.isArray(value)) return value.map(apply);
      return fn(number(value, name));
    };
  }

  function extreme(pick, name) {
    return function () {
      var items = arguments.length === 1 && Array.isArray(arguments[0])
        ? arguments[0] : Array.prototype.slice.call(arguments);
      if (!items.length) throw new BindError(name + "() of nothing");
      var best = number(items[0], name);
      for (var i = 1; i < items.length; i++) best = pick(best, number(items[i], name));
      return best;
    };
  }

  function sum(values) {
    var total = 0;
    list(values, "sum").forEach(function (item) { total += number(item, "sum"); });
    return total;
  }

  // exp(x / t) normalised to sum to 1; at t = 0 the limit, all the weight on
  // the largest entries. The same operations in the same order as bind.py.
  function softmax(values, t) {
    var xs = list(values, "softmax").map(function (item) { return number(item, "softmax"); });
    if (!xs.length) throw new BindError("softmax() of an empty list");
    t = t === undefined ? 1 : number(t, "softmax");
    var top;
    if (t === 0) {
      top = Math.max.apply(null, xs);
      var hits = xs.map(function (x) { return x === top ? 1 : 0; });
      var count = sum(hits);
      return hits.map(function (hit) { return hit / count; });
    }
    var scaled = xs.map(function (x) { return x / t; });
    top = Math.max.apply(null, scaled);
    var weights = scaled.map(function (s) { return Math.exp(s - top); });
    var total = sum(weights);
    return weights.map(function (w) { return w / total; });
  }

  var FUNCTIONS = {
    abs: [map(Math.abs, "abs"), 1, 1],
    sqrt: [map(Math.sqrt, "sqrt"), 1, 1],
    exp: [map(Math.exp, "exp"), 1, 1],
    log: [map(Math.log, "log"), 1, 1],
    floor: [map(Math.floor, "floor"), 1, 1],
    ceil: [map(Math.ceil, "ceil"), 1, 1],
    round: [function (value, nd) {
      var n = number(value, "round");
      var places = decimals(nd === undefined ? 0 : nd);
      return isFinite(n) ? Number(fixed(n, places)) : n;
    }, 1, 2],
    min: [extreme(function (a, b) { return Math.min(a, b); }, "min"), 1, 64],
    max: [extreme(function (a, b) { return Math.max(a, b); }, "max"), 1, 64],
    pow: [function (a, b) { return arith("^", a, b); }, 2, 2],
    clamp: [function (v, lo, hi) {
      return Math.min(Math.max(number(v, "clamp"), number(lo, "clamp")), number(hi, "clamp"));
    }, 3, 3],
    sum: [sum, 1, 1],
    len: [function (values) { return list(values, "len").length; }, 1, 1],
    softmax: [softmax, 1, 2],
    fmt: [fmt, 1, 2]
  };

  function evaluate(tree, env) {
    var type = tree[0];
    if (type === "num" || type === "str") return tree[1];
    if (type === "name") {
      if (!Object.prototype.hasOwnProperty.call(env, tree[1])) throw new BindError("'" + tree[1] + "' is not defined");
      return env[tree[1]];
    }
    if (type === "list") return tree[1].map(function (item) { return evaluate(item, env); });
    if (type === "index") {
      var items = list(evaluate(tree[1], env), "indexing");
      var index = number(evaluate(tree[2], env), "an index");
      if (index !== Math.floor(index) || index < 0 || index >= items.length) {
        throw new BindError("index " + index + " is outside a list of " + items.length);
      }
      return items[index];
    }
    if (type === "call") {
      if (!Object.prototype.hasOwnProperty.call(FUNCTIONS, tree[1])) throw new BindError("no function '" + tree[1] + "'");
      var spec = FUNCTIONS[tree[1]];
      if (tree[2].length < spec[1] || tree[2].length > spec[2]) {
        throw new BindError(tree[1] + "() takes " + spec[1] + " to " + spec[2] + " arguments, got " + tree[2].length);
      }
      return spec[0].apply(null, tree[2].map(function (arg) { return evaluate(arg, env); }));
    }
    if (type === "neg") return arith("*", evaluate(tree[1], env), -1);
    if (type === "not") return !truth(evaluate(tree[1], env));
    if (type === "cond") return evaluate(truth(evaluate(tree[1], env)) ? tree[2] : tree[3], env);
    var op = tree[1];
    var a = evaluate(tree[2], env);
    if (op === "&&") return truth(a) && truth(evaluate(tree[3], env));
    if (op === "||") return truth(a) || truth(evaluate(tree[3], env));
    var b = evaluate(tree[3], env);
    if (op === "==" || op === "!=") {
      if (Array.isArray(a) || Array.isArray(b)) throw new BindError("'" + op + "' does not compare lists");
      var same;
      if (typeof a !== "number" || typeof b !== "number") same = typeof a === typeof b && a === b;
      else same = a === b;
      return op === "==" ? same : !same;
    }
    if (COMPARE[op]) return COMPARE[op](number(a, "'" + op + "'"), number(b, "'" + op + "'"));
    return arith(op, a, b);
  }

  function run(program, env) {
    var out = {};
    for (var name in env) out[name] = env[name];
    program.forEach(function (step) { out[step[0]] = evaluate(step[1], out); });
    return out;
  }

  function attrValue(value, what) {
    if (typeof value === "string") return value;
    var n = number(value, what);
    if (!isFinite(n)) throw new BindError(what + " is " + n + ", which no attribute can hold");
    return attrNumber(n);
  }

  function textValue(value, what) {
    return typeof value === "string" ? value : fmt(number(value, what));
  }

  var api = {
    BindError: BindError,
    parse: parse,
    parseProgram: parseProgram,
    parseAttrs: parseAttrs,
    evaluate: evaluate,
    run: run,
    fmt: fmt,
    attrValue: attrValue,
    textValue: textValue,
    positional: positional
  };
  root.lkBind = api;
  if (typeof module === "object" && module.exports) module.exports = api;
})(typeof window !== "undefined" ? window : this);
