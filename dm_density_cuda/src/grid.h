/*
 * grids.cpp
 *
 *  Created on: Dec 18, 2012
 *      Author: lyang
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

#include "tetrahedron.h"
#include "griddef.h"
#include "types.h"

template <class GRIDTYPE>
void Grids<GRIDTYPE>::initialize(string filename, int gridsize, int subgridsize, GRIDTYPE init_v){
	filename_ = filename;
	gridsize_ = gridsize;
	subgridsize_ = subgridsize;
	if(gridsize % subgridsize != 0){
		printf("Subgridsize (%d) must be a dividor of Gridsize(%d)!.\n", subgridsize, gridsize);
		exit(1);
	}
	subgrid_num = gridsize / subgridsize;// + 1;
	subgrid_num = subgrid_num * subgrid_num * subgrid_num;
	int loop_j;
	for(loop_j = 0; loop_j < subgrid_num; loop_j ++){
		grid_ = new GRIDTYPE[subgridsize_ * subgridsize_ * subgridsize_];

		int loop_i;
		//clear
		for(loop_i = 0; loop_i < subgridsize_ * subgridsize_ * subgridsize_; loop_i ++){
			grid_[loop_i] = init_v;
		}
		grid_lists.push_back(grid_);
		//for(loop_i = 0; loop_i < subgrid_num; loop_i ++){
		//	current_block_ind = loop_i;
		//	saveGrid();
		//}
	}
	current_block_ind = 0;

}

template <class GRIDTYPE>
Grids<GRIDTYPE>::Grids<GRIDTYPE>(string filename, int gridsize, int subgridsize, GRIDTYPE init_v){
	initialize( filename, gridsize, subgridsize, init_v);
	
	boxStartPoint_.x = 0;
	boxStartPoint_.y = 0;
	boxStartPoint_.z = 0;

	boxEndPoint_.x = 32000;
	boxEndPoint_.y = 32000;
	boxEndPoint_.z = 32000;

	grid_header.BoxSize = 32000;
	grid_header.npart[0] = gridsize;
	grid_header.npartTotal[0] = gridsize * gridsize * gridsize;
}

template <class GRIDTYPE>
Grids<GRIDTYPE>::Grids<GRIDTYPE>(std::string filename, int gridsize, int subgridsize, GRIDTYPE init_v,
		Point boxStartPoint, Point boxEndPoint){
	
	initialize( filename, gridsize, subgridsize, init_v);
	
	boxStartPoint_.x = boxStartPoint.x;
	boxStartPoint_.y = boxStartPoint.y;
	boxStartPoint_.z = boxStartPoint.z;

	boxEndPoint_.x = boxEndPoint.x;
	boxEndPoint_.y = boxEndPoint.y;
	boxEndPoint_.z = boxEndPoint.z;

	grid_header.BoxSize = boxEndPoint.x - boxStartPoint.x;
	grid_header.npart[0] = gridsize;
	grid_header.npartTotal[0] = gridsize * gridsize * gridsize;
}

template <class GRIDTYPE>
Grids<GRIDTYPE>::~Grids<GRIDTYPE>(){
	delete grid_;
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::getSubGridNum(){
	return subgrid_num;
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::getGridSize(){
	return this->gridsize_;
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::getSubGridSize(){
	return this->subgridsize_;
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::getCurrentInd(){
	return this->current_block_ind;
}

template <class GRIDTYPE>
bool Grids<GRIDTYPE>::loadGrid(int ind){
	grid_ = grid_lists[ind];
	current_block_ind = ind;
	return true;
}

template <class GRIDTYPE>
bool Grids<GRIDTYPE>::loadGrid(int i, int j, int k){
	return loadGrid(this->convertToIndex(i, j, k));
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::saveGrid(){
}

template <class GRIDTYPE>
GRIDTYPE * Grids<GRIDTYPE>::getSubGrid(){
	return this -> grid_;
}

template <class GRIDTYPE>
bool Grids<GRIDTYPE>::loadGridByActualCoor(int i, int j, int k){
	if(isInCurrentBlock(i, j, k)){
		return true;
	}else{
		int ind = convertToIndexByActualCoor(i, j, k);
		//printf("ind %d\n", ind);
		if(ind >= this->subgrid_num){
			//printf("afdafadfasf %d\n", subgrid_num);
			return false;
		}else{
			//printf("ind %d\n", ind);
			return this->loadGrid(ind);
		}

	}
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::convertToIndex(int i, int j, int k){
	int l = gridsize_ / subgridsize_;// + 1;
	return k * l * l + j * l + i;
}

template <class GRIDTYPE>
int Grids<GRIDTYPE>::convertToIndexByActualCoor(int i, int j, int k){
	int i_ = i / subgridsize_;
	int j_ = j / subgridsize_;
	int k_ = k / subgridsize_;
	int ind = convertToIndex(i_, j_, k_);
	return ind;
}

template <class GRIDTYPE>
bool Grids<GRIDTYPE>::isInCurrentBlock(int i, int j, int k){
	int ind = convertToIndexByActualCoor(i, j, k);
	//printf("dddd %d \n", ind);
	if(ind == current_block_ind){
		return true;
	}else{
		return false;
	}
}

template <class GRIDTYPE>
string Grids<GRIDTYPE>::getSubGridFileName(int ind){
	string suffix = static_cast<ostringstream*>( &(ostringstream() << ind) )->str();
	return this->filename_ + suffix;
}

template <class GRIDTYPE>
GRIDTYPE Grids<GRIDTYPE>::getValueByActualCoor(int i, int j, int k){
	if(! isInCurrentBlock(i, j, k)){
		//printf("%d %d %d", i, j, k);
		saveGrid();
		loadGridByActualCoor(i, j, k);
	}
	int i_, j_, k_;
	actual2Current(i, j, k, i_, j_, k_);
	return getValue(i_, j_, k_);
}

template <class GRIDTYPE>
GRIDTYPE Grids<GRIDTYPE>::getValue(int i, int j, int k){
	return grid_[k * subgridsize_ * subgridsize_ + j * subgridsize_ + i];
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::actual2Current(int ai, int aj, int ak, int & ci, int & cj, int & ck){
	int i0 = ai / subgridsize_ * subgridsize_;
	int j0 = aj / subgridsize_ * subgridsize_;
	int k0 = ak / subgridsize_ * subgridsize_;
	ci = ai - i0;
	cj = aj - j0;
	ck = ak - k0;
}


template <class GRIDTYPE>
void Grids<GRIDTYPE>::current2Actual(int ci, int cj, int ck, int & ai, int & aj, int & ak){
	int nsubg = gridsize_ / subgridsize_;// + 1;

	int i0 = (current_block_ind % nsubg) * subgridsize_;
	int j0 = (current_block_ind / nsubg % nsubg) * subgridsize_;
	int k0 = (current_block_ind / nsubg / nsubg % nsubg) * subgridsize_;

	//printf("%d %d %d %d %d\n", current_block_ind, i0, j0, k0, nsubg);

	ai = ci + i0;
	aj = cj + j0;
	ak = ck + k0;
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::setValue(int i, int j, int k, GRIDTYPE value){
	grid_[k * subgridsize_ * subgridsize_ + j * subgridsize_ + i] = value;
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::setValueByActualCoor(int i, int j, int k, GRIDTYPE value){
	if(! isInCurrentBlock(i, j, k)){
		saveGrid();
		loadGridByActualCoor(i, j, k);
	}
	int ci, cj, ck;
	actual2Current(i, j, k, ci, cj, ck);
	//cout << "ci cj ck currentind: " << ci << " " << cj << " " << ck << " " << this->current_block_ind << endl;
	setValue(ci, cj, ck, value);
}

template <class GRIDTYPE>
Point Grids<GRIDTYPE>::getStartPoint(){
	return boxStartPoint_;
}

template <class GRIDTYPE>
Point Grids<GRIDTYPE>::getEndPoint(){
	return boxEndPoint_;
}

template <class GRIDTYPE>
Point Grids<GRIDTYPE>::getPointByActualCoor(int i, int j, int k){
	REAL fx, fy, fz;
	Point retP;
	fx = (REAL) i / (REAL) gridsize_;
	fy = (REAL) j / (REAL) gridsize_;
	fz = (REAL) k / (REAL) gridsize_;
	retP.x = fx * (boxEndPoint_.x - boxStartPoint_.x) + boxStartPoint_.x;
	retP.y = fy * (boxEndPoint_.y - boxStartPoint_.y) + boxStartPoint_.y;
	retP.z = fz * (boxEndPoint_.z - boxStartPoint_.z) + boxStartPoint_.z;
	return retP;
}

template <class GRIDTYPE>
Point Grids<GRIDTYPE>::getPoint(int i, int j, int k){
	int ai, aj, ak;
	current2Actual(i, j, k, ai, aj, ak);
	return getPointByActualCoor(ai, aj, ak);
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::saveToFile(){
	ofstream gridFile (filename_.c_str(), ios::out | ios::binary);
	int i,j,k;

	//write the header
	gridFile.write((char *) & grid_header, sizeof(gadget_header));
	GRIDTYPE * tempsubgrid_ = new GRIDTYPE[subgridsize_ * subgridsize_ * subgridsize_];
	int tgs = getGridSize();
	int count = 0;
	for (i = 0; i < tgs; i++) {
		for (j = 0; j < tgs; j++) {
			for (k = 0; k < tgs; k++) {
				GRIDTYPE v = getValueByActualCoor(i, j, k);
				tempsubgrid_[count] = v;
				count ++;
				if(count >= subgridsize_ * subgridsize_ * subgridsize_){
					count =0;
					gridFile.write((char *) tempsubgrid_, sizeof(GRIDTYPE) * subgridsize_ * subgridsize_ * subgridsize_);
				}
			}
		}
	}
	delete tempsubgrid_;

	gridFile.close();
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::saveToFileOld(){
	ofstream gridFile (filename_.c_str(), ios::out | ios::binary);
	int i;
	
	for(i = 0; i < this->getSubGridNum(); i++){
		this->loadGrid(i);
		gridFile.write((char *) &(this->current_block_ind), sizeof(int));
		gridFile.write((char *) &(this->subgridsize_), sizeof(int));
		gridFile.write((char *) &(this->gridsize_), sizeof(int));
		gridFile.write((char *) this->grid_, sizeof(GRIDTYPE) * subgridsize_ * subgridsize_ * subgridsize_);
	}

	gridFile.close();
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::loadFromFileOld(string gridfilename){
	ifstream gridFile (gridfilename.c_str(), ios::in | ios::binary);
	
	int i;
	for(i = 0; i < this->getSubGridNum(); i++){
		this->loadGrid(i);
		gridFile.read((char *) &(this->current_block_ind), sizeof(int));
		gridFile.read((char *) &(this->subgridsize_), sizeof(int));
		gridFile.read((char *) &(this->gridsize_), sizeof(int));
		gridFile.read((char *) this->grid_, sizeof(GRIDTYPE) * subgridsize_ * subgridsize_ * subgridsize_);
	}

	gridFile.close();
}

template <class GRIDTYPE>
void Grids<GRIDTYPE>::loadFromFile(string gridfilename){
	ifstream gridFile (gridfilename.c_str(), ios::in | ios::binary);
	int i,j,k;

	//write the header
	gridFile.read((char *) & grid_header, sizeof(gadget_header));
	if(this->gridsize_ != (grid_header.npart)[0]){
		printf("Input file do not have the same grid with the output file!\n");
		exit(1);
	}
	int tgs = getGridSize();
	for (i = 0; i < tgs; i++) {
		for (j = 0; j < tgs; j++) {
			for (k = 0; k < tgs; k++) {
				GRIDTYPE v;
				gridFile.read((char *) &v, sizeof(GRIDTYPE));
				this->setValueByActualCoor(i, j, k, v);
			}
		}
	}
	gridFile.close();
}