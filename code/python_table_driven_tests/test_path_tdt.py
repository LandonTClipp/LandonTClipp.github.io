from .path import make_relative
from pathlib import Path
from unittest import TestCase
import dataclasses


class TestMakeRelative(TestCase):  # (1)!
    def test_make_relative(self) -> None:
        @dataclasses.dataclass  # (2)!
        class Params:
            path: Path
            expected: Path
            root: Path | None = None

        for test in [
            Params(
                path=Path.home().joinpath("foobar"), root=None, expected=Path("foobar")
            ),
            Params(
                path=Path.home().joinpath("foobar"),
                root=Path.home(),
                expected=Path("foobar"),
            ),
            Params(
                path=Path.home().joinpath("foobar"),
                root=Path("/leetcode/sucks"),
                expected=Path("/leetcode/sucks").joinpath(
                    Path.home().joinpath("foobar").relative_to("/")
                ),
            ),
            Params(
                path=Path.home().joinpath("foobar").relative_to("/"),
                root=Path("/leetcode/sucks"),
                expected=Path("/leetcode/sucks").joinpath(
                    Path.home().joinpath("foobar").relative_to("/")
                ),
            ),
            Params(
                path=Path.home().joinpath("foobar"), root=None, expected=Path("haha")
            ),
        ]:
            with self.subTest(**dataclasses.asdict(test)):  # (3)!
                assert test.expected == make_relative(path=test.path, root=test.root)
