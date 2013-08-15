/*********************************************************************/
/* get the density use the iso-z-cutter.                             */
/*this is very fast, no need to calculate the interpolation every time*/
/*Author: Lin F. Yang                                                */
/*********************************************************************/

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>    // std::find

#if defined(_WIN32) || defined(_WIN64)
//#include "gettimeofday_win.h"
#else
#include <unistd.h>
#include <sys/time.h>
#endif


//#include "grid.h"
#include "tetrahedron.h"
#include "indtetrastream.h"
#include "isoplane.h"
#include "render.h"
#include "denrender.h"


namespace main_space{
    
    int inputmemgrid = 16;			//the input memory grid size
    string filename =  "";          //"I:\\data\\MIP-00-00-00-run_050";
                                    //the input data filename "E:\\multires_150";//
    string densityFilename = "";	//output filename
    string velocityXFilename = "";	//velocity x output filename
    string velocityYFilename = "";  //velocity y output filename
    string velocityZFilename = "";  //velocity z output filename
    string streamFilename = "";     //stream statistics filename
    
    vector<RenderType> renderTypes; //what data component will be rendered
    
    
    bool isVerbose = false;         //output verbose information?
    bool isInOrder = false;         //are all the particles in file in increasing order of ids?
    
   
    bool isHighMem = true;          //load all the particles into memory?
   
    bool isAllData = false;         //return the all particle pointer?
    
        
    int datagridsize = -1;          //if use -1, then use the particle gridsize as the gridsize
                                    //otherwise use the user setting
    
    int parttype = 1;               //the particle type in the gadget file
    
    bool isSetBox = false;          //set up a box for the grids
    
    Point setStartPoint;            //start point of a box
    double gridboxsize;             //the boxsize for grids
    
    double boxsize = 32000.0;       //the boxsize of the simulation data
    
    int imagesize = 512;            //the render imagesize
    int numOfCuts = 0;              //the number of cuts of the feild to render
    float dz = 0;                   //the z-distance of each two cuts
    float startz = 0;               //the starting cuts
    bool isVelocity = false;        //whether encode velocity information
    
    
    void printUsage(string pname){  //print the usage
        fprintf(stdout,
                "Usage: %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n "
                "%s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n"
                , pname.c_str()
                , "[-imsize <imagesize>]"
                , "[-df <datafilename>]"
                , "[-dens <output density file>]"
                , "[-stream <output stream data file>]"
                , "[-velx <output velocity x-component file>]"
                , "[-vely <output velocity y-component file>]"
                , "[-velz <output velocity z-component file>]"
                , "[-t <numbers of tetra in memory>]"
                , "[-order] if the data is in order"
                , "[-v] to show verbose"
                , "[-dgridsize] default: -1 to use the npart^(1/3) as gridsize"
                , "[-parttype] default: 1. Use [0 ~ NTYPE]'s species of data in the GADGET file"
                , "[-lowmem] use low memory mode (don't load all part in mem, not recomended)"
                , "[-nalldata] load all particle data into memory, only usable in highmem mode"
                , "[-startz] the starting z-coordinates to calculate the cuts"
                , "[-dz] the interval between each 2 z-plane"
                , "[-numz] the number of z-planes"
                , "[-box <x0> <y0> <z0> <boxsize>] setup the start point,\n"
                  "and the boxsize. The box should be inside the data's box,\n "
                  "otherwise some unpredictable side effects will comes out"
                );
    }
    
