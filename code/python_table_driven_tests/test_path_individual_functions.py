from .path import make_relative
from pathlib import Path


def test_make_relative_root_is_none():
    path = Path.home().joinpath("foobar")
    root = None
    assert make_relative(path=path, root=root) == Path("foobar")


def test_make_relative_root_is_home():
    path = Path.home().joinpath("foobar")
    root = Path.home()
    assert make_relative(path=path, root=root) == Path("foobar")


def test_make_relative_root_is_unrelated_directory():
    path = Path.home().joinpath("foobar")
    root = Path("/leetcode/sucks")
    expected = root.joinpath(path.relative_to("/"))
    assert make_relative(path=path, root=root) == expected


def test_make_relative_root_is_unrelated_directory_path_is_not_absolute():
    path = Path.home().joinpath("foobar").relative_to("/")
    root = Path("/leetcode/sucks")
    expected = root.joinpath(path)
    assert make_relative(path=path, root=root) == expected
