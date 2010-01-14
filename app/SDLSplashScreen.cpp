/* __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2008, ART+COM AG Berlin, Germany <www.artcom.de>
//
// These coded instructions, statements, and computer programs contain
// proprietary information of ART+COM AG Berlin, and are copy protected
// by law. They may be used, modified and redistributed under the terms
// of GNU General Public License referenced below.
//
// Alternative licensing without the obligations of the GPL is
// available upon request.
//
// GPL v3 Licensing:
//
// This file is part of the ART+COM Y60 Platform.
//
// ART+COM Y60 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// ART+COM Y60 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with ART+COM Y60.  If not, see <http://www.gnu.org/licenses/>.
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Description: TODO
//
// Last Review: NEVER, NOONE
//
//  review status report: (perfect, ok, fair, poor, disaster, notapplicable, unknown)
//    usefullness            : unknown
//    formatting             : unknown
//    documentation          : unknown
//    test coverage          : unknown
//    names                  : unknown
//    style guide conformance: unknown
//    technical soundness    : unknown
//    dead code              : unknown
//    readability            : unknown
//    understandabilty       : unknown
//    interfaces             : unknown
//    confidence             : unknown
//    integration            : unknown
//    dependencies           : unknown
//    cheesyness             : unknown
//
//    overall review status  : unknown
//
//    recommendations:
//       - unknown
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
*/
//
//   $RCSfile: SDLSplashScreen.cpp,v $
//   $Author: oliver $
//   $Revision: 11918 $
//   $Date: 2007/06/07 10:45:40 $
//
//
//  Description: shows an image during application-start and -restart using SDL
//
//
//=============================================================================

#include "SDLSplashScreen.h"

#include <sstream>
#include <iostream>

#include <asl/base/os_functions.h>
#include <SDL/SDL_getenv.h>

#if defined(_MSC_VER)
#   pragma warning (push,1)
#endif //defined(_MSC_VER)
#include <paintlib/planydec.h>
#include <paintlib/planybmp.h>
#if defined(_MSC_VER)
#   pragma warning (pop)
#endif //defined(_MSC_VER)

SDLSplashScreen * SDLSplashScreen::_instance = 0;

SDLSplashScreen * SDLSplashScreen::getInstance() {
    if (_instance == 0)
        _instance = new SDLSplashScreen();
    return _instance;
}

SDLSplashScreen::~SDLSplashScreen() {
    disable();
}

SDLSplashScreen::SDLSplashScreen()
:_myEnabled(false), _myShown(false)
{}

bool
SDLSplashScreen::isEnabled() {
    return _myEnabled;
}

void
SDLSplashScreen::enable(const std::string& theBMPFilePath, int theXPos, int theYPos) {
    try {
        // setting up environment for SDL window offset
        std::stringstream myEnvText;
        myEnvText << theXPos << "," << theYPos;
        asl::set_environment_var("SDL_VIDEO_WINDOW_POS", myEnvText.str());

        // decoding image to PLAnyBmp
        PLAnyPicDecoder myDecoder;
        PLAnyBmp myBmp;
        myDecoder.MakeBmpFromFile (theBMPFilePath.c_str(), &myBmp);
        myDecoder.Close();

        int myWidth  = myBmp.GetWidth();
        int myHeight = myBmp.GetHeight();
        int myBitsPerPixel = myBmp.GetBitsPerPixel();

        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
          int rmask = 0xff000000;
          int gmask = 0x00ff0000;
          int bmask = 0x0000ff00;
          int amask = 0x000000ff;
        #else
          int rmask = 0x000000ff;
          int gmask = 0x0000ff00;
          int bmask = 0x00ff0000;
          int amask = 0xff000000;
        #endif

        // setting up SDL_Surface
        _myImage = SDL_CreateRGBSurface(SDL_SWSURFACE, myWidth, myHeight, 32, rmask, gmask, bmask, amask);

        // setting surface pixels
        if (myBitsPerPixel == 24) {
            PLPixel24 * myBmpPixel = NULL;
            PLPixel24 ** myBmpPixels = myBmp.GetLineArray24();
            setSurfacePixels(myBmpPixel, myBmpPixels, _myImage, myHeight, myWidth);
        } else {
            PLPixel32 * myBmpPixel = NULL;
            PLPixel32 ** myBmpPixels = myBmp.GetLineArray32();
            setSurfacePixels(myBmpPixel, myBmpPixels, _myImage, myHeight, myWidth);
        }

        _myEnabled = true;
    } catch(...) {
        disable();
    }
}

void
SDLSplashScreen::disable() {
    if (_myShown)
        hide();

    if (_myEnabled)
        SDL_FreeSurface(_myImage);

    _myEnabled = false;
}

void
SDLSplashScreen::show() {
    if (!_myEnabled) return;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_ShowCursor(SDL_DISABLE);
    _myScreen = SDL_SetVideoMode(_myImage->w, _myImage->h, 0, SDL_NOFRAME);

    SDL_BlitSurface(_myImage, NULL, _myScreen, NULL);
    SDL_UpdateRect(_myScreen, 0, 0, 0, 0);

    _myShown = true;
}

void
SDLSplashScreen::redraw() {
    if (!_myShown) return;
    SDL_UpdateRect(_myScreen, 0, 0, 0, 0);
}

void
SDLSplashScreen::hide() {
    if (!_myEnabled) return;

    SDL_FreeSurface(_myScreen);
    SDL_Quit();

    _myShown = false;
}

template <class T>
void
SDLSplashScreen::setSurfacePixels(T * theBmpPixel, T ** thePixels, SDL_Surface * theSurface, int theHeight, int theWidth) {
    Uint8     * mySurfacePixel;
    Uint32      myPixelColor;

    for(int y = 0; y < theHeight; y++) {
        for(int x = 0; x < theWidth; x++) {
            theBmpPixel = &thePixels[y][x];
            myPixelColor = SDL_MapRGB(theSurface->format, theBmpPixel->GetR(), theBmpPixel->GetG(), theBmpPixel->GetB());
            mySurfacePixel = (Uint8 *)theSurface->pixels + y * theSurface->pitch + x * 4;
            *(Uint32 *)mySurfacePixel = myPixelColor;
        }
    }
}

