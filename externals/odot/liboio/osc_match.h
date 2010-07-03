#define OSC_MATCH_ENABLE_2STARS		1
#define OSC_MATCH_ENABLE_NSTARS		1

#define OSC_MATCH_ADDRESS_COMPLETE	1
#define OSC_MATCH_PATTERN_COMPLETE	2
/*
typedef struct _osc_callback {
	const char* address;			// Address
	struct _osc_callback *child;		// RAM
	struct _osc_callback *sibling;		// RAM
	struct _osc_callback *parent;		// RAM
	int callback;				// ROM
} osc_callback;
*/

int osc_match(const char *pattern, const char *address, int *pattern_offset, int *address_offset);
int osc_match_star(const char *pattern, const char *address);
int osc_match_star_r(const char *pattern, const char *address);
int osc_match_single_char(const char *pattern, const char *address);
int osc_match_bracket(const char *pattern, const char *address);
int osc_match_curly_brace(const char *pattern, const char *address);