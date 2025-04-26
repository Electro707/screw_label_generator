#include <cmath>
#include <cstring>
#include <iostream>
#include "librsvg/rsvg.h"


class ImageGen{
    public:
        ImageGen();
        ~ImageGen();

        // Set the width and height of the image area (label)
        void setWidthByMm(int printerDpi, float mm);
        void setHeight(int h);

        // Set the area of the printable area
        void setPrintableHeight(int printerDpi, float mm);

        /**
         * @brief Initializes the image context. Must be called after setting the width and height, and before other operation occurs
         * 
         */
        void initImage(void);

        // Adds an SVG icon
        void addSvg(const std::string &svgPath);

        // Adds a text
        void addText(std::string text);

        // Fits the image width to the minimum required
        void fitImageW(void);

        cairo_surface_t *getCairoSurface(void);

        int lbMargin = 4;

        int lbH = -1;
        int lbW = -1;

        
    private:
        std::string fontName = "Noto Sans";

        int lbAvailableW;
        int lbAvailableH;

        cairo_surface_t *surface;
        cairo_t *cr;
};