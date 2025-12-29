def define_env(env):
    @env.macro
    def backblaze_url(url: str) -> str:
        """Given a backblaze URL, return the Cloudflare CDN URL."""
        return url.replace("https://f005.backblazeb2.com/file/landons-blog/assets/posts/", "https://assets.topofmind.dev/posts/")

def on_post_page_macros(env):
    env.markdown = env.markdown.replace("https://f005.backblazeb2.com/file/landons-blog/assets/posts/", "https://assets.topofmind.dev/posts/")