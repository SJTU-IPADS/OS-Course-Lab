import unittest

from lecturekit import Lecture
from lecturekit.model import Block, Page, ValidationError, check_block
from lecturekit.renderers.viewer import render_marp_page
from lecturekit.renderers.viewer.blocks import render_block
from lecturekit.serialize import lecture_to_dict


def _arch_content(layers, *, caption=None, flow=None, ref=None):
    return {"caption": caption, "flow": flow, "layers": layers, "ref": ref}


class ArchitectureAuthoringTest(unittest.TestCase):
    """p.architecture(...) + arch.layer(...) builds one architecture block."""

    def _block(self, body):
        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        return lecture.build().children[0].blocks[0]

    def test_architecture_builds_layers_and_modules(self):
        def body(p):
            p.title("W")
            arch = p.architecture(caption="OS stack", flow="down")
            arch.layer("Application", ["Shell", "Editor"])
            arch.layer("Kernel", ["Scheduler", "VM"])

        block = self._block(body)
        self.assertEqual(block.kind, "architecture")
        self.assertEqual(
            block.content,
            _arch_content(
                [
                    {"title": "Application", "modules": ["Shell", "Editor"]},
                    {"title": "Kernel", "modules": ["Scheduler", "VM"]},
                ],
                caption="OS stack",
                flow="down",
            ),
        )

    def test_architecture_defaults_caption_and_flow_to_none(self):
        def body(p):
            p.title("W")
            p.architecture().layer("L", ["m"])

        block = self._block(body)
        self.assertIsNone(block.content["caption"])
        self.assertIsNone(block.content["flow"])

    def test_layer_chains(self):
        def body(p):
            p.title("W")
            p.architecture().layer("A", ["x"]).layer("B", ["y"])

        block = self._block(body)
        self.assertEqual([l["title"] for l in block.content["layers"]], ["A", "B"])

    def test_layer_coerces_modules_to_str(self):
        def body(p):
            p.title("W")
            p.architecture().layer("A", [1, 2])

        block = self._block(body)
        self.assertEqual(block.content["layers"][0]["modules"], ["1", "2"])

    def test_empty_layer_title_allowed(self):
        def body(p):
            p.title("W")
            p.architecture().layer(None, ["x"])

        block = self._block(body)
        self.assertIsNone(block.content["layers"][0]["title"])

    def test_layer_ellipsis_literal_marks_omitted_modules(self):
        def body(p):
            p.title("W")
            p.architecture().layer("Kernel", ["Scheduler", ...])

        block = self._block(body)
        self.assertEqual(block.content["layers"][0]["modules"], ["Scheduler", "…"])

    def test_layer_ellipsis_string_also_marks_omitted(self):
        def body(p):
            p.title("W")
            p.architecture().layer("Kernel", ["Scheduler", "...", "…"])

        block = self._block(body)
        self.assertEqual(
            block.content["layers"][0]["modules"], ["Scheduler", "…", "…"]
        )

    def test_architecture_chains_footnote(self):
        def body(p):
            p.title("W")
            p.architecture().layer("A", ["x"]).footnote("source")

        block = self._block(body)
        self.assertEqual(block.footnotes, ("source",))

    def test_architecture_rejects_bad_flow_at_authoring(self):
        def body(p):
            p.title("W")
            p.architecture(flow="sideways")

        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()


