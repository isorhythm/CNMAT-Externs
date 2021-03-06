/*
Copyright (c) 1999,2000,01,02,03,04,05,06.  The Regents of the
University of California (Regents).  All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Adrian Freed, The Center for New Music and Audio Technologies,
University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
NAME: decaying-sinusoids~
DESCRIPTION: Additive synthesis of a bank of exponentially decaying sinusoids
AUTHORS: Adrian Freed and Matt Wright
COPYRIGHT_YEARS: 1996,97,98,99,2000,01,02,03,04,05,06
DRUPAL_NODE: /patch/4072
SVN_REVISION: $LastChangedRevision$
VERSION 0.0: Adrian's initial version 
VERSION 0.0.1: Force Package Info Generation
VERSION 0.1: min_vtime and max_vtime affect goto.  Also "clear" now doesn't clear the model.
VERSION 0.2: signal inlet for virtual time.  Also version message.
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  

       	�1988,1989 Adrian Freed
	�1999,2000,01,02,03,04,05,06 UC Regents, All Rights Reserved. 
	
*/
#define NAME "decaying-sinusoids~"
#define DESCRIPTION "Additive synthesis of a bank of exponentially decaying sinusoids"
#define AUTHORS "Adrian Freed and Matt Wright"
#define COPYRIGHT_YEARS "1996-99,2000-06,12,13"



#include "ext.h"
#include "ext_obex.h"

#include "z_dsp.h"
#include <math.h>

#include "version.h"


#ifdef WIN_VERSION
#define sinf sin
#endif


#undef PI
#define PI 3.14159265358979323f
#define MAXOSCILLATORS 256

#define TPOW 14
#define STABSZ (1l<<TPOW)
#define LOGBASE2OFTABLEELEMENT 2

t_class *sinusoids_class;

float Sinetab[STABSZ];

//typedef  unsigned long ulong;


typedef  struct oscdesc
{
//	float next_amplitude;
	float amplitude;		/* amplitude */
	unsigned long phase_current;
//	long next_phase_inc;
	long phase_inc;			/* frequency */
	double gain,rate;
} oscdesc;


/* bank of oscillators */
typedef struct 
{
	t_pxobject b_obj;
//	short b_connected;
	oscdesc base[MAXOSCILLATORS];
	int nosc; 
	float  pk;
	float samplerate;
	float sampleinterval;
	double t;		// time machine
	double rate, stop, backstop;
	double pulse;

} oscbank;
typedef oscbank t_sinusoids;

