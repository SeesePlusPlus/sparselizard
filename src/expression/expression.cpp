#include "expression.h"


expression::expression(field input)
{ 
    mynumrows = input.countcomponents();
    mynumcols = 1;
    
    myoperations.resize(mynumrows);
    for (int row = 0; row < mynumrows; row++)
    {
        // For fields with multiple form function components:
        if (input.getpointer()->countformfunctioncomponents() > 1)
        {
            std::shared_ptr<opfield> op(new opfield(input.getpointer()));
            op->selectformfunctioncomponent(row);
            myoperations[row] = op;
        }
        else
        {
            std::shared_ptr<opfield> op(new opfield(input.comp(row).getpointer()));
            // For printing purposes:
            if (mynumrows > 1)
                op->setfieldcomponent(row);
            myoperations[row] = op;
        }
    }
    
    // Project from the reference element to the physical one a 1 form (hcurl):
    if (input.getpointer()->countformfunctioncomponents() > 1)
    {
        unprojectedfield = {*this};
        // A 1 form E is projected as invjac*E:
        myoperations = ( invjac()*(*this) ).myoperations;
    }
}

expression::expression(double input) { myoperations = {std::shared_ptr<opconstant>(new opconstant(input))}; }

expression::expression(parameter& input)
{ 
    mynumrows = input.countrows();
    mynumcols = input.countcolumns();
            
    myoperations.resize(mynumrows*mynumcols);
    for (int row = 0; row < mynumrows; row++)
    {
        for (int col = 0; col < mynumcols; col++)
            myoperations[row*mynumcols+col] = std::shared_ptr<opparameter>(new opparameter(&input, row, col));
    }
}

expression::expression(int numrows, int numcols, std::vector<expression> input)
{
    mynumrows = numrows;
    mynumcols = numcols;
    
    // In case the user provides only the diagonal part of a square diagonal matrix:
    if (mynumrows == mynumcols && input.size() == mynumrows)
    {
		std::vector<expression> fullinput(mynumrows*mynumcols);
		for (int i = 0; i < mynumrows; i++)
		{
			for (int j = 0; j < mynumcols; j++)
			{
				if (i == j)
					fullinput[i*mynumcols+j] = input[i];
				else			
					fullinput[i*mynumcols+j] = expression(0); 
			}
		}
		input = fullinput;
	}

    // In case the user provides only the lower triangular part of a square symmetric matrix:
    if (mynumrows == mynumcols && input.size() == mynumrows*(mynumcols+1)/2)
    {
		std::vector<expression> fullinput(mynumrows*mynumcols);
		int index = 0;
		for (int i = 0; i < mynumrows; i++)
		{
			for (int j = 0; j <= i; j++)
			{
				if (i == j)
					fullinput[i*mynumcols+j] = input[index];
				else
				{
					fullinput[i*mynumcols+j] = input[index];
					fullinput[j*mynumcols+i] = input[index];
				}
	
				index++;
			}
		}
		input = fullinput;
	}

    if (mynumrows*mynumcols != input.size())
    {
        std::cout << "Error in 'expression' object: a vector of length " << mynumrows*mynumcols << " is required" << std::endl;
        abort();
    }
    
    myoperations.resize(mynumrows*mynumcols);
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (input[i].isscalar())
            myoperations[i] = input[i].myoperations[0];
        else
        {
            std::cout << "Error in 'expression' object: expressions provided in vector must all be scalar" << std::endl;
            abort();
        }
    }
}

expression::expression(expression condexpr, expression exprtrue, expression exprfalse)
{
	// Make sure the conditional expression is a scalar:
	if (condexpr.countrows() != 1 || condexpr.countcolumns() != 1)
	{
		std::cout << "Error in 'expression' object: expected a scalar condition expression (first argument)" << std::endl;
		abort();
	}
	// Make sure the two other expressions have the same size:
	if (exprtrue.countrows() != exprfalse.countrows() || exprtrue.countcolumns() != exprfalse.countcolumns())
	{
		std::cout << "Error in 'expression' object: expected same sized expressions in last two arguments" << std::endl;
		abort();
	}

    if (condexpr.myoperations[0]->isdofincluded() || condexpr.myoperations[0]->istfincluded())
    {
        std::cout << "Error in 'expression object': conditional expression arguments cannot include a dof() or tf()" << std::endl;
        abort();
	}

	// Break into scalars:
	mynumrows = exprtrue.countrows(); mynumcols = exprtrue.countcolumns();

    myoperations.resize(exprtrue.countrows()*exprtrue.countcolumns());
    for (int i = 0; i < exprtrue.countrows()*exprtrue.countcolumns(); i++)
	{
		if (exprtrue.myoperations[i]->isdofincluded() || exprtrue.myoperations[i]->istfincluded() || exprfalse.myoperations[i]->isdofincluded() || exprfalse.myoperations[i]->istfincluded())
		{
		    std::cout << "Error in 'expression object': conditional expression arguments cannot include a dof() or tf()" << std::endl;
		    abort();
		}
    	myoperations[i] = std::shared_ptr<opcondition>(new opcondition(condexpr.myoperations[0], exprtrue.myoperations[i], exprfalse.myoperations[i]));
	}
}

void expression::reorderrows(std::vector<int> neworder)
{
	if (mynumrows != neworder.size())
	{
		std::cout << "Error in 'expression' object: cannot reorder rows with the vector provided (incorrect vector size)" << std::endl;
		abort();
	}

	int minval = *min_element(neworder.begin(), neworder.end());
	int maxval = *max_element(neworder.begin(), neworder.end());
	
	if (minval < 0 || maxval >= neworder.size())
	{
		std::cout << "Error in 'expression' object: cannot reorder rows with the vector provided (out of range integers)" << std::endl;
		abort();
	}

	std::vector<std::shared_ptr<operation>> ops = myoperations;

    for (int i = 0; i < mynumrows; i++)
    {
		for (int j = 0; j < mynumcols; j++)
			myoperations[i*mynumcols+j] = ops[neworder[i]*mynumcols+j];
	}
}

void expression::reordercolumns(std::vector<int> neworder)
{
	if (mynumcols != neworder.size())
	{
		std::cout << "Error in 'expression' object: cannot reorder columns with the vector provided (incorrect vector size)" << std::endl;
		abort();
	}

	int minval = *min_element(neworder.begin(), neworder.end());
	int maxval = *max_element(neworder.begin(), neworder.end());
	
	if (minval < 0 || maxval >= neworder.size())
	{
		std::cout << "Error in 'expression' object: cannot reorder columns with the vector provided (out of range integers)" << std::endl;
		abort();
	}

	std::vector<std::shared_ptr<operation>> ops = myoperations;

    for (int i = 0; i < mynumrows; i++)
    {
		for (int j = 0; j < mynumcols; j++)
			myoperations[i*mynumcols+j] = ops[i*mynumcols+neworder[j]];
	}
}


std::vector<double> expression::max(int physreg, int refinement, std::vector<double> xyzrange) 
{ 
	return max(physreg, NULL, refinement, xyzrange); 
}

std::vector<double> expression::max(int physreg, expression meshdeform, int refinement, std::vector<double> xyzrange) 
{ 
	return max(physreg, &meshdeform, refinement, xyzrange); 
}

std::vector<double> expression::min(int physreg, int refinement, std::vector<double> xyzrange)
{ 
    // The actual min value is minus the max value found:
    std::vector<double> output = (-(*this)).max(physreg, NULL, refinement, xyzrange); 
    if (output.size() > 0)
        output[0] = -output[0];
    return output;
}

std::vector<double> expression::min(int physreg, expression meshdeform, int refinement, std::vector<double> xyzrange) 
{ 
    // The actual min value is minus the max value found:
    std::vector<double> output =  (-(*this)).max(physreg, &meshdeform, refinement, xyzrange); 
    if (output.size() > 0)
        output[0] = -output[0];
    return output;
}

