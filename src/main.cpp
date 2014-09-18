/*!
 * \file main.cpp
 * \author Cédric Andreolli (Intel)
 * \date 10 April 2013
 * This file contains the main algorithm used to match templates in a bmp file. The algorithm here is very simple and completly not optimized. 
 * Here are different things you can do:
 *<ul><li>Improve the algorithm:
 *		<ul><li>Take the rotations in consideration</li>
 * 		<li>Take a finesse granularity of scales</li>
 * 		<li>Take lights and luminosity in consideration</li>
 *		<li>Your goal is to create a good and fast algorithm to retrieve a pattern in a BMP file</li></ul>
 *   </li>
 *   <li>Optimize your algorithm:
 * 		<ul><li>Be careful with the memory utilization</li>
 *		<li>Avoid cache misses</li>
 *		<li>Use vectorization and Intrinsic functions</li></ul>
 *  </li>
 *<ul>
 * The notation will take in consideration your results on the xeon phi. Programming for the xeon phi is not easy (even if a classic C++ code can run natively on this card),
 * you will need to think your program as a program able to scale on massive parallel hardware. To help you debug and optimize your program, feel free to use Intel softwares
 * such as VTune Amplifier or Inspector (free one year licenses are available on <a href="http://www.intel-software-academic-program.com">this website</a>.
 */


/*! \mainpage Accelerate Your Code contest - Early 2013 
 *
 * \section intro Introduction
 * This section describes the code sample provided for the Intel* Accelerate Contests 2013 - early edition.
 * \section todo What do you have to do?
 * The code provided is just a very simple implementation of a pattern matching algorithm. You need to modify and improve this algorithm to achieve 2 goals:
 * <ul>
 *	<li>Create a better algorithm</li>
 *	<li>Create a faster algorithm</li>
 * </ul>
 * Those 2 aspects will be evaluated by Intel engineers and will be taken in consideration to choose the winners.
 * \section better_algo Create a better algorithm
 * By creating a better algorithm, we mean that your algorithm should be able to introduce rotation, luminosity and what ever you think important in a good pattern matching algorithm. You can also play with the scale factor
 * with more finesse. 
 * \section faster_algo Create a faster algorithm
 * To make your algorithm faster, you will need to optimize it. Optimization can take a lot of different aspects:
 * <ul>
 * 	<li>Avoiding cache misses</li>
 *	<li>Using compiler options</li>
 *	<li>Using vectorization</li>
 *	<li>Working on the algorithm to avoid un-necessarry computations</li>
 * </ul>
 * We will provide a simple documentation about this specific topic during the contest. 
 * \section xeon_phi The Xeon Phi
 * For this contest, you will have the chance to use the new Intel Xeon Phi. The Xeon Phi is a co-processor plugged on the motherboard via PCI. It allows to run 240 threads at a time. The great thing with Xeon Phi is that it is
 * able to run C++ native code. You also have access to 512 bits registers that gives vectorization a new meaning :)
 * But programming for such a device means that your program must be very well optimized to handle such a massive parallelism. A special attention will be given to the candidates who have good performances on this device.
 * \section questions Questions?
 * If you have any questions feel free to contact us though the forum or directly by email:
 * <ul>
 *	<li><a href="mailto:paul.guermonprez@intel.com" >Paul Guermonprez</a> - Academic Program Manager</li>
 *	<li><a href="mailto:cedric.andreolli@intel.com" >Cédric Andreolli</a> - Academic Program Software Developer Intern</li>
 * </ul>
 */

#include <iostream>
#include "BitmapImporter.h"
#include <omp.h>
#include <string>
#include <list>

using namespace std;

/*!
 *\struct Parameters
 *\brief This structure holds the application parameters
 */
typedef struct{
	int nb_threads;
	string main_image_name;
	list<std::string> template_names;
	int max_scale;
}Parameters;

/*!
 *\brief This structure hold a single result
 */
typedef struct{
	int pattern_ID;
	int position_x;
	int position_y;
}Result;

/*!
 * \brief Try to match the exact template in the main image starting at coordinates [h,w] in the main image.
 * \param main_image The main image.
 * \param template_image The template.
 * \param h The row number.
 * \param w the coloumn number.
 * \return template has been found at position [h,w] in the main image.  
 */
bool match_template(const Accelerate::Image& main_image, const Accelerate::Image& template_image, unsigned int h, unsigned int w);

