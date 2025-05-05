# 🛠️ autoheal

**autoheal** is a fast and lightweight C utility for preprocessing images by segmenting them into color-based layers using K-Means clustering, smoothing the resulting masks, and exporting each layer as a separate grayscale PNG.

It was developed to support pipelines that require clean vector-friendly regions, such as SVG generation for AI-generated images.

---

## 🚀 Features

- Fast in-memory K-Means clustering over sampled pixels
- Automatic small-region healing via flood-fill + neighborhood voting
- Morphological smoothing (erode → dilate)
- Outputs:
  - `layer_<id>_r<r>_g<g>_b<b>.png`: binary mask PNGs per color region
  - `palette.json`: maps layer IDs to their RGB colors

---

## 🧪 Example Usage

```bash
./autoheal --width 512 --height 512 --clusters 4
````

The program expects an `input.png` in the current directory, sized exactly to the given dimensions.

---

## 🧰 Dependencies

This tool uses [stb\_image](https://github.com/nothings/stb) and [stb\_image\_write](https://github.com/nothings/stb) headers for image I/O. These are included in the `include/` folder and require no external installation.

---

## 📦 Build Instructions

```bash
gcc autoheal.c -O3 -o autoheal
```

Or with Clang:

```bash
clang autoheal.c -O3 -o autoheal
```

Ensure `include/stb_image.h` and `include/stb_image_write.h` are available.

---

## 📁 Output Files

* `layer_0_r255_g0_b0.png`, etc. — individual binary masks (after smoothing)
* `palette.json` — mapping of cluster IDs to their RGB values

---

## 📚 Citation

If you use `autoheal` in academic work or derivative projects, please cite:

> Gompert, J. (2025). *autoheal: Automatic mask cleaner for image preprocessing* \[C source code]. GitHub. [https://github.com/ginkorea/autoheal](https://github.com/ginkorea/autoheal)

---

## 📝 License

MIT License. See [LICENSE](./LICENSE).

---

## 🧠 Motivation

Originally developed to support vectorization in pipelines using AI-generated images (e.g. Kandinsky 2.2 → SVG), `autoheal` improves contour clarity and region stability for postprocessing steps such as `potrace`.

---

## 🙋‍♂️ Maintainer

**Josh Gompert**
GitHub: [@ginkorea](https://github.com/ginkorea)


