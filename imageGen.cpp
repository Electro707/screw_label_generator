#include <iostream>
#include <cstdint>
#include "imageGen.h"

std::tuple<double, double> get_text_size(cairo_t *cr, double fontSize, const char *text){
    cairo_text_extents_t extents;
    cairo_set_font_size(cr, fontSize);
    cairo_text_extents(cr, text, &extents);

    return std::make_tuple(extents.width + extents.x_bearing, extents.height);
}

double fit_font_size(cairo_t *cr, const char *text, double max_width, double max_height) {
    cairo_text_extents_t extents;
    double test_size = 100.0;

    cairo_set_font_size(cr, test_size);
    cairo_text_extents(cr, text, &extents);

    double width_scale, height_scale = INFINITY;

    if(max_width != -1){
        width_scale = max_width / extents.width;
    }
    if(max_height != -1){
        height_scale = max_height / extents.height;
    }

    return test_size * fmin(width_scale, height_scale);
}

ImageGen::ImageGen(){
    cr = NULL;
    surface = NULL;
}

ImageGen::~ImageGen(){
    cairo_destroy (cr);
    cairo_surface_destroy (surface);
}

void ImageGen::setWidthByMm(int printerDpi, float mm){
    float inch = mm / 25.4;
    lbW = (int)((float)printerDpi * inch);

    lbAvailableW = lbW - (lbMargin*2);

    // std::cout << "Set label width to " << lbW << " pixels" << std::endl;
}

void ImageGen::setHeight(int h){
    lbH = h;
    lbAvailableH = lbH - (lbMargin*2);
}

void ImageGen::setPrintableHeight(int printerDpi, float mm){
    float inch = mm / 25.4;
    lbAvailableH = (int)((float)printerDpi * inch);
    lbAvailableH -= (lbMargin*2);
    if(lbAvailableH > lbH){
        printf("ERROR: set available height to > label height");
    }
}

void ImageGen::initImage(void){
    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, lbW, lbH);
    cr = cairo_create (surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, lbW, lbH);
    cairo_fill(cr);
}

void ImageGen::fitImageW(void){
    int newW = lbW - lbAvailableW;
    newW += lbMargin*2;

    cairo_surface_t *newS = cairo_image_surface_create (CAIRO_FORMAT_RGB24, newW, lbH);
    cairo_t *newCr = cairo_create (newS);

    cairo_set_source_surface(newCr, surface, 0, 0);
    cairo_rectangle(newCr, 0, 0, newW, lbH);
    cairo_fill(newCr);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    surface = newS;
    cr = newCr;

    lbW = newW;
}

void ImageGen::addSvg(const std::string &svgPath){
    char *svgPathC = const_cast<char*>(svgPath.c_str());

    GError *error = NULL;
    RsvgHandle *handle = rsvg_handle_new_from_file(svgPathC, &error);

    RsvgRectangle svgViewbox;
    rsvg_handle_get_intrinsic_dimensions(handle, NULL, NULL, NULL, NULL, NULL, &svgViewbox);
    float svgAspectRatio = svgViewbox.width / svgViewbox.height;

    float svgRenderW = ((gdouble)lbAvailableH) * svgAspectRatio;
    RsvgRectangle viewport = {
        .x = (gdouble)lbMargin,
        .y = (gdouble)lbMargin,
        .width = svgRenderW,
        .height = (gdouble)lbAvailableH,
    };

    std::string cssStyle = ":root {shape-rendering: crispEdges;}";

    rsvg_handle_set_stylesheet(handle, (uint8_t *)cssStyle.c_str(), cssStyle.length(), &error);
    if(!rsvg_handle_render_document (handle, cr, &viewport, &error)){
      g_printerr("could not render: %s", error->message);
      exit(1);
    }

    lbAvailableW -= svgRenderW;

    g_object_unref (handle);

}

void ImageGen::addText(std::string text){
    const int textYOffset = 0;
    char *textC = const_cast<char*>(text.c_str());

    cairo_font_options_t *font_opts = cairo_font_options_create();
    cairo_font_options_set_antialias(font_opts, CAIRO_ANTIALIAS_NONE);
    cairo_set_font_options(cr, font_opts);

    cairo_select_font_face(cr,
                           const_cast<char*>(fontName.c_str()),
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    double optFontSize = fit_font_size(cr, textC, (float)(lbAvailableW-textYOffset-2*lbMargin), (float)(lbAvailableH-2*lbMargin));
    
    cairo_set_font_size(cr, optFontSize);
    cairo_set_source_rgb(cr, 0, 0, 0);
    auto [textW, textH] = get_text_size(cr, optFontSize, textC);
    cairo_move_to(cr, lbW-lbAvailableW+textYOffset, (lbAvailableH-textH)/2 + textH + lbMargin);
    cairo_show_text(cr, textC);

    lbAvailableW -= textW+textYOffset;

    // std::cout << "optimal font size: " << optFontSize << std::endl;
}

cairo_surface_t * ImageGen::getCairoSurface(void){
    return surface;
}