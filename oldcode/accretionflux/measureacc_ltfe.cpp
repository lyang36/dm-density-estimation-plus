#include <cstring>
#include <cstdio>
#include <cstdlib>     /* atoi */
#include <fitsio.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include "accretion.h"
#include "haloread.h"
#include "../library/types.h"

using namespace std;

void printusage(){
    printf("Usage: measureacc_ltfe datafile halocatalog\n");
    printf("[-radius <the radius measured in r_vir>]\n");
    printf("[-halo <{ list of halos to measure }>]\n");
    printf("[-mask <mask file for the halo >]\n");
    printf("[-massthd <mass threshold for halos, under which to be calculated (-1 default)>]\n");
    printf("\n");
    printf("Measure the accretion rate of halos in the datafile (gadget file)\n descripted by the halocatalog (fits file)\n");
    printf("The default radius is 1.5\n");
}



int main(int argc, char *argv[])
{
    string datafile;
    string halofile;
    string maskfile = "";
    
    double radtimes = 1.5;
    double massthred = -1.0;
    
    std::vector<int> halovec;
    bool isUseHaloVec = false;
    
    
    //string filename_ =  "/Users/lyang/data/32Mpc_S1_PM_000";
    Point halocenter;
    //double r = 10000;
    //double dr = 100;
    
    //load all the particles into memory?
    bool isHighMem = true;
    //return the all particle pointer?
    bool isAllData = false;
    
    //if use -1, then use the particle gridsize as the gridsize
    //otherwise use the user setting
    int datagridsize = 256;
    //the particle type in the gadget file
    int parttype = 1;
    int inputmemgrid = 32;
    
    bool isInOrder = false;
    bool isVelocity = true;
    
    char * halomask;
    fstream maskstream;
    

    
    if (argc < 3) {
        printusage();
        return 1;
    }else{
        datafile = argv[1];
        halofile = argv[2];
        int k = 3;
        while(k < argc){
            stringstream ss;
            //printf("%s\n", argv[k]);
			if(strcmp(argv[k], "-radius") == 0){
				ss << argv[k + 1];
				k +=2;
                ss >> radtimes;
                //printf("%f\n", radtimes);
			}else if(strcmp(argv[k], "-halo") == 0){
                k ++;
                isUseHaloVec = true;
                if(strcmp(argv[k], "{") != 0){
                    printusage();
                    return 1;
                }else{
                    k ++;
                    while((k < argc) && (strcmp(argv[k], "}") != 0)){
                        int halon;
                        stringstream ss1;
                        ss1 << argv[k];
                        ss1 >> halon;
                        halovec.push_back(halon);
                        k++;
                        //printf("%d\n", halon);
                    }
                    if((k == argc) || (strcmp(argv[k], "}") != 0)){
                        printusage();
                        return 1;
                    }else{
                        k++;
                    }
                }
                
            }else if(strcmp(argv[k], "-mask") == 0){
				ss << argv[k + 1];
				k +=2;
                ss >> maskfile;
                //printf("%f\n", radtimes);
			}else if(strcmp(argv[k], "-massthd") == 0){
				ss << argv[k + 1];
				k +=2;
                ss >> massthred;
                //printf("%f\n", radtimes);
			}else{
                printusage();
                return 1;
            }
        }
    }
    
    
    int hmin =0;
    int hmax = getTotalHaloNum(halofile.c_str());
    halomask = new char[hmax];
    for(int i = 0; i < hmax; i++){
        halomask[i] = 1;
    }
    
    
    if(maskfile != ""){
        //for(int i = 0; i < hmax; i++){
        //    halomask[i] = 0;
        //}
        maskstream.open(maskfile.c_str(), ios::in | ios::binary);
        if(maskstream.good()){
            int head[64];
            maskstream.read((char *) head, sizeof(int) * 64);
            if(head[0] != hmax){
                printf("Number of halos in halo mask file doesnot match! \n");
                maskstream.close();
                exit(1);
            }else{
                maskstream.read((char *) halomask, hmax);
            }
        }else{
            printf("Mask file corrupted!\n");
            exit(1);
        }
    }
    maskstream.close();
    
    /*int m = 0;
    for(int i = 0; i < hmax; i++){
        printf("%d\n", halomask[i]);
        m += halomask[i];
    }
    printf("Total = %d\n", m);
    */

    if(isUseHaloVec){
        hmin = 0;
        hmax = halovec.size();
    }
    Halo halo;
    
    TetraStreamer streamer(datafile,
                            inputmemgrid,
                            parttype,
                            datagridsize,
                            isHighMem,
                            isAllData,
                            isVelocity,
                            true,
                            isInOrder);
    
    printf("ID    MASS    X    Y    Z    RADIUS    ACCRETION_RATE\n");
    
    for(int i = hmin; i < hmax; i++){
        int haloid = 0;
        if(isUseHaloVec){
            haloid = halovec[i];
        }else{
            haloid = i + 1;
        }
        
       
        printf("Haloid = %d, halomask=%d\n", haloid, halomask[haloid - 1]);
        if(halomask[haloid - 1] == 0){
            continue;
        }
        

        continue;
        
        int status = getHaloById(halofile.c_str(), haloid, &halo);
        if(status != 0){
            break;
        }

        
        halocenter.x = halo.x;
        halocenter.y = halo.y;
        halocenter.z = halo.z;
        double mass = halo.mass;
        double r = radtimes * halo.radius;
        
        double accrate = accretion_tetra_rate(                                                streamer,
            mass,
            halocenter,
            r);
        
        printf("%d   %f   %f   %f   %f   %f   %f \n", haloid, mass, halocenter.x,
               halocenter.y, halocenter.z, halo.radius, accrate);
        cout.flush();
    }
    
    delete halomask;
}
