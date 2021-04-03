/*
$ make
$ rm -rf ./obj-intel64/sparse_matrix.exe; g++ -g -Wall sparse_matrix.cpp -o ./obj-intel64/sparse_matrix.exe; ./obj-intel64/sparse_matrix.exe
$ valgrind --tool=memcheck --leak-check=full -s ./obj-intel64/sparse_matrix.exe
*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string.h>
#include <sys/time.h>
#include <cassert>
#include<pthread.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////

/* --- primary CSparse routines and data structures --- */
typedef struct cs_sparse    /* matrix in compressed-column or triplet form */
{
    int nzmax ;     /* maximum number of entries */
    int m ;         /* number of rows */
    int n ;         /* number of columns */
    int *p ;        /* column pointers (size n+1) or col indices (size nzmax) */
    int *i ;        /* row indices, size nzmax */
    double *x ;          /* numerical values, size nzmax */
    int nz ;        /* # of entries in triplet matrix, -1 for compressed-col */
} cs ;

////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
////////////////////////////////////////////////////////////////////////////

int main(int, char *[]);
void *child(void *);
void print_matrix(double **, int, int);
void print_sparse_matrix(cs *, int);
void trans2SparseMatrix(double **, int, int, cs *);
void multiplyMatrix(double **, int *, int *, double **, int *, int *, double **, int *, int *);
void multiplySparseMatrix(cs *, cs *, cs *);
double wtime();

////////////////////////////////////////////////////////////////////////////
// GLOBALS
////////////////////////////////////////////////////////////////////////////

pthread_mutex_t mutex;