class ArchitectureValidationTest(unittest.TestCase):
    def test_accepts_a_well_formed_block(self):
        block = Block(
            kind="architecture",
            content=_arch_content(
                [{"title": "A", "modules": ["x"]}], flow="down"
            ),
        )
        check_block(block, page_id="p")  # does not raise

    def test_rejects_zero_layers(self):
        block = Block(kind="architecture", content=_arch_content([]))
        with self.assertRaises(ValidationError):
            check_block(block, page_id="p")

    def test_rejects_layer_with_no_modules(self):
        block = Block(
            kind="architecture",
            content=_arch_content([{"title": "A", "modules": []}]),
        )
        with self.assertRaises(ValidationError):
            check_block(block, page_id="p")

    def test_rejects_blank_module(self):
        block = Block(
            kind="architecture",
            content=_arch_content([{"title": "A", "modules": ["  "]}]),
        )
        with self.assertRaises(ValidationError):
            check_block(block, page_id="p")

    def test_rejects_bad_flow(self):
        block = Block(
            kind="architecture",
            content=_arch_content([{"title": "A", "modules": ["x"]}], flow="nope"),
        )
        with self.assertRaises(ValidationError):
            check_block(block, page_id="p")


class ArchitectureSerializeTest(unittest.TestCase):
    def test_content_is_dumped(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.architecture(caption="c", flow="up").layer("A", ["x"])

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertEqual(block["kind"], "architecture")
        self.assertEqual(
            block["content"],
            _arch_content([{"title": "A", "modules": ["x"]}], caption="c", flow="up"),
        )


class ArchitectureRenderTest(unittest.TestCase):
    def _render(self, layers, **kw):
        block = Block(kind="architecture", content=_arch_content(layers, **kw))
        return "\n".join(render_block(block))

    def test_emits_layer_and_module_divs(self):
        html = self._render(
            [
                {"title": "Application", "modules": ["Shell", "Editor"]},
                {"title": "Kernel", "modules": ["Scheduler"]},
            ]
        )
        self.assertIn('<figure class="lk-arch">', html)
        self.assertEqual(html.count('class="lk-arch-layer"'), 2)
        self.assertEqual(html.count('class="lk-arch-module"'), 3)
        self.assertIn(">Application</div>", html)
        self.assertIn(">Shell</div>", html)

    def test_label_omitted_when_blank(self):
        html = self._render([{"title": None, "modules": ["x"]}])
        self.assertNotIn("lk-arch-label", html)

    def test_caption_rendered_only_when_set(self):
        with_cap = self._render([{"title": "A", "modules": ["x"]}], caption="Fig 1")
        self.assertIn("<figcaption>Fig 1</figcaption>", with_cap)
        without = self._render([{"title": "A", "modules": ["x"]}])
        self.assertNotIn("figcaption", without)

    def test_flow_connector_present_with_direction(self):
        html = self._render(
            [
                {"title": "A", "modules": ["x"]},
                {"title": "B", "modules": ["y"]},
            ],
            flow="down",
        )
        # one connector between the two layers, carrying the direction modifier
        self.assertEqual(html.count('<div class="lk-arch-flow'), 1)
        self.assertIn("lk-arch-flow--down", html)

    def test_no_flow_connector_when_flow_none(self):
        html = self._render(
            [
                {"title": "A", "modules": ["x"]},
                {"title": "B", "modules": ["y"]},
            ]
        )
        self.assertNotIn("lk-arch-flow", html)

    def test_ellipsis_module_gets_more_modifier(self):
        html = self._render([{"title": "Kernel", "modules": ["Scheduler", "…"]}])
        # the omitted-modules box carries a modifier so the theme can fade it
        self.assertIn(
            '<div class="lk-arch-module lk-arch-module--more">…</div>', html
        )
        # a normal module stays a plain module box
        self.assertIn('<div class="lk-arch-module">Scheduler</div>', html)

    def test_escapes_module_and_label_text(self):
        html = self._render([{"title": "A<b>", "modules": ["x&y"]}])
        self.assertIn("A&lt;b&gt;", html)
        self.assertIn("x&amp;y", html)

    def test_page_integration_emits_figure(self):
        page = Page(
            id="p",
            title="T",
            blocks=[
                Block(
                    kind="architecture",
                    content=_arch_content([{"title": "A", "modules": ["x"]}]),
                )
            ],
        )
        md = render_marp_page(page)
        self.assertIn('<figure class="lk-arch">', md)


if __name__ == "__main__":
    unittest.main()
