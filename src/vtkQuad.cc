/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuad.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkQuad.hh"
#include "vtkPolygon.hh"
#include "vtkPlane.hh"
#include "vtkMath.hh"
#include "vtkCellArray.hh"
#include "vtkLine.hh"

static vtkMath math;
static vtkLine line;
static vtkPolygon poly;
static vtkPlane plane;

// Description:
// Deep copy of cell.
vtkQuad::vtkQuad(const vtkQuad& q)
{
  this->Points = q.Points;
  this->PointIds = q.PointIds;
}

#define MAX_ITERATION 10
#define CONVERGED 1.e-03

int vtkQuad::EvaluatePosition(float x[3], float closestPoint[3],
                             int& subId, float pcoords[3], 
                             float& dist2, float weights[VTK_MAX_CELL_SIZE])
{
  int i, j;
  float *pt1, *pt2, *pt3, *pt, n[3];
  float det;
  float maxComponent;
  int idx, indices[2];
  int iteration, converged;
  float  params[2];
  float  fcol[2], rcol[2], scol[2];
  float derivs[8];

  subId = 0;
  pcoords[0] = pcoords[1] = params[0] = params[1] = 0.5;
//
// Get normal for quadrilateral
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  plane.ProjectPoint(x,pt1,n,closestPoint);
//
// Construct matrices.  Since we have over determined system, need to find
// which 2 out of 3 equations to use to develop equations. (Any 2 should 
// work since we've projected point to plane.)
//
  for (maxComponent=0.0, i=0; i<3; i++)
    {
    if (fabs(n[i]) > maxComponent)
      {
      maxComponent = fabs(n[i]);
      idx = i;
      }
    }
  for (j=0, i=0; i<3; i++)  
    {
    if ( i != idx ) indices[j++] = i;
    }
//
// Use Newton's method to solve for parametric coordinates
//  
  for (iteration=converged=0; !converged && (iteration < MAX_ITERATION);
  iteration++) 
    {
//
//  calculate element interpolation functions and derivatives
//
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);
//
//  calculate newton functions
//
    for (i=0; i<2; i++) 
      {
      fcol[i] = rcol[i] = scol[i] = 0.0;
      }
    for (i=0; i<4; i++)
      {
      pt = this->Points.GetPoint(i);
      for (j=0; j<2; j++)
        {
        fcol[j] += pt[indices[j]] * weights[i];
        rcol[j] += pt[indices[j]] * derivs[i];
        scol[j] += pt[indices[j]] * derivs[i+4];
        }
      }

    for (j=0; j<2; j++) fcol[j] -= closestPoint[indices[j]];
//
//  compute determinants and generate improvements
//
    if ( (det=math.Determinant2x2(rcol,scol)) == 0.0 ) return -1;

    pcoords[0] = params[0] - math.Determinant2x2 (fcol,scol) / det;
    pcoords[1] = params[1] - math.Determinant2x2 (rcol,fcol) / det;
//
//  check for convergence
//
    if ( ((fabs(pcoords[0]-params[0])) < CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < CONVERGED) )
      {
      converged = 1;
      }
//
//  if not converged, repeat
//
    else 
      {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      }
    }
//
//  if not converged, set the parametric coordinates to arbitrary values
//  outside of element
//
  if ( !converged ) return -1;

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
       pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
    {
    dist2 = math.Distance2BetweenPoints(closestPoint,x); //projection distance
    return 1;
    }
  else
    {
    float t;
    float *pt4 = this->Points.GetPoint(3);

    if ( pcoords[0] < 0.0 && pcoords[1] < 0.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt1);
      for (i=0; i<3; i++) closestPoint[i] = pt1[i];
      }
    else if ( pcoords[0] > 1.0 && pcoords[1] < 0.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt2);
      for (i=0; i<3; i++) closestPoint[i] = pt2[i];
      }
    else if ( pcoords[0] > 1.0 && pcoords[1] > 1.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt3);
      for (i=0; i<3; i++) closestPoint[i] = pt3[i];
      }
    else if ( pcoords[0] < 0.0 && pcoords[1] > 1.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt4);
      for (i=0; i<3; i++) closestPoint[i] = pt4[i];
      }
    else if ( pcoords[0] < 0.0 )
      {
      dist2 = line.DistanceToLine(x,pt1,pt4,t,closestPoint);
      }
    else if ( pcoords[0] > 1.0 )
      {
      dist2 = line.DistanceToLine(x,pt2,pt3,t,closestPoint);
      }
    else if ( pcoords[1] < 0.0 )
      {
      dist2 = line.DistanceToLine(x,pt1,pt2,t,closestPoint);
      }
    else if ( pcoords[1] > 1.0 )
      {
      dist2 = line.DistanceToLine(x,pt3,pt4,t,closestPoint);
      }
    return 0;
    }
}

void vtkQuad::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                              float weights[VTK_MAX_CELL_SIZE])
{
  int i, j;
  float *pt;

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<4; i++)
    {
    pt = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

//
// Compute iso-parametrix interpolation functions
//
void vtkQuad::InterpolationFunctions(float pcoords[3], float sf[4])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = pcoords[0] * pcoords[1];
  sf[3] = rm * pcoords[1];
}

