from pathlib import Path
from PIL import Image

KB = 1024
MB = KB * 1024

while True:
    found_outlier = False

    for image_path in Path("docs", "images").glob("**/*"):
        size = image_path.stat().st_size
        if size > 1 * MB:
            found_outlier = True
            print(f"{image_path} is over threshold at {size/MB:.2f} MiB")
            compressed_path_tmp = Path(str(image_path) + ".tmp")
            image = Image.open(image_path)
            image.save(compressed_path_tmp, "JPEG", optimize=True, quality=30)

            compressed_path_tmp.rename(image_path)
    exit(0)
    
    if not found_outlier:
        break