std::vector<double> expression::max(int physreg, expression* meshdeform, int refinement, std::vector<double> xyzrange)
{        
    field x("x"), y("y"), z("z");

    // Minimum refinement order is 1!
    if (refinement < 1)
        refinement = 1;

    // Make sure this expression is scalar and the mesh 
    // deformation expression has the right size. 
    if (not(isscalar()))
    {
        std::cout << "Error in 'expression' object: cannot get the max/min of a nonscalar expression" << std::endl;
        abort();
    }
    int problemdimension = universe::mymesh->getmeshdimension();
    if (meshdeform != NULL && (meshdeform->countcolumns() != 1 || meshdeform->countrows() < problemdimension))
    {
        std::cout << "Error in 'expression' object: mesh deformation expression has size " << meshdeform->countrows() << "x" << meshdeform->countcolumns() << " (expected " << problemdimension << "x1)" << std::endl;
        abort();
    }
    
    // Get only the disjoint regions with highest dimension elements:
    std::vector<int> selecteddisjregs = ((universe::mymesh->getphysicalregions())->get(physreg))->getdisjointregions();

    // Multiharmonic expressions are not allowed.
    if (not(isharmonicone(selecteddisjregs)))
    {
        std::cout << "Error in 'expression' object: cannot get the max/min of a multiharmonic expression (only constant harmonic 1)" << std::endl;
        abort();
    }
    if (meshdeform != NULL && not(meshdeform->isharmonicone(selecteddisjregs)))
    {
        std::cout << "Error in 'expression' object: the mesh deformation expression cannot be multiharmonic (only constant harmonic 1)" << std::endl;
        abort();
    }
    
    // This will be the output:
    std::vector<double> maxval = {};

    // Send the disjoint regions with same element type numbers together:
    disjointregionselector mydisjregselector(selecteddisjregs, {});
    for (int i = 0; i < mydisjregselector.countgroups(); i++)
    {
        std::vector<int> mydisjregs = mydisjregselector.getgroup(i);
        
        // Get the node coordinates in the refined element:
        int elementtypenumber = (universe::mymesh->getdisjointregions())->getelementtypenumber(mydisjregs[0]);
        element myelement(elementtypenumber, refinement);
        std::vector<double> evaluationpoints = myelement.listnodecoordinates();

        // Loop on all total orientations (if required):
        bool isorientationdependent = isvalueorientationdependent(mydisjregs) || (meshdeform != NULL && meshdeform->isvalueorientationdependent(mydisjregs));
        elementselector myselector(mydisjregs, isorientationdependent);
        do 
        {
            universe::allowreuse();

            // Compute the expression at the evaluation points:
            densematrix compxval = myoperations[0]->interpolate(myselector, evaluationpoints, meshdeform)[1][0];  
            // Get the coordinates corresponding to the interpolated values:
            densematrix xval = expression(x).myoperations[0]->interpolate(myselector, evaluationpoints, meshdeform)[1][0];       
            densematrix yval = expression(y).myoperations[0]->interpolate(myselector, evaluationpoints, meshdeform)[1][0];       
            densematrix zval = expression(z).myoperations[0]->interpolate(myselector, evaluationpoints, meshdeform)[1][0]; 
      
            universe::forbidreuse();
            
            double* valuesptr = compxval.getvalues();
            double* xvalptr = xval.getvalues();
            double* yvalptr = yval.getvalues();
            double* zvalptr = zval.getvalues();
            
            // Loop on all data points.
            for (int d = 0; d < compxval.countrows() * compxval.countcolumns(); d++)
            {
                bool isinboundedregion = (xyzrange.size() == 0) || (xyzrange[0] < xvalptr[d] && xyzrange[1] > xvalptr[d] && xyzrange[2] < yvalptr[d] && xyzrange[3] > yvalptr[d] && xyzrange[4] < zvalptr[d] && xyzrange[5] > zvalptr[d]);
                
                if ( isinboundedregion && (maxval.size() == 0 || maxval[0] < valuesptr[d]) )	
                	    maxval = {valuesptr[d], xvalptr[d], yvalptr[d], zvalptr[d]};
            }
        } 
        while (myselector.next());
    }

    return maxval;
}

void expression::interpolate(int physreg, std::vector<double>& xyzcoord, std::vector<double>& interpolated, std::vector<bool>& isfound)
{
	interpolate(physreg, NULL, xyzcoord, interpolated, isfound);
}

void expression::interpolate(int physreg, expression meshdeform, std::vector<double>& xyzcoord, std::vector<double>& interpolated, std::vector<bool>& isfound)
{
	interpolate(physreg, &meshdeform, xyzcoord, interpolated, isfound);
}

std::vector<double> expression::interpolate(int physreg, const std::vector<double> xyzcoord)
{
	std::vector<double> xyz = xyzcoord;

	if (xyz.size() != 3)
	{
        std::cout << "Error in 'expression' object: expected a coordinate vector of length 3" << std::endl;
        abort();
	}

	std::vector<double> interpolated = {};
	std::vector<bool> isfound = {};
	
	interpolate(physreg, NULL, xyz, interpolated, isfound);
	
	if (isfound[0])
		return interpolated;
	else
		return {};
}

std::vector<double> expression::interpolate(int physreg, expression meshdeform, const std::vector<double> xyzcoord)
{
	std::vector<double> xyz = xyzcoord;
	
	if (xyz.size() != 3)
	{
        std::cout << "Error in 'expression' object: expected a coordinate vector of length 3" << std::endl;
        abort();
	}

	std::vector<double> interpolated = {};
	std::vector<bool> isfound = {};
	
	interpolate(physreg, &meshdeform, xyz, interpolated, isfound);
	
	if (isfound[0])
		return interpolated;
	else
		return {};
}

void expression::interpolate(int physreg, expression* meshdeform, std::vector<double>& xyzcoord, std::vector<double>& interpolated, std::vector<bool>& isfound)
{
    // Make sure the mesh deformation expression has the right size. 
    int problemdimension = universe::mymesh->getmeshdimension();
    if (meshdeform != NULL && (meshdeform->countcolumns() != 1 || meshdeform->countrows() < problemdimension))
    {
        std::cout << "Error in 'expression' object: mesh deformation expression has size " << meshdeform->countrows() << "x" << meshdeform->countcolumns() << " (expected " << problemdimension << "x1)" << std::endl;
        abort();
    }
    
    // Get only the disjoint regions with highest dimension elements:
    std::vector<int> disjregs = ((universe::mymesh->getphysicalregions())->get(physreg))->getdisjointregions(problemdimension);

	if (universe::mymesh->getphysicalregions()->get(physreg)->getelementdimension() != problemdimension)
	{
        std::cout << "Error in 'expression' object: expected a physical region with " << problemdimension << "D elements in 'interpolate'" << std::endl;
        abort();
	}

    // Multiharmonic expressions are not allowed.
    if (not(isharmonicone(disjregs)))
    {
        std::cout << "Error in 'expression' object: cannot interpolate a multiharmonic expression (only constant harmonic 1)" << std::endl;
        abort();
    }
    if (meshdeform != NULL && not(meshdeform->isharmonicone(disjregs)))
    {
        std::cout << "Error in 'expression' object: the mesh deformation expression cannot be multiharmonic (only constant harmonic 1)" << std::endl;
        abort();
    }
    if (xyzcoord.size()%3 != 0)
    {
        std::cout << "Error in 'expression' object: the interpolation coordinates vector should have a length that is a multiple of 3" << std::endl;
        abort();
    }
    
    
    // Preallocate the input containers:
    int numcoords = xyzcoord.size()/3;
    isfound.resize(numcoords);
    for (int i = 0; i < numcoords; i++)
    	isfound[i] = false;
	int exprlen = mynumrows*mynumcols;
    interpolated.resize(numcoords*exprlen);
    for (int i = 0; i < interpolated.size(); i++)
    	interpolated[i] = 0.0;

	// Sort the coordinates vector:
	std::vector<int> reorderingvector = {};
	std::vector<double> noisethres = universe::mymesh->getnodes()->getnoisethreshold();
	myalgorithm::stablecoordinatesort(noisethres, xyzcoord, reorderingvector);
	std::vector<double> xyz = xyzcoord;
	for (int i = 0; i < numcoords; i++)
    {
    	xyz[3*i+0] = xyzcoord[3*reorderingvector[i]+0];
    	xyz[3*i+1] = xyzcoord[3*reorderingvector[i]+1];
    	xyz[3*i+2] = xyzcoord[3*reorderingvector[i]+2];
    }
		
    disjointregions* mydisjregs = universe::mymesh->getdisjointregions();
	elements* myelems = universe::mymesh->getelements();
	
	int elemorder = myelems->getcurvatureorder();

	// Parameter that gives the distance (in multiples of the max barycenter-element node distance)
	// starting from which one can safely assume to always be outside the element for any point.
	// For straight elements this always holds for alpha equal to 1 (plus roundoff safety):
	double alpha = 1+1e-10;
	if (elemorder > 1)
		alpha = 2.0;


    for (int i = 0; i < disjregs.size(); i++)
    {
    	int elemtypenum = mydisjregs->getelementtypenumber(disjregs[i]);
    	int elemdim = mydisjregs->getelementdimension(disjregs[i]);
    	
    	lagrangeformfunction mylagrange(elemtypenum, elemorder, {});
		element myel(elemtypenum, elemorder);
				
    	// Get the element barycenter coordinates:
        std::vector<double>* barycenters = myelems->getbarycenters(elemtypenum);
        // Get the radius of the sphere centered at the barycenter and surrounding all nodes in an element:
        std::vector<double>* sphereradius = myelems->getsphereradius(elemtypenum);
    
    	// Loop on all elements in the disjoint region:
    	int rangebegin = mydisjregs->getrangebegin(disjregs[i]), rangeend = mydisjregs->getrangeend(disjregs[i]);
    	for (int current = rangebegin; current <= rangeend; current++)
    	{    		
    		double maxelemsize = alpha*sphereradius->at(current);
    		double xbary = barycenters->at(3*current+0), ybary = barycenters->at(3*current+1), zbary = barycenters->at(3*current+2);

			std::vector<polynomial> poly = {};
			
			// Get the index of the coordinate whose x is closest to the barycenter of the current element (using bisection):
			int first = 0, last = numcoords-1;
			int startindex;
	
			while (first < last-1)
			{
				if ((last+first)%2 == 0)
					startindex = (last+first)/2;
				else
					startindex = (last+first-1)/2;
	
				if (xyz[3*startindex+0]-xbary > noisethres[0])
					last = startindex;
				else
					first = startindex;
			}
			// This is required for cases where first = last-1:
			if (std::abs(xyz[3*last+0]-xbary) >= std::abs(xyz[3*first+0]-xbary))
				startindex = first;
			else
				startindex = last;
			
			// Move away in the increasing index direction from 'startindex':
			for (int coordindex = startindex; coordindex < numcoords; coordindex++)
			{    		
				// If out of range in x it will not be found here.
				if (std::abs(xyz[3*coordindex+0]-xbary) < maxelemsize)
				{
					// Only process when not yet found and when the coordinate is close enough to the element barycenter.
					if (isfound[coordindex] || std::abs(xyz[3*coordindex+1]-ybary) > maxelemsize || std::abs(xyz[3*coordindex+2]-zbary) > maxelemsize) {}
					else
					{    			
						// Reset to initial guess:
						std::vector<double> kietaphi = {0,0,0};
						std::vector<double> rhs = {xyz[3*coordindex+0], xyz[3*coordindex+1], xyz[3*coordindex+2]};

						// Only create once for all coordinates the polynomials and only for the required elements:
						if (poly.size() == 0)
						{
							poly.resize(elemdim);
							for (int j = 0; j < elemdim; j++)
								poly[j] = mylagrange.getinterpolationpolynomial(myelems->getnodecoordinates(elemtypenum, current, j));
						}
				
						if (myalgorithm::getroot(poly, rhs, kietaphi))
						{
							// Check if the (ki,eta,phi) coordinates are inside the element:
							if (myel.isinsideelement(kietaphi[0], kietaphi[1], kietaphi[2]))
							{
								isfound[coordindex] = true;
					
								// Evaluate the expression on that element at the (ki,eta,phi) reference coordinate:
								elementselector myselector(disjregs[i], current-rangebegin);
								for (int k = 0; k < exprlen; k++)
								{
									densematrix interp = myoperations[k]->interpolate(myselector, kietaphi, meshdeform)[1][0];
									interpolated[coordindex*exprlen+k] = interp.getvalues()[0];
								}
							}
						}
					}	
				}
				else
					break;
			}

			
			// Move away in the decreasing index direction from 'startindex':
			for (int coordindex = startindex-1; coordindex >= 0; coordindex--)
			{    		
				// If out of range in x it will not be found here.
				if (std::abs(xyz[3*coordindex+0]-xbary) < maxelemsize)
				{
					// Only process when not yet found and when the coordinate is close enough to the element barycenter.
					if (isfound[coordindex] || std::abs(xyz[3*coordindex+1]-ybary) > maxelemsize || std::abs(xyz[3*coordindex+2]-zbary) > maxelemsize) {}
					else
					{    			
						// Reset to initial guess:
						std::vector<double> kietaphi = {0,0,0};
						std::vector<double> rhs = {xyz[3*coordindex+0], xyz[3*coordindex+1], xyz[3*coordindex+2]};

						// Only create once for all coordinates the polynomials and only for the required elements:
						if (poly.size() == 0)
						{
							poly.resize(elemdim);
							for (int j = 0; j < elemdim; j++)
								poly[j] = mylagrange.getinterpolationpolynomial(myelems->getnodecoordinates(elemtypenum, current, j));
						}
				
						if (myalgorithm::getroot(poly, rhs, kietaphi))
						{
							// Check if the (ki,eta,phi) coordinates are inside the element:
							if (myel.isinsideelement(kietaphi[0], kietaphi[1], kietaphi[2]))
							{
								isfound[coordindex] = true;
					
								// Evaluate the expression on that element at the (ki,eta,phi) reference coordinate:
								elementselector myselector(disjregs[i], current-rangebegin);
								for (int k = 0; k < exprlen; k++)
								{
									densematrix interp = myoperations[k]->interpolate(myselector, kietaphi, meshdeform)[1][0];
									interpolated[coordindex*exprlen+k] = interp.getvalues()[0];
								}
							}
						}
					}		
				}
				else
					break;
			}
		}
    }
    
    // Unsort the containers:
    std::vector<double> interpolatedcopy = interpolated;
    std::vector<bool> isfoundcopy = isfound;
    for (int i = 0; i < numcoords; i++)
    {
    	isfound[reorderingvector[i]] = isfoundcopy[i];
    	for (int j = 0; j < exprlen; j++)
    		interpolated[reorderingvector[i]*exprlen+j] = interpolatedcopy[i*exprlen+j];
    }
}


