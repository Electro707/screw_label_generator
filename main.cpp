#include <iostream>
#include <cstring>
#include <cmath>
#include <cassert>
#include "librsvg/rsvg.h"
#include <cxxopts.hpp>
#include "imageGen.h"
#include <unistd.h>

extern "C" {
#include "ptouch.h"
}

// the variables below depends on the label area on the gridfinity box
#define PRINT_WIDTH     36  // mm, the width of the label area to print out
#define PRINT_HEIGHT    8   // mm, the height of the label area to print

using namespace std;

ptouch_dev ptdev = NULL;

unordered_map<string, string> screwToFile = {
    {"hex", "images/head_hex.svg"},
    {"flat", "images/head_flat.svg"},
    {"phillips", "images/head_phillips.svg"},
    {"hex-external", "images/hex_external.svg"},
    {"nut", "images/nut_hex.svg"},
    {"standoff_ext", "images/standoff_in_out.svg"},
    {"standoff_int", "images/standoff_in_in.svg"}
};

int pTouchInit(void){
    if(ptouch_open(&ptdev) != 0){
        cout << "Unable to open P-Touch device" << endl;
        return -1;
    }
    if(ptouch_init(ptdev) != 0){
        cout << "Unable to initialize P-Touch device" << endl;
        ptouch_close(ptdev);
        return -1;
    }
    if(ptouch_getstatus(ptdev, ptdev->devinfo->min_timeout) != 0){
        cout << "Unable to get printer status" << endl;
        ptouch_close(ptdev);
        return -1;
    }

    cout << "DPI of printer: " << ptdev->devinfo->dpi << endl;
    cout << "maxPx of printer: " << ptouch_get_max_width(ptdev) << endl;
    return 0;
}

