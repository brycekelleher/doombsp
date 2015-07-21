#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "doomlib.h"
#include "vec2.h"
#include "box2.h"
#include "plane2.h"

float globalepsilon = 0.2f;

// ______________________________________________
// errors and warnings

void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];
	
	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);
	
	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "Error: %s", buffer);
	fprintf(stderr, "\x1b[0m");
	exit(1);
}

void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];
	
	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);
	
	fprintf(stderr, "\x1b[33m");
	fprintf(stderr, "Warning: %s", buffer);
	fprintf(stderr, "\x1b[0m");
}

// ______________________________________________
// Memory allocation

void *Malloc(int numbytes)
{
	void *mem;
	
	mem = malloc(numbytes);

	if(!mem)
	{
		Error("Malloc: Failed to allocated memory");
	}

	return mem;
}

void *MallocZeroed(int numbytes)
{
	void *mem;
	
	mem = Malloc(numbytes);

	memset(mem, 0, numbytes);

	return mem;
}

vec2 Plane_GetNormal(plane_t plane)
{
	return plane.GetNormal();
}

float GetDistance(plane_t plane)
{
	return plane.GetDistance();
}

float Plane_PointDistance(plane_t plane, vec2 p)
{
	return plane.Distance(p);
}

int Plane_PointOnPlaneSide(plane_t plane, vec2 p, float epsilon)
{
	return plane.PointOnPlaneSide(p, epsilon);
}

typedef struct line_s
{
	vec2	v[2];

} line_t;

line_t * Line_Alloc()
{
	return (line_t*)malloc(sizeof(line_t));
}

line_t *Line_Copy(line_t *s)
{
	line_t *d;

	d = Line_Alloc();
	d->v[0] = s->v[0];
	d->v[1] = s->v[1];

	return d;
}

int Line_OnPlaneSide(line_t *l, plane_t plane, float epsilon) 
{
	int sides[2];

	sides[0] = Plane_PointOnPlaneSide(plane, l->v[0], epsilon);
	sides[1] = Plane_PointOnPlaneSide(plane, l->v[1], epsilon);

	if (sides[0] == PLANE_SIDE_ON && sides[1] == PLANE_SIDE_ON)
		return PLANE_SIDE_ON;

	// if nothing on the back side then line is in front
	if (sides[0] != PLANE_SIDE_BACK && sides[1] != PLANE_SIDE_BACK)
		return PLANE_SIDE_FRONT;

	// if nothing on front side then line is on back side
	if (sides[0] != PLANE_SIDE_FRONT && sides[1] != PLANE_SIDE_FRONT)
		return PLANE_SIDE_BACK;

	return PLANE_SIDE_CROSS;
}

void Line_SplitWithPlane(line_t *l, plane_t plane, float epsilon, line_t **f, line_t **b)
{
	int sides[2];

	sides[0] = Plane_PointOnPlaneSide(plane, l->v[0], epsilon);
	sides[1] = Plane_PointOnPlaneSide(plane, l->v[1], epsilon);

	// both points are on the plane
	if (sides[0] == PLANE_SIDE_ON && sides[0] == PLANE_SIDE_ON)
	{
		*f = NULL;
		*b = NULL;
		return;
	}

	// if nothing on back side then line is in front
	if (sides[0] != PLANE_SIDE_BACK && sides[1] != PLANE_SIDE_BACK)
	{
		*f = Line_Copy(l);
		*b = NULL;
		return;
	}

	// if nothing on front side then line is on back side
	if (sides[0] != PLANE_SIDE_FRONT && sides[1] != PLANE_SIDE_FRONT)
	{
		*f = NULL;
		*b = Line_Copy(l);
		return;
	}

	// the points cross the plane so generate a split point
	{
		vec2 mid;
		int i;

		// calculate split point
		for (i = 0; i < 2; i++)
		{
			// avoid round off error when possible
			if (plane[i] == 1)
			{
				mid[i] = -plane[2];
			}
			else if (plane[i] == -1)
			{
				mid[i] = plane[2];
			}
			else
			{
				float dist1, dist2, dot;
				
				dist1 = Distance(plane, l->v[0]);
				dist2 = Distance(plane, l->v[1]);
				dot = dist1 / (dist1 - dist2);
				mid[i] = (l->v[0][i] * (1.0f - dot)) + (dot * l->v[1][i]);
			}
		}

		// create the new front and back lines
		if (sides[0] == PLANE_SIDE_FRONT)
		{
			*f = Line_Alloc();
			*b = Line_Alloc();
			(*f)->v[0] = l->v[0];
			(*f)->v[1] = mid;
			(*b)->v[0] = mid;
			(*b)->v[1] = l->v[1];
		}
		else
		{
			*f = Line_Alloc();
			*b = Line_Alloc();
			(*b)->v[0] = l->v[0];
			(*b)->v[1] = mid;
			(*f)->v[0] = mid;
			(*f)->v[1] = l->v[1];
		}
	}
}

