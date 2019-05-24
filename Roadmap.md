# Roadmap
### Already implemented
- The basics of loading TTF files
- Loading simple glyph outlines
- A rasterizer with pixel-perfect anti-aliasing
- An example program outputting BMP files
- Sample code to mmap a file on Windows or Unix (in limbo right now)
- Example program with SDL2
- cmap format 4
- SIMD rasterizer optimization
- cmap format 6
- bmp example UTF8
- Total independence from the C stdlib
### To be done before v1.0
- cmap format 1
- cmap format 12
- Manual array bounds checking in the entire TTF loader
- Verifying min / max values in TTF data
- Text composing
- Kerning
- Output format controllable by a generous list of enums ala OpenGL
- Gamma Correction & dpi conversion
- take image stride
- CPU-dispatch code
- Alternate code paths for SIMD-ified functions
### Coming after v1.0
- avx2?
- Font Collections?
- Variable Fonts?
- Compound glyphs
- Vertical composing
- nostdlib example program maybe?
- UTF16 convenience functions
- A subset of the Unicode BiDi Algorithm
- Utilising GPOS, GSUB, JUSTF tables
- Optional sub-pixel rendering for LCD screens
- Ideomatic C++ wrapper
### Crackpot ideas that may or may not be worth the effort
- Replacing the entire rasterizer with a new one entirely based on Signed Distance Fields
- Hinting (huge undertaking, but at least not encumbered by patents anymore)
- Optional transparent caching layer over the entire API for easy speedups
- Other language wrappers?
- stb\_truetype compatibility layer
