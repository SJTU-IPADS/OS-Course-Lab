# solve-bomb -- fetch your bomb from the server of the course.
#
#   make bomb ID=525xxxxxxxxx      with your own student number
#
# The server builds the bomb that belongs to the number and sends it back
# as the file bomb. The number is shown on the screen of the bomb: check it
# before you start. Asking again gives the same bomb.
#
# The connection is TLS: curl and wget check the certificate of the server
# as they do for every https address.
#
# Needs curl or wget.

SERVER := https://bomb.hydroiodic.site:15213

bomb:
	@printf '%s' '$(ID)' | grep -Eqx '[0-9]{1,20}' || \
	    { echo 'usage: make bomb ID=525xxxxxxxxx   (your student number, digits only)' >&2; exit 2; }
	@rm -f bomb.part; \
	if command -v curl >/dev/null 2>&1; then \
	    code=$$(curl -sS -o bomb.part -w '%{http_code}' \
	        -d 'id=$(ID)' $(SERVER)/bomb) || code=000; \
	elif command -v wget >/dev/null 2>&1; then \
	    if wget -q --content-on-error -O bomb.part \
	        --post-data 'id=$(ID)' $(SERVER)/bomb; \
	    then code=200; else code=error; fi; \
	else \
	    echo 'make bomb: install curl or wget first' >&2; exit 1; \
	fi; \
	if [ "$$code" = 200 ]; then \
	    mv bomb.part bomb && chmod +x bomb && echo 'bomb: the bomb of $(ID)'; \
	else \
	    echo "make bomb: no bomb from $(SERVER) ($$code)" >&2; \
	    [ ! -s bomb.part ] || { head -c 300 bomb.part; echo; } >&2; \
	    rm -f bomb.part; exit 1; \
	fi

.PHONY: bomb