vec2 Line_GetNormal(line_t *l)
{
	vec2 n;

	n = l->v[1] - l->v[0];
	n = Skew(n);
	n = Normalize(n);

	return n;
}

plane_t Line_Plane(line_t *l)
{
	vec2		n;
	plane_t		plane;

	n = Line_GetNormal(l);

	plane[0] = n[0];
	plane[1] = n[1];
	plane[2] = -Dot(n, l->v[0]);

	return plane;
}
	
// ______________________________________________
// doomlib

typedef struct linedef_s
{
	int	vertices[2];

} linedef_t;

int numvertices;
vec2 *vertices;
int numlinedefs;
linedef_t *linedefs;

static void DumpVertices(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	numvertices		= lumpsize / (2 * sizeof(short));
	vertices		= (vec2*)MallocZeroed(numvertices * sizeof(vec2));

	short *fixedptr		= (short*)data;

	for(int i = 0; i < numvertices; i++)
	{
		float xy[2];

		xy[0] = fixedptr[0];
		xy[1] = fixedptr[1];
		fixedptr += 2;

		vertices[i][0] = xy[0];
		vertices[i][1] = xy[1];

		//printf("vertex %4i: %12.4f, %12.4f\n", i, xy[0], xy[1]);
	}
}

static void DumpLinedefs(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	numlinedefs		= lumpsize / sizeof(dlinedef_t);
	linedefs		= (linedef_t*)MallocZeroed(numlinedefs * sizeof(linedef_t));

	dlinedef_t *lptr	= (dlinedef_t*)data;

	for(int i = 0; i < numlinedefs; i++)
	{
		//printf("linedefs %4i:", i);
		//printf("\n\tvertices (%i %i)", lptr->vertices[0], lptr->vertices[1]);
		//printf("\n\tsidedefs (%i %i)", lptr->sidedefs[0], lptr->sidedefs[1]);
		//printf("\n");

		linedefs[i].vertices[0] = lptr->vertices[0];
		linedefs[i].vertices[1] = lptr->vertices[1];

		lptr++;
	}
}

static void DumpMapData(const char *mapname)
{
	int baselump = Doom_LumpNumFromName(mapname);

	if(baselump < 0 || Doom_LumpLength(baselump) != 0)
	{
		Error("Map \"%s\" not found\n", mapname);
		exit(-1);
	}

	DumpLinedefs(baselump + LINEDEFS_OFFSET);
	DumpVertices(baselump + VERTICES_OFFSET);
}

// ______________________________________________
// bsp

// nodes
typedef struct bspnode_s
{
	struct bspnode_s	*next;
	struct bspnode_s	*treenext;
	struct bspnode_s	*leafnext;
	struct bspnode_s	*parent;
	struct bspnode_s	*children[2];
	struct bsptree_s	*tree;
	
	// the node split plane
	plane_t			plane;
	
} bspnode_t;

// tree
typedef struct bsptree_s
{
	bspnode_t	*nodes;
	int		numnodes;
	int		numleafs;
	int		depth;
	
	bspnode_t	*root;
	bspnode_t	*leafs;
	
} bsptree_t;

typedef struct bspline_s
{
	struct bspline_s	*next;
	line_t			*line;

} bspline_t;

// global list of bsp nodes
static bspnode_t	*bspnodes;

static bspnode_t *AllocNode()
{
	bspnode_t *n;
	
	n = (bspnode_t*)MallocZeroed(sizeof(bspnode_t));
	
	// link the node into the global list
	n->next = bspnodes;
	bspnodes = n;
	
	return n;
}

static bsptree_t *MallocTree()
{
	return (bsptree_t*)MallocZeroed(sizeof(bsptree_t));
}

static bspline_t *MallocBSPLine(line_t *line)
{
	bspline_t	*p;
	
	p = (bspline_t*)MallocZeroed(sizeof(bspline_t));
	p->line = line;
	
	return p;
}

static bspnode_t *MallocBSPNode(bsptree_t *tree, bspnode_t *parent)
{
	bspnode_t *n = AllocNode();
	
	n->parent = parent;
	n->tree	= tree;
	
	// link the node into the tree list
	n->treenext = tree->nodes;
	tree->nodes = n;
	tree->numnodes++;
	
	return n;
}

