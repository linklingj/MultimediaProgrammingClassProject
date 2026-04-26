#include "MatrixInverse.h"
#include <math.h>

bool InverseMatrixGJ8(float A[][8], float invA[][8])
{
	int i, j;
	int n_size = 8;
	float** n_m;
	float** n_sol;

	n_m = new float* [n_size + 1];
	for (i = 0; i < n_size + 1; i++)
		n_m[i] = new float[n_size + 1];

	n_sol = new float* [n_size + 1];
	for (i = 0; i < n_size + 1; i++)
		n_sol[i] = new float[2];

	for (i = 0; i <= n_size; i++)
		n_sol[i][1] = 10.0f;				// DUMMY: AnyValue is okay

	for (i = 0; i < n_size; i++)
		for (j = 0; j < n_size; j++)
			n_m[i + 1][j + 1] = A[i][j];

	if (!ImGaussj(n_m, n_size, n_sol, 1))
		return false;

	for (i = 0; i < n_size; i++)
		for (j = 0; j < n_size; j++)
			invA[i][j] = n_m[i + 1][j + 1];

	for (i = 0; i < n_size + 1; i++)
		delete[] n_m[i];
	delete[] n_m;
	for (i = 0; i < n_size + 1; i++)
		delete[] n_sol[i];
	delete[] n_sol;
	return true;
}

bool InverseMatrixGJ3(float A[][3], float invA[][3])
{
	int i,j;
	int n_size = 3;
	float ** n_m;
	float ** n_sol;

	n_m = new float * [n_size+1];
	for(i=0; i<n_size+1; i++)
		n_m[i] = new float [n_size+1];

	n_sol = new float * [n_size+1];
	for(i=0; i<n_size+1; i++)
		n_sol[i] = new float [2];

	for(i=0; i<=n_size; i++)
		n_sol[i][1] = 10.0f;				// DUMMY: AnyValue is okay

	for(i=0; i<n_size; i++)
		for(j=0; j<n_size; j++)
			n_m[i+1][j+1] = A[i][j];

	if(!ImGaussj(n_m, n_size, n_sol, 1))
		return false;

	for(i=0; i<n_size; i++)
		for(j=0; j<n_size; j++)
			invA[i][j] = n_m[i+1][j+1];

	for(i=0; i<n_size+1; i++)
		delete [] n_m[i];
	delete [] n_m;
	for(i=0; i<n_size+1; i++)
		delete [] n_sol[i];
	delete [] n_sol;
	return true;
}


bool ImGaussj(float **a, int n, float **b, int m)
{
	int *indxc,*indxr,*ipiv;
	int i,icol,irow,j,k,l,ll;
	float big,dum,pivinv;

	indxc = new int [n+1];
	indxr = new int [n+1];
	ipiv = new int [n+1]; 

	for (j=1;j<=n;j++) ipiv[j]=0;
	for (i=1;i<=n;i++) 
	{
		big=0.0;
		for (j=1;j<=n;j++)
			if (ipiv[j] != 1)
				for (k=1;k<=n;k++) 
				{
					if (ipiv[k] == 0) 
					{
						if (fabs(double(a[j][k])) >= big) 
						{
							big=float(fabs(double(a[j][k])));
							irow=j;
							icol=k;
						}
					} 
					else if (ipiv[k] > 1){
//						AfxMessageBox("GAUSSJ: Singular Matrix-1");
						return false;
					};
				}
		++(ipiv[icol]);
		if (irow != icol) 
		{
			float temp;
			for (l=1;l<=n;l++) 
			{
				temp = a[irow][l];
				a[irow][l] = a[icol][l];
				a[icol][l] = temp;
			}
			for (l=1;l<=m;l++)
			{
				temp = b[irow][l];
				b[irow][l] = b[icol][l];
				b[icol][l] = temp;
//				SWAP(b[irow][l],b[icol][l])
			}
		}
		indxr[i]=irow;
		indxc[i]=icol;
		if (a[icol][icol] == 0.0) {
//			AfxMessageBox("GAUSSJ: Singular Matrix-2");
			return false;
		}
		pivinv=1.0f/a[icol][icol];
		a[icol][icol]=1.0;
		for (l=1;l<=n;l++) a[icol][l] *= pivinv;
		for (l=1;l<=m;l++) b[icol][l] *= pivinv;
		for (ll=1;ll<=n;ll++)
			if (ll != icol) 
			{
				dum=a[ll][icol];
				a[ll][icol]=0.0;
				for (l=1;l<=n;l++) a[ll][l] -= a[icol][l]*dum;
				for (l=1;l<=m;l++) b[ll][l] -= b[icol][l]*dum;
			}
	}
	for (l=n;l>=1;l--) 
	{
		if (indxr[l] != indxc[l])
		{
			float temp;
			for (k=1;k<=n;k++)
			{
//				SWAP(a[k][indxr[l]],a[k][indxc[l]]);k
				temp = a[k][indxr[l]];
				a[k][indxr[l]] = a[k][indxc[l]];
				a[k][indxc[l]] = temp;
			}
		}
	}
	
	delete [] ipiv;
	delete [] indxr;
	delete [] indxc;
	return true;
}