                                    //read parameters
    void readParameters(int argv, char * args[]){
        int k = 1;
        if(argv == 1){
            return;
        }else{
            while(k < argv){
                stringstream ss;
                if(strcmp(args[k], "-imsize") == 0){
                    ss << args[k + 1];
                    ss >> imagesize;
                }else if(strcmp(args[k], "-df") == 0){
                    ss << args[k + 1];
                    ss >> filename;
                }else if(strcmp(args[k], "-dens") == 0){
                    ss << args[k + 1];
                    ss >> densityFilename;
                    if(densityFilename != "" &&
                       find (renderTypes.begin(), renderTypes.end(), DENSITY) == renderTypes.end()
                       ){
                        renderTypes.push_back(DENSITY);
                    }
                }else if(strcmp(args[k], "-velx") == 0){
                    ss << args[k + 1];
                    ss >> velocityXFilename;
                    if(velocityXFilename != "" &&
                       find (renderTypes.begin(), renderTypes.end(), VELOCITY_X) == renderTypes.end()
                       ){
                        renderTypes.push_back(VELOCITY_X);
                        isVelocity = true;
                    }
                }else if(strcmp(args[k], "-vely") == 0){
                    ss << args[k + 1];
                    ss >> velocityYFilename;
                    if(velocityYFilename != ""  &&
                       find (renderTypes.begin(), renderTypes.end(), VELOCITY_Y) == renderTypes.end()
                       ){
                        renderTypes.push_back(VELOCITY_Y);
                        isVelocity = true;
                    }
                }else if(strcmp(args[k], "-velz") == 0){
                    ss << args[k + 1];
                    ss >> velocityZFilename;
                    if(velocityZFilename != ""  &&
                       find (renderTypes.begin(), renderTypes.end(), VELOCITY_Z) == renderTypes.end()
                       ){
                        renderTypes.push_back(VELOCITY_Z);
                        isVelocity = true;
                    }
                }else if(strcmp(args[k], "-stream") == 0){
                    ss << args[k + 1];
                    ss >> streamFilename;
                    if(streamFilename != ""  &&
                       find (renderTypes.begin(), renderTypes.end(), STREAM) == renderTypes.end()
                       ){
                        renderTypes.push_back(STREAM);
                    }
                }else if(strcmp(args[k], "-t") == 0){
                    ss << args[k + 1];
                    ss >> inputmemgrid;
                }else if(strcmp(args[k], "-v") == 0){
                    isVerbose = true;
                    k = k -1;
                }else if(strcmp(args[k], "-order") == 0){
                    isInOrder = true;
                    k = k -1;
                }else if(strcmp(args[k], "-dgridsize") == 0){
                    ss << args[k + 1];
                    ss >> datagridsize;
                }else if(strcmp(args[k], "-parttype") == 0){
                    ss << args[k + 1];
                    ss >> parttype;
                }else if(strcmp(args[k], "-lowmem") == 0){
                    isHighMem = false;
                    k = k -1;
                }else if(strcmp(args[k], "-alldata") == 0){
                    isAllData = true;
                    k = k -1;
                }else if(strcmp(args[k], "-startz") == 0){
                    ss << args[k + 1];
                    ss >> startz;
                }else if(strcmp(args[k], "-dz") == 0){
                    ss << args[k + 1];
                    ss >> dz;
                }else if(strcmp(args[k], "-numz") == 0){
                    ss << args[k + 1];
                    ss >> numOfCuts;
                }else if(strcmp(args[k], "-box") == 0){
                    isSetBox = true;
                    k++;
                    ss << args[k] << " ";
                    ss << args[k + 1] << " ";
                    ss << args[k + 2] << " ";
                    ss << args[k + 3] << " ";
                    ss >> setStartPoint.x;
                    ss >> setStartPoint.y;
                    ss >> setStartPoint.z;
                    ss >> gridboxsize;
                    k += 2;
                }else{
                    printUsage(args[0]);
                    exit(1);
                }
                k += 2;
            }
        }
    }
    
}



using namespace main_space;

