/*
  Written by John MacCallum, The Center for New Music and Audio Technologies, 
  University of California, Berkeley.  Copyright (c) 2009, The Regents of 
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
  NAME: tempo_editor
  DESCRIPTION: GUI for the tempocurver
  AUTHORS: John MacCallum 
  COPYRIGHT_YEARS: 2009 
  SVN_REVISION: $LastChangedRevision: 587 $ 
  VERSION 0.0: First try 
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
*/

/*
  todo:
figure out a way to get the first beat.
make sure jumps in phase and freq are really handled properly
*/

#include "version.h" 
#include "ext.h" 
#include "ext_obex.h" 
#include "ext_obex_util.h" 
#include "ext_critical.h"
#include "jpatcher_api.h" 
#include "jgraphics.h" 
#include "z_dsp.h"
#include "version.c" 

#define MAX_NUM_FUNCTIONS 32
#define POINT_WIDTH 14

#define BEFORE_FIRST_POINT 1
#define AFTER_LAST_POINT 2

typedef struct _point{ 
 	t_pt screen_coords; 
	double phase;
 	struct _point *next; 
 	struct _point *prev; 
} t_point; 

typedef struct _plan{ 
	int state;
	double startTime, endTime;
	double startFreq, endFreq;
	double startPhase, endPhase;
	double phaseError, phaseError_start;
	double segmentDuration_sec;
	double m, b;
 	struct _plan *next; 
 	struct _plan *prev; 
} t_plan; 

typedef struct _te{ 
 	t_pxjbox box; 
 	void *out_info;
	void *proxy;
	long in;
	t_critical lock;
 	t_point **functions; 
	t_point *selected;
 	int currentFunction; 
 	int numFunctions; 
	t_plan plan;
	int dirty; // set this to non-zero to indicate that a point has been added
	double time_min, time_max;
	double freq_min, freq_max;
	int mode;
	int show_correction;
 	t_jrgba bgcolor; 
	t_jrgba pointColor;
	t_jrgba lineColor;
	t_jrgba bgFuncColor;
	t_jrgba selectionColor;
	t_symbol *name;
	t_float **ptrs;
} t_te; 

void *te_class; 

t_symbol *ps_cellblock;

void te_paint(t_te *x, t_object *patcherview); 
t_int *te_perform(t_int *w);
void te_makePlan(t_te *x, float f, int function, t_plan *plan);
int te_isPlanValid(t_te *x, double time, t_plan *plan, int function);
void te_computePhaseError(t_te *x, t_plan *plan);
double te_computeTempo(double t, t_plan *p);
double te_computePhase(double t, t_plan *p);
void te_editSel(t_te *x, double xx, double yy, double zz);
void te_list(t_te *x, t_symbol *msg, short argc, t_atom *argv);
void te_float(t_te *x, double f);
void te_find_btn(t_point *function, double x, t_point **left, t_point **right);
t_point *te_select(t_te *x, t_pt p);
void te_draw_bounds(t_te *x, t_jgraphics *g, t_rect *rect); 
void te_assist(t_te *x, void *b, long m, long a, char *s); 
void te_free(t_te *x); 
t_max_err te_notify(t_te *x, t_symbol *s, t_symbol *msg, void *sender, void *data); 
void te_mousedown(t_te *x, t_object *patcherview, t_pt pt, long modifiers); 
void te_mousedrag(t_te *x, t_object *patcherview, t_pt pt, long modifiers); 
void te_mouseup(t_te *x, t_object *patcherview, t_pt pt, long modifiers); 
void te_outputSelection(t_te *x);
void te_addFunction(t_te *x); 
void te_setFunction(t_te *x, long f); 
double te_scale(double f, double min_in, double max_in, double min_out, double max_out);
void te_getNormCoords(t_rect r, t_pt screen_coords, t_pt *norm_coords); 
void te_getScreenCoords(t_rect r, t_pt norm_coords, t_pt *screen_coords);
t_point *te_insertPoint(t_te *x, t_pt screen_coords, int functionNum);
void te_removePoint(t_te *x, t_point *point, int functionNum);
void te_dump(t_te *x);
void te_dumpCellblock(t_te *x);
void te_time_min(t_te *x, double f);
void te_time_max(t_te *x, double f);
void te_time_minmax(t_te *x, double min, double max);
void te_freq_min(t_te *x, double f);
void te_freq_max(t_te *x, double f);
void te_freq_minmax(t_te *x, double min, double max);
void te_clear(t_te *x);
//void te_read(t_te *x, t_symbol *msg, short argc, t_atom *argv);
//void te_doread(t_te *x, t_symbol *msg, short argc, t_atom *argv);
//void te_write(t_te *x, t_symbol *msg, short argc, t_atom *argv);
//void te_dowrite(t_te *x, t_symbol *msg, short argc, t_atom *argv);
void te_dsp(t_te *x, t_signal **sp, short *count);
t_symbol *te_mangleName(t_symbol *name, int i, int fnum);
int main(void); 
void *te_new(t_symbol *s, long argc, t_atom *argv); 