/*!
 * \brief Read the parameters
 * \param argc The number of parameters (including the program call).
 * \param argv The array of parameters.
 * \return The parameters have been read correctly.
 */
bool read_parameters(int argc, char* argv[], Parameters& parameters);


bool compare_results(Result& first, Result& second){
	if(first.pattern_ID < second.pattern_ID) return true;
	else if(first.pattern_ID > second.pattern_ID) return false;

	if(first.position_x < second.position_x) return true;
	else if(first.position_x > second.position_x) return false;

	if(first.position_y < second.position_y) return true;
	else if(first.position_y > second.position_y) return false;

	return true;	
}

bool read_parameters(int argc, char* argv[], Parameters& parameters){
	if(argc < 4) return false;

	parameters.nb_threads = atoi(argv[1]);
	if(parameters.nb_threads < 0) return false;

	parameters.max_scale = atoi(argv[2]);
	if(parameters.max_scale <= 0) return false;

	parameters.main_image_name = string(argv[3]);

	for(unsigned int i=4; i<argc; i++){
		parameters.template_names.push_back(string(argv[i]));
	}
	return true;	
}

int main(int argc, char* argv[]){
	if(argc<=1) return 0;
	if(argv == NULL) return 0;

	Parameters parameters;
	list<Result> result_list;
	if(!read_parameters(argc, argv, parameters)){
		cout<<"Wrong number of parameters or invalid parameters..."<<endl;
		cout<<"The program must be called with the following parameters:"<<endl;
		cout<<"\t- num_threads: The number of threads"<<endl;
		cout<<"\t- max_scale: The maximum scale that can be applied to the templates in the main image"<<endl;
		cout<<"\t- main_image: The main image path"<<endl;
		cout<<"\t- t1 ... tn: The list of the template paths. Each template separated by a space"<<endl;
		cout<<endl<<"For example : ./run 4 3 img.bmp template1.bmp template2.bmp"<<endl;
		return -1;
	}
	//Read the main image
	Accelerate::Image main_image = Accelerate::Image::create_image_from_bitmap(parameters.main_image_name);
	//iterates over the pattern images
	for(string temp_name : parameters.template_names){ 
		//Read a specific pattern image
		Accelerate::Image template_image = Accelerate::Image::create_image_from_bitmap(temp_name);
		//Then retrieve the ID of the image (the 3 first characters
		int template_id = atoi(temp_name.substr(0, 3).c_str());	
		//Iterate over some possible scales (you can add more steps and you can also check rotations)
		//The sample is really simple because you have to create a good algorithm able to match
		//a pattern in an image
		for(unsigned int s=1; s<=parameters.max_scale; s++){
			//Create a scaled image
			Accelerate::Image temp = template_image.scale_image(s);
			//iterates on the main image pixels
			for(unsigned int wm=0; wm<main_image.get_width(); wm++){
				for(unsigned int hm=0; hm<main_image.get_height(); hm++){
					//Try to match the template
					if(match_template(main_image, temp, hm, wm)){
						//The pattern image has been found so save the result
						Result result;
						result.pattern_ID = template_id;
						result.position_x = wm;
						result.position_y = hm;
						result_list.push_back(result);		
					}
				}
			} 
	
		}
	}
	//sort the result list
	result_list.sort(compare_results);
	//Print the results
	for(Result res : result_list){
		cout<<res.pattern_ID<<"\t"<<res.position_x<<"\t"<<res.position_y<<endl;
	}

	return 0;
}



bool match_template(const Accelerate::Image& main_image, const Accelerate::Image& template_image, unsigned int h, unsigned int w){
	//The next cases are not possible
	if(main_image.get_width() - w < template_image.get_width()) return false;
	if(main_image.get_height() - h < template_image.get_height()) return false;
	//Iterates over the pattern and compare each pixel with the pixels of the main image
	for(unsigned int wt=0; wt<template_image.get_width(); wt++){
		for(unsigned int ht=0; ht<template_image.get_height(); ht++){
			//If a single pixel do not match, we return false
			//You can also improve this part of the algorithm as some images can have different contrasts,
			//lights, etc
			if(template_image.get_pixel(ht, wt).r != main_image.get_pixel(h+(ht), w+(wt)).r ||
				template_image.get_pixel(ht, wt).g != main_image.get_pixel(h+(ht), w+(wt)).g || 
				template_image.get_pixel(ht, wt).b != main_image.get_pixel(h+(ht), w+(wt)).b){ 
				return false;
			}
		}
	}
	return true;
}

