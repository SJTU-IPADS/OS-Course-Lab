# attack-elf -- your programs, and the answers you write for them.
#
#   make attack ID=525xxxxxxxxx    fetch the three programs of your number
#   make local1                    run answers/attack1.txt here, and judge it
#   make local2                    the same for phase 2
#   make local3
#   make remote1 ID=525xxxxxxxxx   send answers/attack1.txt to the server
#   make remote2 ID=525xxxxxxxxx
#   make remote3 ID=525xxxxxxxxx
#   make local                     the three locally
#   make remote                    the three to the server
#   make submit ID=525xxxxxxxxx    the name the handout used to use: make remote
#   make score                     the board, as the programs show it to you
#
# make attack compiles unhex and asmhex, the writers of attack bytes, from the
# sources in tools/ -- into tools/, next to them: they are not part of the
# challenge and nothing about them is secret -- and brings the three programs
# of your number from the server. The same command again gives the same bytes.
# The number is on the banner of every program: check it before you start.
#
# An answer is hex text in answers/attackN.txt (comments start with # and do
# not count). make localN feeds it to your own program on standard input and
# says whether the phase passed and how the run ended; make remoteN sends the
# same file through tools/submit.py, which reads it with unhex and posts the
# bytes. The server runs this number's own program on them and answers with
# the result and how the run ended. Phases 1 and 3 pass both sides with one
# answer. Phase 2 has a canary the server pins per number while your program
# carries a fixed local one, so answers/attack2.txt passes one side at a time
# (README.md, 阶段 2, says which value is which).
#
# The programs talk to the server only for the board (-s). The connection is
# TLS: curl checks the certificate of the server as it does for every https
# address. Needs make, a C compiler (cc or gcc), curl and python3.

SERVER := bomb.hydroiodic.site:15214
SCHEME := https
CC ?= cc
ID ?=
ANSWERS := answers

# The three programs the server sends for one number.
FILES := attack1:1 attack2:2 attack3:3

.PHONY: attack score submit local remote local1 local2 local3 \
        remote1 remote2 remote3 clean check-id

check-id:
	@printf '%s' '$(ID)' | grep -Eqx '[0-9]{1,32}' \
	    || { echo "usage: make attack ID=525xxxxxxxxx   (your student number, digits only)" >&2; exit 2; }

# The two writers of attack bytes, built here from their sources into tools/:
# hex text in one end, raw bytes out; assembly in, the bytes it assembles to
# out. Nothing about them is secret, so they ship as source and are compiled
# where you are.
tools/unhex: tools/unhex.c
	@$(CC) -O2 -std=c11 -Wall -o $@ $<

tools/asmhex: tools/asmhex.c
	@$(CC) -O2 -std=c11 -Wall -o $@ $<

attack: tools/unhex tools/asmhex check-id
	@set -e; for pair in $(FILES); do \
	    f=$${pair%%:*}; p=$${pair##*:}; \
	    echo "fetching $$f for student $(ID) from $(SERVER) ..."; \
	    code=$$(printf 'id=%s\nphase=%s\n' '$(ID)' "$$p" \
	        | curl -sS -o $$f.part -w '%{http_code}' --data-binary @- "$(SCHEME)://$(SERVER)/build") \
	        || { echo "could not reach $(SERVER), or the build failed" >&2; rm -f $$f.part; exit 1; }; \
	    if [ "$$code" = 200 ]; then \
	        mv $$f.part $$f && chmod +x $$f && echo "wrote ./$$f"; \
	    else \
	        echo "no $$f from $(SERVER) (HTTP $$code)" >&2; \
	        [ ! -s $$f.part ] || { head -c 300 $$f.part; echo; } >&2; \
	        rm -f $$f.part; exit 1; \
	    fi; \
	done
	@echo "now put q4_0.gguf in this directory (see README.md), then run:  ./attack1"

# The board comes from the programs: they carry the number of this student.
score:
	@test -x ./attack1 || { echo "make attack ID=<your student number> first" >&2; exit 2; }
	@./attack1 -s

# The shell of one local run: $(1) is the phase. The pass line is the phase's
# own; the verdict names how the run ended, the way the server words it.
define local_run
out=$$( (tools/unhex < $(ANSWERS)/attack$(1).txt | ./attack$(1) -q) 2>&1 ); st=$$?; \
printf '%s\n' "$$out" | grep -q 'phase $(1) passed' && r=pass || r=fail; \
case $$st in \
  0) s='exit 0' ;; \
  *) if [ $$st -gt 128 ]; then s="signal $$((st - 128))"; else s="exit $$st"; fi ;; \
esac; \
[ -z "$$out" ] || printf '%s\n' "$$out"; \
echo "phase $(1): $$r ($$s)"; \
[ "$$r" = pass ]
endef

local1: tools/unhex
	@test -x ./attack1 || { echo "no ./attack1 here: run make attack ID=<your student number> first" >&2; exit 2; }
	@test -e q4_0.gguf || { echo "put q4_0.gguf in this directory first (README.md says where it comes from)" >&2; exit 2; }
	@test -s $(ANSWERS)/attack1.txt || { echo "no bytes in $(ANSWERS)/attack1.txt yet" >&2; exit 1; }
	@$(call local_run,1)

local2: tools/unhex
	@test -x ./attack2 || { echo "no ./attack2 here: run make attack ID=<your student number> first" >&2; exit 2; }
	@test -e q4_0.gguf || { echo "put q4_0.gguf in this directory first (README.md says where it comes from)" >&2; exit 2; }
	@test -s $(ANSWERS)/attack2.txt || { echo "no bytes in $(ANSWERS)/attack2.txt yet" >&2; exit 1; }
	@$(call local_run,2)

local3: tools/unhex
	@test -x ./attack3 || { echo "no ./attack3 here: run make attack ID=<your student number> first" >&2; exit 2; }
	@test -e q4_0.gguf || { echo "put q4_0.gguf in this directory first (README.md says where it comes from)" >&2; exit 2; }
	@test -s $(ANSWERS)/attack3.txt || { echo "no bytes in $(ANSWERS)/attack3.txt yet" >&2; exit 1; }
	@$(call local_run,3)

# All three, each judged before the status is reported.
local: tools/unhex
	@bad=0; for p in 1 2 3; do \
	    $(MAKE) --no-print-directory local$$p || bad=1; \
	done; \
	exit $$bad

# The answers go to the server as bytes; tools/submit.py reads the directory,
# calls unhex and posts each file. Without a phase number all three are sent.
remote: check-id tools/unhex
	@python3 tools/submit.py --scheme $(SCHEME) --server $(SERVER) $(ID)

remote1: check-id tools/unhex
	@python3 tools/submit.py --scheme $(SCHEME) --server $(SERVER) $(ID) 1

remote2: check-id tools/unhex
	@python3 tools/submit.py --scheme $(SCHEME) --server $(SERVER) $(ID) 2

remote3: check-id tools/unhex
	@python3 tools/submit.py --scheme $(SCHEME) --server $(SERVER) $(ID) 3

clean:
	rm -f attack1 attack2 attack3 tools/unhex tools/asmhex *.part
