from lecturekit import dsl


def _body(p):
    p.title("Hi")
    p.slide("body")


def test_lecture_page_returns_handle():
    lec = dsl.Lecture(id="live", title="A Story")
    handle = lec.page("pg", body=_body)
    assert isinstance(handle, dsl.PageHandle)
    assert handle.spec.id == "pg"
    assert handle.lecture is lec


def test_section_page_returns_handle_with_owning_lecture():
    lec = dsl.Lecture(id="live", title="A Story", ratio="4:3")
    sec = lec.section("Case")
    handle = sec.page("pg", body=_body)
    assert isinstance(handle, dsl.PageHandle)
    assert handle.lecture is lec
    assert handle.lecture.ratio == "4:3"


def test_nested_section_page_threads_lecture():
    lec = dsl.Lecture(id="live", title="A Story")
    inner = lec.section("Outer").section("Inner")
    handle = inner.page("pg", body=_body)
    assert handle.lecture is lec


def test_assets_param_defaults_none_and_stores():
    assert dsl.Lecture(id="a", title="t").assets is None
    assert dsl.Lecture(id="a", title="t", assets=".").assets == "."


def test_existing_return_ignoring_usage_still_builds():
    lec = dsl.Lecture(id="live", title="A Story")
    sec = lec.section("Case")
    sec.page("pg", body=_body)          # return value ignored, as today
    built = lec.build()
    assert built.children[0].children[0].id == "pg"


def test_repr_html_delegates_to_notebook(monkeypatch):
    from lecturekit import notebook
    seen = {}

    def fake(lecture, spec, **kw):
        seen["args"] = (lecture, spec)
        return "<iframe>OK</iframe>"

    monkeypatch.setattr(notebook, "render_page_html", fake)
    lec = dsl.Lecture(id="live", title="A Story")
    handle = lec.page("pg", body=_body)
    assert handle._repr_html_() == "<iframe>OK</iframe>"
    assert seen["args"][0] is lec
    assert seen["args"][1] is handle.spec