void vtkQuad::InterpolationDerivs(float pcoords[3], float derivs[8])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = pcoords[1];
  derivs[3] = -pcoords[1];
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = pcoords[0];
  derivs[7] = rm;
}

int vtkQuad::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];

  pts.Reset();

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(2));
    }

  else if ( t1 < 0.0 && t2 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(2));
    pts.SetId(1,this->PointIds.GetId(3));
    }

  else //( t1 < 0.0 && t2 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(3));
    pts.SetId(1,this->PointIds.GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
  pcoords[1] < 0.0 || pcoords[1] > 1.0 )
    return 0;
  else
    return 1;
}

//
// Marching (convex) quadrilaterals
//
static int edges[4][2] = { {0,1}, {1,2}, {2,3}, {3,0} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[5];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {-1, -1, -1, -1, -1},
  {0, 3, -1, -1, -1},
  {1, 0, -1, -1, -1},
  {1, 3, -1, -1, -1},
  {2, 1, -1, -1, -1},
  {0, 3, 2, 1, -1},
  {2, 0, -1, -1, -1},
  {2, 3, -1, -1, -1},
  {3, 2, -1, -1, -1},
  {0, 2, -1, -1, -1},
  {1, 0, 3, 2, -1},
  {1, 2, -1, -1, -1},
  {3, 1, -1, -1, -1},
  {0, 1, -1, -1, -1},
  {3, 0, -1, -1, -1},
  {-1, -1, -1, -1, -1}
};

void vtkQuad::Contour(float value, vtkFloatScalars *cellScalars, 
                     vtkFloatPoints *points, vtkCellArray *verts, 
                     vtkCellArray *lines, vtkCellArray *polys, 
                     vtkFloatScalars *scalars)
{
  static int CASE_MASK[4] = {1,2,4,8};
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int pts[2];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
      if (cellScalars->GetScalar(i) >= value)
          index |= CASE_MASK[i];

  lineCase = lineCases + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
    {
    for (i=0; i<2; i++) // insert line
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points.GetPoint(vert[0]);
      x2 = this->Points.GetPoint(vert[1]);
      for (j=0; j<3; j++) x[j] = x1[j] + t * (x2[j] - x1[j]);
      pts[i] = points->InsertNextPoint(x);
      scalars->InsertNextScalar(value);
      }
    lines->InsertNextCell(2,pts);
    }
}


vtkCell *vtkQuad::GetEdge(int edgeId)
{
  static vtkLine line;
  int edgeIdPlus1 = edgeId + 1;
  
  if (edgeIdPlus1 > 3) edgeIdPlus1 = 0;

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(edgeId));
  line.PointIds.SetId(1,this->PointIds.GetId(edgeIdPlus1));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(edgeId));
  line.Points.SetPoint(1,this->Points.GetPoint(edgeIdPlus1));

  return &line;
}

// 
// Intersect plane; see whether point is in quadrilateral.
//
int vtkQuad::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                              float x[3], float pcoords[3], int& subId)
{
  float *pt1, *pt2, *pt3, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[VTK_MAX_CELL_SIZE];

  subId = 0;
  pcoords[0] = pcoords[1] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Intersect plane of triangle with line
//
  if ( ! plane.IntersectWithLine(p1,p2,n,pt1,t,x) ) return 0;
//
// See whether point is in triangle by evaluating its position.
//
  if ( this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    if ( dist2 <= tol2 ) return 1;

  return 0;
}

int vtkQuad::Triangulate(int index, vtkFloatPoints &pts)
{
  float d1, d2;

  pts.Reset();

  // use minimum diagonal (Delaunay triangles)
  d1 = math.Distance2BetweenPoints(this->Points.GetPoint(0), 
                                   this->Points.GetPoint(2));
  d2 = math.Distance2BetweenPoints(this->Points.GetPoint(1), 
                                   this->Points.GetPoint(3));

  if ( d1 < d2 )
    {
    pts.InsertPoint(0,this->Points.GetPoint(0));
    pts.InsertPoint(1,this->Points.GetPoint(1));
    pts.InsertPoint(2,this->Points.GetPoint(2));

    pts.InsertPoint(3,this->Points.GetPoint(0));
    pts.InsertPoint(4,this->Points.GetPoint(2));
    pts.InsertPoint(5,this->Points.GetPoint(3));
    }
  else
    {
    pts.InsertPoint(0,this->Points.GetPoint(0));
    pts.InsertPoint(1,this->Points.GetPoint(1));
    pts.InsertPoint(2,this->Points.GetPoint(3));

    pts.InsertPoint(3,this->Points.GetPoint(1));
    pts.InsertPoint(4,this->Points.GetPoint(2));
    pts.InsertPoint(5,this->Points.GetPoint(3));
    }

  return 1;
}

void vtkQuad::Derivatives(int subId, float pcoords[3], float *values, 
                          int dim, float *derivs)
{

}

