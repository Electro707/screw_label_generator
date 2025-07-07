#include <iostream>
#include <cstring>
#include <cmath>
#include "librsvg/rsvg.h"
#include <cxxopts.hpp>
#include "imageGen.h"
#include <unistd.h>

extern "C" {
#include "ptouch.h"
}

using namespace std;

ptouch_dev ptdev = NULL;

unordered_map<string, string> screwToFile = {
    {"hex", "images/head_hex.svg"},
    {"flat", "images/head_flat.svg"},
    {"phillips", "images/head_phillips.svg"},
    {"hex-external", "images/hex_external.svg"},
    {"nut", "images/nut_hex.svg"}
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

    ptouch_rasterstart(ptdev);

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

    // ptouch_eject(ptdev);
    ptouch_ff(ptdev);

    // wait for print to finish
    while(1){
        ptouch_read_status(ptdev, 0);
        if(ptdev->status->phase_type == 0 && ptdev->status->status_type == 6){
            break;
        }
        usleep(1000);
    }
}

filesystem::path getExecutablePath(void){
    return filesystem::canonical("/proc/self/exe").parent_path();
}

int main(int argc, char **argv){
    cout << "Hello World!" << endl;


    cxxopts::Options options("GridFinity Screw Printer", "Prints label with an image for Gridfinity");

    options.add_options()
        ("h,help", "Print usage")
        ("s,screw", "Screw size to use", cxxopts::value<vector<string>>())
        ("t,text", "Text to Print", cxxopts::value<vector<string>>()->default_value({"TEST"}))
        ("n,dry", "Dry Run (only print to image), file name optional", cxxopts::value<string>()->implicit_value("test.png"))
        ("labelMargin", "Margin for the label", cxxopts::value<int>()->default_value("0"))
        ("elementXMargin", "Margin for the elements in the X direction", cxxopts::value<int>()->default_value("2"))
        ("elementYMargin", "Margin for the elements in the Y direction", cxxopts::value<int>()->default_value("2"))
    ;

    vector<string> screwHeads;
    vector<string> toPrintS;
    bool isSplit = false;

    cxxopts::ParseResult argParse;
    try{
        argParse = options.parse(argc, argv);

        if (argParse.count("help"))
        {
            cout << options.help() << endl;
            exit(0);
        }
        
        if(argParse.count("screw")){

            screwHeads = argParse["screw"].as<vector<string>>();

            if(screwHeads.size() > 2){
                cout << "Unable to do more than 2 screw options" << endl;
                exit(0);
            }

            if(screwHeads.size() == 2){ // if we have multiple screws, must be a split option
                isSplit = true;
            }

            for(auto screwHead : screwHeads){
                // set to lowercase
                // from https://stackoverflow.com/questions/313970/how-to-convert-an-instance-of-stdstring-to-lower-case
                std::transform(screwHead.begin(), screwHead.end(), screwHead.begin(), [](unsigned char c){ return std::tolower(c);});
                // if the screw type is not in options, exit
                if (screwToFile.find(screwHead) == screwToFile.end()){
                    cout << "Invalid screw type, must be of type: ";
                    for (auto i : screwToFile){
                        cout << i.first << ", ";
                    }
                    cout << endl;
                    exit(0);
                }
            }
        }

        toPrintS = argParse["text"].as<vector<string>>();

        if(screwHeads.size() != toPrintS.size()){
            cout << "You forgot to specify a text given a print size" << endl;
            exit(1);
        }

        // cout << argParse["dry"].as<bool>() << endl;
    }catch(cxxopts::exceptions::exception &e){
        cout << "Error in args: " << e.what() << endl;
        return -1;
    }
    
    int tapeWidth, tapeDpi;

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

    
    imGen.lbMargin = argParse["labelMargin"].as<int>();
    imGen.elementsXMargin = argParse["elementXMargin"].as<int>();
    imGen.elementsYMargin = argParse["elementYMargin"].as<int>();

    imGen.setHeight(tapeWidth);
    // todo: set as arguments
    imGen.setWidthByMm(tapeDpi, 36);
    imGen.setPrintableHeight(tapeDpi, 8);
    imGen.initImage();
    
    int secondSplitX = 0;
    if(isSplit){
        secondSplitX = imGen.setPrintableSplit();
    }

    filesystem::path screwImageP;

    if(argParse.count("screw")){
        screwImageP = getExecutablePath();
        screwImageP /= filesystem::path(screwToFile[screwHeads[0]]);
        imGen.addSvg(screwImageP);
    }
    imGen.addText(toPrintS[0]);

    if(isSplit){
        imGen.moveSplitArea(secondSplitX);

        if(argParse.count("screw")){
            screwImageP = getExecutablePath();
            screwImageP /= filesystem::path(screwToFile[screwHeads[1]]);
            imGen.addSvg(screwImageP);
        }
        imGen.addText(toPrintS[1]);
    }

    imGen.fitImageW();
    imGen.addEndDottedLines();

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
    if(ptdev->h){
        ptouch_close(ptdev);
    }

    return 0;
}