double expression::integrate(int physreg, int integrationorder) { return integrate(physreg, NULL, integrationorder); }
double expression::integrate(int physreg, expression meshdeform, int integrationorder) { return integrate(physreg, &meshdeform, integrationorder); }

double expression::integrate(int physreg, expression* meshdeform, int integrationorder)
{        
    // Make sure this expression is scalar and the mesh 
    // deformation expression has the right size. 
    if (not(isscalar()))
    {
        std::cout << "Error in 'expression' object: cannot integrate a nonscalar expression" << std::endl;
        abort();
    }
    int problemdimension = universe::mymesh->getmeshdimension();
    if (meshdeform != NULL && (meshdeform->countcolumns() != 1 || meshdeform->countrows() < problemdimension))
    {
        std::cout << "Error in 'expression' object: mesh deformation expression has size " << meshdeform->countrows() << "x" << meshdeform->countcolumns() << " (expected " << problemdimension << "x1)" << std::endl;
        abort();
    }
    
    // Get only the disjoint regions with highest dimension elements:
    std::vector<int> selecteddisjregs = ((universe::mymesh->getphysicalregions())->get(physreg))->getdisjointregions();

    // Multiharmonic expressions are not allowed.
    if (not(isharmonicone(selecteddisjregs)))
    {
        std::cout << "Error in 'expression' object: cannot integrate a multiharmonic expression (only constant harmonic 1)" << std::endl;
        abort();
    }
    if (meshdeform != NULL && not(meshdeform->isharmonicone(selecteddisjregs)))
    {
        std::cout << "Error in 'expression' object: the mesh deformation expression cannot be multiharmonic (only constant harmonic 1)" << std::endl;
        abort();
    }
    
    double integralvalue = 0;
    // Send the disjoint regions with same element type numbers together:
    disjointregionselector mydisjregselector(selecteddisjregs, {});
    for (int i = 0; i < mydisjregselector.countgroups(); i++)
    {
        std::vector<int> mydisjregs = mydisjregselector.getgroup(i);
        
        // Get the Gauss points:
        int elementtypenumber = (universe::mymesh->getdisjointregions())->getelementtypenumber(mydisjregs[0]);      
        gausspoints mygausspoints(elementtypenumber, integrationorder);
        std::vector<double> evaluationpoints = mygausspoints.getcoordinates();
        std::vector<double> weights = mygausspoints.getweights();

        // Loop on all total orientations (if required):
        bool isorientationdependent = isvalueorientationdependent(mydisjregs) || (meshdeform != NULL && meshdeform->isvalueorientationdependent(mydisjregs));
        elementselector myselector(mydisjregs, isorientationdependent);
        do 
        {
	    shared_ptr<jacobian> myjacobian;

	    if (universe::forcejacobianreuse && universe::computedjacobian != NULL)
		myjacobian = universe::computedjacobian;
	    else
                myjacobian = shared_ptr<jacobian>(new jacobian(myselector, evaluationpoints, meshdeform));

            densematrix detjac = myjacobian->getdetjac();
    		// The Jacobian determinant should be positive irrespective of the node numbering:
			detjac.abs();
            // Store it in the universe for reuse.
            universe::computedjacobian = myjacobian;
            universe::allowreuse();
            
            densematrix compxinterpolated = myoperations[0]->interpolate(myselector, evaluationpoints, meshdeform)[1][0];       
            compxinterpolated.multiplyelementwise(detjac);
            
            universe::forbidreuse();
            
            densematrix weightsmat(mygausspoints.count(), 1, weights);
            compxinterpolated = compxinterpolated.multiply(weightsmat);
            
            integralvalue += compxinterpolated.sum();
        } 
        while (myselector.next());
    }
    return integralvalue;
}

void expression::write(int physreg, int numfftharms, std::string filename, int lagrangeorder)
{        
    write(physreg, numfftharms, NULL, filename, lagrangeorder, -1);
}

void expression::write(int physreg, int numfftharms, expression meshdeform, std::string filename, int lagrangeorder)
{        
    write(physreg, numfftharms, &meshdeform, filename, lagrangeorder, -1);
}
        
void expression::write(int physreg, std::string filename, int lagrangeorder, int numtimesteps)
{        
    write(physreg, -1, NULL, filename, lagrangeorder, numtimesteps);
}

void expression::write(int physreg, expression meshdeform, std::string filename, int lagrangeorder, int numtimesteps)
{        
    write(physreg, -1, &meshdeform, filename, lagrangeorder, numtimesteps);
}


