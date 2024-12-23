import posixpath
import re

from mkdocs.config.defaults import MkDocsConfig
from mkdocs.structure.files import Files
from mkdocs.structure.pages import Page
from re import Match

# @todo
def on_page_markdown(
    markdown: str, *, page: Page, config: MkDocsConfig, files: Files
) -> str:

    # Replace callback
    def replace(match: Match) -> str:
        return match.string + "{ .external }"

    # Find and replace all external asset URLs in current page
    return re.sub(
        r"[^!]\[[a-zA-Z 0-9\/]+\]\([a-z:\/.0-9A-Z&?%_#]+\)",
        replace, markdown, flags = re.I | re.M
    )
