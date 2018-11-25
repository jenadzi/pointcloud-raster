#include <iostream>
#include <pointcloud_raster/core/image.hpp>
#include <gtest/gtest.h>
#include <png.h>

using namespace pointcloud_raster;

/**
 * This function can only read PNGs generated by this own library
 * @param filename PNG image path
 * @return A valid image if it could be read
 */
std::optional<Image>
ReadImageFromFile(const std::string &filename)
{
    /**
     * from: http://zarb.org/~gc/html/libpng.html
     */
    std::FILE* fp = std::fopen(filename.c_str(), "rb");
    if (fp == nullptr)
        return std::nullopt;

    /* initialize stuff */
    auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (!png_ptr)
        return std::nullopt;

    auto info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        return std::nullopt;

    if (setjmp(png_jmpbuf(png_ptr)))
        throw std::runtime_error("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 0);

    png_read_info(png_ptr, info_ptr);

    auto width = png_get_image_width(png_ptr, info_ptr);
    auto height = png_get_image_height(png_ptr, info_ptr);
    png_read_update_info(png_ptr, info_ptr);

    std::vector<png_bytep> row_pointers;
    row_pointers.resize(height);

    Image image(ImageSize(width, height));
    auto imagePtr = image.Prt();

    for (int y = 0; y < height; y++)
        row_pointers[y] = const_cast<png_bytep>(&imagePtr[y*width*4]);

    // Commit read
    png_read_image(png_ptr, row_pointers.data());

    fclose(fp);
    return image;
}


/**
 * Creates an PNG image (transparent background) with
 * a cross on it.
 */
TEST(Core, ImageWritePNG)
{
    std::string outputImage = std::string(TESTS_OUTPUT_DIR) + "/core_image_out.png";
    constexpr int IMAGE_WIDTH = 32;
    Color color(128, 0, 255, 255);

    // Create cross image
    {
        Image image(ImageSize(IMAGE_WIDTH, IMAGE_WIDTH));
        for (int row = 0; row < IMAGE_WIDTH; row++)
            for (int col = 0; col < IMAGE_WIDTH; col++)
            {
                if (row == col || (IMAGE_WIDTH - row) == col)
                    image.Set(col, row, color);
            }
        image.SaveAsPNG(outputImage, 0);
    }

    // Read back image
    {
        Color defaultColor;
        if (auto savedImage = ReadImageFromFile(outputImage))
        {
            EXPECT_EQ(savedImage->Width(), IMAGE_WIDTH);
            EXPECT_EQ(savedImage->Height(), IMAGE_WIDTH);
            for (int row = 0; row < IMAGE_WIDTH; row++)
                for (int col = 0; col < IMAGE_WIDTH; col++)
                {
                    if (row == col || (IMAGE_WIDTH - row) == col)
                        EXPECT_TRUE(savedImage->Get(col, row) == color);
                    else
                        EXPECT_TRUE(savedImage->Get(col, row) == defaultColor);
                }
        }
        else
            FAIL() << "Could not read saved image " << outputImage;
    }
}
