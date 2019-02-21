// depends on rational.h

#ifdef OLT_INTERN_PARSE_H
#error multiple inclusion
#endif
#define OLT_INTERN_PARSE_H

// Please update glossary when messing with units.
typedef struct {
	double x;
	double y;
} Point;

typedef struct {
	Point beg;
	Point diff;
} Line;

typedef struct {
	int px;
	int py;
	int bx;
	int by;
	int ex;
	int ey;
} Dot;

typedef struct {
	Point beg;
	Point ctrl;
	Point end;
} Curve;

struct olt_Parse {
	int numCurves;
	Curve *curves;
};

typedef struct olt_Parse olt_Parse; // REDUNDANT

extern olt_Parse olt_GLOBAL_parse;

void olt_INTERN_parse_outline(void *addr);