void pTouchPrint(cairo_surface_t *surface, int imageW, int imageH){
    int offset, max_pixels, rasterLineH;
    uint32_t pixel;
    int ySet;

    cairo_surface_flush(surface);
    uint32_t *imageDat = (uint32_t *)cairo_image_surface_get_data(surface);

    max_pixels = ptouch_get_max_width(ptdev);
    rasterLineH = max_pixels/8;

    printf("max_pixels: %d, rasterLineH: %d\n", max_pixels, rasterLineH);

    uint8_t rasterline[rasterLineH];

    offset=((int)max_pixels / 2)-(imageH/2);	/* always print centered  */

    ptouch_rasterstart(ptdev);
    if ((ptdev->devinfo->flags & FLAG_RASTER_PACKBITS) == FLAG_RASTER_PACKBITS) {
		ptouch_enable_packbits(ptdev);
	}
    if ((ptdev->devinfo->flags & FLAG_D460BT_MAGIC) == FLAG_D460BT_MAGIC) {
		ptouch_send_d460bt_magic(ptdev);
	}

    for(int x=0;x<imageW;x++){
        memset(rasterline, 0, rasterLineH);

        for(int y=0;y<imageH;y++){
            ySet = y + offset;

            pixel = imageDat[x+((imageH-1-y)*imageW)];
            pixel &= 0xF0F0F0;  // use upper MSB for RGB to determine threshold if active (>grey) or not (<grey)
            if(pixel == 0){
                rasterline[rasterLineH-1-(ySet/8)] |= (1 << (ySet%8));
            }
        }

        if (ptouch_sendraster(ptdev, rasterline, rasterLineH) != 0) {
            cout << "Error when printing to printer" << endl;
            // todo: handle error
            return;
        }
    }

    ptouch_finalize(ptdev, false);

    // wait for print to finish
    while(1){
        ptouch_getstatus_nosend(ptdev, 1);
        if(ptdev->status->status_type == 0x01){
            break;
        }
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
        ("t,text", "Text to Print", cxxopts::value<vector<string>>())
        ("n,dry", "Dry Run (only print to image), file name optional", cxxopts::value<string>()->implicit_value("test.png"))
        ("isThreaded", "Set if the screw is threaded. Must be set to \"True\" or \"False\"", cxxopts::value<vector<bool>>())
        ("labelMargin", "Margin for the label", cxxopts::value<int>()->default_value("0"))
        ("elementXMargin", "Margin for the elements in the X direction", cxxopts::value<int>()->default_value("2"))
        ("elementYMargin", "Margin for the elements in the Y direction", cxxopts::value<int>()->default_value("2"))
    ;

    vector<string> screwHeads;
    vector<bool> isThreadeds;
    vector<string> toPrintS;
    bool isSplit = false;
    bool isPrinter = false;

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

            if(argParse.count("isThreaded")){
                isThreadeds = argParse["isThreaded"].as<vector<bool>>();

                if(isThreadeds.size() != screwHeads.size()){
                    cout << "Thread and screw options must equal" << endl;
                    exit(0);
                }
            }

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

        if(argParse.count("text")){
            toPrintS = argParse["text"].as<vector<string>>();
            if(screwHeads.size() != toPrintS.size()){
                cout << "You forgot to specify a text for a screw" << endl;
                exit(1);
            }
        }
        else{
            cout << "Did not give any text to be printed" << endl;
            exit(1);
        }

        // cout << argParse["dry"].as<bool>() << endl;
    }catch(cxxopts::exceptions::parsing &e){
        cout << "Error in args: " << e.what() << endl;
        exit(-1);
    }

    int tapeHeight, tapeDpi;

    if(pTouchInit() == 0){
        tapeHeight = ptouch_get_tape_width(ptdev);      // what PTouch calls width is actually the height
        cout << "Tape Width: " << tapeHeight << endl;
        tapeDpi = ptdev->devinfo->dpi;
        isPrinter = true;
    } else {
        cout << "Unable to open printer" << endl;
        if(argParse.count("dry") == 0){
            cout << "Exiting" << endl;
            exit(1);
        }
        cout << "Assuming a width of 76px, dpi of 180" << endl;
        tapeHeight = 76;
        tapeDpi = 180;
    }

    ImageGen imGen;

    imGen.lbMargin = argParse["labelMargin"].as<int>();
    imGen.elementsXMargin = argParse["elementXMargin"].as<int>();
    imGen.elementsYMargin = argParse["elementYMargin"].as<int>();

    imGen.setHeight(tapeHeight);
    imGen.setWidthByMm(tapeDpi, PRINT_WIDTH);
    imGen.setPrintableHeight(tapeDpi, PRINT_HEIGHT);
    imGen.initImage();

    int secondSplitX = 0;
    if(isSplit){
        secondSplitX = imGen.setPrintableSplit();
    }

    filesystem::path screwImageP;

    if(argParse.count("screw")){
        screwImageP = getExecutablePath();
        screwImageP /= filesystem::path(screwToFile[screwHeads[0]]);
        imGen.addSvg(screwImageP.string());

        if(isThreadeds.size()){
            if(isThreadeds[0]){
                screwImageP = getExecutablePath();
                screwImageP /= filesystem::path("images/threadedInsert.svg");
                imGen.addSvg(screwImageP.string());
            }
        }
    }
    imGen.addText(toPrintS[0]);

    if(isSplit){
        imGen.moveSplitArea(secondSplitX);

        if(argParse.count("screw")){
            screwImageP = getExecutablePath();
            screwImageP /= filesystem::path(screwToFile[screwHeads[1]]);
            imGen.addSvg(screwImageP.string());

            if(isThreadeds.size()){
                assert(isThreadeds.size() == 2);
                if(isThreadeds[1]){
                    screwImageP = getExecutablePath();
                    screwImageP /= filesystem::path("images/threadedInsert.svg");
                    imGen.addSvg(screwImageP.string());
                }
            } else {
                // intentional, no action
            }
        }
        imGen.addText(toPrintS[1]);
    }

    imGen.fitImageW();
    imGen.addEndDottedLines();

    if(argParse.count("dry")){
        string fName = argParse["dry"].as<string>();
        const char *fNameS = fName.c_str();
        printf("Dry run, saving to PNG %s\n", fNameS);
        if (cairo_surface_write_to_png (imGen.getCairoSurface(), fNameS) != CAIRO_STATUS_SUCCESS){
            g_printerr("could not write output file");
        }
    }
    else {
        assert(isPrinter == true);
        pTouchPrint(imGen.getCairoSurface(), imGen.lbW, tapeHeight);
    }

    // cleanup
    if(isPrinter){
        ptouch_close(ptdev);
    }

    return 0;
}