////////////////////////////////////////////////////////////////////////////
// INPLEMENTATIONS
////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

    pthread_attr_t attr;
    int nthreads = (argc==2) ? atoi(argv[1]) : 4;
    pthread_t* thread = new pthread_t[nthreads];
    int r;
    r = pthread_mutex_init(&mutex, 0);
    assert(r==0);
    r = pthread_attr_init(&attr);
    assert(r==0);
    for(int i=0;i<nthreads;i++) {
        r = pthread_create( thread+i,
                            &attr,
                            child,
                            argv );
        assert(r==0);
    }

    string str = "";
    stringstream ss;
    ifstream fileA, fileB;

    double my_default_matrix[4][4] = {{1, 2, 0, 0},
                                      {7, 0, 0, 4},
                                      {0, 0, 0, 1},
                                      {0, 9, 8, 1}};
    int Ar=4, Ac=4, Br=4, Bc=4;

    fileA.open("matrixA.txt", ios::in);
    fileB.open("matrixB.txt", ios::in);

    if(fileA.is_open()) {
        getline(fileA, str);
        ss.clear(); ss << str; ss >> Ar; ss >> Ac;
    }

    if(fileB.is_open()) {
        getline(fileB, str);
        ss.clear(); ss << str; ss >> Br; ss >> Bc;
    }

    double matrixA[Ar][Ac], matrixB[Br][Bc];
    double *p_matrixA[Ar], *p_matrixB[Br];
    int Ai=0, Bi=0;

    if(fileA.is_open()) {
        while(!fileA.eof()) {
            getline(fileA, str); 
            ss.clear(); ss << str;
            for(int i=0; i<Ac; i++) ss >> matrixA[Ai][i];
            Ai++;
        }
        for(int i=0; i<Ar; i++) p_matrixA[i] = &matrixA[i][0];
    }
    else {
        for(int i=0; i<Ar; i++) p_matrixA[i] = &my_default_matrix[i][0];
    }

    if(fileB.is_open()) {
        while(!fileB.eof()) {
            getline(fileB, str); 
            ss.clear(); ss << str;
            for(int i=0; i<Ac; i++) ss >> matrixB[Bi][i];
            Bi++;
        }
        for(int i=0; i<Br; i++) p_matrixB[i] = &matrixB[i][0];
    }
    else {
        for(int i=0; i<Br; i++) p_matrixB[i] = &my_default_matrix[i][0];
    }

    // print_matrix(p_matrixA, Ar, Ac);
    // print_matrix(p_matrixB, Br, Bc);

    fileA.close();
    fileB.close();

    cs sparse_matrixA, sparse_matrixB, sparse_matrixC;
    int Cr=Ar, Cc=Bc;
    double ** p_matrix_c = new double *[Cr];
    trans2SparseMatrix(p_matrixA, Ar, Ac, &sparse_matrixA);
    trans2SparseMatrix(p_matrixB, Br, Bc, &sparse_matrixB);

    r = pthread_mutex_lock(&mutex);
    assert(r==0);

    double t0, t1, t2;
    t0 = wtime();
    multiplyMatrix(p_matrixA, &Ar, &Ac, p_matrixB, &Br, &Bc, p_matrix_c, &Cr, &Cc);
    t1 = wtime();
    multiplySparseMatrix(&sparse_matrixA, &sparse_matrixB, &sparse_matrixC);
    t2 = wtime();
    multiplyMatrix(p_matrixA, &Ar, &Ac, p_matrixB, &Br, &Bc, p_matrix_c, &Cr, &Cc);
    multiplySparseMatrix(&sparse_matrixA, &sparse_matrixB, &sparse_matrixC);

    // print_sparse_matrix(&sparse_matrixC, 0);
    // print_matrix(p_matrix_c, Cr, Cc);

    cout << "###############################################" << endl;
    cout << "Mother" << endl;
    cout << "###############################################" << endl;
    cout << "A(" << Ar << "x" << Ac << ") multiply by B(" << Br << "x" << Bc << "): " << endl;
    cout << "multiplyMatrix:       " << 1e6 * (t1 - t0) << "ms" << endl;
    cout << "multiplySparseMatrix: " << 1e6 * (t2 - t1) << "ms" << endl;
    cout << "###############################################" << endl;

    delete [] sparse_matrixA.p;
    sparse_matrixA.p = NULL;
    delete [] sparse_matrixA.i;
    sparse_matrixA.i = NULL;
    delete [] sparse_matrixA.x;
    sparse_matrixA.x = NULL;
    delete [] sparse_matrixB.p;
    sparse_matrixB.p = NULL;
    delete [] sparse_matrixB.i;
    sparse_matrixB.i = NULL;
    delete [] sparse_matrixB.x;
    sparse_matrixB.x = NULL;
    delete [] sparse_matrixC.p;
    sparse_matrixC.p = NULL;
    delete [] sparse_matrixC.i;
    sparse_matrixC.i = NULL;
    delete [] sparse_matrixC.x;
    sparse_matrixC.x = NULL;
    for(int i=0; i<Cc; i++) {
        delete [] *(p_matrix_c+i);
        *(p_matrix_c+i) = NULL;
    }
    delete [] p_matrix_c;
    p_matrix_c = NULL;

    r = pthread_mutex_unlock(&mutex);
    assert(r==0);

    for(int i=0;i<nthreads;i++) {
        r = pthread_join(thread[i], 0);
        assert(r==0);
    }

    return 0;
}

