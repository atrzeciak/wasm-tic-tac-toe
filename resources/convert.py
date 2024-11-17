import sys
from pathlib import Path

from PIL import Image

def convert_png_to_h(png_file: Path):
    """Converts a png image to include"""
    try:
        img = Image.open(png_file)
        bmp_file = png_file.with_suffix(".bmp")
        img.save(bmp_file, "BMP")
        print(f"Successfully converted {png_file} to {bmp_file}")

        target_name = str(bmp_file.name).replace('.', '_').replace('-', '_')
        h_file = png_file.with_name(target_name).with_suffix(".h")
        with open(h_file, "w") as fout:
            print(f"static const unsigned char {target_name}[] = {{", file=fout)
            print(f"{','.join(hex(b) for b in open(bmp_file, 'rb').read())}", file=fout)
            print("};", file=fout)
            print(f"Successfully converted {png_file} to {h_file}")
        bmp_file.unlink()
    except Exception as exc:
        print(f"Error converting image: {exc}")

def convert_ttf_to_h(ttf_file: Path):
    """Converts a ttf image to include"""
    try:
        target_name = str(ttf_file.name).replace('.', '_').replace('-', '_')
        h_file = ttf_file.with_name(target_name).with_suffix(".h")
        with open(h_file, "w") as fout:
            print(f"static const unsigned char {target_name}[] = {{", file=fout)
            print(f"{','.join(hex(b) for b in open(ttf_file, 'rb').read())}", file=fout)
            print("};", file=fout)
            print(f"Successfully converted {ttf_file} to {h_file}")
        #bmp_file.unlink()

    except Exception as exc:
        print(f"Error converting image: {exc}")

def main(file_path: Path):
    """!main"""
    if file_path.suffix == ".png":
        convert_png_to_h(file_path)
        return 0
    if file_path.suffix == ".ttf":
        convert_ttf_to_h(file_path)
        return 0
    return 1

if __name__ == "__main__":
    main(Path(sys.argv[1]))
