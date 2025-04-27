
.PHONY: website middleware

website:
	python3 -m http.server --directory website

middleware:
	$(MAKE) -C node_proxy
