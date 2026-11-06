/*
 *  Copyright (C) 2026 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "../src/PdfPages.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <jpeglib.h>
#include <png.h>
#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>
#include <poppler-page.h>

namespace
{
void Require(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(EXIT_FAILURE);
  }
}

std::vector<char> MakePdf(const std::vector<std::string>& objects)
{
  std::string pdf = "%PDF-1.5\n";
  std::vector<size_t> offsets;
  for (size_t i = 0; i < objects.size(); i++)
  {
    offsets.push_back(pdf.size());
    pdf += std::to_string(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
  }
  const size_t xref = pdf.size();
  pdf += "xref\n0 " + std::to_string(objects.size() + 1) + "\n0000000000 65535 f \n";
  for (const size_t offset : offsets)
  {
    char line[24];
    std::snprintf(line, sizeof(line), "%010zu 00000 n \n", offset);
    pdf += line;
  }
  pdf += "trailer\n<< /Size " + std::to_string(objects.size() + 1) + " /Root 1 0 R >>\n" +
         "startxref\n" + std::to_string(xref) + "\n%%EOF\n";
  return {pdf.begin(), pdf.end()};
}

/*!
 * Two pages: a landscape one, 200 by 100 points, whose left half is black,
 * and a portrait one, 100 by 200 points, left blank
 */
std::vector<char> TwoPagePdf()
{
  const std::string content = "0 0 0 rg 0 0 100 100 re f";
  return MakePdf({
      "<< /Type /Catalog /Pages 2 0 R >>",
      "<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>",
      "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] /Contents 4 0 R >>",
      "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream",
      "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 200] >>",
  });
}

std::vector<char> ImagePagePdf(const std::string& image, const std::string& filter)
{
  const std::string content = "q 100 0 0 100 0 0 cm /Im0 Do Q";
  return MakePdf({
      "<< /Type /Catalog /Pages 2 0 R >>",
      "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
      "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] /Contents 4 0 R "
      "/Resources << /XObject << /Im0 5 0 R >> >> >>",
      "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream",
      "<< /Type /XObject /Subtype /Image /Width 16 /Height 16 /ColorSpace /DeviceGray "
      "/BitsPerComponent 8 /Filter /" + filter + " /Length " + std::to_string(image.size()) +
          " >>\nstream\n" + image + "\nendstream",
  });
}

std::vector<char> JpegPagePdf()
{
  jpeg_compress_struct cinfo{};
  jpeg_error_mgr jerr{};
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  unsigned char* buffer = nullptr;
  unsigned long size = 0;
  jpeg_mem_dest(&cinfo, &buffer, &size);
  cinfo.image_width = 16;
  cinfo.image_height = 16;
  cinfo.input_components = 1;
  cinfo.in_color_space = JCS_GRAYSCALE;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, 100, TRUE);
  jpeg_start_compress(&cinfo, TRUE);
  unsigned char row[16] = {0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255};
  while (cinfo.next_scanline < cinfo.image_height)
  {
    JSAMPROW scanline = row;
    jpeg_write_scanlines(&cinfo, &scanline, 1);
  }
  jpeg_finish_compress(&cinfo);
  const std::string image(reinterpret_cast<char*>(buffer), size);
  jpeg_destroy_compress(&cinfo);
  std::free(buffer);

  return ImagePagePdf(image, "DCTDecode");
}