void te_paint(t_te *x, t_object *patcherview){ 
 	t_rect rect; 
 	t_jrgba c; 
 	c.red = c.green = c.blue = 0; 
 	c.alpha = 1.; 

 	// get graphics context 
 	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(patcherview); 

 	// get our box's rectangle 
 	jbox_get_rect_for_view((t_object *)x, patcherview, &rect); 

 	// draw the outline of the box 
 	jgraphics_set_source_jrgba(g, &c); 
 	jgraphics_set_line_width(g, 1); 
 	jgraphics_rectangle(g, 0., 0., rect.width, rect.height); 
 	jgraphics_stroke(g); 

 	jgraphics_set_source_jrgba(g, &(x->bgcolor)); 
 	jgraphics_rectangle(g, 0., 0., rect.width, rect.height); 
 	jgraphics_fill(g); 

	critical_enter(x->lock);

	// draw the beat lines
	{
		int i, j;
		double prev_t = te_scale(0, 0, rect.width, x->time_min, x->time_max);
		t_plan plan;
		for(j = 0; j < x->numFunctions; j++){
			if(x->functions[j] == NULL){
				continue;
			}
			if(j == x->currentFunction){
				jgraphics_set_source_jrgba(g, &(x->lineColor));
			}else{
				jgraphics_set_source_jrgba(g, &(x->bgFuncColor));
			}
			//te_makePlan(x, prev_t, x->currentFunction, &plan);
			te_makePlan(x, prev_t, j, &plan);
			double prev_phase = te_computePhase(prev_t, &plan);
			for(i = 1; i < rect.width * 1; i++){
				double idx = i / 1.;
				double t = te_scale(idx, 0, rect.width, x->time_min, x->time_max);
				if(!te_isPlanValid(x, t, &plan, j)){
					//post("making plan for time %f", t);
					//te_makePlan(x, t, x->currentFunction, &plan);
					te_makePlan(x, t, j, &plan);
				}
				double p = te_computePhase(t, &plan);
				//if(p < prev_phase){
				//post("went negative at time %f", t);
				//}
				//post("%d %f %f %f %f", idx, prev_t, prev_phase, t, p);
				if((p - ((int)p)) < (prev_phase - ((int)prev_phase))){
					jgraphics_move_to(g, idx, rect.height);
					jgraphics_line_to(g, idx, te_scale(te_computeTempo(t, &plan), x->freq_min, x->freq_max, rect.height, 0));
					jgraphics_stroke(g);
					//post("moveto %f %f", (float)i, rect.height);
					//post("lineto %f %f", (float)i, te_scale(te_computeTempo(t, &plan), x->freq_min, x->freq_max, rect.height, 0));
				}
				prev_t = t;
				prev_phase = p;
			}
		}
	}

	// draw lines
 	{ 
		int i;
		//jgraphics_set_source_jrgba(g, &(x->lineColor));
		//jgraphics_set_line_width(g, 1); 
		for(i = 0; i < x->numFunctions; i++){
			t_point *p = x->functions[i];
			if(p){
				jgraphics_move_to(g, p->screen_coords.x, p->screen_coords.y);
				p = p->next;
			}
			while(p){
				jgraphics_set_source_jrgba(g, &(x->lineColor));
				jgraphics_set_line_width(g, 1); 
				t_pt norm_coords;
				jgraphics_move_to(g, p->prev->screen_coords.x, p->prev->screen_coords.y);
				jgraphics_line_to(g, p->screen_coords.x, p->screen_coords.y);
				jgraphics_stroke(g);

				if(x->show_correction){
					jgraphics_set_source_jrgba(g, &(x->selectionColor));
					jgraphics_set_line_width(g, 1); 
					t_plan plan;
					te_makePlan(x, plan.startTime, i, &plan);
					double t = plan.startTime + plan.segmentDuration_sec;
					double phase1 = te_computePhase(t, &plan);
					double phase2 = te_computePhase(t + (1./44100.), &plan);
					double freq = (phase2 - phase1) * 44100.;
					jgraphics_move_to(g, te_scale(t, x->time_min, x->time_max, 0, rect.width), te_scale(freq, x->freq_min, x->freq_max, rect.height, 0.));
					t = plan.startTime + (2. * plan.segmentDuration_sec);
					phase1 = te_computePhase(t - (1. / 44100.), &plan);
					phase2 = te_computePhase(t, &plan);
					freq = (phase2 - phase1) * 44100.;
					jgraphics_line_to(g, te_scale(t, x->time_min, x->time_max, 0, rect.width), te_scale(freq, x->freq_min, x->freq_max, rect.height, 0.));
					jgraphics_stroke(g);
				}
				p = p->next;
			}
		}
 	} 

	// draw points 
 	{ 
		int i;
		double ps = POINT_WIDTH; // point size
		double ps2 = ps / 2.; // point size divided by 2
 		//jgraphics_set_line_width(g, 0.25); 
		for(i = 0; i < x->numFunctions; i++){
			t_point *p = x->functions[i];
			double xx, yy;
			if(p){
				if(p == x->selected){
					jgraphics_set_source_jrgba(g, &(x->selectionColor));
				}else{
					if(i == x->currentFunction){
						jgraphics_set_source_jrgba(g, &(x->pointColor));
					}else{
						jgraphics_set_source_jrgba(g, &(x->bgFuncColor));
					}
				}
				jgraphics_ellipse(g, p->screen_coords.x - ps2, p->screen_coords.y - ps2, ps, ps);
				jgraphics_fill(g);

				jgraphics_set_source_jrgba(g, &(x->bgcolor));
				jgraphics_move_to(g, p->screen_coords.x, p->screen_coords.y);

				xx = ps2 * sin(2 * M_PI * p->phase);
				yy = ps2 * cos(2 * M_PI * p->phase);
				jgraphics_line_to(g, p->screen_coords.x + xx, p->screen_coords.y - yy);
				jgraphics_stroke(g);

				p = p->next;
			}
			while(p){
				t_pt norm_coords;

				if(p == x->selected){
					jgraphics_set_source_jrgba(g, &(x->selectionColor));
				}else{
					if(i == x->currentFunction){
						jgraphics_set_source_jrgba(g, &(x->pointColor));
					}else{
						jgraphics_set_source_jrgba(g, &(x->bgFuncColor));
					}
				}
				jgraphics_ellipse(g, p->screen_coords.x - ps2, p->screen_coords.y - ps2, ps, ps);
				jgraphics_fill(g);

 				//jgraphics_set_line_width(g, 1); 
				jgraphics_set_source_jrgba(g, &(x->bgcolor));
				jgraphics_move_to(g, p->screen_coords.x, p->screen_coords.y);

				xx = ps2 * sin(2 * M_PI * p->phase);
				yy = ps2 * cos(2 * M_PI * p->phase);
				jgraphics_line_to(g, p->screen_coords.x + xx, p->screen_coords.y - yy);
				jgraphics_stroke(g);

				p = p->next;
			}
		}
 	} 
 out:
	critical_exit(x->lock);
} 

