/*!
 * \file BitmapImporter.cpp
 * \brief Contains the implementation of the functions declared in BitmapImporter.h
 * \author Cédric Andreolli (Intel)
 * \date 10 April 2013
 */
#include "BitmapImporter.h" 

using namespace std;

Accelerate::Image Accelerate::Image::create_image_from_bitmap(const std::string file_name){
	Accelerate::Image result;
	ifstream file(file_name.c_str(), ios::in|ios::binary|ios::ate);
	if (file.is_open())
  	{
		Accelerate::HeaderStr header;
    		file.seekg (0, ios::beg);
		//Read the header first
    		file.read ((char*)&header, sizeof(HeaderStr));

		result.width = header.width;
		result.height = header.height;

		//Put the cursor on the BMP data
		file.seekg(header.offset, ios::beg);
		int bytes_per_pixel = header.bit_per_pixel / 8;
		int padding = ((header.width*bytes_per_pixel) % 4 == 0) ? 0 : 4 - ((header.width*bytes_per_pixel) % 4);
		//Allocate the size needed to read the BMP data
		int size = sizeof(char)*(header.height*(header.width + padding))*bytes_per_pixel;
		unsigned char* data = (unsigned char*) malloc(size);
		//Read the data
		file.read((char*)data, size);
		//Create the Accelerate::Bitmap object
		result.pixel_data = (PixelStr*) malloc(header.width*header.height*sizeof(PixelStr));
		unsigned int offset = 0;
		//In the Bitmap format, pixels are in a reversed order
		for(int i=header.height-1; i>=0; i--){
			for(int j=0; j<header.width; j++){
				result.pixel_data[i*header.width + j].b = data[offset++];
				result.pixel_data[i*header.width + j].g = data[offset++];
				result.pixel_data[i*header.width + j].r = data[offset++];
			}
			offset+=padding;			
		}
		file.close();
		free(data);
	}
	return result;
}

Accelerate::Image::Image(){
		
}

Accelerate::Image::~Image(){
	free(pixel_data);
}

Accelerate::PixelStr Accelerate::Image::get_pixel(unsigned int i, unsigned int j) const{
	return this->pixel_data[this->width*i + j];
}

std::ostream& operator<<(std::ostream &o, const Accelerate::PixelStr& p){
    return o << "[" << (int)p.r << ", " << (int)p.g << ", " << (int)p.b  << "] ";
}

Accelerate::Image Accelerate::Image::scale_image(unsigned int scale) const{
	Accelerate::Image result;

	result.width = width * scale;
	result.height = height * scale;
	
	result.pixel_data = (PixelStr*) malloc(result.width*result.height*sizeof(PixelStr));

	for(unsigned int w=0; w<this->width; w++){
		for(unsigned int i=0; i<scale; i++)
			copy_column(result, w, scale); 
	}
	
	return result;
}

void Accelerate::Image::copy_column(Accelerate::Image& result, unsigned int column_number, unsigned int scale) const{
	//retrieve the column indice in the result image
	unsigned int first_column_indice = column_number * scale;
	for(unsigned int i=0; i<scale; i++){
		for(unsigned int row=0; row<height; row++){
			for(unsigned int j=0; j<scale; j++){
				result.pixel_data[(row*scale + j)*result.width + first_column_indice + i] = this->pixel_data[row*this->width + column_number]; 
			}
		}
	}
} 


std::ostream& operator<<(std::ostream &o, Accelerate::Image& im){
	Accelerate::PixelStr* pixels = im.get_pixels();	
	for(unsigned int i=0; i<im.get_height(); i++){
		for(unsigned int j=0; j<im.get_width(); j++){
			Accelerate::PixelStr pixel = pixels[i*im.get_width() + j];
			o<<pixel<<" ";
		}
		o<<std::endl;
	}

	return o; 
}
