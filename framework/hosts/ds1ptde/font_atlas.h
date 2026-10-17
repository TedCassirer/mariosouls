#pragma once
// PTDE Windows font rasterization; the renderer receives only RGBA pixels.
static std::vector<uint8_t> presentationFontPixels;

static bool presentationFontCreate() {
    if (!presentationFontPixels.empty())
        return true;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 256;
    info.bmiHeader.biHeight = -128;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc)
        return false;
    void *pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HFONT font =
        CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH, "Consolas");
    if (!bitmap || !font) {
        if (bitmap)
            DeleteObject(bitmap);
        if (font)
            DeleteObject(font);
        DeleteDC(dc);
        return false;
    }
    auto oldBitmap = SelectObject(dc, bitmap), oldFont = SelectObject(dc, font);
    memset(pixels, 0, 256 * 128 * 4);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    for (int c = 32; c < 127; c++) {
        char s = char(c);
        TextOutA(dc, (c % 16) * 16, (c / 16) * 16, &s, 1);
    }
    GdiFlush();
    presentationFontPixels.resize(256 * 128 * 4);
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 256; x++) {
            uint32_t value = ((uint32_t *)pixels)[y * 256 + x];
            auto p = &presentationFontPixels[(y * 256 + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = uint8_t(value & 255);
        }
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return true;
}

static SCRenderAPI::Texture presentationFont() {
    return {presentationFontPixels.data(), 256, 128};
}