t_int *te_perform(t_int *w){
	t_te *x = (t_te *)w[1];
	int n = (int)w[2];
	float sr = 44100.;
	t_float *in = (t_float *)w[3];
	t_float *out_phase_wrapped = (t_float *)w[4];
	t_float *out_phase = (t_float *)w[5];
	t_float *out_bps = (t_float *)w[6];
	int i, j;
	t_plan *plan = &(x->plan);

	//post("x = %p, %d, plan = %p, %p %p %p %p", x, n, plan, in, out_phase_wrapped, out_phase, out_bps);

	double m, b;
	for(j = 0; j < x->numFunctions; j++){
		t_symbol *name1, *name2, *name3;
		if(name1 = te_mangleName(x->name, 1, j)){
			name1->s_thing = (t_object *)(x->ptrs[j * 3]);
			//post("name1 = %s, %p", name1->s_name, x->ptrs[j * 3]);
		}
		if(name2 = te_mangleName(x->name, 2, j)){
			name2->s_thing = (t_object *)(x->ptrs[(j * 3) + 1]);
			//post("name2 = %s, %p", name2->s_name, x->ptrs[j * 3 + 1]);
		}
		if(name3 = te_mangleName(x->name, 3, j)){
			name3->s_thing = (t_object *)(x->ptrs[(j * 3) + 2]);
			//post("name3 = %s, %p", name3->s_name, x->ptrs[j * 3 + 2]);
		}

		for(i = 0; i < n; i++){
			if(te_isPlanValid(x, in[i], plan, j) == 0){
				te_makePlan(x, in[i], j, plan);
			}
			m = plan->m;
			b = plan->b;

			//out_phase[i] = ((in[i] * ((2. * b) + (m * in[i]))) / 2.) - plan->phaseError_start;
			/*
			x->ptrs[(j * 3) + 1][i] = ((in[i] * ((2. * b) + (m * in[i]))) / 2.) - plan->phaseError_start;
			if(in[i] < plan->startTime + plan->segmentDuration_sec){
			}else if(in[i] < plan->startTime + (2. * plan->segmentDuration_sec)){
				//out_phase[i] = out_phase[i] + ((plan->phaseError / (plan->segmentDuration_sec * sr)) * ((in[i] - (plan->startTime + plan->segmentDuration_sec)) * sr));
				x->ptrs[(j * 3) + 1][i] = x->ptrs[(j * 3) + 1][i] + ((plan->phaseError / (plan->segmentDuration_sec * sr)) * ((in[i] - (plan->startTime + plan->segmentDuration_sec)) * sr));
			}else{
				//out_phase[i] = out_phase[i] + plan->phaseError;
				x->ptrs[(j * 3) + 1][i] = x->ptrs[(j * 3) + 1][i] + plan->phaseError;
			}
			*/
			x->ptrs[(j * 3) + 1][i] = te_computePhase(in[i], plan);
			//out_phase_wrapped[i] = out_phase[i] - ((int)(out_phase[i]));
			x->ptrs[(j * 3)][i] = x->ptrs[j * 3 + 1][i] - ((int)(x->ptrs[j * 3 + 1][i]));
			//out_bps[i] = m * in[i] + b;
			x->ptrs[(j * 3) + 2][i] = m * in[i] + b;
		}
	}
	memcpy(out_phase_wrapped, x->ptrs[x->currentFunction * 3], n * sizeof(t_float));
	memcpy(out_phase, x->ptrs[x->currentFunction * 3 + 1], n * sizeof(t_float));
	memcpy(out_bps, x->ptrs[x->currentFunction * 3 + 2], n * sizeof(t_float));

	return w + 7;
}

void te_makePlan(t_te *x, float f, int function, t_plan *plan){
	if(!plan){
		return;
	}
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	double f_sc = te_scale(f, x->time_min, x->time_max, 0, r.width);
	t_point *p = x->functions[function];
	t_point *next = NULL;
	if(p){
		next = p->next;
	}else{
		memset(plan, 0, sizeof(plan));
		return;
	}

	// we are somewhere between the minimum time (probably 0) and the first
	// point.  so we'll make a plan with a freq of 0.
	if(f_sc < p->screen_coords.x){
		plan->state = BEFORE_FIRST_POINT;
		plan->startTime = x->time_min;
		plan->endTime = te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max);
		plan->startFreq = 0.;
		plan->endFreq = 0.;
		plan->startPhase = 0.;
		// we don't want to introduce any phase error here that would cause the 
		// algorithm to try to compensate for it.
		plan->endPhase = 0.; 
		te_computePhaseError(x, plan);
		plan->segmentDuration_sec = (plan->endTime - plan->startTime) / 3.;
		return;
	}

	while(p && next){
		if(f_sc >= p->screen_coords.x && f_sc < next->screen_coords.x){
			plan->state = 0;
			plan->startTime = te_scale(p->screen_coords.x, 0., r.width, x->time_min, x->time_max);
			plan->endTime = te_scale(next->screen_coords.x, 0., r.width, x->time_min, x->time_max);
			plan->startFreq = te_scale(p->screen_coords.y, r.height, 0., x->freq_min, x->freq_max);
			plan->endFreq = te_scale(next->screen_coords.y, r.height, 0., x->freq_min, x->freq_max);
			plan->startPhase = p->phase;
			plan->endPhase = next->phase;
			te_computePhaseError(x, plan);
			plan->segmentDuration_sec = (plan->endTime - plan->startTime) / 3.;
			/*
			  post("startTime = %f", plan->startTime);
			  post("endTime = %f", plan->endTime);
			  post("startFreq = %f", plan->startFreq);
			  post("endFreq = %f", plan->endFreq);
			  post("startPhase = %f", plan->startPhase);
			  post("endPhase = %f", plan->endPhase);

			  post("phaseError = %f, %f", plan->phaseError, plan->phaseError_start);
			  post("segdur = %f", plan->segmentDuration_sec);
			*/
			return;
		}
		p = next;
		next = next->next;
	}

	// if we made it here, we're somewhere between the last point and time_max
	if(f_sc >= p->screen_coords.x){
		plan->state = AFTER_LAST_POINT;
		plan->startTime = te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max);
		plan->endTime = x->time_max;
		plan->startFreq = 0.;
		plan->endFreq = 0.;
		plan->startPhase = 0.;
		// we don't want to introduce any phase error here that would cause the 
		// algorithm to try to compensate for it.  this will effectively be a jump to phase and freq 0.
		plan->endPhase = 0.; 
		te_computePhaseError(x, plan);
		plan->segmentDuration_sec = (plan->endTime - plan->startTime) / 3.;
		return;

	}
}