std::vector<char> Jpeg2000PagePdf()
{
  // Lossless 16x16 grayscale JP2: eight black columns, then eight white.
  const unsigned char image[] = {
      0x00, 0x00, 0x00, 0x0c, 0x6a, 0x50, 0x20, 0x20, 0x0d, 0x0a, 0x87, 0x0a,
      0x00, 0x00, 0x00, 0x14, 0x66, 0x74, 0x79, 0x70, 0x6a, 0x70, 0x32, 0x20,
      0x00, 0x00, 0x00, 0x00, 0x6a, 0x70, 0x32, 0x20, 0x00, 0x00, 0x00, 0x2d,
      0x6a, 0x70, 0x32, 0x68, 0x00, 0x00, 0x00, 0x16, 0x69, 0x68, 0x64, 0x72,
      0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x07, 0x07,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x63, 0x6f, 0x6c, 0x72, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x9a, 0x6a, 0x70, 0x32,
      0x63,
      0xff, 0x4f, 0xff, 0x51, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
      0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x01, 0xff, 0x52, 0x00,
      0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x04, 0x04, 0x00, 0x01, 0xff,
      0x5c, 0x00, 0x0d, 0x40, 0x40, 0x48, 0x48, 0x50, 0x48, 0x48, 0x50, 0x48,
      0x48, 0x50, 0xff, 0x64, 0x00, 0x12, 0x00, 0x01, 0x76, 0x66, 0x73, 0x2e,
      0x70, 0x64, 0x66, 0x20, 0x68, 0x61, 0x6c, 0x76, 0x65, 0x73, 0xff, 0x90,
      0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x01, 0xff, 0x93,
      0xdf, 0x80, 0x28, 0x09, 0x15, 0x5a, 0x74, 0xc3, 0xcf, 0xc0, 0x14, 0x08,
      0xf1, 0x4b, 0x5d, 0x25, 0xcf, 0xc0, 0x24, 0x17, 0x2c, 0xa8, 0x89, 0x3a,
      0x26, 0x16, 0x57, 0xad, 0xc7, 0xda, 0x08, 0x00, 0x21, 0x6e, 0x07, 0xfa,
      0xff, 0xd9,
  };
  return ImagePagePdf({reinterpret_cast<const char*>(image), sizeof(image)}, "JPXDecode");
}

std::vector<char> TextPagePdf(const std::string& font)
{
  const std::string content = "BT /F1 32 Tf 10 40 Td (Fallback) Tj ET";
  const std::string encoding = font == "Symbol" || font == "ZapfDingbats"
                                   ? ""
                                   : " /Encoding /WinAnsiEncoding";
  return MakePdf({
      "<< /Type /Catalog /Pages 2 0 R >>",
      "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
      "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] /Contents 4 0 R "
      "/Resources << /Font << /F1 5 0 R >> >> >>",
      "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream",
      "<< /Type /Font /Subtype /Type1 /BaseFont /" + font + encoding + " >>",
  });
}

struct Picture
{
  unsigned int width{0};
  unsigned int height{0};
  std::vector<unsigned char> rgb;

  int Brightness(unsigned int x, unsigned int y) const
  {
    const size_t i = (static_cast<size_t>(y) * width + x) * 3;
    return (rgb[i] + rgb[i + 1] + rgb[i + 2]) / 3;
  }
};

Picture Decode(const std::vector<uint8_t>& jpeg)
{
  jpeg_decompress_struct cinfo{};
  jpeg_error_mgr jerr{};
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, jpeg.data(), static_cast<unsigned long>(jpeg.size()));
  Require(jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK, "not a JPEG");
  cinfo.out_color_space = JCS_RGB;
  jpeg_start_decompress(&cinfo);
  Picture picture;
  picture.width = cinfo.output_width;
  picture.height = cinfo.output_height;
  picture.rgb.resize(static_cast<size_t>(picture.width) * picture.height * 3);
  while (cinfo.output_scanline < cinfo.output_height)
  {
    JSAMPROW row = &picture.rgb[static_cast<size_t>(cinfo.output_scanline) * picture.width * 3];
    jpeg_read_scanlines(&cinfo, &row, 1);
  }
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  return picture;
}

void TestPngExport()
{
  const std::vector<char> data = TwoPagePdf();
  const std::unique_ptr<poppler::document> document(
      poppler::document::load_from_raw_data(data.data(), static_cast<int>(data.size())));
  Require(document != nullptr, "the PNG test PDF did not load");
  const std::unique_ptr<poppler::page> page(document->create_page(0));
  poppler::page_renderer renderer;
  renderer.set_paper_color(0xffffffff);
  const poppler::image image = renderer.render_page(page.get(), 144, 144);
  const char* filename = "test_pdfpages.png";
  Require(image.save(filename, "png"), "Poppler PNG export failed");

  png_image png{};
  png.version = PNG_IMAGE_VERSION;
  Require(png_image_begin_read_from_file(&png, filename) != 0, "the exported PNG is unreadable");
  png.format = PNG_FORMAT_RGB;
  Picture picture;
  picture.width = png.width;
  picture.height = png.height;
  picture.rgb.resize(PNG_IMAGE_SIZE(png));
  Require(png_image_finish_read(&png, nullptr, picture.rgb.data(), 0, nullptr) != 0,
          "the exported PNG did not decode");
  png_image_free(&png);
  std::remove(filename);
  Require(picture.width == 400 && picture.height == 200, "PNG dimensions must match the page");
  Require(picture.Brightness(50, 100) == 0 && picture.Brightness(350, 100) == 255,
          "the exported PNG must preserve black drawing and white paper");
}

