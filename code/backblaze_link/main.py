#!/usr/bin/env python3
"""
Convert JPEG files in a directory to markdown image links.
"""
import argparse
from pathlib import Path
from urllib.parse import quote


def generate_markdown_links(directory_path, base_url=None, post_slug=None):
    """
    Generate markdown image links for all JPEG files in the directory.
    
    Args:
        directory_path: Path to directory containing JPEG files
        base_url: Base URL for the links (default: backblaze bucket)
        post_slug: Post slug for the URL path (default: derived from directory name)
    """
    # Default base URL
    if base_url is None:
        base_url = "https://f005.backblazeb2.com/file/landons-blog/assets/posts"
    
    # Convert to Path object
    dir_path = Path(directory_path).expanduser().resolve()
    
    if not dir_path.exists():
        print(f"Error: Directory '{directory_path}' does not exist")
        return
    
    if not dir_path.is_dir():
        print(f"Error: '{directory_path}' is not a directory")
        return
    
    # Derive post slug from directory name if not provided
    if post_slug is None:
        post_slug = "2025-12-09-san-juan-mountains"  # default from example
    
    # Find all JPEG files
    jpeg_files = sorted([
        f for f in dir_path.iterdir() 
        if f.is_file() and f.suffix.lower() in ['.jpeg', '.jpg', '.mov', '.mp4']
    ])
    
    if not jpeg_files:
        print(f"No JPEG files found in '{directory_path}'")
        return
    
    print(f"Found {len(jpeg_files)} JPEG file(s):\n")
    
    print("<div class=\"grid cards\" markdown>\n")
    # Generate markdown links
    for jpeg_file in jpeg_files:
        # URL encode the filename (spaces become +)
        name = jpeg_file.name.replace(' ', '+')
        encoded_name = quote(name, safe='+')
        
        # Construct the full URL
        url = f"{base_url}/{post_slug}/{encoded_name}"
        
        # Generate markdown
        if jpeg_file.suffix.lower() in ['.mov', '.mp4']:
            markdown = f"""- <video controls>
  <source id="mov" src="{url}" type="video/mp4">
</video>"""
        else:
            markdown = f"- ![]({url}){{ loading=lazy }}"
        print(markdown)
    print("\n</div>\n")


def main():
    parser = argparse.ArgumentParser(
        description="Generate markdown image links for JPEG files in a directory"
    )
    parser.add_argument(
        "directory",
        help="Path to directory containing JPEG files"
    )
    parser.add_argument(
        "--base-url",
        help="Base URL for the links (default: backblaze bucket)",
        default=None
    )
    parser.add_argument(
        "--post-slug",
        help="Post slug for URL path (default: 2025-12-09-san-juan-mountains)",
        default=None
    )
    
    args = parser.parse_args()
    generate_markdown_links(args.directory, args.base_url, args.post_slug)


if __name__ == "__main__":
    main()