void *child(void *arg) {
    int r;
    r = pthread_mutex_lock(&mutex);
    assert(r==0);

    string str = "";
    stringstream ss;
    ifstream fileA, fileB;

    double my_default_matrix[4][4] = {{1, 2, 0, 0},
                                      {7, 0, 0, 4},
                                      {0, 0, 0, 1},
                                      {0, 9, 8, 1}};
    int Ar=4, Ac=4, Br=4, Bc=4;

    fileA.open("matrixA.txt", ios::in);
    fileB.open("matrixB.txt", ios::in);

    if(fileA.is_open()) {
        getline(fileA, str);
        ss.clear(); ss << str; ss >> Ar; ss >> Ac;
    }

    if(fileB.is_open()) {
        getline(fileB, str);
        ss.clear(); ss << str; ss >> Br; ss >> Bc;
    }

    double matrixA[Ar][Ac], matrixB[Br][Bc];
    double *p_matrixA[Ar], *p_matrixB[Br];
    int Ai=0, Bi=0;

    if(fileA.is_open()) {
        while(!fileA.eof()) {
            getline(fileA, str); 
            ss.clear(); ss << str;
            for(int i=0; i<Ac; i++) ss >> matrixA[Ai][i];
            Ai++;
        }
        for(int i=0; i<Ar; i++) p_matrixA[i] = &matrixA[i][0];
    }
    else {
        for(int i=0; i<Ar; i++) p_matrixA[i] = &my_default_matrix[i][0];
    }

    if(fileB.is_open()) {
        while(!fileB.eof()) {
            getline(fileB, str); 
            ss.clear(); ss << str;
            for(int i=0; i<Ac; i++) ss >> matrixB[Bi][i];
            Bi++;
        }
        for(int i=0; i<Br; i++) p_matrixB[i] = &matrixB[i][0];
    }
    else {
        for(int i=0; i<Br; i++) p_matrixB[i] = &my_default_matrix[i][0];
    }

    // print_matrix(p_matrixA, Ar, Ac);
    // print_matrix(p_matrixB, Br, Bc);

    fileA.close();
    fileB.close();

    cs sparse_matrixA, sparse_matrixB, sparse_matrixC;
    int Cr=Ar, Cc=Bc;
    double ** p_matrix_c = new double *[Cr];
    trans2SparseMatrix(p_matrixA, Ar, Ac, &sparse_matrixA);
    trans2SparseMatrix(p_matrixB, Br, Bc, &sparse_matrixB);

    double t0, t1, t2;
    t0 = wtime();
    multiplyMatrix(p_matrixA, &Ar, &Ac, p_matrixB, &Br, &Bc, p_matrix_c, &Cr, &Cc);
    t1 = wtime();
    multiplySparseMatrix(&sparse_matrixA, &sparse_matrixB, &sparse_matrixC);
    t2 = wtime();

    // print_sparse_matrix(&sparse_matrixC, 0);
    // print_matrix(p_matrix_c, Cr, Cc);


    cout << "###############################################" << endl;
    cout << "Child" << endl;
    cout << "###############################################" << endl;
    cout << "A(" << Ar << "x" << Ac << ") multiply by B(" << Br << "x" << Bc << "): " << endl;
    cout << "multiplyMatrix:       " << 1e6 * (t1 - t0) << "ms" << endl;
    cout << "multiplySparseMatrix: " << 1e6 * (t2 - t1) << "ms" << endl;
    cout << "###############################################" << endl;

    delete [] sparse_matrixA.p;
    sparse_matrixA.p = NULL;
    delete [] sparse_matrixA.i;
    sparse_matrixA.i = NULL;
    delete [] sparse_matrixA.x;
    sparse_matrixA.x = NULL;
    delete [] sparse_matrixB.p;
    sparse_matrixB.p = NULL;
    delete [] sparse_matrixB.i;
    sparse_matrixB.i = NULL;
    delete [] sparse_matrixB.x;
    sparse_matrixB.x = NULL;
    delete [] sparse_matrixC.p;
    sparse_matrixC.p = NULL;
    delete [] sparse_matrixC.i;
    sparse_matrixC.i = NULL;
    delete [] sparse_matrixC.x;
    sparse_matrixC.x = NULL;
    for(int i=0; i<Cc; i++) {
        delete [] *(p_matrix_c+i);
        *(p_matrix_c+i) = NULL;
    }
    delete [] p_matrix_c;
    p_matrix_c = NULL;

    r =pthread_mutex_unlock(&mutex);
    assert(r==0);
    pthread_exit(NULL);
}

void print_matrix(double **Xi_matrix, int row, int col) {
    cout << "###############################################" << endl;
    cout << "            Print the Normal Matrix            " << endl;
    cout << "###############################################" << endl;
    for(int i=0; i<row; i++) {
        for(int j=0 ;j<col; j++) {
            printf("%5.1f ", *(*(Xi_matrix+i)+j));
        }
        cout << endl;
    }
    cout << "###############################################" << endl;
}

