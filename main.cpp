#include <iostream>
#include <cstring>
#include <cmath>
#include "librsvg/rsvg.h"
#include <cxxopts.hpp>
#include "imageGen.h"

extern "C" {
#include "ptouch.h"
}

using namespace std;

ptouch_dev ptdev = NULL;

const vector<string> availableScrews = {"hex", "flat", "phillips"};

unordered_map<string, string> screwToFile = {
    {"hex", "images/head_hex.svg"},
    {"flat", "images/head_flat.svg"},
    {"phillips", "images/head_phillips.svg"}
};

int pTouchInit(void){
    if(ptouch_open(&ptdev) != 0){
        cout << "Unable to open P-Touch device" << endl;
        return -1;
    }
    if(ptouch_init(ptdev) != 0){
        cout << "Unable to initialize P-Touch device" << endl;
        return -1;
    }
    if(ptouch_getstatus(ptdev) != 0){
        cout << "Unable to get printer status" << endl;
        return -1;
    }

    cout << "DPI of printer: " << ptdev->devinfo->dpi << endl;
    cout << "maxPx of printer: " << ptdev->devinfo->max_px << endl;
    return 0;
}

void pTouchPrint(cairo_surface_t *surface, int imageW, int imageH){
    int offset, max_pixels;
    uint32_t pixel;
    int ySet;

    cairo_surface_flush(surface);
    uint32_t *imageDat = (uint32_t *)cairo_image_surface_get_data(surface);

    int rasterLineH = (ptdev->devinfo->max_px)/8;
    uint8_t rasterline[rasterLineH];

    max_pixels=ptouch_get_max_width(ptdev);
    offset=((int)max_pixels / 2)-(imageH/2);	/* always print centered  */

    for(int x=0;x<imageW;x++){
        memset(rasterline, 0, rasterLineH);

        for(int y=0;y<imageH;y++){
            ySet = y + offset;

            pixel = imageDat[x+((imageH-1-y)*imageW)];
            pixel &= 0xFFFFFF;
            pixel &= 0xF0F0F0;  // use upper MSB for RGB to determine threshold if active (>grey) or not (<grey)
            // printf("%x\n", pixel);
            if(pixel == 0){
                rasterline[rasterLineH-1-(ySet/8)] |= (1 << (ySet%8));
            }
        }

        if (ptouch_sendraster(ptdev, rasterline, 16) != 0) {
            cout << "Error when printing to printer" << endl;
        }
    }

    ptouch_eject(ptdev);
}

int main(int argc, char **argv){
    cout << "Hello World!" << endl;


    cxxopts::Options options("GridFinity Screw Printer", "Prints label with an image for Gridfinity");

    options.add_options()
        ("n,dry", "Dry Run, file name optional", cxxopts::value<string>()->implicit_value("test.png"))
        ("t,text", "Text to Print", cxxopts::value<string>()->default_value("TEST"))
        ("s,screw", "Screw size to use", cxxopts::value<string>())
        ("h,help", "Print usage")
    ;

    string screwHead;

    cxxopts::ParseResult argParse;
    try{
        argParse = options.parse(argc, argv);

        if (argParse.count("help"))
        {
            cout << options.help() << endl;
            exit(0);
        }
        
        if(argParse.count("screw")){
            screwHead = argParse["screw"].as<string>();
            // set to lowercase
            // from https://stackoverflow.com/questions/313970/how-to-convert-an-instance-of-stdstring-to-lower-case
            std::transform(screwHead.begin(), screwHead.end(), screwHead.begin(), [](unsigned char c){ return std::tolower(c);});
            // if the screw type is not in options, exit
            if (find(availableScrews.begin(), availableScrews.end(), screwHead) == availableScrews.end()){
                cout << "Invalid screw type, must be of type: ";
                for (auto i : availableScrews){
                    cout << i << ", ";
                }
                cout << endl;
                exit(0);
            }
        }

        // cout << argParse["dry"].as<bool>() << endl;
    }catch(cxxopts::exceptions::exception &e){
        cout << "Error in args: " << e.what() << endl;
        return -1;
    }
    
    int tapeWidth, tapeDpi;
    string toPrintS = argParse["text"].as<string>();

    if(pTouchInit()){
        cout << "Unable to open printer" << endl;
        if(argParse.count("dry") == 0){
            cout << "Exiting" << endl;
            exit(1);
        }
        cout << "Assuming a width of 76px, dpi of 180";
        tapeWidth = 76;
        tapeDpi = 180;
    } else {
        tapeWidth = ptouch_get_tape_width(ptdev);
        cout << "Tape Width: " << tapeWidth << endl;
        tapeDpi = ptdev->devinfo->dpi;
    }
    
    ImageGen imGen;

    
    imGen.lbMargin = 1;
    imGen.setHeight(tapeWidth);
    imGen.setWidthByMm(tapeDpi, 36);
    imGen.setPrintableHeight(tapeDpi, 8);
    imGen.initImage();
    

    if(argParse.count("screw")){
        imGen.addSvg(screwToFile[screwHead]);
    }
    imGen.addText(toPrintS);

    imGen.fitImageW();
    

    if (argParse.count("dry")){
        string fName = argParse["dry"].as<string>();
        const char *fNameS = fName.c_str();
        printf("Dry run, saving to PNG %s\n", fNameS);
        if (cairo_surface_write_to_png (imGen.getCairoSurface(), fNameS) != CAIRO_STATUS_SUCCESS){
            g_printerr ("could not write output file");
        }
    }
    else {
        pTouchPrint(imGen.getCairoSurface(), imGen.lbW, tapeWidth);
    }

    // cleanup
    if(ptdev){
        ptouch_close(ptdev);
    }

    return 0;
}
