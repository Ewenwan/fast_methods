#ifndef NDGRIDMAP_H_
#define NDGRIDMAP_H_

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cstddef>
#include <array> 

//#define MAXSIZE 1000000 // Uncomment if array ncells_ chosen

#include "../console/console.h"

template <class T, size_t ndims> class nDGridMap {
	friend std::ostream& operator << 
	(std::ostream & os, const nDGridMap<T,ndims> & g) {
		os << console::str_info("Grid cell information");
		os << "\t" << g.getCell(0).type() << std::endl;
		os << "\t" << g.ncells_ << " cells." << std::endl;
		os << "\t" << g.leafsize_ << " leafsize (m)." << std::endl;
		os << "\t" << ndims << " dimensions:" << std::endl;
		
		for (int i = 0; i < ndims; ++i)
			os << "\t\t" << "d" << i << "\tsize: " << g.dimsize_[i] << std::endl;   
			
		return os;
	}
	
    public: 
    
        nDGridMap<T,ndims>() {leafsize_ = 0.05;} // Default constructor not used.
        nDGridMap<T,ndims>
        (const std::array<int, ndims> & dimsize, const float leafsize = 0.05) {
			leafsize_ = leafsize;
			ncells_= 1;
			n_neighs = 0;
			
			resize(dimsize);
			
		}
       
        virtual ~nDGridMap<T,ndims>() {};  
        
        void resize
        (const std::array<int, ndims> & dimsize) {
			dimsize_ = dimsize;
			ncells_= 1;
			
			for (int i = 0; i < ndims; ++i) {
				ncells_ *= dimsize_[i];
				d_[i] = ncells_;
				
			}
			
			dd_[0] = d_[0];
			for (int i = 1; i < ndims; ++i) 
				dd_[i] *= d_[i] - dd_[i-1];
			
			//Resizing gridmap and initializing with default values.
			cells_.clear();
			cells_.resize(ncells_, T()); // Comment if array ncells_ chosen

			
			for (int i = 0; i < cells_.size(); ++i)
				cells_[i].setIndex(i);	
		}
		
        
        // grid[i] equivalent to grid.cells_[i];
        T & operator[]
        (const int idx) {
			return cells_[idx];
		} 
        
        float getLeafSize() const {return leafsize_;}
        
        int getNDims() const {return ndims;}
        
        T getCell 
        (const int idx) const {
			return cells_[idx];
			}
         
        std::array<int, ndims> getDimSizes() const     { return dimsize_;}
               
        float getMinValueInDim
        (const int idx, const int dim)   {
			n_neighs = 0;
			getNeighboursInDim(idx,n,dim);
			
			if (n_neighs == 1)
				return cells_[n[0]].getValue();
			else
				return (cells_[n[0]].getValue()<cells_[n[1]].getValue()) ? cells_[n[0]].getValue() : cells_[n[1]].getValue();
			
		}	
		
		// Returns the number of neighbours found. Assumes 4-connectivity.     
        int getNeighbours 
        (const int idx, std::array<int, 2*ndims> & neighs) {
			n_neighs = 0;
			for (int i = 0; i < ndims; ++i)
				getNeighboursInDim(idx,neighs,i);
				
			return n_neighs;
		}
		
		// Returns the number of neighbours found in this dimension. Assumes 4-connectivity.   
		void getNeighboursInDim
        (const int idx, std::array<int, 2*ndims>& neighs, const int dim) {
			int c1,c2;
			if (dim == 0) {
				c1 = idx-1;
				c2 = idx+1;
				// Checking neighbour 1.
				if ((c1 >= 0) && (c1/d_[0] == idx/d_[0]))
					neighs[n_neighs++] = c1;
				// Checking neighbour 2.
				//if ((c2 < ncells_) && (c2/d_[0] == idx/d_[0])) // full check, not necessary.
				if (c2/d_[0] == idx/d_[0])
					neighs[n_neighs++] = c2;
			}
			else {
				// Neighbours proposed.
				c1 = idx-d_[dim-1];
				c2 = idx+d_[dim-1];
				// Checking neighbour 1.
				if ((c1 >= 0) && (c1/d_[dim] == idx/d_[dim]))
					neighs[n_neighs++] = c1;
				// Checking neighbour 2.
				//if ((c2 < ncells_) && (c2/d_[dimi] == idx/d_[dim])) // full check, not necessary.
				if (c2/d_[dim] == idx/d_[dim])
					neighs[n_neighs++] = c2;
			}
		}
		
		
		int idx2coord 
		(const int idx, std::array<int, ndims> & coords) {
			if (coords.size() != ndims)
				return -1;
			else {
				coords[ndims-1] = idx/d_[ndims-2]; // First step done apart.
				int aux = idx - coords[ndims-1]*d_[ndims-2];
				for (int i = ndims - 2; i > 0; --i) {
					coords[i] = aux/d_[i-1];
					aux -= coords[i]*d_[i-1];
				}
				coords[0] = aux; //Last step done apart.
			}
			return 1;
		}
		
		int coord2idx
		(const std::array<int, ndims> & coords, int & idx) {
			if (coords.size() != ndims)
				return -1;
			else {
				idx = coords[0];
				for(int i = 1; i < ndims; ++i)
					idx += coords[i]*d_[i-1];
			}
			return 1;
		}
		
		void showCoords
		(const int idx) {
			std::array<int, ndims> coords(ndims);
			idx2coord(idx, coords);
			for (int i = 0; i < ndims; ++i)
				std::cout << coords[i] << "\t";
			std::cout << std::endl;
		}
		
		void showIdx
		(const std::array<int, ndims> & coords) {
			int idx;
			coord2idx(coords, idx);
			std::cout << idx << std::endl;
		}
				
		/* Saved grid format:
		 * CellClass - info of the cell type\n  (string)
		 * leafsize_\n 							(float)
		 * s\n									(size_t)
		 * dimsize_[0]\n						(int)
		 * dimsize_[1]\n						(int)
		 * ...
		 * dimsize_[ndims_-1]\n					(int)
		 * getCell(0).getValue()\n 	(			depends on whattosave)
		 * ...
		 * getCell(ncells_-1).getValue(whattosave)\n 	(depends on whattosave)
		 * */
        void saveGrid
        (const std::string filename, const int whattosave = 0) {
			std::ofstream ofs;
			ofs.open (filename,  std::ofstream::out | std::ofstream::trunc);
			
			ofs << getCell(0).type() << std::endl;
			ofs << leafsize_ << std::endl << ndims;
			
			for (int i = 0; i < ndims; ++i)
				ofs << std::endl << dimsize_[i] << "\t";
				   
			for (int i = 0; i < ncells_; ++i)
			ofs << std::endl << getCell(i).getValue();  
		}
		
		int size
		() const {
			return ncells_;
		}
		
		float getMaxValue
		() {
			float max = 0;
			for (T & c:cells_) {
				if (!isinf(c.getValue()) && c.getValue() > max)
					max = c.getValue();
			}
			return max;
		};
		
		// Choose between vector-based or array-based.
		//std::array<T, MAXSIZE> cells_; // A bit faster (1-5%) but allocates maximum memory. Does not allow resizing.
		std::vector<T> cells_;
        
    protected:
        std::array<int, ndims> dimsize_; // Vector containing the size of each dimension.
        float leafsize_; // It is assumed that the cells in the grid are cubic.
        int ncells_;
        
        // Auxiliar vectors to speed things up.
        std::array<int, ndims> d_; // Stores parcial multiplications of dimensions sizes. d_[0] = dimsize_[0];
																			   //   d_[1] = dimsize_[0]*dimsize_[1]; etc.
		std::array<int, ndims> dd_; // dd_[0] = d_[0], dd_[1] = d_[1] - dd_[0], and so on.
		std::array<int, 2*ndims> n; // For getMinValueInDim function. Size should be only ndims but for compatibility with getNeighboursInDim function.
		int n_neighs; // Internal variable that counts the number of neighbours found in every iteration. Modified by getNeighbours functions.
};


#endif /* NDGRIDCELL_H_*/