void TestFontFallback()
{
  for (const char* font : {"Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
                           "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
                           "Helvetica-BoldOblique", "Times-Roman", "Times-Bold", "Times-Italic",
                           "Times-BoldItalic", "Symbol", "ZapfDingbats", "VfsPdfMissingFont"})
  {
    std::string error;
    const std::unique_ptr<CPdfPages> pages = CPdfPages::Load(TextPagePdf(font), error);
    Require(pages != nullptr, "the unembedded-font PDF did not load");
    std::vector<uint8_t> jpeg;
    Require(pages->RenderJpeg(0, 400, jpeg), "the unembedded-font page did not render");
    const Picture picture = Decode(jpeg);
    size_t darkPixels = 0;
    for (unsigned int y = 0; y < picture.height; ++y)
      for (unsigned int x = 0; x < picture.width; ++x)
        if (picture.Brightness(x, y) < 80)
          ++darkPixels;
    Require(darkPixels > 500,
            (std::string("font fallback must render visible text for ") + font).c_str());
  }
}
} // namespace

int main(int argc, char** argv)
{
  if (argc == 2 && std::string(argv[1]) == "--font-fallback")
  {
    TestFontFallback();
    std::puts("PASS: all 14 unembedded standard fonts and missing font fallback");
    return EXIT_SUCCESS;
  }

  std::string error;
  std::unique_ptr<CPdfPages> pages = CPdfPages::Load(TwoPagePdf(), error);
  Require(pages != nullptr, "the PDF did not load");
  Require(pages->PageCount() == 2, "wrong page count");

  std::vector<uint8_t> jpeg;
  Require(pages->RenderJpeg(0, 400, jpeg), "page 1 did not render");
  const Picture landscape = Decode(jpeg);
  Require(landscape.width == 400 && landscape.height == 200,
          "the longer side must fit the size asked for, keeping the shape");
  Require(landscape.Brightness(50, 100) < 40, "the drawn half must be dark");
  Require(landscape.Brightness(350, 100) > 215, "the rest must be white paper");

  Require(pages->RenderJpeg(1, 400, jpeg), "page 2 did not render");
  const Picture portrait = Decode(jpeg);
  Require(portrait.width == 200 && portrait.height == 400, "a portrait page fits by its height");

  Require(!pages->RenderJpeg(2, 400, jpeg), "there is no third page");
  Require(!pages->RenderJpeg(static_cast<unsigned int>(-1), 400, jpeg),
          "an invalid page index must be refused");

  pages = CPdfPages::Load(JpegPagePdf(), error);
  Require(pages != nullptr && pages->PageCount() == 1, "the single-page image PDF did not load");
  Require(pages->RenderJpeg(0, 200, jpeg), "the JPEG image page did not render");
  const Picture image = Decode(jpeg);
  Require(image.width == 200 && image.height == 200, "the image page must remain square");
  Require(image.Brightness(25, 100) < 40 && image.Brightness(175, 100) > 215,
          "the embedded JPEG must retain its dark and light halves");

  pages = CPdfPages::Load(Jpeg2000PagePdf(), error);
  Require(pages != nullptr && pages->PageCount() == 1, "the JPEG 2000 PDF did not load");
  Require(pages->RenderJpeg(0, 200, jpeg), "the JPEG 2000 image page did not render");
  const Picture jpxImage = Decode(jpeg);
  Require(jpxImage.width == 200 && jpxImage.height == 200, "the JPEG 2000 page must remain square");
  Require(jpxImage.Brightness(25, 100) < 40 && jpxImage.Brightness(175, 100) > 215,
          "the embedded JPEG 2000 must retain its dark and light halves");

  TestPngExport();

  std::vector<char> garbage{'n', 'o', 't', ' ', 'a', ' ', 'p', 'd', 'f'};
  Require(CPdfPages::Load(garbage, error) == nullptr && !error.empty(),
          "something that is not a PDF must be refused with a reason");

  std::puts("PASS: single/multiple pages, portrait/landscape dimensions, embedded JPEG/JPEG 2000, "
            "PNG export, white paper, invalid indices and malformed input");
  return EXIT_SUCCESS;
}
