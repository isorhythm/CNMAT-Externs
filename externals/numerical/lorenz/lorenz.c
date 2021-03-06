/*
 Written by Michael Zbyszynski, The Center for New Music and Audio Technologies,
 University of California, Berkeley.  Copyright (c) 2006-09, The Regents of
 the University of California (Regents). 
 Permission to use, copy, modify, distribute, and distribute modified versions
 of this software and its documentation without fee and without a signed
 licensing agreement, is hereby granted, provided that the above copyright
 notice, this paragraph and the following two paragraphs appear in all copies,
 modifications, and distributions.
 
 IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 
 
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 NAME: lorenz
 DESCRIPTION: generate a lorenz atttractor
 AUTHORS: Michael Zbyszynski
 COPYRIGHT_YEARS: 2006-09
 DRUPAL_NODE: /patch/6666
 SVN_REVISION: $LastChangedRevision: 2500 $
 VERSION 1.0: first release
 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 */

/* starting point for a Max external object ------- */

/* the required include file(s) */
#include "ext.h"

/* structure definition of object */
typedef struct lorenz
{
	t_object l_ob;		// object header - ALL objects MUST begin with this...
	
	float l_x;					// stored values
	float l_y;
	float l_z;
	float l_h;
	
	void *l_out0;				// outlets
	void *l_out1;
	void *l_out2;
					
} t_lorenz;

/* global pointer that holds the class definition */
void *lorenz_class;

/* prototypes for methods */

void lorenz_bang(t_lorenz *x);
void lorenz_float(t_lorenz *x, float n);   
void lorenz_ft1(t_lorenz *x, float o);
void lorenz_ft2(t_lorenz *x, float p);
void lorenz_ft3(t_lorenz *x, float q);
void *lorenz_new(t_symbol *s, short argc, t_atom *argv);

/* initialization routine */

int main(void)
{
	/* define class. */
	setup((t_messlist **)&lorenz_class, (method)lorenz_new, (method)0L, (short)sizeof(t_lorenz), 0L, A_GIMME,0);
	
	/* bind methods to symbols */
	addbang((method)lorenz_bang);
	addfloat((method)lorenz_float);
	addftx((method)lorenz_ft1, 1);	
	addftx((method)lorenz_ft2, 2);
	addftx((method)lorenz_ft3, 3);
	
	post("lorenz object by Michael F. Zbyszynski, 29.v.02",0);
	
	/* list object in the new object list */
	finder_addclass("Data","lorenz");
	
	return 0;
}
//--------------------------------------------------------------------------

/* methods */

void lorenz_bang(t_lorenz *x)
{
	float xnew;
	float ynew;
	float znew;
	

	znew = x->l_z+(x->l_h * (( x->l_x * x->l_y) - ((8.0/3.0) * x->l_z)));
	outlet_float(x->l_out2, znew);
	
	ynew = x->l_y + (x->l_h * ((28.0 * x->l_x - x->l_y) - ( x->l_x * x->l_z)));
	outlet_float(x->l_out1, ynew);
	
	xnew = ( x->l_x + ((x->l_h * 10.0) * (x->l_y - x->l_x )));
	outlet_float(x->l_out0, xnew);
	
	
	/* set all the variables to new values */
	
	x->l_x = xnew;
	x->l_y = ynew;
	x->l_z = znew;
}

//--------------------------------------------------------------------------

void lorenz_float(t_lorenz *x, float n)
{
	float xnew;
	float ynew;
	float znew;
	
	x->l_x = n;									// store a value inside the object
	

		
	znew = x->l_z+(x->l_h * (( n * x->l_y) - ((8.0/3.0) * x->l_z)));
	outlet_float(x->l_out2, znew);
	
	ynew = x->l_y + (x->l_h * ((28.0 * n - x->l_y) - ( n * x->l_z)));
	outlet_float(x->l_out1, ynew);
	
	xnew = ( n + ((x->l_h * 10.0) * (x->l_y - n )));
	outlet_float(x->l_out0, xnew);
	
	/* set all the variables to new values */
	
	x->l_x = xnew;
	x->l_y = ynew;
	x->l_z = znew;	
	
}

//--------------------------------------------------------------------------


void lorenz_ft1(t_lorenz *x, float o)
{
	
	x->l_y = o;									// store a value inside the object
	
}

//--------------------------------------------------------------------------


void lorenz_ft2(t_lorenz *x, float p)
{
	
	
	x->l_z = p;									// store a value inside the object
	
}

//--------------------------------------------------------------------------


void lorenz_ft3(t_lorenz *x, float q)
{

	
	x->l_h = q;									// store a value inside the object
	
}

//--------------------------------------------------------------------------

/* instance creation routine */

void *lorenz_new(t_symbol *s, short argc, t_atom *argv)
{
	t_lorenz *x;
	float EX, WHY, ZEE, HACH;
	
	x = (t_lorenz *)newobject(lorenz_class);		// get memory for a new object & initialize 
	

	if (argc>0){
		if (argv[0].a_type == A_FLOAT)
			EX = argv[0].a_w.w_float;
		else if (argv[0].a_type == A_LONG)
			EX = (float)argv[0].a_w.w_long;
		else 	EX = 0.6;
	}
	else EX = ZEE = WHY = 0.6, HACH = 0.01;
	
	 if (argc>1){
		if (argv[1].a_type == A_FLOAT)
			WHY = argv[1].a_w.w_float;
		else if (argv[1].a_type == A_LONG)
			WHY = (float)argv[1].a_w.w_long;
		else 	WHY = 0.6;
	}
	if (argc>2){
		if (argv[2].a_type == A_FLOAT)
			ZEE = argv[2].a_w.w_float;
		else if (argv[2].a_type == A_LONG)
			ZEE = (float)argv[2].a_w.w_long;
		else 	ZEE = 0.6;
	}
	 if (argc>3){
		if (argv[3].a_type == A_FLOAT)
			HACH = argv[3].a_w.w_float;
		else if (argv[3].a_type == A_LONG)
			HACH = (float)argv[3].a_w.w_long;
		else 	HACH = 0.01;
	}
	  	
	
	floatin(x,3);
	floatin(x,2);									// create inlets
	floatin(x,1);
	
	x->l_out2 = floatout(x);						// create outlets
	x->l_out1 = floatout(x);
	x->l_out0 = floatout(x);
	
	
	
	x->l_h = HACH;
	x->l_z = ZEE;
	x->l_y = WHY;
	x->l_x = EX;

	return (x);									// return newly created object to caller
}

	
	

	