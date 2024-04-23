from .path import make_relative
from pathlib import Path
import pytest


@pytest.mark.parametrize(
    ("path", "root", "expected"),
    [
        (Path.home().joinpath("foobar"), None, Path("foobar")),
        (Path.home().joinpath("foobar"), Path.home(), Path("foobar")),
        (
            Path.home().joinpath("foobar"),
            Path("/leetcode/sucks"),
            Path("/leetcode/sucks").joinpath(
                Path.home().joinpath("foobar").relative_to("/")
            ),
        ),
        (
            Path.home().joinpath("foobar").relative_to("/"),
            Path("/leetcode/sucks"),
            Path("/leetcode/sucks").joinpath(
                Path.home().joinpath("foobar").relative_to("/")
            ),
        ),
    ],
)
def test_make_relative(path: Path, root: Path | None, expected: Path) -> None:
    assert make_relative(path=path, root=root) == expected
