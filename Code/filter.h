/********************************************************************************
 * Generated bandpass filter from http://www.schwietering.com/jayduino/filtuino/
 * Bandpass 100Hz-250Hz
 *******************************************************************************/

#ifndef FILTER_H
#define FILTER_H

class  FilterBuBp1
{
	public:
		FilterBuBp1()
		{
			v[0]=0.0;
			v[1]=0.0;
			v[2]=0.0;
		}
	private:
		float v[3];
	public:
		float bp_filter(float x) //class II 
		{
			v[0] = v[1];
			v[1] = v[2];
			v[2] = (4.604959984597731260e-2 * x)
				 + (-0.90992998817773751430 * v[0])
				 + (1.90050563479174350334 * v[1]);
			return 
				 (v[2] - v[0]);
		}
};

#endif