int te_isPlanValid(t_te *x, double time, t_plan *plan, int function){
	if(!plan){
		return 0;
	}
	if(time < plan->startTime || time > plan->endTime){
		return 0;
	}
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	double stsc, etsc;
	stsc = te_scale(plan->startTime, x->time_min, x->time_max, 0, r.width);
	etsc = te_scale(plan->endTime, x->time_min, x->time_max, 0, r.width);
	t_point *p = x->functions[function];
	if(!p){
		return 0;
	}
	if(p->screen_coords.x - etsc < .001 && plan->startTime == 0.){
		return 1;
	}
	while(p){
		if(fabs(p->screen_coords.x - stsc) < .001){
			//post("%f == %f", p->screen_coords.x, stsc);
			if(p->next){
				if(fabs(p->next->screen_coords.x - etsc) < .001){
					//post("there is p->next and %f == %f", p->next->screen_coords.x, etsc);
					return 1;
				}else{
					//post("there is p->next but %f != %f", p->next->screen_coords.x, etsc);
					return 0;
				}
			}else if(fabs(etsc - r.width) < 0.001){
				//post("there is no p->next but %f == %f", etsc, r.width)
				return 1;
			}else{
				//post("man, this plan is fucked");
				return 0;
			}
		}else{
			//post("%f != %f", p->screen_coords.x, stsc);
		}
		p = p->next;
	}
	return 0;
}

void te_computePhaseError(t_te *x, t_plan *plan){
	double m = (plan->endFreq - plan->startFreq) / (plan->endTime - plan->startTime);
	double b = plan->startFreq - (m * plan->startTime);
	plan->m = m;
	plan->b = b;

	// this is the offset that we need to subtract in order to get this part of the plan to start at 0.
	plan->phaseError_start = ((plan->startTime * ((2. * b) + (m * plan->startTime))) / 2.) - plan->startPhase;

	// evaluate the integral at the endpoint and translate by the offset computed above.
	plan->phaseError = ((plan->endTime * ((2. * b) + (m * plan->endTime))) / 2.) - plan->phaseError_start;

	// wrap to [0,1]
	plan->phaseError = plan->phaseError - ((int)(plan->phaseError));

	// compute the actual error
	plan->phaseError = plan->endPhase - plan->phaseError;

	// add 1 to the phase if necessary
	if(x->mode < 0){
	}else if(x->mode > 0){
		plan->phaseError += 1.;
	}else{
		if(fabs(plan->phaseError) > 0.5){
			if(plan->phaseError < 0.){
				plan->phaseError += 1.;
			}else{
				plan->phaseError -= 1.;
			}
		}
	}

	double i;
	double st, et, inc;
	st = plan->startTime + plan->segmentDuration_sec;
	et = (plan->startTime + (2. * plan->segmentDuration_sec));
	inc = (et - st) / 10.;
	//post("%f %f %f", st, et, inc);
	double prev_phase = te_computePhase(st, plan);
	double prev_time = st;
	for(i = st + inc; i < et; i+= inc){
		double ph = te_computePhase(i, plan);
		//post("%f %f %f %f %f", i, ph, prev_time, prev_phase, ((ph - prev_phase) / (i - prev_time)));
		if(((ph - prev_phase) / (i - prev_time)) < 0){
			//post("***");
			plan->phaseError += 1;
			return;
		}
		prev_phase = ph;
		prev_time = i;
	}

}

double te_computeTempo(double t, t_plan *p){
	return t * p->m + p->b;
}

double te_computePhase(double t, t_plan *p){
	switch(p->state){
	case BEFORE_FIRST_POINT:
		return 0.999;
	case AFTER_LAST_POINT:
		return 0.;
	default:
		{
			double error = 0;
			double sr = 44100.;
			if(t >= p->startTime && t < p->startTime + p->segmentDuration_sec){
			}else if(t >= p->startTime + p->segmentDuration_sec && t < p->startTime + (2. * p->segmentDuration_sec)){
				error = ((p->phaseError / (p->segmentDuration_sec * sr)) * ((t - (p->startTime + p->segmentDuration_sec)) * sr));
			}else if(t >= p->startTime + (2. * p->segmentDuration_sec) && t <= p->startTime + (3. * p->segmentDuration_sec)){
				error = p->phaseError;
			}

			return (((t * ((2. * p->b) + (p->m * t))) / 2.) - p->phaseError_start) + error;
		}
	}
}

void te_editSel(t_te *x, double xx, double yy, double zz){
	t_pt screen_coords;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	//te_getScreenCoords(r, norm_coords, &screen_coords);
	screen_coords.x = te_scale(xx, x->time_min, x->time_max, 0, r.width);
	screen_coords.y = te_scale(yy, x->freq_min, x->freq_max, r.height, 0);
	//post("%f %f %f %f", norm_coords.x, norm_coords.y, screen_coords.x, screen_coords.y);
	te_removePoint(x, x->selected, x->currentFunction);
	x->selected = te_insertPoint(x, screen_coords, x->currentFunction);
	x->selected->phase = zz;
	jbox_redraw((t_jbox *)x);
	te_dumpCellblock(x);
}