t_int *sinusoids2_perform(t_int *w);
t_int *decayingsinusoids_perform_time_input_signal(t_int *w);
void sinusoids2_perform64(t_sinusoids *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void decayingsinusoids_perform_time_input_signal64(t_sinusoids *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void sinusoids_dsp64(t_sinusoids *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void sinusoids_dsp(t_sinusoids *x, t_signal **sp, short *connect);
 void sinusoids_list(t_sinusoids *x, t_symbol *s, short argc, t_atom *argv);
 void sinusoids_clear(t_sinusoids *x);
 void sinusoids_assist(t_sinusoids *x, void *b, long m, long a, char *s);
 void *sinusoids_new(t_symbol *s, short argc, t_atom *argv);

void sinusoids_dsp64(t_sinusoids *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	if (count[0]) {
		// Signal inlet is connected; that's virtual time
		object_method(dsp64, gensym("dsp_add64"), x, decayingsinusoids_perform_time_input_signal64, 0, NULL);
	} else {
		// Old perform method, with only message control of time
		object_method(dsp64, gensym("dsp_add64"), x, sinusoids2_perform64, 0, NULL);
	}
}

void sinusoids_dsp(t_sinusoids *x, t_signal **sp, short *count)
{

	if (count[0]) {
		// Signal inlet is connected; that's virtual time
		dsp_add(decayingsinusoids_perform_time_input_signal, 
				4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
	} else {
		// Old perform method, with only message control of time
		dsp_add(sinusoids2_perform, 3, x, sp[1]->s_vec,  sp[1]->s_n);
	}
}

void sinusoids2_perform64(t_sinusoids *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	//t_float *out = (t_float *)(w[2]);
	double *out = outs[0];
	t_sinusoids *op = x;
	long n = sampleframes;
	int nosc = op->nosc;
	int i,j;
	oscdesc *o = op->base;
	const char *st = (const char *)Sinetab;
	float rate ;
	//	if(op->b_obj.z_disabled)
	//		goto out;
	
	rate = 1.0f/n;
	for(j=0;j<n;++j)
		out[j] = 0.0f;


	for(i=0;i<nosc;++i)
		{
			register float a = op->pulse*exp(-o->rate*op->t)*o->gain;
			register float ascale = exp(-o->rate*op->sampleinterval*op->rate);
			register long pi = o->phase_inc;
			register unsigned long pc = o->phase_current;
			//		register float astep = (nexta - o->amplitude)*rate;

			//		if(op->t<1.0)
			//			post("%d", o->phase_current);
			for(j=0;j<n;++j)
				{

					out[j] +=  a  * 
						*((float *)(st + (((pc) >> (32-TPOW-LOGBASE2OFTABLEELEMENT))
								  & ((STABSZ-1)*sizeof(*Sinetab)))));
					pc +=  pi;
					a *=ascale;

				}
			o->phase_current = pc;
			++o;
		}
	op->t += op->rate*n*op->sampleinterval;
	if(op->t<op->backstop)
		op->t = op->backstop;
	if(op->t>op->stop)
		op->t = op->stop;
}


t_int *sinusoids2_perform(t_int *w)
{
		t_float *out = (t_float *)(w[2]);
	t_sinusoids *op = (t_sinusoids *)(w[1]);
	int n = (int)(w[3]);
	int nosc = op->nosc;
	 int i,j;
	 oscdesc *o = op->base;
	 const char *st = (const char *)Sinetab;
	 float rate ;
//	if(op->b_obj.z_disabled)
//		goto out;
	
	rate = 1.0f/n;
	for(j=0;j<n;++j)
		out[j] = 0.0f;


	for(i=0;i<nosc;++i)
	{
		register float a = op->pulse*exp(-o->rate*op->t)*o->gain;
		register float ascale = exp(-o->rate*op->sampleinterval*op->rate);
		register long pi = o->phase_inc;
		register unsigned long pc = o->phase_current;
//		register float astep = (nexta - o->amplitude)*rate;

//		if(op->t<1.0)
//			post("%d", o->phase_current);
		for(j=0;j<n;++j)
		{

			out[j] +=  a  * 
		*((float *)(st + (((pc) >> (32-TPOW-LOGBASE2OFTABLEELEMENT))
					& ((STABSZ-1)*sizeof(*Sinetab)))));
			pc +=  pi;
			a *=ascale;

		}
		o->phase_current = pc;
		++o;
	}
	op->t += op->rate*n*op->sampleinterval;
	if(op->t<op->backstop)
		op->t = op->backstop;
	if(op->t>op->stop)
		op->t = op->stop;
out:	return (w+4);
}

void decayingsinusoids_perform_time_input_signal64(t_sinusoids *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_sinusoids *op = x;
	double *time = ins[0];
	double *out = outs[0];
	long n = sampleframes;
	int nosc = op->nosc;
	int i,j;
	oscdesc *o = op->base;
	const char *st = (const char *)Sinetab;
	float rate ;
	//	if(op->b_obj.z_disabled)
	//		goto out;
	
	rate = 1.0f/n;
	for(j=0;j<n;++j)
		out[j] = 0.0f;


	for(i=0;i<nosc;++i)
		{
			// register float a = op->pulse*exp(-o->rate*op->t)*o->gain;
			// register float ascale = exp(-o->rate*op->sampleinterval*op->rate);
			register long pi = o->phase_inc;
			register unsigned long pc = o->phase_current;
			//		register float astep = (nexta - o->amplitude)*rate;
			register float t, a;

			//		if(op->t<1.0)
			//			post("%d", o->phase_current);
			for(j=0;j<n;++j)
				{
					// Super-inefficient version calls exp in the inner loop!
					a = op->pulse *exp(-o->rate*time[j])*o->gain;
					out[j] +=  a  * 
						*((float *)(st + (((pc) >> (32-TPOW-LOGBASE2OFTABLEELEMENT))
								  & ((STABSZ-1)*sizeof(*Sinetab)))));
					pc +=  pi;
					//			a *=ascale;

				}
			o->phase_current = pc;
			++o;
		}
	op->t += op->rate*n*op->sampleinterval;
	if(op->t<op->backstop)
		op->t = op->backstop;
	if(op->t>op->stop)
		op->t = op->stop;
}

t_int *decayingsinusoids_perform_time_input_signal(t_int *w) {
	t_sinusoids *op = (t_sinusoids *)(w[1]);
	t_float *time = (t_float *)(w[2]);
	t_float *out = (t_float *)(w[3]);
	int n = (int)(w[4]);
	int nosc = op->nosc;
	 int i,j;
	 oscdesc *o = op->base;
	 const char *st = (const char *)Sinetab;
	 float rate ;
//	if(op->b_obj.z_disabled)
//		goto out;
	
	rate = 1.0f/n;
	for(j=0;j<n;++j)
		out[j] = 0.0f;


	for(i=0;i<nosc;++i)
	{
		// register float a = op->pulse*exp(-o->rate*op->t)*o->gain;
		// register float ascale = exp(-o->rate*op->sampleinterval*op->rate);
		register long pi = o->phase_inc;
		register unsigned long pc = o->phase_current;
//		register float astep = (nexta - o->amplitude)*rate;
		register float t, a;

//		if(op->t<1.0)
//			post("%d", o->phase_current);
		for(j=0;j<n;++j)
		{
			// Super-inefficient version calls exp in the inner loop!
			a = op->pulse *exp(-o->rate*time[j])*o->gain;
			out[j] +=  a  * 
		*((float *)(st + (((pc) >> (32-TPOW-LOGBASE2OFTABLEELEMENT))
					& ((STABSZ-1)*sizeof(*Sinetab)))));
			pc +=  pi;
//			a *=ascale;

		}
		o->phase_current = pc;
		++o;
	}
	op->t += op->rate*n*op->sampleinterval;
	if(op->t<op->backstop)
		op->t = op->backstop;
	if(op->t>op->stop)
		op->t = op->stop;
out:	return (w+5);
}


void sinusoids_clearmodel(t_sinusoids *x) {
	int i;
	oscdesc *p = x->base;
	for(i=0;i<MAXOSCILLATORS;++i, p++)
	{
	
	//	p->next_phase_inc = 0;
	//	p->next_amplitude = 0.0f;
		p->amplitude = 0.0f;
		p->phase_inc = 0;
		p->phase_current = 0;
//		p->phaseadd = 0;
//		p->next_phaseadd = 0;
	}
}  


void sinusoids_clear(t_sinusoids *x)
{
	x->rate = 1.0;
	x->pulse = 1.0;
	x->t = 1e30;
	x->backstop = 0.0;
	x->stop = 1e30;
}

void sinusoids_list(t_sinusoids *x, t_symbol *s, short argc, t_atom *argv)
{
	int i;
	if(argc%3!=0)
	{
		object_post((t_object *)x, "multiple of 3 floats required");
	}
	else
	{
		oscdesc *fp = x->base;
		int nosc;
		nosc = argc/3;
		if(nosc>MAXOSCILLATORS)
			nosc = MAXOSCILLATORS;


		for(i=x->nosc;i<nosc;++i)
		{
// xxxx	 mess with ampl		fp[i].out1 = fp[i].out2 = 0.0f;
		}
		x->nosc = nosc;
		for(i=0;i<nosc;++i)
		{
			float f = atom_getfloatarg(i*3,argc,argv);
			float g = 	atom_getfloatarg(i*3+1,argc,argv);
			float rate = atom_getfloatarg(i*3+2,argc,argv);

			if((f<=0.0f) || (f>=(x->samplerate*0.5f)) || (rate<=0.0f))
			{
				fp[i].rate = fp[i].gain = fp[i].phase_inc = 0.0f;
				
			}
			else
			{
			fp[i].rate = rate;
			fp[i].phase_inc = f*x->pk;
			fp[i].gain = g;

			}
//		post("%d %f %d %f", i, fp[i].rate, fp[i].phase_inc, g);
		}
	}
}

void sinusoids_assist(t_sinusoids *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_OUTLET)
		sprintf(s,"(signal) oscillator bank output");
	else {
		sprintf(s,"list of float triplets: frequency, gain, decay rate");
	}
}

void *sinusoids_new(t_symbol *s, short argc, t_atom *argv)
{
    t_sinusoids *x = (t_sinusoids *)object_alloc(sinusoids_class);
	if(!x){
		return NULL;
	}

    dsp_setup((t_pxobject *)x,1);   // One signal inlet, for virtual time
    outlet_new((t_object *)x, "signal");
	x->samplerate =  sys_getsr();
	if(x->samplerate<=0.0f)
		x->samplerate = 44100.0f;
	x->sampleinterval = 1.0f/x->samplerate;
	x->pk = (STABSZ*x->sampleinterval)*(1l<<(32-TPOW)) ;
     x->nosc = 0;
     sinusoids_clearmodel(x);
     sinusoids_clear(x);

    sinusoids_list(x,s,argc,argv);
//post("%d %f %f %f", x->nosc, x->pk, x->sampleinterval, x->samplerate);
    return (x);
}

void SineFunction(int n, float *stab, int stride, float from, float to);
void SineFunction(int n, float *stab, int stride, float from, float to)
{
	int j;
	float f = (to-from)/n;

	for(j=0;j<n;++j)
	{
		stab[j*stride] = sinf(from +j*f);
//		if(j%16==0)
//			post("%f", stab[j*stride]);
	}
}
void Makeoscsinetable();
void Makeoscsinetable()
{
		SineFunction(STABSZ, Sinetab, 1, 0.0f, 2.0f*(float)PI);
}

void sinusoids_rate(t_sinusoids *x, double f);
void sinusoids_rate(t_sinusoids *x, double f)
{
	x->rate = f;
}

void sinusoids_goto(t_sinusoids *x, double f);
void sinusoids_goto(t_sinusoids *x, double f)
{
  if (f < x->backstop) {
    f = x->backstop;
  } else if (f > x->stop) {
    f = x->stop;
  }
  x->t = f;
}

void sinusoids_float(t_sinusoids *x, double f);
void sinusoids_float(t_sinusoids *x, double f)
{
//post("%f %f", x->t, x->pulse);
	x->t = 0.0;
	x->pulse = f;
}
void sinusoids_mintime(t_sinusoids *x, double f);
void sinusoids_mintime(t_sinusoids *x, double f)
{
	x->backstop = f;
}
void sinusoids_maxtime(t_sinusoids *x, double f);
void sinusoids_maxtime(t_sinusoids *x, double f)
{
	x->stop = f;
}


int main(void){
	sinusoids_class = class_new("decaying-sinusoids~", (method)sinusoids_new, (method)dsp_free,
		  (short)sizeof(t_sinusoids), 0L, A_GIMME, 0);
	version_post_copyright();
	post("Maximum Oscillators: %d", MAXOSCILLATORS);
	post("Never expires");


	Makeoscsinetable();
	class_addmethod(sinusoids_class, (method)version, "version", 0);
	class_addmethod(sinusoids_class, (method)sinusoids_dsp, "dsp", A_CANT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_float, "float", A_FLOAT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_goto, "goto", A_FLOAT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_rate, "rate", A_FLOAT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_maxtime, "max_vtime", A_FLOAT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_mintime, "min_vtime", A_FLOAT, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_list, "list", A_GIMME, 0);
	class_addmethod(sinusoids_class, (method)sinusoids_clear, "clear", 0);
	class_addmethod(sinusoids_class, (method)sinusoids_assist, "assist", A_CANT, 0);
	
	class_dspinit(sinusoids_class);

	class_register(CLASS_BOX, sinusoids_class);
	return 0;
}


