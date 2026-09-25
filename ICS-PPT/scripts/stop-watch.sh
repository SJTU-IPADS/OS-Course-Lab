#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage: stop-watch.sh [PORT]" >&2
}

if [ "$#" -gt 1 ]; then
  usage
  exit 2
fi

target_port="${1:-}"
if [[ "$target_port" == *[!0-9]* ]]; then
  usage
  exit 2
fi

watch_port() {
  local cmd="$1"
  if [[ "$cmd" =~ --port(=|[[:space:]]+)([0-9]+) ]]; then
    echo "${BASH_REMATCH[2]}"
  else
    echo "3030"
  fi
}

# True only for a real Python interpreter running `lecturekit.cli view` -- not a
# shell whose argv merely mentions the command (e.g. the very shell that
# launched the watcher, or `bash -c '... lecturekit.cli view --watch ...'`).
# The executable is the first whitespace-delimited token; we match on its
# basename so `/usr/bin/python3` and `.../MacOS/Python` both qualify while
# `/bin/zsh` does not. Matching the shell wrapper is what used to make the
# script kill the wrong process.
is_lecturekit_view() {
  local cmd="$1"
  local exe="${cmd%% *}"
  case "${exe##*/}" in
    [Pp]ython | [Pp]ython[0-9]*) ;;
    *) return 1 ;;
  esac
  [[ "$cmd" == *"lecturekit.cli view"* ]]
}

add_unique_port() {
  local port="$1"
  local seen
  for seen in "${ports[@]-}"; do
    if [ "$seen" = "$port" ]; then
      return
    fi
  done
  ports+=("$port")
}

join_ports() {
  local joined=""
  local port
  for port in "$@"; do
    if [ -z "$joined" ]; then
      joined="$port"
    else
      joined="$joined, $port"
    fi
  done
  echo "$joined"
}

# Echo the subset of the given pids that are still alive (empty if all gone).
running_of() {
  local pid
  local -a out=()
  for pid in "$@"; do
    if kill -0 "$pid" 2>/dev/null; then
      out+=("$pid")
    fi
  done
  echo "${out[*]-}"
}

# Poll up to ~5s for the given pids to exit; echo whatever is still running.
wait_until_gone() {
  local _ left
  for _ in {1..50}; do
    left="$(running_of "$@")"
    if [ -z "$left" ]; then
      echo ""
      return 0
    fi
    sleep 0.1
  done
  running_of "$@"
}

declare -a pids=()
declare -a ports=()
if ! process_list="$(ps -axo pid=,command=)"; then
  echo "stop-watch.sh: failed to list processes" >&2
  exit 1
fi

# `read -r pid cmd` splits on the default IFS: pid is the first field, cmd is the
# rest of the line. This also absorbs ps's right-justified column padding -- a
# pid narrower than the column comes back space-padded, and a manual
# `${line%% *}` split would then take that leading space as the pid and lose the
# real one, so nothing ever matched.
while read -r pid cmd; do
  if [ "$pid" = "$$" ]; then
    continue
  fi
  if ! is_lecturekit_view "$cmd"; then
    continue
  fi
  port="$(watch_port "$cmd")"
  if [ -n "$target_port" ] && [ "$port" != "$target_port" ]; then
    continue
  fi
  pids+=("$pid")
  add_unique_port "$port"
done <<< "$process_list"

if [ "${#pids[@]}" -eq 0 ]; then
  if [ -n "$target_port" ]; then
    echo "stop-watch.sh: no lecturekit watch process found on port $target_port"
  else
    echo "stop-watch.sh: no lecturekit watch process found"
  fi
  exit 0
fi

port_list="$(join_ports "${ports[@]}")"
echo "stop-watch.sh: stopping lecturekit watch process(es) on port(s) $port_list: ${pids[*]}"

# Escalate SIGINT -> SIGTERM -> SIGKILL, verifying between each so nothing
# survives (a backgrounded watcher inherits SIG_IGN for SIGINT, so INT alone can
# be a no-op). SIGINT first lets the watcher's own shutdown run, which takes down
# its marp worker and releases the HTTP port cleanly.
kill -INT "${pids[@]}" 2>/dev/null || true
running="$(wait_until_gone "${pids[@]}")"

if [ -n "$running" ]; then
  echo "stop-watch.sh: still running after interrupt, sending TERM: $running" >&2
  # shellcheck disable=SC2086
  kill -TERM $running 2>/dev/null || true
  # shellcheck disable=SC2086
  running="$(wait_until_gone $running)"
fi

if [ -n "$running" ]; then
  echo "stop-watch.sh: still running after TERM, sending KILL: $running" >&2
  # shellcheck disable=SC2086
  kill -KILL $running 2>/dev/null || true
  # shellcheck disable=SC2086
  running="$(wait_until_gone $running)"
fi

# Backstop: if a watcher was hard-killed its shutdown never ran, orphaning the
# `... marp slides.md ... --watch` worker that holds marp's fixed live-reload
# port. Reap those so the port is freed too. Only in kill-all mode (no PORT
# filter), since marp's port is a single shared one and we don't map it per HTTP
# port.
if [ -z "$target_port" ]; then
  declare -a marp_pids=()
  while read -r pid cmd; do
    if [ "$pid" = "$$" ]; then
      continue
    fi
    if [[ "$cmd" == *marp* && "$cmd" == *slides.md* && "$cmd" == *"--watch"* ]]; then
      marp_pids+=("$pid")
    fi
  done <<< "$process_list"

  if [ "${#marp_pids[@]}" -gt 0 ]; then
    echo "stop-watch.sh: reaping orphaned marp watch worker(s): ${marp_pids[*]}"
    kill -TERM "${marp_pids[@]}" 2>/dev/null || true
    m_running="$(wait_until_gone "${marp_pids[@]}")"
    if [ -n "$m_running" ]; then
      # shellcheck disable=SC2086
      kill -KILL $m_running 2>/dev/null || true
    fi
  fi
fi

if [ -n "$running" ]; then
  echo "stop-watch.sh: could not stop: $running" >&2
  exit 1
fi
echo "stop-watch.sh: stopped"