void te_list(t_te *x, t_symbol *msg, short argc, t_atom *argv){
	t_atom *a = argv;
	int functionNum = x->currentFunction;
	t_pt screen_coords;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);

	switch(proxy_getinlet((t_object *)x)){
	case 0:
		if(argc == 4){
			functionNum = atom_getlong(a++);
			if(functionNum > x->numFunctions - 1){
				x->numFunctions = functionNum + 1;
			}
		}

		//norm_coords.x = atom_getfloat(a++);
		//norm_coords.y = atom_getfloat(a++);
		//te_getScreenCoords(r, norm_coords, &screen_coords);
		screen_coords.x = te_scale(atom_getfloat(a++), x->time_min, x->time_max, 0, r.width);
		screen_coords.y = te_scale(atom_getfloat(a++), x->freq_min, x->freq_max, r.height, 0);
		//post("%f %f %f %f", norm_coords.x, norm_coords.y, screen_coords.x, screen_coords.y);
		x->selected = te_insertPoint(x, screen_coords, functionNum);
		x->selected->phase = atom_getfloat(a);
		break;
	case 1:
		{
			if(argc == 2){
				return;
			}
			t_point *p = x->functions[x->currentFunction];
			int i;
			int row = atom_getlong(&(argv[1]));
			int col = atom_getlong(&(argv[0]));
			double val = atom_getfloat(&(argv[2]));
			for(i = 0; p; i++){
				if(i == row){
					break;
				}
				p = p->next;
			}
			if(p){
				x->selected = p;
				switch(col){
				case 0:
					// time
					p->screen_coords.x = te_scale(val, x->time_min, x->time_max, 0, r.width);
					break;
				case 1:
					// freq
					p->screen_coords.y = te_scale(val, x->freq_min, x->freq_max, r.height, 0);
					break;
				case 2:
					// phase
					p->phase = val;
					break;
				}
				if(col == 0){
					double ph = p->phase;
					t_pt sc = p->screen_coords;
					te_removePoint(x, p, x->currentFunction);
					x->selected = te_insertPoint(x, sc, x->currentFunction);
					x->selected->phase = ph;
				}
			}else{
				t_pt sc = {0., r.height};
				double ph = 0.;
				switch(col){
				case 0:
					sc.x = te_scale(val, x->time_min, x->time_max, 0, r.width);
					break;
				case 1:
					sc.y = te_scale(val, x->freq_min, x->freq_max, r.height, 0);
					break;
				case 2:
					ph = val;
					break;
				}
				x->selected = te_insertPoint(x, sc, x->currentFunction);
				x->selected->phase = ph;
			}
		}
		break;
	}
	jbox_redraw((t_jbox *)x);
	te_dumpCellblock(x);
}

void te_float(t_te *x, double f){
	if(f < 0.) f = 0.;
	if(f > 1.) f = 1.;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	double screenx = f * r.width;
	t_atom out[3]; // function number, x, y
	atom_setfloat(&(out[1]), f);
	int i;
	for(i = 0; i < x->numFunctions; i++){
		t_point *p = x->functions[i];
		atom_setlong(&(out[0]), i);
		t_point *left = NULL, *right = NULL;
		te_find_btn(p, screenx, &left, &right);
		//return;
		if(!left || !right){
			atom_setfloat(&(out[2]), 0.);
			//outlet_list(x->out_main, NULL, 3, out);
		}else{
			double m = (right->screen_coords.y - left->screen_coords.y) / (right->screen_coords.x - left->screen_coords.x);
			double b = left->screen_coords.y - (m * left->screen_coords.x);
			double y = (m * screenx) + b;
			atom_setfloat(&(out[2]), fabs(y - r.height) / r.height);
			//outlet_list(x->out_main, NULL, 3, out);
		}
	}
}

void te_find_btn(t_point *function, double x, t_point **left, t_point **right){
	//post("function = %p", function);
	if(!function){
		return;
	}
	t_point *ptr = function;
	//post("%f %f %f", x, function->screen_coords.x, function->screen_coords.y);
	//return;
	if(x < function->screen_coords.x){
		*left = NULL;
		*right = function;
		return;
	}
	while(ptr->next){
		if(x >= ptr->screen_coords.x && x <= ptr->next->screen_coords.x){
			*left = ptr;
			*right = ptr->next;
			return;
		}
		ptr = ptr->next;
	}
	*left = ptr;
	*right = NULL;
}

t_point *te_select(t_te *x, t_pt p){
	double min = 1000000000.;
	t_point *min_ptr = NULL;
	t_point *ptr = x->functions[x->currentFunction];
	double xdif, ydif;
	while(ptr){
		if((xdif = fabs(p.x - ptr->screen_coords.x)) < POINT_WIDTH / 2. && (ydif = fabs(p.y - ptr->screen_coords.y)) < POINT_WIDTH / 2.){
			if(xdif + ydif < min){
				min = xdif + ydif;
				min_ptr = ptr;
			}
		}
		ptr = ptr->next;
	}
	return min_ptr;
}

t_max_err te_notify(t_te *x, t_symbol *s, t_symbol *msg, void *sender, void *data){ 
 	//t_symbol *attrname; 
 	if (msg == gensym("attr_modified")){ 
 		//attrname = (t_symbol *)object_method((t_object *)data, gensym("getname")); 
 		//x->sel_x.click = x->sel_x.drag = x->sel_y.click = x->sel_y.drag = -1; 
 		jbox_redraw((t_jbox *)x); 
	} 
	return 0; 
} 

// 0x10 = no modifiers 
// 0x11 = command 
// 0x12 = shift 
// 0x94 = control 
// 0x18 = option 
void te_mousedown(t_te *x, t_object *patcherview, t_pt pt, long modifiers){ 
 	//post("0x%X", modifiers); 
 	t_rect r; 
	//t_pt norm_coords;
	jbox_get_rect_for_view((t_object *)x, patcherview, &r);
	//te_getNormCoords(r, pt, &norm_coords);
	switch(modifiers){
	case 0x10:
		// no modifiers.  
		//post("inserting %f %f", pt.x, pt.y);
		if(x->selected = te_select(x, pt)){
			break;
		}else{
			x->selected = te_insertPoint(x, pt, x->currentFunction);
		}
		break;
	case 0x12:
		// shift.  
		if(x->selected = te_select(x, pt)){
			te_removePoint(x, x->selected, x->currentFunction);
			break;
		}
		break;
	case 0x13:
		break;
	}
	te_outputSelection(x);
	jbox_redraw((t_jbox *)x);
	te_dumpCellblock(x);
}