void expression::write(int physreg, int numfftharms, expression* meshdeform, std::string filename, int lagrangeorder, int numtimesteps)
{        
    // Make sure the filename includes the extension:
    if (filename.size() < 5 || filename.substr(filename.size()-4,4) != ".pos")
    {
        std::cout << "Error in 'expression' object: cannot write to file '" << filename << "' (unknown or missing file extension)" << std::endl;
        abort();
    }
    // Remove the extension:
    filename = filename.substr(0, filename.size()-4);
    // Make sure this expression is a column vector and the 
    // mesh deformation expression has the right size. 
    if (mynumrows > 3 || mynumcols != 1)
    {
        std::cout << "Error in 'expression' object: can not write a " << mynumrows << "x" << mynumcols << " expression to file (only scalars or 2x1 or 3x1 column vectors)" << std::endl;
        abort();
    }
    int problemdimension = universe::mymesh->getmeshdimension();
    if (meshdeform != NULL && (meshdeform->countcolumns() != 1 || meshdeform->countrows() < problemdimension))
    {
        std::cout << "Error in 'expression' object: mesh deformation expression has size " << meshdeform->countrows() << "x" << meshdeform->countcolumns() << " (expected " << problemdimension << "x1)" << std::endl;
        abort();
    }
        
    // Minimum lagrange order is 1!
    if (lagrangeorder < 1)
        lagrangeorder = 1;
    
    field x("x"), y("y"), z("z");
    expression xyz(3,1, {x,y,z});

    // In case there are multiple element types we append the type name to the view:
    std::string elementtype = "";
    
    // Loop on all disjoint regions:
    std::vector<int> selecteddisjregs = ((universe::mymesh->getphysicalregions())->get(physreg))->getdisjointregions();
        
    // Make sure the 'meshdeform' expression is constant in time:
    if (meshdeform != NULL && not(meshdeform->isharmonicone(selecteddisjregs)))
    {
        std::cout << "Error in 'expression' object: the mesh deformation expression cannot be multiharmonic (only constant harmonic 1)" << std::endl;
        abort();
    }
    
    // Send the disjoint regions with same element type numbers together:
    disjointregionselector mydisjregselector(selecteddisjregs, {});
    for (int i = 0; i < mydisjregselector.countgroups(); i++)
    {
        std::vector<int> mydisjregs = mydisjregselector.getgroup(i);
    
        int elementtypenumber = (universe::mymesh->getdisjointregions())->getelementtypenumber(mydisjregs[0]);
        element myelement(elementtypenumber);
        if (mydisjregselector.countgroups() > 1)
            elementtype = "_" + myelement.gettypename();
        
        // The expression will be interpolated at the following Lagrange nodes:
        lagrangeformfunction mylagrange(elementtypenumber,lagrangeorder, {});
        std::vector<double> lagrangecoords = mylagrange.getnodecoordinates();
        std::vector<polynomial> poly = mylagrange.getformfunctionpolynomials();
        
        std::vector<double> lagrangecoordsxyz = lagrangecoords;
        std::vector<polynomial> polyxyz = poly;
        // The elements might not be curved:
        if (universe::mymesh->getelements()->getcurvatureorder() == 1 && meshdeform == NULL)
        {
		    lagrangeformfunction mylagrangexyz(elementtypenumber,1, {});
		    lagrangecoordsxyz = mylagrangexyz.getnodecoordinates();
		    polyxyz = mylagrangexyz.getformfunctionpolynomials();
	    }
        
        std::vector<int> harms = {};
        std::vector<mystring> harmfilename = {};
        
        bool writeheader = true;
        
        // Loop on all total orientations (if required):
        bool isorientationdependent = isvalueorientationdependent(mydisjregs) || (meshdeform != NULL && meshdeform->isvalueorientationdependent(mydisjregs));
        elementselector myselector(mydisjregs, isorientationdependent);
        do 
        {
            // Compute the mesh coordinates. Initialise all coordinates to zero.
            std::vector<densematrix> coords(3,densematrix(myselector.countinselection(), lagrangecoordsxyz.size()/3,0));
            for (int i = 0; i < problemdimension; i++)
            {
                coords[i] = (xyz.myoperations[i]->interpolate(myselector, lagrangecoordsxyz, NULL))[1][0];
                if (meshdeform != NULL)
                    coords[i].add((meshdeform->myoperations[i]->interpolate(myselector, lagrangecoordsxyz, NULL))[1][0]);
            }
            // Interpolate the current expression:
            std::vector<  std::vector<std::vector<densematrix>>  > expr(countrows());
            std::vector<densematrix> fftexpr(countrows());
            // Reuse what's possible to reuse during the interpolation:
            universe::allowreuse();
            for (int i = 0; i < countrows(); i++)
            {
                if (numtimesteps <= 0)
                {
                    if (numfftharms <= 0)
                        expr[i] = myoperations[i]->interpolate(myselector, lagrangecoords, meshdeform);
                    else
                        expr[i] = myfft::fft(myoperations[i]->multiharmonicinterpolate(numfftharms, myselector, lagrangecoords, meshdeform), myselector.countinselection(), lagrangecoords.size()/3);
                }
                else
                    fftexpr[i] = myfft::toelementrowformat(myoperations[i]->multiharmonicinterpolate(numtimesteps, myselector, lagrangecoords, meshdeform), myselector.countinselection());
            }
            universe::forbidreuse();

            if (harms.size() == 0)
            {
                // Get a vector containing all harmonic numbers in 'expr':
                if (numtimesteps <= 0)
                {
                    for (int h = 0; h < expr[0].size(); h++)
                    {
                        if (expr[0][h].size() == 1)
                        {
                            harms.push_back(h);
                            // For a constant (harmonic 1) we do not append the harmonic number.
                            if (h == 1)
                                harmfilename.push_back(mystring(filename));
                            else
                                harmfilename.push_back(mystring(filename + "_harm" + std::to_string(h)));
                        }
                    }
                }
            }

            // Write the header:
            if (writeheader)
            {
                if (numtimesteps <= 0)
                {
                    for (int h = 0; h < harms.size(); h++)
                        gmshinterface::openview(harmfilename[h].getstring() + ".pos", harmfilename[h].getstring() + elementtype, 0, i == 0);
                }
                else
                    gmshinterface::openview(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos", filename + "_" + std::to_string(numtimesteps) + "timesteps" + elementtype, 0, i == 0);
                writeheader = false;
            }
            // Append the data:
            if (numtimesteps <= 0)
            {
                for (int h = 0; h < harms.size(); h++)
                {
                    if (countrows() == 1)
                        gmshinterface::appendtoview(harmfilename[h].getstring() + ".pos", elementtypenumber, coords[0], coords[1], coords[2], expr[0][harms[h]][0]);
                    if (countrows() == 2)
                        gmshinterface::appendtoview(harmfilename[h].getstring() + ".pos", elementtypenumber, coords[0], coords[1], coords[2], expr[0][harms[h]][0], expr[1][harms[h]][0], densematrix(expr[0][harms[h]][0].countrows(), expr[0][harms[h]][0].countcolumns(),0));
                    if (countrows() == 3)
                        gmshinterface::appendtoview(harmfilename[h].getstring() + ".pos", elementtypenumber, coords[0], coords[1], coords[2], expr[0][harms[h]][0], expr[1][harms[h]][0], expr[2][harms[h]][0]);
                }
            }
            else
            {
                if (countrows() == 1)
                    gmshinterface::appendtoview(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos", elementtypenumber, coords[0], coords[1], coords[2], fftexpr[0]);
                if (countrows() == 2)
                    gmshinterface::appendtoview(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos", elementtypenumber, coords[0], coords[1], coords[2], fftexpr[0], fftexpr[1], densematrix(fftexpr[0].countrows(), fftexpr[0].countcolumns(),0));
                if (countrows() == 3)
                    gmshinterface::appendtoview(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos", elementtypenumber, coords[0], coords[1], coords[2], fftexpr[0], fftexpr[1], fftexpr[2]);
            }
        } 
        while (myselector.next());
        
        // Write the interpolation scheme:
        if (numtimesteps <= 0)
        {
            for (int h = 0; h < harms.size(); h++)
            {
                gmshinterface::writeinterpolationscheme(harmfilename[h].getstring() + ".pos", {poly, polyxyz});
                gmshinterface::closeview(harmfilename[h].getstring() + ".pos");
            }
        }
        else
        {
            gmshinterface::writeinterpolationscheme(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos", {poly, polyxyz});
            gmshinterface::closeview(filename + "_" + std::to_string(numtimesteps) + "timesteps" + ".pos");
        }
    }
}

void expression::streamline(int physreg, std::string filename, const std::vector<double>& startcoords, double stepsize, bool downstreamonly)
{
	streamline(physreg, NULL, filename, startcoords, stepsize, downstreamonly);
}

void expression::streamline(int physreg, expression meshdeform, std::string filename, const std::vector<double>& startcoords, double stepsize, bool downstreamonly)
{
	streamline(physreg, &meshdeform, filename, startcoords, stepsize, downstreamonly);
}

void expression::streamline(int physreg, expression* meshdeform, std::string filename, const std::vector<double>& startcoords, double stepsize, bool downstreamonly)
{
    // Make sure the filename includes the extension:
    if (filename.size() < 5 || filename.substr(filename.size()-4,4) != ".pos")
    {
        std::cout << "Error in 'expression' object: cannot write stream lines to file '" << filename << "' (unknown or missing file extension)" << std::endl;
        abort();
    }
    // Remove the extension:
    std::string filenamenoextension = filename.substr(0, filename.size()-4);

	// Stream lines can only be obtained for expressions with at least as many components as the geometry dimension:
	int problemdimension = universe::mymesh->getmeshdimension();
    if (mynumrows < problemdimension || mynumrows > 3 || mynumcols != 1)
    {
        std::cout << "Error in 'expression' object: expected a column vector expression with " << problemdimension << " to 3 components to get the stream lines" << std::endl;
        abort();
    }
    // For simplicity the code below is written only for 3x1 expressions:
    if (mynumrows == 1)
    {
    	expression(3,1,{*this,0,0}).streamline(physreg, meshdeform, filename, startcoords, stepsize, downstreamonly);
    	return;
    }
    if (mynumrows == 2)
    {
    	expression(3,1,{this->at(0,0),this->at(1,0),0}).streamline(physreg, meshdeform, filename, startcoords, stepsize, downstreamonly);
    	return;
    }
    
    int meshexprlen = 0;
    if (meshdeform != NULL)
    	meshexprlen = meshdeform->mynumrows*meshdeform->mynumcols;
    
    if (startcoords.size()%3 != 0)
    {
        std::cout << "Error in 'expression' object: expected a vector with a length multiple of 3 for the stream line starting coordinates" << std::endl;
        abort();
    }
	
	int numnodes = startcoords.size()/3;
	
	// If upstream AND downstream calculation there are double the amount of starting coords:
	int factortwo = 2;
	if (downstreamonly)
		factortwo = 1;
		
	std::vector<std::vector<double>> xcoords(factortwo*numnodes), ycoords(factortwo*numnodes), zcoords(factortwo*numnodes), magnitude(factortwo*numnodes);

	// Current coordinates:
	std::vector<double> curcoords(factortwo* 3*numnodes);
	// Know which stream line is still active:
	std::vector<bool> isactive(factortwo*numnodes, true);
	
	// Stepsize vector for every starting point:
	std::vector<double> h(factortwo*numnodes);
	
	
	for (int i = 0; i < numnodes; i++)
	{
		curcoords[factortwo*3*i+0] = startcoords[3*i+0];
		curcoords[factortwo*3*i+1] = startcoords[3*i+1];
		curcoords[factortwo*3*i+2] = startcoords[3*i+2];
		
		h[factortwo*i+0] = stepsize;
		
		if (downstreamonly == false)
		{
			curcoords[factortwo*3*i+3] = startcoords[3*i+0];
			curcoords[factortwo*3*i+4] = startcoords[3*i+1];
			curcoords[factortwo*3*i+5] = startcoords[3*i+2];
		
			h[factortwo*i+1] = -stepsize;
		}
	}
	std::vector<double> originalh = h;
	

	std::vector<double> k1, k2, k3, k4, meshdeforminterpol;
	std::vector<bool> isfound;
	
	double maxflowspeed = (mathop::norm(*this)).max(physreg, 1)[0];

	bool isdone = false;
	while (isdone == false)
	{
		// Calculate the mesh deformation expression:
		if (meshdeform != NULL)
			interpolate(physreg, NULL, curcoords, meshdeforminterpol, isfound);
			
		// Calculate the Runge-Kutta parameter k1:
		interpolate(physreg, meshdeform, curcoords, k1, isfound);
		
		// Normalize the stepsize according to the flow velocity:
		double curflowspeed;
		for (int i = 0; i < h.size(); i++)
		{
			curflowspeed = std::sqrt(k1[3*i+0]*k1[3*i+0] + k1[3*i+1]*k1[3*i+1] + k1[3*i+2]*k1[3*i+2]);
			
			if (curflowspeed > 0)
				h[i] = originalh[i] * maxflowspeed/curflowspeed;
		}
		
		// Calculate the Runge-Kutta parameters k2, k3 and k4:
		std::vector<double> ynplushk1over2 = curcoords;
		for (int i = 0; i < curcoords.size(); i++)
			ynplushk1over2[i] += h[(i-i%3)/3]*0.5*k1[i];
		interpolate(physreg, meshdeform, ynplushk1over2, k2, isfound);
		std::vector<double> ynplushk2over2 = curcoords;
		for (int i = 0; i < curcoords.size(); i++)
			ynplushk2over2[i] += h[(i-i%3)/3]*0.5*k2[i];
		interpolate(physreg, meshdeform, ynplushk2over2, k3, isfound);
		std::vector<double> ynplushk3 = curcoords;
		for (int i = 0; i < curcoords.size(); i++)
			ynplushk3[i] += h[(i-i%3)/3]*k3[i];
		interpolate(physreg, meshdeform, ynplushk3, k4, isfound);
		
		// Append data to write to disk:
		for (int i = 0; i < isactive.size(); i++)
		{
			if (isactive[i])
			{
				std::vector<double> meshdefinterpol(3,0);
				if (meshdeform != NULL)
				{
					for (int j = 0; j < meshexprlen; j++)
						meshdefinterpol[j] = meshdeforminterpol[meshexprlen*i+j];
				}
			
				xcoords[i].push_back(curcoords[3*i+0] + meshdefinterpol[0]);
				ycoords[i].push_back(curcoords[3*i+1] + meshdefinterpol[1]);
				zcoords[i].push_back(curcoords[3*i+2] + meshdefinterpol[2]);
		
				double flowspeed = std::sqrt(k1[3*i+0]*k1[3*i+0] + k1[3*i+1]*k1[3*i+1] + k1[3*i+2]*k1[3*i+2]);
				
				magnitude[i].push_back(flowspeed);
			}
		}
		
		// Update the coordinates:
		for (int i = 0; i < curcoords.size(); i++)
			curcoords[i] += h[(i-i%3)/3]/6.0*( k1[i]+2.0*k2[i]+2.0*k3[i]+k4[i] );
		
		
		isdone = true;
		for (int i = 0; i < isfound.size(); i++)
		{
			if (isfound[i] == false)
				isactive[i] = false;
			if (isactive[i])
				isdone = false;
		}
	}
	
	
	// Write to file:
	gmshinterface::openview(filename, filenamenoextension, 0, true);
	for (int m = 0; m < xcoords.size(); m++)
	{
		int numlines = xcoords[m].size();
		densematrix xcoordsmat(numlines,2, 0), ycoordsmat(numlines,2, 0), zcoordsmat(numlines,2, 0), flowspeedmat(numlines,2, 0);
		double* xptr = xcoordsmat.getvalues(); double* yptr = ycoordsmat.getvalues(); double* zptr = zcoordsmat.getvalues();
		double* fsptr = flowspeedmat.getvalues();
	
		for (int i = 0; i < numlines-1; i++)
		{
			xptr[2*i+0] = xcoords[m][i%numlines]; xptr[2*i+1] = xcoords[m][(i+1)%numlines];
			yptr[2*i+0] = ycoords[m][i%numlines]; yptr[2*i+1] = ycoords[m][(i+1)%numlines];
			zptr[2*i+0] = zcoords[m][i%numlines]; zptr[2*i+1] = zcoords[m][(i+1)%numlines];
		
			fsptr[2*i+0] = magnitude[m][i%numlines]; fsptr[2*i+1] = magnitude[m][(i+1)%numlines];
		}
	
		gmshinterface::appendtoview(filename, 1, xcoordsmat, ycoordsmat, zcoordsmat, flowspeedmat);
	}
	gmshinterface::closeview(filename);
}

std::vector<double> expression::shapecut(int physreg, shape myshape, std::string filename)
{
	return shapecut(physreg, NULL, myshape, filename);
}

std::vector<double> expression::shapecut(int physreg, expression meshdeform, shape myshape, std::string filename)
{
	return shapecut(physreg, &meshdeform, myshape, filename);
}

std::vector<double> expression::shapecut(int physreg, expression* meshdeform, shape myshape, std::string filename)
{
	int exprlen = mynumrows*mynumcols;
	
	// Get the mesh node coordinates of the shape:
	std::vector<double> xyzcoord = *(myshape.getpointer()->getcoords());
	
	// Interpolate at the mesh nodes:
	std::vector<double> interpolated;
	std::vector<bool> isfound;
	
    interpolate(physreg, meshdeform, xyzcoord, interpolated, isfound);
	
	// Filter out the nodes outside the physical region:
	int numfound = 0;
	for (int i = 0; i < isfound.size(); i++)
	{
		if (isfound[i])
			numfound++;
	}
	
	int blocklen = 3+exprlen, index = 0;
	std::vector<double> output(numfound*blocklen);
	for (int i = 0; i < isfound.size(); i++)
	{
		if (isfound[i])
		{
			output[index*blocklen+0] = xyzcoord[3*i+0];
			output[index*blocklen+1] = xyzcoord[3*i+1];
			output[index*blocklen+2] = xyzcoord[3*i+2];
		
			for (int j = 0; j < exprlen; j++)
				output[index*blocklen+3+j] = interpolated[i*exprlen+j];
		
			index++;
		}
	}
	
	// Write to disk if a filename is provided. The filename validity is checked in 'scatterwrite'.
	if (filename.size() > 0)
	{
		if (mynumrows > 3 || mynumcols != 1)
		{
		    std::cout << "Error in 'expression' object: writing a shape cut to file is only allowed for up to length 3 column vector expressions" << std::endl;
		    abort();
		}
		
		std::vector<double> outputcopy = output;
			
		// Save on the deformed mesh if requested:
		if (meshdeform != NULL)
		{
			int meshexprlen = meshdeform->countrows()*meshdeform->countcolumns();
			std::vector<double> interpolmesh;
			meshdeform->interpolate(physreg, NULL, xyzcoord, interpolmesh, isfound);
			int index = 0;
			for (int i = 0; i < isfound.size(); i++)
			{
				if (isfound[i])
				{
					for (int j = 0; j < meshexprlen; j++)
						outputcopy[index*blocklen+j] += interpolmesh[i*meshexprlen+j];
					index++;
				}
			}
		}
		
		std::vector<std::vector<double>> outputsplit = myalgorithm::splitvector(outputcopy, blocklen);
		outputsplit.resize(3+3);

		mathop::scatterwrite(filename, outputsplit[0], outputsplit[1], outputsplit[2], outputsplit[3], outputsplit[4], outputsplit[5]);
	}
	
	return output;
}

void expression::reuseit(bool istobereused)
{
    for (int i = 0; i < myoperations.size(); i++)
    {
        if (myoperations[i]->isdofincluded() == false && myoperations[i]->istfincluded() == false)
            myoperations[i]->reuseit(istobereused);
        else
        {
            std::cout << "Error in 'expression object': cannot reuse an expression including a dof() or tf()" << std::endl;
            abort();
        }
    }
}

bool expression::isharmonicone(std::vector<int> disjregs)
{
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (myoperations[i]->isharmonicone(disjregs) == false)
            return false;
    }
    return true;
}

bool expression::isvalueorientationdependent(std::vector<int> disjregs)
{
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (myoperations[i]->isvalueorientationdependent(disjregs))
            return true;
    }
    return false;
}

bool expression::iszero(void)
{
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (myoperations[i]->isconstant() == false || myoperations[i]->getvalue() != 0)
            return false;
    }
    return true;
}

vec expression::atbarycenter(int physreg, field onefield)
{
    // The field must be a "one" type:
	if (onefield.getpointer()->gettypename() != "one")
    {
        std::cout << "Error in 'expression' object: .atbarycenter requires a 'one' type field" << std::endl;
        abort();
    }
	if (countcolumns() != 1 || onefield.countcomponents() != countrows())
    {
        std::cout << "Error in 'expression' object: in .atbarycenter the size of the expression and of the argument field must match" << std::endl;
        abort();
    }
    
	// Compute the expression at the barycenter.
	// When there is a single Gauss point it is at the barycenter --> set integration order to 0. 
	// The default integration order is 1 (order of a 'one' field) + 2 :
	formulation formul;
	formul += integration(physreg, - mathop::transpose(mathop::tf(onefield))*(*this) / detjac(), -3);
	
	universe::skipgausspointweightproduct = true;
	formul.generate();
	universe::skipgausspointweightproduct = false;
	
	return formul.rhs();
}

vec expression::integrateonelements(int physreg, field onefield, int integrationorder)
{
    // The expression must be scalar and the field must be a scalar "one" type:
	if (onefield.getpointer()->gettypename() != "one" || onefield.countcomponents() != 1 || isscalar() == false)
    {
        std::cout << "Error in 'expression' object: .integrateonelements requires a scalar expression and a scalar 'one' type field" << std::endl;
        abort();
    }
    
	formulation formul;
	// The default integration order is 1 (order of a 'one' field) + 2 :
	formul += integration(physreg, - mathop::transpose(mathop::tf(onefield))*(*this), -3+integrationorder);
	formul.generate();
	
	return formul.rhs();
}

void expression::print(void)
{
    std::cout << std::endl;

    for (int row = 0; row < mynumrows; row++)
    {          
        for (int col = 0; col < mynumcols; col++)
        {      
            std::cout << " @ row " << row << ", col " << col << " :" << std::endl;
            myoperations[row*mynumcols+col]->print();
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

expression expression::at(int row, int col)
{
    expression arrayentry;

    if (mynumrows < row+1 || mynumcols < col+1)
    {
        std::cout << "Error in 'expression' object: trying to get entry (" << row << ", " << col << ") in a " << mynumrows << " by " << mynumcols << " expression array" << std::endl;
        abort();
    }
    arrayentry.myoperations = {myoperations[row*mynumcols+col]};

    return arrayentry;
}

std::vector<double> expression::evaluate(std::vector<double>& xcoords, std::vector<double>& ycoords, std::vector<double>& zcoords)
{
	if (isscalar())
	{
		if (xcoords.size() == ycoords.size() && xcoords.size() == zcoords.size())
		{
			if (xcoords.size() == 0)
				return std::vector<double>(0);
			else		
				return myoperations[0]->evaluate(xcoords, ycoords, zcoords);
		}
		else
		{
			std::cout << "Error in 'expression' object: expected vectors of same length as arguments in 'evaluate'" << std::endl;
			abort();
		}	
	}
	else
	{
		std::cout << "Error in 'expression' object: 'evaluate' can only be called on a scalar expression" << std::endl;
		abort();
	}
}


expression expression::spacederivative(int whichderivative)
{
	if (mynumrows > 3 || mynumcols != 1)
	{
		std::cout << "Error in 'expression' object: can only take space derivatives of column vectors with up to 3 components" << std::endl;
		abort();
	}

    int problemdimension = universe::mymesh->getmeshdimension();
    
    expression derivated = *this;
    
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (derivated.myoperations[i]->isconstant())
            derivated.myoperations[i] = std::shared_ptr<operation>(new opconstant(0));
        else
        {
			// In case the problem is axisymmetric the z derivatives of 'h1xyz' field u are all 0 
			// except for the z component uz for which it is 1/x*ux. dx and dy are unchanged.
			if (universe::isaxisymmetric && whichderivative == 3 && i == 2)
			{ 
					field x("x");
					derivated.myoperations[i] = (1/x*mathop::compx(*this)).myoperations[0]; 
					continue;
			}

		    if (whichderivative > problemdimension)
		    {
		        std::shared_ptr<opproduct> op(new opproduct( {derivated.myoperations[i], std::shared_ptr<operation>(new opconstant(0))} ));
		        derivated.myoperations[i] = op;
		    }
		    else
		    {
		        std::shared_ptr<operation> op = derivated.myoperations[i]->copy();
		        op->setspacederivative(whichderivative);
		        derivated.myoperations[i] = op;
		    }
	    }
    }
    
    return derivated;
}

expression expression::kietaphiderivative(int whichderivative)
{
    expression derivated = *this;
    
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        std::shared_ptr<operation> op = derivated.myoperations[i]->copy();
        op->setkietaphiderivative(whichderivative);
        derivated.myoperations[i] = op;
    }
    
    return derivated;
}

expression expression::timederivative(int derivativeorder)
{
    expression derivated = getunprojectedfield();
    
    for (int i = 0; i < mynumrows*mynumcols; i++)
    {
        if (derivated.myoperations[i]->isconstant())
            derivated.myoperations[i] = std::shared_ptr<operation>(new opconstant(0));
        else
        {
		    std::shared_ptr<operation> op = derivated.myoperations[i]->copy();
		    op->increasetimederivativeorder(derivativeorder);
		    derivated.myoperations[i] = op;
	    }
    }
    
    // Project from the reference element to the physical one a 1 form (hcurl):
    if (unprojectedfield.size() == 1)        
    {
        derivated.unprojectedfield = {derivated};
        // A 1 form E is projected as invjac*E:
        derivated.myoperations = ( invjac()*(derivated) ).myoperations;
    }
    return derivated;
}

expression expression::getunprojectedfield(void)
{
    if (unprojectedfield.size() == 0)
        return *this;
    else
        return unprojectedfield[0];
}

expression expression::transpose(void)
{
    expression transposed;

    // Inverted!
    transposed.mynumrows = mynumcols;
    transposed.mynumcols = mynumrows;
        
    transposed.myoperations.resize(mynumrows*mynumcols);
        
    for (int row = 0; row < transposed.mynumrows; row++)
    {          
        for (int col = 0; col < transposed.mynumcols; col++)
            transposed.myoperations[row*transposed.mynumcols+col] = myoperations[col*mynumcols+row];
    }
    return transposed;
}

expression expression::removerowandcol(int rowtoremove, int coltoremove)
{
    expression submatrix;
    submatrix.mynumrows = mynumrows-1;
    submatrix.mynumcols = mynumcols-1;
    submatrix.myoperations.resize((mynumrows-1)*(mynumcols-1));
    
    int subrow = 0;
    for (int row = 0; row < mynumrows; row++)
    {   
        if (row == rowtoremove) { continue; }
        int subcol = 0;
        for (int col = 0; col < mynumcols; col++)
        {
            if (col == coltoremove) { continue; }
            submatrix.myoperations[subrow*(mynumcols-1)+subcol] = myoperations[row*mynumcols+col];
            subcol++;
        }
        subrow++;
    }
    return submatrix;
}

expression expression::determinant(void)
{
    if (mynumrows == 1 && mynumcols == 1)
        return *this;
    
    if (mynumrows != mynumcols)
    {
        std::cout << "Error in 'expression' object: cannot get the determinant of a non-square matrix" << std::endl;
        abort();
    }
    
    std::shared_ptr<opsum> determ(new opsum());

    int row = 0;
    for (int col = 0; col < mynumcols; col++)
    {
        shared_ptr<operation> subdeterm = removerowandcol(row, col).determinant().myoperations[0];

        if ((row+col)%2 == 0)
            determ->addterm( std::shared_ptr<opproduct>(new opproduct( {myoperations[row*mynumcols+col], subdeterm} )) );
        else
            determ->addterm( std::shared_ptr<opproduct>(new opproduct( {std::shared_ptr<opconstant>(new opconstant(-1)), myoperations[row*mynumcols+col], subdeterm} )) );
    }
    
    expression output;
    output.myoperations = {determ};
    return output;
}

expression expression::cofactormatrix(void)
{
    expression cofactors = *this;

    for (int row = 0; row < mynumrows; row++)
    {          
        for (int col = 0; col < mynumcols; col++)
        {
            shared_ptr<operation> op = removerowandcol(row,col).determinant().myoperations[0];
            
            if ((row+col)%2 == 0)
                cofactors.myoperations[row*mynumcols+col] = op;
            else
                cofactors.myoperations[row*mynumcols+col] = std::shared_ptr<opproduct>(new opproduct( {std::shared_ptr<opconstant>(new opconstant(-1)), op}));
        }
    }
    return cofactors;
}

expression expression::invert(void)
{
    if (mynumrows == 1 && mynumcols == 1)
        return 1/(*this);
    
    if (mynumrows != mynumcols)
    {
        std::cout << "Error in 'expression' object: cannot invert a non-square matrix" << std::endl;
        abort();
    }
    
    expression invdet = 1/determinant();
    // The determinant inverse will be reused a lot:
    invdet.reuseit();
    
    return invdet * cofactormatrix().transpose();
}

expression expression::pow(expression input)
{
    expression powered = *this;

    // The base and exponent expressions must be scalar:
    if (not(isscalar()) || not(input.isscalar()))
    {
        std::cout << "Error in 'expression' object: cannot have non scalar arguments in a power" << std::endl; 
        abort();
    }

    std::shared_ptr<operation> myexponent = input.myoperations[0];
    std::shared_ptr<operation> mybase = myoperations[0];
    // In case the exponent is 0 or 1 we simplify.
    // Remember that doubles can exactly represent integers up to a large enough value.
    if (myexponent->isconstant() && (myexponent->getvalue() == 0 || myexponent->getvalue() == 1))
    {
        double val = myexponent->getvalue();
        // With a 0 power we get a constant. With a 1 power we get the base.
        if (val == 0)
            powered.myoperations = {std::shared_ptr<opconstant>(new opconstant(1))};
        else
            powered.myoperations = {mybase};
    }
    else
        powered.myoperations = {std::shared_ptr<oppower>(new oppower(mybase, myexponent))};
        
    return powered;
}

expression expression::dof(int physreg)
{
    expression doftag = getunprojectedfield();

    for (int i = 0; i < mynumrows*mynumcols; i++)
    {            
        if (doftag.myoperations[i]->isfield())
        {
            if (doftag.myoperations[i]->getspacederivative() != 0 || doftag.myoperations[i]->getkietaphiderivative() != 0 || doftag.myoperations[i]->gettimederivative() != 0)
            {
                std::cout << "Error in 'expression' object: cannot apply space or time derivatives to the dof() field argument" << std::endl; 
                abort();
            }   
            std::shared_ptr<opdof> op(new opdof(doftag.myoperations[i]->getfieldpointer(), physreg));
            op->selectformfunctioncomponent(doftag.myoperations[i]->getformfunctioncomponent());
            op->setfieldcomponent(doftag.myoperations[i]->getfieldcomponent());
            doftag.myoperations[i] = op;
        }
        else
        {
            std::cout << "Error in 'expression' object: the argument of dof() must be a field or field expression" << std::endl; 
            abort();
        }     
    }
    
    // Project from the reference element to the physical one a 1 form (hcurl):
    if (unprojectedfield.size() == 1)        
    {
        doftag.unprojectedfield = {doftag};
        // A 1 form E is projected as invjac*E:
        doftag.myoperations = ( invjac()*(doftag) ).myoperations;
    }
    return doftag;
}

expression expression::tf(int physreg)
{
    expression tftag = getunprojectedfield();

    for (int i = 0; i < mynumrows*mynumcols; i++)
    {            
        if (tftag.myoperations[i]->isfield())
        {
            if (tftag.myoperations[i]->getspacederivative() != 0 || tftag.myoperations[i]->getkietaphiderivative() != 0 || tftag.myoperations[i]->gettimederivative() != 0)
            {
                std::cout << "Error in 'expression' object: cannot apply space or time derivatives to the tf() field argument" << std::endl; 
                abort();
            }   
            std::shared_ptr<optf> op(new optf(tftag.myoperations[i]->getfieldpointer(), physreg));
            op->selectformfunctioncomponent(tftag.myoperations[i]->getformfunctioncomponent());
            op->setfieldcomponent(tftag.myoperations[i]->getfieldcomponent());
            tftag.myoperations[i] = op;
        }
        else
        {
            std::cout << "Error in 'expression' object: the argument of tf() must be a field or field expression" << std::endl; 
            abort();
        }     
    }
    
    // Project from the reference element to the physical one a 1 form (hcurl):
    if (unprojectedfield.size() == 1)        
    {
        tftag.unprojectedfield = {tftag};
        // A 1 form E is projected as invjac*E:
        tftag.myoperations = ( invjac()*(tftag) ).myoperations;
    }
    return tftag;
}

expression expression::sin(void)
{
    expression sinexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        sinexpr.myoperations[i] = std::shared_ptr<opsin>(new opsin(myoperations[i]));
    
    return sinexpr;
}

expression expression::cos(void)
{
    expression cosexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        cosexpr.myoperations[i] = std::shared_ptr<opcos>(new opcos(myoperations[i]));
    
    return cosexpr;
}

expression expression::tan(void)
{
    expression tanexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        tanexpr.myoperations[i] = std::shared_ptr<optan>(new optan(myoperations[i]));
    
    return tanexpr;
}

expression expression::asin(void)
{
    expression asinexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        asinexpr.myoperations[i] = std::shared_ptr<opasin>(new opasin(myoperations[i]));
    
    return asinexpr;
}

expression expression::acos(void)
{
    expression acosexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        acosexpr.myoperations[i] = std::shared_ptr<opacos>(new opacos(myoperations[i]));
    
    return acosexpr;
}

expression expression::atan(void)
{
    expression atanexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        atanexpr.myoperations[i] = std::shared_ptr<opatan>(new opatan(myoperations[i]));
    
    return atanexpr;
}

expression expression::abs(void)
{
    expression absexpr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        absexpr.myoperations[i] = std::shared_ptr<opabs>(new opabs(myoperations[i]));
    
    return absexpr;
}

expression expression::log10(void)
{
    expression log10expr = *this;

    for (int i = 0; i < mynumrows*mynumcols; i++)
        log10expr.myoperations[i] = std::shared_ptr<oplog10>(new oplog10(myoperations[i]));
    
    return log10expr;
}

expression expression::time(void)
{
    expression exp;
    exp.myoperations = {std::shared_ptr<optime>(new optime)};
    
    return exp;
}

expression expression::invjac(int row, int col)
{
    expression exp;
    exp.myoperations = {std::shared_ptr<opinvjac>(new opinvjac(row,col))};
    
    return exp;
}

expression expression::jac(int row, int col)
{
    expression exp;
    exp.myoperations = {std::shared_ptr<opjac>(new opjac(row,col))};
    
    return exp;
}

expression expression::detjac(void)
{
    expression exp;
    exp.myoperations = {std::shared_ptr<opdetjac>(new opdetjac)};
    
    return exp;
}

expression expression::invjac(void)
{
    int problemdimension = universe::mymesh->getmeshdimension();
    switch (problemdimension)
    {
        case 1:
            return expression(3,3,{invjac(0,0),0,0,   0,1,0,   0,0,1});
        case 2:
            return expression(3,3,{invjac(0,0),invjac(0,1),0,   invjac(1,0),invjac(1,1),0,   0,0,1});
        case 3:
            return expression(3,3,{invjac(0,0),invjac(0,1),invjac(0,2),   invjac(1,0),invjac(1,1),invjac(1,2),   invjac(2,0),invjac(2,1),invjac(2,2)});
    }
}

expression expression::jac(void)
{
    int problemdimension = universe::mymesh->getmeshdimension();
    switch (problemdimension)
    {
        case 1:
            return expression(3,3,{jac(0,0),0,0,   0,1,0,   0,0,1});
        case 2:
            return expression(3,3,{jac(0,0),jac(0,1),0,   jac(1,0),jac(1,1),0,   0,0,1});
        case 3:
            return expression(3,3,{jac(0,0),jac(0,1),jac(0,2),   jac(1,0),jac(1,1),jac(1,2),   jac(2,0),jac(2,1),jac(2,2)});
    }
}

std::shared_ptr<operation> expression::getoperationinarray(int row, int col)
{
    if (mynumrows < row+1 || mynumcols < col+1)
    {
        std::cout << "Error in 'expression' object: trying to get entry (" << row << ", " << col << ") in a " << mynumrows << " by " << mynumcols << " expression array" << std::endl;
        abort();
    }
    return myoperations[row*mynumcols+col];
}

void expression::expand(void)
{
    if (isscalar())
        myoperations[0] = myoperations[0]->expand();
    else
    {
        std::cout << "Error in 'expression' object: expand is only defined for scalar expressions" << std::endl;
        std::cout << "Did you try to define a non scalarisable formulation?" << std::endl;
        abort();
    }
}

std::vector< std::vector<std::vector<std::shared_ptr<operation>>> > expression::extractdoftfpolynomial(int elementdimension)
{
	// Simplify the operation:
	myoperations[0]->simplify({});

    // To make sure the format of the expression is ok.
    // Otherwise an error is thrown at the end.
    bool isformatok = true;

    // Every slice in coeffs, dofs and tfs corresponds to a unique 
    // dof field-tf field combination (with a unique combination 
    // of time derivative operator and form function component).
    std::vector<std::vector<std::shared_ptr<operation>>> coeffs = {}; 
    std::vector<std::vector<std::shared_ptr<operation>>> dofs = {};
    std::vector<std::vector<std::shared_ptr<operation>>> tfs = {};
    
    // Extract the sum terms from the operation:
    std::vector<std::shared_ptr<operation>> sumterms;
    if (myoperations[0]->issum())
        sumterms = myoperations[0]->getarguments();
    else
        sumterms = {myoperations[0]};

    // Loop on all elementary sum terms in the formulation: 
    for (int i = 0; i < sumterms.size(); i++)
    {            
        // Deal with the very specific case of a tf() term without 
        // coefficient by multiplying it by a constant 1 to get a product:
        if (sumterms[i]->istf())
        {
            shared_ptr<opproduct> op(new opproduct);
            op->multiplybyterm(sumterms[i]);
            op->multiplybyterm(shared_ptr<operation>(new opconstant(1)));
            sumterms[i] = op;
        }
        // In a valid formulation term sumterms[i] must now always be a product:
        if (sumterms[i]->isproduct())
        {
            std::shared_ptr<operation> currentdof(new opdof(NULL));
            std::shared_ptr<operation> currenttf(new optf(NULL));
            
            // The coef is what remains after dof() and tf() removal.
            // We thus remove the dof and tf terms.
            std::shared_ptr<operation> currentcoef = sumterms[i]->copy();
            
            std::vector<std::shared_ptr<operation>> productterms = sumterms[i]->getarguments();

            for (int j = productterms.size()-1; j >= 0; j--)
            {
                // Remove the dof and tf terms to get the coefficient:
                if (productterms[j]->isdof())
                {
                    currentdof = productterms[j];
                    currentcoef->removeterm(j);
                }
                if (productterms[j]->istf())
                {
                    currenttf = productterms[j];
                    currentcoef->removeterm(j);
                }
            }
            
            // A coef without factors actually has value 1:
            if (currentcoef->count() == 0)
                currentcoef = std::shared_ptr<opconstant>(new opconstant(1));
            
            // Do some error checking.
            // Make sure there is no dof or tf in the coef.
            if (currentcoef->isdofincluded() || currentcoef->istfincluded())
                isformatok = false;
            if (currenttf->getfieldpointer() == NULL)
            {
                std::cout << "Error in 'expression' object: malformed expression provided to the formulation (the test function is missing)" << std::endl;
                abort();
            }
            
            // Know which slice (i.e. dof field-tf field combination) we are at.
            // In a given slice all dofs and tfs must be defined on the same physical region!
            int currentslice = -1;
            for (int slice = 0; slice < tfs.size(); slice++)
            {
                // The pointed field, the physical region and the time derivative must be identical:
                if (dofs[slice][0]->getfieldpointer() == currentdof->getfieldpointer() && tfs[slice][0]->getfieldpointer() == currenttf->getfieldpointer() && dofs[slice][0]->getphysicalregion() == currentdof->getphysicalregion() && tfs[slice][0]->getphysicalregion() == currenttf->getphysicalregion() && dofs[slice][0]->gettimederivative() == currentdof->gettimederivative() && tfs[slice][0]->gettimederivative() == currenttf->gettimederivative())
                {
                    currentslice = slice;
                    break;
                }
            }
            // If the slice does not yet exist create it.
            if (currentslice == -1)
            {
                currentslice = tfs.size();
                
                coeffs.push_back({});
                dofs.push_back({});
                tfs.push_back({});
            }
            
            // Explode any x, y, z space derivatived dof and tf into a sum of 
            // products of invjac terms and ki, eta and phi derivatives.
            int numdofterms = 1, numtfterms = 1;
            if (currentdof->getspacederivative() > 0)
                numdofterms = elementdimension;
            if (currenttf->getspacederivative() > 0)
                numtfterms = elementdimension;
            
            std::vector<std::shared_ptr<operation>> currentdofsplit(numdofterms*numtfterms);
            std::vector<std::shared_ptr<operation>> currenttfsplit(numdofterms*numtfterms);
            std::vector<std::shared_ptr<operation>> currentcoefsplit(numdofterms*numtfterms);
            // Loop on every term in the product of the two sums:
            for (int i = 0; i < numdofterms; i++)
            {
                for (int j = 0; j < numtfterms; j++)
                {
                    std::shared_ptr<operation> copieddof = currentdof->copy();
                    std::shared_ptr<operation> copiedtf = currenttf->copy();
                    std::shared_ptr<operation> newcoef = currentcoef;
                    // Precompute the current coef when generating since it might appear several times:
                    currentcoef->reuseit(true);
                    // Multiply the coef by the appropriate invjac term:
                    if (currentdof->getspacederivative() > 0)
                    {
                        std::shared_ptr<opinvjac> invjacob(new opinvjac(currentdof->getspacederivative()-1, i));
                        newcoef = std::shared_ptr<operation>(new opproduct({newcoef, invjacob}));
                        copieddof->setkietaphiderivative(i+1);
                    }
                    if (currenttf->getspacederivative() > 0)
                    {
                        std::shared_ptr<opinvjac> invjacob(new opinvjac(currenttf->getspacederivative()-1, j));
                        newcoef = std::shared_ptr<operation>(new opproduct({newcoef, invjacob}));
                        copiedtf->setkietaphiderivative(j+1);
                    }
                    currentdofsplit[i*numtfterms+j] = copieddof;
                    currenttfsplit[i*numtfterms+j] = copiedtf;
                    currentcoefsplit[i*numtfterms+j] = newcoef;
                }
            }
            
            // Find the appropriate indexes in the current slice to put the split terms:
            for (int term = 0; term < currenttfsplit.size(); term++)
            {
                // In the current slice find the entry at which to 
                // add the current coef, tf and dof (if existing).
                int currententry = -1;
                for (int entry = 0; entry < tfs[currentslice].size(); entry++)
                {
                    if (dofs[currentslice][entry]->getformfunctioncomponent() == currentdofsplit[term]->getformfunctioncomponent() && dofs[currentslice][entry]->getkietaphiderivative() == currentdofsplit[term]->getkietaphiderivative() && tfs[currentslice][entry]->getformfunctioncomponent() == currenttfsplit[term]->getformfunctioncomponent() && tfs[currentslice][entry]->getkietaphiderivative() == currenttfsplit[term]->getkietaphiderivative())
                    {
                        currententry = entry;
                        // Add the current coefficient to the entry:
                        std::shared_ptr<opsum> op(new opsum);
                        op->addterm(coeffs[currentslice][currententry]);
                        op->addterm(currentcoefsplit[term]);
                        coeffs[currentslice][currententry] = op;
                        break;
                    }
                }
                // If the entry does not yet exist create it.
                if (currententry == -1)
                {
                    currententry = tfs[currentslice].size();

                    coeffs[currentslice].push_back(currentcoefsplit[term]);
                    dofs[currentslice].push_back(currentdofsplit[term]);
                    tfs[currentslice].push_back(currenttfsplit[term]);
                }
            }
        }
        else
            isformatok = false;
    }

    if (not(isformatok))
    {
        std::cout << "Error in 'expression' object: don't know what to do with the expression provided to the formulation" << std::endl;
        std::cout << "The expression should be rewritable into a sum of products of the form coef*dof*tf (derivatives allowed)" << std::endl;
        abort();
    }
    
    return {coeffs, dofs, tfs};
}


expression expression::operator+(void) { return *this; }
expression expression::operator-(void) { return (*this * -1.0); }

expression expression::operator+(expression input)
{
    expression output = *this;

    if (mynumrows != input.mynumrows || mynumcols != input.mynumcols)
    {
        std::cout << "Error in 'expression' object: trying to add a " << mynumrows << "x" << mynumcols << " expression to a " << input.mynumrows << "x" << input.mynumcols << std::endl;
        abort();
    }
    for (int i = 0; i < mynumrows*mynumcols; i++)
        output.myoperations[i] = shared_ptr<opsum>(new opsum( {myoperations[i], input.myoperations[i]} ));
    return output;
}

expression expression::operator-(expression input) { return (*this + input*-1); }

expression expression::operator*(expression input)
{
    expression output;

    // Products by scalars are always allowed:
    if (isscalar())
    {
        output = input;
        
        for (int i = 0; i < input.mynumrows*input.mynumcols; i++)
            output.myoperations[i] = std::shared_ptr<opproduct>(new opproduct( {myoperations[0], input.myoperations[i]} ));
        return output;
    }
    if (input.isscalar())
    {
        output = *this;

        for (int i = 0; i < mynumrows*mynumcols; i++)
            output.myoperations[i] = std::shared_ptr<opproduct>(new opproduct( {myoperations[i], input.myoperations[0]} ));
        return output;
    }

    // Scalar products of two row or two column vectors are allowed:
    if ( (mynumrows == 1 || mynumcols == 1) && mynumrows == input.mynumrows && mynumcols == input.mynumcols)
    {
        std::shared_ptr<opsum> op(new opsum);
        for (int i = 0; i < std::max(mynumrows,mynumcols); i++)
            op->addterm( std::shared_ptr<opproduct>(new opproduct( {myoperations[i], input.myoperations[i]} )) );
        output.myoperations = {op};
        return output;
    }

    // For all other products sizes must match.
    if (mynumcols != input.mynumrows)
    {
        std::cout << "Error in 'expression' object: trying to multiply a " << mynumrows << "x" << mynumcols << " expression by a " << input.mynumrows << "x" << input.mynumcols << std::endl;
        abort();
    }

    output.mynumrows = mynumrows;
    output.mynumcols = input.mynumcols;
    output.myoperations.resize(mynumrows*input.mynumcols);

    for (int row = 0; row < mynumrows; row++)
    {       
        for (int col = 0; col < input.mynumcols; col++)
        {        
            // Build every element in the output matrix:
            std::shared_ptr<opsum> op(new opsum);

            for (int k = 0; k < mynumcols; k++)
                op->addterm( std::shared_ptr<opproduct>(new opproduct( {myoperations[row*mynumcols+k], input.myoperations[k*input.mynumcols+col]} )) );

            output.myoperations[row*output.mynumcols+col] = op;
        }
    }
    return output;
}

expression expression::operator/(expression input)
{
    expression output;

    // Only divisions by scalars are allowed:
    if (input.isscalar())
    {
        output = *this;

        for (int i = 0; i < mynumrows*mynumcols; i++)
        {       
            std::shared_ptr<opinversion> opdiv(new opinversion(input.myoperations[0]));
            std::shared_ptr<opproduct> opprod(new opproduct({myoperations[i],opdiv}));
            output.myoperations[i] = opprod;
        }
        return output;
    }
    else
    {
        std::cout << "Error in 'expression' object: only divisions by scalars are allowed" << std::endl;
        abort();
    }
}

expression expression::operator+(field inputfield) { return *this + (expression)inputfield; }
expression expression::operator-(field inputfield) { return *this - (expression)inputfield; }
expression expression::operator*(field inputfield) { return *this * (expression)inputfield; }
expression expression::operator/(field inputfield) { return *this / (expression)inputfield; }

expression expression::operator+(double val) { return *this + (expression)val; }
expression expression::operator-(double val) { return *this - (expression)val; }
expression expression::operator*(double val) { return *this * (expression)val; }
expression expression::operator/(double val) { return *this / (expression)val; }

expression expression::operator+(parameter& param) { return *this + (expression)param; }
expression expression::operator-(parameter& param) { return *this - (expression)param; }
expression expression::operator*(parameter& param) { return *this * (expression)param; }
expression expression::operator/(parameter& param) { return *this / (expression)param; }    


expression operator+(double val, expression expr) { return expr+val; }
expression operator-(double val, expression expr) { return -expr+val; }
expression operator*(double val, expression expr) { return expr*val; }
expression operator/(double val, expression expr) { return ( (expression)val ) / expr; }

expression operator+(field inputfield, expression expr) { return expr+inputfield; }
expression operator-(field inputfield, expression expr) { return -expr+inputfield; }
expression operator*(field inputfield, expression expr) { return expr*inputfield; }
expression operator/(field inputfield, expression expr) { return ( (expression)inputfield ) / expr; }

expression operator+(parameter& param, expression expr) { return expr+param; }
expression operator-(parameter& param, expression expr) { return -expr+param; }
expression operator*(parameter& param, expression expr) { return expr*param; }
expression operator/(parameter& param, expression expr) { return ( (expression)param ) / expr; }




