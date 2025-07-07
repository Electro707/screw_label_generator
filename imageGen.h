#include <cmath>
#include <cstring>
#include <iostream>
#include "librsvg/rsvg.h"

/**
 * # Architecture of drawing
 *
 * This module is architrecturerd as follows:
 * - There is an overall label X and Y size, which is the total
 * - Inside of this label are "canvases", available drawing areas. There can be up to 2 of these per label
 */

class ImageGen{
    public:
        ImageGen();
        ~ImageGen();

        // Set the width and height of the image area (label)
        void setWidthByMm(int printerDpi, float mm);
        void setHeight(int h);

        // Set the area of the printable area
        void setPrintableHeight(int printerDpi, float mm);

        // split the image in two, in which case a pointer will be returned for the new X position
        // this will return an X variable to be used by the `moveSplitArea` function
        int setPrintableSplit(void);

        void moveSplitArea(int startX);       // move from the first split area (must be split beforehand) to the other area

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
        void fitImageW(int endMargin = 0);

        // adds dotted lines at the end of the label to know where to cut
        void addEndDottedLines(void);

        cairo_surface_t *getCairoSurface(void);

        int lbMargin = 4;       // margin between label and outside
        int elementsXMargin = 2; // margin between different elements
        int elementsYMargin = 2;

        // width and height of the label has a whole
        int lbH = -1;
        int lbW = -1;

        
    private:
        int getLeftOverX(void);

        std::string fontName = "Noto Sans";

        int canvasXLoc;     // starting X position in our canvas in reference to the entire label

        // printable area of a canvas
        int canvasXSize;
        int canvasYSize;

        int drawXLoc;         // X location in our "canvas"

        cairo_surface_t *surface;
        cairo_t *cr;
};