void te_mousedrag(t_te *x, t_object *patcherview, t_pt pt, long modifiers){
	t_rect r;
	jbox_get_rect_for_view((t_object *)x, patcherview, &r);
	if(pt.x < 0){
		pt.x = 0;
	}else if(pt.x > r.width){
		pt.x = r.width;
	}
	if(pt.y < 0){
		pt.y = 0;
	}else if(pt.y > r.height){
		pt.y = r.height;
	}

	switch(modifiers){
	case 0x10:
		{
			double ph = x->selected->phase;
			te_removePoint(x, x->selected, x->currentFunction);
			x->selected = te_insertPoint(x, pt, x->currentFunction);
			x->selected->phase = ph;
		}
		break;
	case 0x12:
		break;
	case 0x13:
		break;
	}
	te_outputSelection(x);
	jbox_redraw((t_jbox *)x);
	te_dumpCellblock(x);
}

void te_mouseup(t_te *x, t_object *patcherview, t_pt pt, long modifiers){
}

void te_outputSelection(t_te *x){
	if(!(x->selected)){
		return;
	}
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	t_pt norm_coords;
	t_atom out[3];
	te_getNormCoords(r, x->selected->screen_coords, &norm_coords);
	atom_setlong(&(out[0]), x->currentFunction);
	atom_setfloat(&(out[1]), norm_coords.x);
	atom_setfloat(&(out[2]), norm_coords.y);
	//outlet_list(x->out_sel, NULL, 3, out);
}

void te_addFunction(t_te *x){
	critical_enter(x->lock);
	if(x->numFunctions + 1 > MAX_NUM_FUNCTIONS){
		error("te: maximum number of functions: %d", MAX_NUM_FUNCTIONS);
		return;
	}
	x->numFunctions++;
	critical_exit(x->lock);
	te_setFunction(x, x->numFunctions - 1);
	jbox_redraw((t_jbox *)x);
}

void te_setFunction(t_te *x, long f){
	if(f > MAX_NUM_FUNCTIONS){
		error("te: maximum number of functions: %d", MAX_NUM_FUNCTIONS);
		return;
	}
	if(f > x->numFunctions - 1){
		while(f > x->numFunctions - 1){
			x->numFunctions++;
		}
	}
	critical_enter(x->lock);
	x->currentFunction = f;
	x->selected = NULL;
	critical_exit(x->lock);
	jbox_redraw((t_jbox *)x);
}

double te_scale(double f, double min_in, double max_in, double min_out, double max_out){
	float m = (max_out - min_out) / (max_in - min_in);
	float b = (min_out - (m * min_in));
	return m * f + b;
}

void te_getNormCoords(t_rect r, t_pt screen_coords, t_pt *norm_coords){
	norm_coords->x = screen_coords.x / r.width;
	norm_coords->y = fabs(r.height - screen_coords.y) / r.height;
}

void te_getScreenCoords(t_rect r, t_pt norm_coords, t_pt *screen_coords){
	screen_coords->x = norm_coords.x * r.width;
	screen_coords->y = fabs((norm_coords.y * r.height) - r.height);
}

t_point *te_insertPoint(t_te *x, t_pt screen_coords, int functionNum){
	critical_enter(x->lock);
	t_point **function = &(x->functions[functionNum]);
	t_point *p = (t_point *)calloc(1, sizeof(t_point));
	p->screen_coords.x = screen_coords.x;
	p->screen_coords.y = screen_coords.y;
	if(*function == NULL){
		p->prev = NULL;
		p->next = NULL;
		*function = p;
	}else if(p->screen_coords.x < (*function)->screen_coords.x){
		p->prev = NULL;
		p->next = (*function);
		(*function)->prev = p;
		x->functions[functionNum] = p;
	}else{
		int i = 1;
		t_point *current, *next;
		current = (*function);
		next = current->next;
		while(next){
			if(p->screen_coords.x >= current->screen_coords.x && p->screen_coords.x <= next->screen_coords.x){
				current->next = p;
				next->prev = p;
				p->next = next;
				p->prev = current;
				goto out;
			}
			current = next;
			next = next->next;
			i++;
		}
		p->prev = current;
		p->next = NULL;
		current->next = p;
	}
 out:
	//post("inserted point %p %f %f", p, screen_coords.x, screen_coords.y);
	critical_exit(x->lock);
	return p;
}

void te_removePoint(t_te *x, t_point *point, int functionNum){
	if(!point){
		return;
	}
	critical_enter(x->lock);
	//post("removing point %p %f %f", point, point->screen_coords.x, point->screen_coords.y);
	t_point **function = &(x->functions[functionNum]);
	t_point *p = *function;
	int i = 0;
	while(p){
		if(p == point){
			if(p->prev){
				if(p->next){
					p->prev->next = p->next;
				}else{
					p->prev->next = NULL;
				}
			}
			if(p->next){
				if(p->prev){
					p->next->prev = p->prev;
				}else{
					p->next->prev = NULL;
				}
			}
			if(i == 0){
				x->functions[functionNum] = p->next;
			}

			if(p){
				free(p);
			}
			goto out;
		}
		i++;
		p = p->next;
	}
 out:
	critical_exit(x->lock);
}

void te_dump(t_te *x){
	int i;
	t_atom out[3];
	t_pt norm_coords;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		t_point *p = x->functions[i];
		//atom_setlong(&(out[0]), i);
		while(p){
			//te_getNormCoords(r, p->screen_coords, &norm_coords);
			atom_setfloat(&(out[0]), te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max));
			atom_setfloat(&(out[1]), te_scale(p->screen_coords.y, r.height, 0, x->freq_min, x->freq_max));
			atom_setfloat(&(out[2]), p->phase);
			outlet_anything(x->out_info, _sym_dump, 3, out);
			p = p->next;
		}
	}
	atom_setsym(&(out[0]), _sym_done);
	outlet_anything(x->out_info, _sym_dump, 1, out);
	//outlet_bang(x->out_bang);
}