int main(int argv, char * args[]){
    
	readParameters(argv, args);
    
    //test
    TetraStreamer streamer(filename,
                           inputmemgrid,
                           parttype,
                           datagridsize,
                           isHighMem,
                           isAllData,
                           isVelocity,
                           true,
                           isInOrder);
    
    boxsize = streamer.getIndTetraStream()->getHeader().BoxSize;
    if(numOfCuts == 0){
        numOfCuts = imagesize;
        dz = boxsize / numOfCuts;
    }

    if(datagridsize == -1){
        datagridsize = ceil(pow(streamer.getIndTetraStream()->getHeader().npart[parttype], 1.0 / 3.0));
    }
    
    
    printf("\n=========================DENSITY ESTIMATION==========================\n");
	printf("*****************************PARAMETERES*****************************\n");
    printf("Render Image Size       = %d\n", imagesize);
	printf("Data File               = %s\n", filename.c_str());
    if(datagridsize == -1){
        printf("DataGridsize            = [to be determined by data]\n");
    }else{
        printf("DataGridsize            = %d\n", datagridsize);
    }
    printf("Particle Type           = %d\n", parttype);
    
    
    if(densityFilename != "")
        printf("Output File             = %s\n", densityFilename.c_str());
    
    if(streamFilename != "")
        printf("Stream Data File        = %s\n", streamFilename.c_str());
    
    if(velocityXFilename != "")
        printf("Velocity X File         = %s\n", velocityXFilename.c_str());
    
    if(velocityYFilename != "")
        printf("Velocity Y File         = %s\n", velocityYFilename.c_str());
    
    if(velocityZFilename != "")
        printf("Velocity Z File         = %s\n", velocityZFilename.c_str());
    
	printf("Tetra in Mem            = %d\n", inputmemgrid);
    printf("Rendering %d z-cuts of the density field. \nStart from z = %f, with dz = %f\n", numOfCuts, startz, dz);
    
    if(isSetBox){
        printf("Box                     = %f %f %f %f\n",
               setStartPoint.x, setStartPoint.y, setStartPoint.z, gridboxsize);
    }
	if(isInOrder){
		printf("The data is already in right order for speed up...\n");
	}
    if(!isHighMem){
        printf("Low Memory mode: slower in reading file...\n");
    }else{
        printf("Block Memory Operation:\n");
        if(!isAllData){
            printf("    Use Memory Copy Mode -- but may be faster without regenerating the tetras...\n");
        }else{
            printf("    Without Memory Copying Mode -- but may be slower in regenerating tetras...\n");
        }
        
    }
    
    printf("*********************************************************************\n");
    
    
    DenRender render(imagesize, boxsize,
                     startz, dz, numOfCuts, renderTypes);
    
    int count = 0;
    
    //render

    int tcount = datagridsize * datagridsize * datagridsize * 6 / 10;
    
    printf("Start rendering ...\n");
    while(streamer.hasNext()){
        int nums;
        Tetrahedron * tetras;
        tetras = streamer.getNext(nums);
        for(int i= 0; i < nums; i++){
            render.rend(tetras[i]);
            if((count %  tcount )== 0){
                printf(">");
                cout.flush();
            }
            count ++;
        }
    }
    render.finish();
    float * result = render.getResult();
    
    printf("\nFinished. In total %d tetrahedron rendered.\nSaving ...\n", count);
    
    //head used 256 bytes
    //the first is imagesize
    //the second the numOfCuts
    //the third is a float number boxsize
    //the 4-th is a float number startz
    //the 5-th is a fload number dz
    //All others are 0
    int head[59];
    
    string outputFilenames[] = {
        densityFilename,
        streamFilename,
        velocityXFilename,
        velocityYFilename,
        velocityZFilename
    };
    
    int numofrendertyps = renderTypes.size();
    if(numofrendertyps > render.NUM_OF_RENDERTRYPE_LIMIT)
        numofrendertyps = render.NUM_OF_RENDERTRYPE_LIMIT;
    
    fstream * outstreams = new fstream[numofrendertyps];
    
    for(int i = 0; i < numofrendertyps; i ++ ){
        
        if(outputFilenames[renderTypes[i]] != ""){
            outstreams[i].open(outputFilenames[renderTypes[i]].c_str(), ios::out | ios::binary);
            while(!outstreams[i].good()){
                printf("File error, calculation not saved for rendering type %d...!\n", renderTypes[i]);
                printf("Input new filename:\n");
                cin >> outputFilenames[renderTypes[i]];
                outstreams[i].clear();
                outstreams[i].open(outputFilenames[renderTypes[i]].c_str(), ios::out | ios::binary);
            }
            outstreams[i].write((char *) &imagesize, sizeof(int));
            outstreams[i].write((char *) &numOfCuts, sizeof(int));
            outstreams[i].write((char *) &boxsize, sizeof(float));
            outstreams[i].write((char *) &startz, sizeof(float));
            outstreams[i].write((char *) &dz, sizeof(float));
            outstreams[i].write((char *) head, sizeof(int) * 59);
            
            //printf("%d %d\n", renderTypes.size(), i);
            for(int j = 0; j < imagesize * imagesize * numOfCuts; j ++ ){
                
                outstreams[i].write((char *) (result + j * numofrendertyps + i),
                                    sizeof(float));
            }
            //outstreams[i].write((char *) result,
            //                    sizeof(float) * imagesize * imagesize * numOfCuts);
            outstreams[i].flush();
            outstreams[i].close();
        }
        
    }
    //outstream.open(gridfilename.c_str(), ios::out | ios::binary);
    printf("Finished!\n");
    
    return 0;
}

