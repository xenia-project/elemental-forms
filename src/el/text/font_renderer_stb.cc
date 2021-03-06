/**
 ******************************************************************************
 * Elemental Forms : a lightweight user interface framework                   *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved. Licensed as BSD 3-clause.   *
 * Portions ©2011-2015 Emil Segerås: https://github.com/fruxo/turbobadger     *
 ******************************************************************************
 */

#include "el/graphics/renderer.h"
#include "el/text/font_face.h"
#include "el/text/font_manager.h"
#include "el/text/font_renderer.h"

#ifdef EL_FONT_RENDERER_STB

namespace el {
namespace text {

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate
                                     // implementation
#include "third_party/stb/stb_truetype.h"

// SFontRenderer renders fonts using stb_truetype.h (http://nothings.org/).

using UCS4 = el::text::utf8::UCS4;

class SFontRenderer : public FontRenderer {
 public:
  SFontRenderer();
  ~SFontRenderer();

  bool Load(const char* filename, int size);

  std::unique_ptr<FontFace> Create(FontManager* font_manager,
                                   const std::string& filename,
                                   const FontDescription& font_desc) override;

  bool RenderGlyph(FontGlyphData* dst_bitmap, UCS4 cp) override;
  void GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) override;
  FontMetrics GetMetrics() override;

 private:
  stbtt_fontinfo font;
  unsigned char* ttf_buffer;
  unsigned char* render_data;
  int font_size;
  float scale;
};

SFontRenderer::SFontRenderer() : ttf_buffer(nullptr), render_data(nullptr) {}

SFontRenderer::~SFontRenderer() {
  delete[] ttf_buffer;
  delete[] render_data;
}

bool SFontRenderer::RenderGlyph(FontGlyphData* data, UCS4 cp) {
  delete[] render_data;
  render_data =
      stbtt_GetCodepointBitmap(&font, 0, scale, cp, &data->w, &data->h, 0, 0);
  data->data8 = render_data;
  data->stride = data->w;
  data->rgb = false;
  return data->data8 ? true : false;
}

void SFontRenderer::GetGlyphMetrics(GlyphMetrics* metrics, UCS4 cp) {
  int advanceWidth, leftSideBearing;
  stbtt_GetCodepointHMetrics(&font, cp, &advanceWidth, &leftSideBearing);
  metrics->advance = static_cast<int>(advanceWidth * scale + 0.5f);
  int ix0, iy0, ix1, iy1;
  stbtt_GetCodepointBitmapBox(&font, cp, 0, scale, &ix0, &iy0, &ix1, &iy1);
  metrics->x = ix0;
  metrics->y = iy0;
}

FontMetrics SFontRenderer::GetMetrics() {
  FontMetrics metrics;
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
  metrics.ascent = static_cast<int>(ascent * scale + 0.5f);
  metrics.descent = static_cast<int>((-descent) * scale + 0.5f);
  metrics.height =
      static_cast<int>((ascent - descent + lineGap) * scale + 0.5f);
  return metrics;
}

bool SFontRenderer::Load(const char* filename, int size) {
  auto file = util::File::Open(filename, util::File::Mode::kRead);
  if (!file) return false;

  size_t ttf_buf_size = file->Size();
  ttf_buffer = new unsigned char[ttf_buf_size];
  ttf_buf_size = file->Read(ttf_buffer, 1, ttf_buf_size);

  stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));

  font_size =
      static_cast<int>(size * 1.3f);  // FIX: Constant taken out of thin air
                                      // because fonts get too small.
  scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(font_size));
  return true;
}

std::unique_ptr<FontFace> SFontRenderer::Create(
    FontManager* font_manager, const std::string& filename,
    const FontDescription& font_desc) {
  auto font_renderer = std::make_unique<SFontRenderer>();
  if (font_renderer->Load(filename.c_str(),
                          static_cast<int>(font_desc.size()))) {
    return std::make_unique<FontFace>(font_manager->glyph_cache(),
                                      std::move(font_renderer), font_desc);
  }
  return nullptr;
}

}  // namespace text
}  // namespace el

void register_stb_font_renderer() {
  el::text::FontManager::get()->RegisterRenderer(
      std::make_unique<el::text::SFontRenderer>());
}

#endif  // EL_FONT_RENDERER_STB
