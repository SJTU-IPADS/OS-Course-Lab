def make(title):
    def body(p):
        p.title(title)
        p.slide(f"- {title}")

    return body


alpha = make("Alpha")
beta = make("Beta")
gamma = make("Gamma")
delta = make("Delta")