void print_sparse_matrix(cs *Xi_sparseMatrix, int setup = 0) {
    cout << "###############################################" << endl;
    cout << "            Print the Sparse Matrix            " << endl;
    cout << "###############################################" << endl;
    if(setup) {
        printf("Xi_sparseMatrix->nzmax: %3d\n", Xi_sparseMatrix->nzmax);
        printf("Xi_sparseMatrix->m:     %3d\n", Xi_sparseMatrix->m);
        printf("Xi_sparseMatrix->n:     %3d\n", Xi_sparseMatrix->n);
        printf("Xi_sparseMatrix->p:     ");
        for(int i=0; i<Xi_sparseMatrix->m+1; i++) printf("%3d ", *(Xi_sparseMatrix->p+i));
        printf("\n");
        printf("Xi_sparseMatrix->i:     ");
        for(int i=0; i<Xi_sparseMatrix->nzmax; i++) printf("%3d ", *(Xi_sparseMatrix->i+i));
        printf("\n");
        printf("Xi_sparseMatrix->x:     ");
        for(int i=0; i<Xi_sparseMatrix->nzmax; i++) printf("%3.0f ", *(Xi_sparseMatrix->x+i));
        printf("\n");
        printf("Xi_sparseMatrix->nz:    %3d\n", Xi_sparseMatrix->nz);
        cout << "###############################################" << endl;
        return;
    }
    for(int i=0; i<Xi_sparseMatrix->m; i++) {
        for(int j=*(Xi_sparseMatrix->p+i); j<*(Xi_sparseMatrix->p+i+1); j++) {
            printf("M(%d,%d)=%3.0f ", i, *(Xi_sparseMatrix->i+j), *(Xi_sparseMatrix->x+j));
        }
        printf("\n");
    }
    cout << "###############################################" << endl;
}

void trans2SparseMatrix(double **Xi_matrix, int row, int col, cs *Xo_sparseMatrix) {
    Xo_sparseMatrix->nzmax = 0;
    Xo_sparseMatrix->m = row;
    Xo_sparseMatrix->n = col;
    Xo_sparseMatrix->nz = -1;
    int *l_p = new int[row+1];
    int *l_i = new int[row*col];
    double *l_x = new double[row*col];

    // print_matrix(Xi_matrix, row, col);
    *(l_p) = Xo_sparseMatrix->nzmax;
    for(int i=0; i<row; i++) {
        for(int j=0 ;j<col; j++) {
            if(Xi_matrix[i][j] != 0) {
                *(l_i + Xo_sparseMatrix->nzmax) = j;
                *(l_x + Xo_sparseMatrix->nzmax) = Xi_matrix[i][j];
                Xo_sparseMatrix->nzmax += 1;
            }
            *(l_p+i+1) = Xo_sparseMatrix->nzmax;
        }
    }
    // print_matrix(Xi_matrix, row, col);
    Xo_sparseMatrix->p = l_p;
    Xo_sparseMatrix->i = l_i;
    Xo_sparseMatrix->x = l_x;
    // print_sparse_matrix(Xo_sparseMatrix, 1);
}

