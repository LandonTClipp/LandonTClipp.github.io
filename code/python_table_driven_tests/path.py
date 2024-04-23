import pathlib


def make_relative(path: pathlib.Path, root: pathlib.Path | None = None) -> pathlib.Path:
    """
    Make path relative to root. If root is None, set it to the home directory. If path
    can't be made relative to root, add path to root after making path relative to
    "/".
    """
    if root is None:
        root = pathlib.Path.home()
    try:
        return path.relative_to(root)
    except ValueError:
        pass

    if path.is_absolute():
        path = path.relative_to("/")

    return root.joinpath(path)