void te_dumpCellblock(t_te *x){
	int i = 0;
	t_point *p = x->functions[x->currentFunction];
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	t_atom out[4];
	atom_setsym(&(out[0]), _sym_clear);
	atom_setsym(&(out[1]), gensym("all"));
	outlet_anything(x->out_info, ps_cellblock, 2, out);
	atom_setsym(&(out[0]), _sym_set);
	while(p){
		/*
		if(x->selected == p){
			atom_setsym(&(out[0]), _sym_select);
			atom_setlong(&(out[1]), 0);
			atom_setlong(&(out[2]), i);
			outlet_anything(x->out_info, ps_cellblock, 3, out);
			atom_setsym(&(out[0]), _sym_set);
		}
		*/
		atom_setlong(&(out[2]), i);
		atom_setlong(&(out[1]), 0);
		atom_setfloat(&(out[3]), te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max));
		outlet_anything(x->out_info, ps_cellblock, 4, out);

		atom_setlong(&(out[1]), 1);
		atom_setfloat(&(out[3]), te_scale(p->screen_coords.y, r.height, 0, x->freq_min, x->freq_max));
		outlet_anything(x->out_info, ps_cellblock, 4, out);

		atom_setlong(&(out[1]), 2);
		atom_setfloat(&(out[3]), p->phase);
		outlet_anything(x->out_info, ps_cellblock, 4, out);

		p = p->next;
		i++;
	}
}

void te_time_min(t_te *x, double f){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.x = te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max);
			p->screen_coords.x = te_scale(p->screen_coords.x, f, x->time_max, 0, r.width);
			p = p->next;
		}
	}
	x->time_min = f;
	jbox_redraw((t_jbox *)x);
}

void te_time_max(t_te *x, double f){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.x = te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max);
			p->screen_coords.x = te_scale(p->screen_coords.x, x->time_min, f, 0, r.width);
			p = p->next;
		}
	}
	x->time_max = f;
	jbox_redraw((t_jbox *)x);
}

void te_time_minmax(t_te *x, double min, double max){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.x = te_scale(p->screen_coords.x, 0, r.width, x->time_min, x->time_max);
			p->screen_coords.x = te_scale(p->screen_coords.x, min, max, 0, r.width);
			p = p->next;
		}
	}
	x->time_min = min;
	x->time_max = max;
	jbox_redraw((t_jbox *)x);
}

void te_freq_min(t_te *x, double f){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.y = te_scale(p->screen_coords.y, r.height, 0, x->freq_min, x->freq_max);
			p->screen_coords.y = te_scale(p->screen_coords.y, f, x->freq_max, r.height, 0);
			p = p->next;
		}
	}
	x->freq_min = f;
	jbox_redraw((t_jbox *)x);
}

void te_freq_max(t_te *x, double f){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.y = te_scale(p->screen_coords.y, r.height, 0, x->freq_min, x->freq_max);
			p->screen_coords.y = te_scale(p->screen_coords.y, x->freq_min, f, r.height, 0);
			p = p->next;
		}
	}

	x->freq_max = f;
	jbox_redraw((t_jbox *)x);
}

void te_freq_minmax(t_te *x, double min, double max){
	int i;
	t_point *p;
	t_rect r;
	jbox_get_patching_rect(&(x->box.z_box.b_ob), &r);
	for(i = 0; i < x->numFunctions; i++){
		p = x->functions[i];
		while(p){
			p->screen_coords.y = te_scale(p->screen_coords.y, r.height, 0, x->freq_min, x->freq_max);
			p->screen_coords.y = te_scale(p->screen_coords.y, min, max, r.height, 0);
			p = p->next;
		}
	}

	x->freq_min = min;
	x->freq_max = max;
	jbox_redraw((t_jbox *)x);
}

void te_clear(t_te *x){
	int i;
	for(i = 0; i < x->numFunctions; i++){
		t_point *p = x->functions[i];
		if(p){
			free(p);
		}
		x->functions[i] = NULL;
	}
	jbox_redraw((t_jbox *)x);
}

void te_assist(t_te *x, void *b, long m, long a, char *s){ 
 	if (m == ASSIST_OUTLET){ 
 	}else{ 
 		switch (a) {	 
 		case 0: 
 			break; 
 		} 
 	} 
} 

void te_free(t_te *x){ 
	dsp_freejbox((t_pxjbox *)x);
	jbox_free((t_jbox *)x);

	object_free(x->proxy);

	int i;
	for(i = 0; i < x->numFunctions; i++){
		t_point *p = x->functions[i];
		t_point *next;
		while(p){
			next = p->next;
			free(p);
			p = next;
		}
		if(x->ptrs[i * 3]){
			free(x->ptrs[i * 3]);
		}
		if(x->ptrs[i * 3 + 1]){
			free(x->ptrs[i * 3 + 1]);
		}
		if(x->ptrs[i * 3 + 2]){
			free(x->ptrs[i * 3 + 2]);
		}
	}
	if(x->ptrs){
		free(x->ptrs);
	}
} 

t_symbol *te_mangleName(t_symbol *name, int i, int fnum){
	if(!name){
		return NULL;
	}
	char buf[256];
	sprintf(buf, "tempo_editor_%s_%d_%d", name->s_name, i, fnum);
	return gensym(buf);
}