void multiplyMatrix(double **Xi_MatrixA, int *Xi_Arow, int *Xi_Acol, 
                    double **Xi_MatrixB, int *Xi_Brow, int *Xi_Bcol, 
                    double **Xo_MatrixC, int *Xo_Crow, int *Xo_Ccol) {
    if(*Xi_Acol != *Xi_Brow) exit (-1);
    *Xo_Crow = *Xi_Arow;
    *Xo_Ccol = *Xi_Bcol;
    // double *l_Matrix[*Xi_Arow] = new (double *)[(*Xi_Bcol)];
    // double *l_Matrix = new double(*Xi_Bcol);
    // double *l_Matrix = new (double)(*Xi_Bcol);

    for(int i=0; i<(*Xi_Arow); i++) {
        *(Xo_MatrixC+i) = new double[(*Xi_Bcol)];
        memset(*(Xo_MatrixC+i), 0, sizeof(double) * (*Xi_Bcol));
        for(int j=0; j<(*Xi_Acol); j++) {
            for(int k=0; k<(*Xi_Bcol); k++) {
                *(*(Xo_MatrixC+i)+j) += *(*(Xi_MatrixA+i)+k) * *(*(Xi_MatrixB+k)+j);
                // *(*(Xo_MatrixC+i)+j) += Xi_MatrixA[i][k] * Xi_MatrixB[k][j];
            }
        }
    }
    // print_matrix(Xo_MatrixC, *Xo_Crow, *Xo_Ccol);
    return;
}

void multiplySparseMatrix(cs *Xi_sparseMatrixA, cs *Xi_sparseMatrixB, cs *Xo_sparseMatrixC) {
    if(Xi_sparseMatrixA->n != Xi_sparseMatrixB->m) exit (-1);

    Xo_sparseMatrixC->nzmax = 0;
    Xo_sparseMatrixC->m = Xi_sparseMatrixA->m;
    Xo_sparseMatrixC->n = Xi_sparseMatrixB->n;
    Xo_sparseMatrixC->p = new int[Xo_sparseMatrixC->m+1];
    Xo_sparseMatrixC->i = new int[Xo_sparseMatrixC->m * Xo_sparseMatrixC->n];
    Xo_sparseMatrixC->x = new double[Xo_sparseMatrixC->m * Xo_sparseMatrixC->n];
    Xo_sparseMatrixC->nz = int(-1);
    memset(Xo_sparseMatrixC->p, int(-1), sizeof(int) * (Xo_sparseMatrixC->m + 1));
    memset(Xo_sparseMatrixC->i, int(-1), sizeof(int) * Xo_sparseMatrixC->m * Xo_sparseMatrixC->n);
    memset(Xo_sparseMatrixC->x, double(0), sizeof(double) * Xo_sparseMatrixC->m * Xo_sparseMatrixC->n);
    // print_sparse_matrix(Xo_sparseMatrixC, 1);

    int *Ap = Xi_sparseMatrixA->p;
    int *Ai = Xi_sparseMatrixA->i;
    double *Ax = Xi_sparseMatrixA->x;

    int *Bp = Xi_sparseMatrixB->p;
    int *Bi = Xi_sparseMatrixB->i;
    double *Bx = Xi_sparseMatrixB->x;

    int *Cnzmax = &(Xo_sparseMatrixC->nzmax);
    int Cm = Xo_sparseMatrixC->m;
    int Cn = Xo_sparseMatrixC->n;
    int *Cp = Xo_sparseMatrixC->p;
    int *Ci = Xo_sparseMatrixC->i;
    double *Cx = Xo_sparseMatrixC->x;
    double temp_Cx[Cn]; 

    *(Cp) = *Cnzmax;
    for(int i=0; i<Cm; i++) {
        memset(temp_Cx, 0, sizeof(double)* Cn);
        for (int j=*(Ap+i); j<*(Ap+i+1); j++) {
            for(int k=*(Bp+*(Ai+j)); k<*(Bp+*(Ai+j)+1); k++) {
                temp_Cx[*(Bi+k)] += *(Ax+j) * *(Bx+k);
            }
        }
        for(int x=0; x<Cn; x++) {
            if(temp_Cx[x] != 0) {
                *(Ci+(*Cnzmax)) = x;
                *(Cx+(*Cnzmax)) = temp_Cx[x];
                (*Cnzmax)++;
            }
            // cout << temp_Cx[x] << " ";
        }
        *(Cp+i+1) = *Cnzmax;
        // cout << endl;
    }
    // print_sparse_matrix(Xo_sparseMatrixC, 0);
}

double wtime()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + 1e-6 * tv.tv_usec;
}