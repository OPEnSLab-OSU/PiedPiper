#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <direct.h>
using namespace std;

/********************************************************
*	Read recorded audio data from Pied Piper SD card
*	and separate them into individual recordings for
*	analysis
*	Requires the generated RECORD.txt file from Pied 
*	Piper 
********************************************************/

int main(){
	string path;	//saves full path to file location
	string dir;	//saves just the parent directory of the file location
	path = "D:\\RECORD.txt";
	
	/*	printf("Input file path: ");		//uncomment to set path in runtime
	cin >> path;
	*/

	int direct = path.length();	//determine the home directory
	while(path[direct]!='\\'){
		direct--;
	}
	dir = path.substr(0,direct+1);	//set the home directory
	printf("Path: %s Directory: %s\n",path.c_str(),dir.c_str());	//display directory for debug purposes
	ifstream input;	//create object for inputting
	
	input.open(path.c_str());	//opens file location
	string temp;	//string for holding line from text file
	string f_name;	//string for file name for each recording
	string save = dir;	//sets path for saving in defined directory
	save+="Rec\\";
	_mkdir(save.c_str()); //creates a directory to save recordings into
	while(!input.eof()){	//read the entire text file
		getline(input,temp);	
		
		if(temp.find("[")!=-1){	//if a new recording is found, save it into its own text file
			int posf = temp.find('[');
			int pose = temp.rfind(']');
			f_name = temp.substr(posf+1,pose-posf-1);	//get the time and date of the recording
			f_name+=".txt";
			getline(input,temp);	//move to next line
			getline(input,temp);	//get the audio data from text file
			save = dir;
			save+="Rec\\";
			save+=f_name;	//generate the path for the save file
			cout << endl << "Opening this: " << save << endl;
			ofstream output(save.c_str());	//create new text file with time/date as name
			if(!output.is_open())
				cout << "Could not open file\n";
			else{
				output << temp;	//write audio data to text file
				output.close();
			}
		}
	}
	input.close();
	return 0;
}