void te_dsp(t_te *x, t_signal **sp, short *count){
	if(count[0]){
		dsp_add(te_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
	}
}

int main(void){ 
 	t_class *c = class_new("tempo_editor", (method)te_new, (method)te_free, sizeof(t_te), 0L, A_GIMME, 0); 
	class_dspinitjbox(c);

 	c->c_flags |= CLASS_FLAG_NEWDICTIONARY; 
 	jbox_initclass(c, JBOX_FIXWIDTH | JBOX_COLOR | JBOX_FONTATTR); 

	class_addmethod(c, (method)te_dsp, "dsp", A_CANT, 0);
 	class_addmethod(c, (method)te_paint, "paint", A_CANT, 0); 
 	class_addmethod(c, (method)version, "version", 0); 
 	class_addmethod(c, (method)te_assist, "assist", A_CANT, 0); 
 	class_addmethod(c, (method)te_notify, "notify", A_CANT, 0); 
 	class_addmethod(c, (method)te_mousedown, "mousedown", A_CANT, 0); 
 	class_addmethod(c, (method)te_mousedrag, "mousedrag", A_CANT, 0); 
 	class_addmethod(c, (method)te_mouseup, "mouseup", A_CANT, 0); 
 	class_addmethod(c, (method)te_addFunction, "addFunction", 0); 
 	class_addmethod(c, (method)te_setFunction, "setFunction", A_LONG, 0); 
	class_addmethod(c, (method)te_editSel, "editSelection", A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(c, (method)te_list, "list", A_GIMME, 0);
	class_addmethod(c, (method)te_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)te_clear, "clear", 0);
	class_addmethod(c, (method)te_dump, "dump", 0);
	class_addmethod(c, (method)te_time_min, "time_min", A_FLOAT, 0);
	class_addmethod(c, (method)te_time_max, "time_max", A_FLOAT, 0);
	class_addmethod(c, (method)te_time_minmax, "time_minmax", A_FLOAT, A_FLOAT, 0);
	class_addmethod(c, (method)te_freq_min, "freq_min", A_FLOAT, 0);
	class_addmethod(c, (method)te_freq_max, "freq_max", A_FLOAT, 0);
	class_addmethod(c, (method)te_freq_minmax, "freq_minmax", A_FLOAT, A_FLOAT, 0);

	//class_addmethod(c, (method)te_read, "read", A_GIMME, 0);
	//class_addmethod(c, (method)te_write, "write", A_GIMME, 0);
	//class_addmethod(c, (method)myobject_write, "test", A_DEFSYM, 0);

	CLASS_ATTR_LONG(c, "mode", 0, t_te, mode);
	CLASS_ATTR_SAVE(c, "mode", 0);
	CLASS_ATTR_SYM(c, "name", 0, t_te, name);
	CLASS_ATTR_SAVE(c, "name", 0);
	CLASS_ATTR_SYM(c, "show_correction", 0, t_te, show_correction);
	CLASS_ATTR_SAVE(c, "show_correction", 0);

 	CLASS_STICKY_ATTR(c, "category", 0, "Color"); 

 	CLASS_ATTR_RGBA(c, "bgcolor", 0, t_te, bgcolor); 
 	CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c, "bgcolor", 0, "1. 1. 1. 1."); 
 	CLASS_ATTR_STYLE_LABEL(c, "bgcolor", 0, "rgba", "Background Color"); 

 	CLASS_ATTR_RGBA(c, "pointColor", 0, t_te, pointColor); 
 	CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c, "pointColor", 0, "0. 0. 0. 1."); 
 	CLASS_ATTR_STYLE_LABEL(c, "pointColor", 0, "rgba", "Point Color"); 

 	CLASS_ATTR_RGBA(c, "lineColor", 0, t_te, lineColor); 
 	CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c, "lineColor", 0, "0. 0. 0. 1."); 
 	CLASS_ATTR_STYLE_LABEL(c, "lineColor", 0, "rgba", "Line Color"); 

 	CLASS_ATTR_RGBA(c, "bgFuncColor", 0, t_te, bgFuncColor); 
 	CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c, "bgFuncColor", 0, "0. 0. 0. 0.5"); 
 	CLASS_ATTR_STYLE_LABEL(c, "bgFuncColor", 0, "rgba", "Background Function Color"); 

 	CLASS_ATTR_RGBA(c, "selectionColor", 0, t_te, selectionColor); 
 	CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c, "selectionColor", 0, "1. 0. 0. 1."); 
 	CLASS_ATTR_STYLE_LABEL(c, "selectionColor", 0, "rgba", "Selection Color"); 

 	CLASS_STICKY_ATTR_CLEAR(c, "category"); 

 	CLASS_ATTR_DEFAULT(c, "patching_rect", 0, "0. 0. 300. 100."); 

 	class_register(CLASS_BOX, c); 
 	te_class = c; 

	common_symbols_init();
	ps_cellblock = gensym("cellblock");

 	version(0); 
	
 	return 0; 
} 

void *te_new(t_symbol *s, long argc, t_atom *argv){ 
 	t_te *x = NULL; 
 	t_dictionary *d = NULL; 
 	long boxflags; 

	// box setup 
	if(!(d = object_dictionaryarg(argc, argv))){ 
		return NULL; 
	} 

	boxflags = 0 
		| JBOX_DRAWFIRSTIN  
		//| JBOX_NODRAWBOX 
		| JBOX_DRAWINLAST 
		//| JBOX_TRANSPARENT   
		//      | JBOX_NOGROW 
		//| JBOX_GROWY 
		| JBOX_GROWBOTH 
		//      | JBOX_HILITE 
		| JBOX_BACKGROUND 
		| JBOX_DRAWBACKGROUND 
		//      | JBOX_NOFLOATINSPECTOR 
		//      | JBOX_MOUSEDRAGDELTA 
		//      | JBOX_TEXTFIELD 
		; 

 	if(x = (t_te *)object_alloc(te_class)){ 

 		jbox_new((t_jbox *)x, boxflags, argc, argv); 
 		x->box.z_box.b_firstin = (void *)x; 

		dsp_setupjbox((t_pxjbox *)x, 1);

		x->proxy = proxy_new((t_object *)x, 1, &(x->in));

		x->out_info = outlet_new((t_object *)x, NULL);
 		outlet_new((t_object *)x, "signal"); 
 		outlet_new((t_object *)x, "signal"); 
 		outlet_new((t_object *)x, "signal"); 

		x->time_min = 0.;
		x->time_max = 10.;
		x->freq_min = 0.;
		x->freq_max = 10.;
		x->mode = 0;

 		x->functions = (t_point **)calloc(MAX_NUM_FUNCTIONS, sizeof(t_point *));
 		x->numFunctions = 1; 
 		x->currentFunction = 0; 
		critical_new(&(x->lock));

		x->ptrs = (t_float **)malloc((MAX_NUM_FUNCTIONS * 3) * sizeof(t_float *));
		int i;
		for(i = 0; i < MAX_NUM_FUNCTIONS * 3; i++){
			x->ptrs[i] = (t_float *)malloc(2048 * sizeof(t_float));
		}

		x->name = NULL;

 		attr_dictionary_process(x, d); 
 		jbox_ready((t_jbox *)x); 

		x->box.z_misc = Z_PUT_FIRST;

 		return x; 
 	} 
 	return NULL; 
} 