static void SplitLine(plane_t plane, bspline_t *l, float epsilon, bspline_t **f, bspline_t **b)
{
	line_t *ff, *bb;

	*f = *b = NULL;
	
	// split the line
	Line_SplitWithPlane(l->line, plane, epsilon, &ff, &bb);
	
	if (ff)
		*f = MallocBSPLine(ff);
	if (bb)
		*b = MallocBSPLine(bb);
	
	// check that the split polygon sits in the original's plane
	//if (*f && !CheckPolygonOnPlane(*f, PolygonPlane(p)))
	//	Error("Front polygon doesn't sit on original plane after split\n");
	//if (*b && !CheckPolygonOnPlane(*b, PolygonPlane(p)))
	//	Error("Back polygon doesn't sit on original plane after split\n");
}

static int CalculateSplitPlaneScore(plane_t plane, bspline_t *list)
{
	int score = 0;
	
	for(; list; list = list->next)
	{
		// favour polygons that don't cause splits
		int side = Line_OnPlaneSide(list->line, plane, globalepsilon);
		if(side != PLANE_SIDE_ON && side != PLANE_SIDE_CROSS)
			score += 1;
		
		// favour planes which are axial
		if(plane.IsAxial())
			score += 1;
	}
	
	return score;
}

static plane_t SelectSplitPlane(bspline_t *list)
{
	int bestscore = 0;
	plane_t bestplane;
	bspline_t *l;
	
	for(l = list; l; l = l->next)
	{
		plane_t plane = Line_Plane(l->line);
		int score = CalculateSplitPlaneScore(plane, list);
	
		if(!bestscore || score > bestscore)
		{
			bestscore	= score;
			bestplane	= plane;
		}
			
	}
	
	return bestplane;
}

static void PartitionLineList(plane_t plane, bspline_t *list, bspline_t **sides)
{
	sides[0] = NULL;
	sides[1] = NULL;
	
	for(; list; list = list->next)
	{
		bspline_t *split[2];
		int i;

		SplitLine(plane, list, globalepsilon, &split[0], &split[1]);

		// process the front (0) and back (1) splits
		for(i = 0; i < 2; i++)
		{
			if(split[i])
			{
				split[i]->next = sides[i];
				sides[i] = split[i];
			}
		}
	}
}

void BuildTreeRecursive(bsptree_t *tree, bspnode_t *node, bspline_t *lines)
{
	plane_t		plane;
	bspline_t	*sides[2];

	if (!lines)
	{
		tree->numleafs++;
		return;
	}

	plane = SelectSplitPlane(lines);

	PartitionLineList(plane, lines, sides);

	node->plane = plane;
	
	// add two new nodes to the tree
	node->children[0] = MallocBSPNode(tree, node);
	node->children[1] = MallocBSPNode(tree, node);
	
	// recurse down the front and back sides
	BuildTreeRecursive(tree, node->children[0], sides[0]);
	BuildTreeRecursive(tree, node->children[1], sides[1]);
}

bspline_t *MakeLineList()
{
	bspline_t *list = NULL;

	for(int i = 0; i < numlinedefs; i++)
	{
		line_t *line = Line_Alloc();

		line->v[0][0] = vertices[linedefs[i].vertices[0]][0];
		line->v[0][1] = vertices[linedefs[i].vertices[0]][1];
		line->v[1][0] = vertices[linedefs[i].vertices[1]][0];
		line->v[1][1] = vertices[linedefs[i].vertices[1]][1];

		//printf("line %i, %f, %f, %f, %f\n",
		//	i,
		//	line->v[0][0],
		//	line->v[0][1],
		//	line->v[1][0],
		//	line->v[1][1]);

		bspline_t *bspline = MallocBSPLine(line);
		bspline->next = list;
		list = bspline;
	}

	return list;
}

bsptree_t *MakeEmptyTree()
{
	bsptree_t	*tree;
	
	tree = (bsptree_t*)MallocZeroed(sizeof(bsptree_t));
	tree->root = MallocBSPNode(tree, NULL);

	return tree;
}

bsptree_t *BuildTree()
{
	 bspline_t *lines = MakeLineList();

	bsptree_t *tree = MakeEmptyTree();

	BuildTreeRecursive(tree, tree->root, lines);

	return tree;
}

static void PrintUsage()
{
	printf("dumplvl <wadfile> <mapname>\n");
}

int main(int argc, const char * argv[])
{
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	DumpMapData(argv[2]);

	Doom_CloseAll();

	bsptree_t *tree = BuildTree();
	printf("numvertices %i\n", numvertices);
	printf("numlinedefs %i\n", numlinedefs);
	printf("numnodes %i\n", tree->numnodes);
	printf("numleafs %i\n", tree->numleafs);
	
	return 0;
}
