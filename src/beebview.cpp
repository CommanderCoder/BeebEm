//
//  beebview.cpp
//  BeebEm5
//
//  Created by Commander Coder on 24/06/2020.
//  Copyright © 2020 Andrew Hague. All rights reserved.
//

#include <stdio.h>


struct PixelData
{
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};



// cannot access C++ (i.e. classes) from Swift so need to convert them to C
extern "C"
{
    int getIntFromCPP()
    {
        return 500;
    }

    PixelData d[200*200];

    void setPixelsInCPP(unsigned char* data)
    {
        printf("[setPixelsInCPP] %d\n",data[0]);
        data[0]=0;
    }

}

