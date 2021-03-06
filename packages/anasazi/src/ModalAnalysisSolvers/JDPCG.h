// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
//                 Copyright 2004 Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

// This software is a result of the research described in the report
//
//     "A comparison of algorithms for modal analysis in the absence
//     of a sparse direct method", P. Arbenz, R. Lehoucq, and U. Hetmaniuk,
//     Sandia National Laboratories, Technical report SAND2003-1028J.
//
// It is based on the Epetra, AztecOO, and ML packages defined in the Trilinos
// framework ( http://trilinos.org/ ).

#ifndef JACOBI_DAVIDSON_PCG_H
#define JACOBI_DAVIDSON_PCG_H

#include "Epetra_ConfigDefs.h"

#include "Epetra_BLAS.h"
#include "Epetra_Comm.h"
#include "Epetra_LAPACK.h"
#include "Epetra_Operator.h"
#include "Epetra_Time.h"

#include "CheckingTools.h"
#include "FortranRoutines.h"
#include "ModalAnalysisSolver.h"
#include "MyMemory.h"
#include "ModalTools.h"
#include "SortingTools.h"

class JDPCG : public ModalAnalysisSolver {

  private:

    const CheckingTools myVerify;
    const Epetra_BLAS callBLAS;
    const FortranRoutines callFortran;
    const Epetra_LAPACK callLAPACK;
    ModalTools modalTool;
    const SortingTools mySort;

    const Epetra_Comm &MyComm;
    const Epetra_Operator *K;
    const Epetra_Operator *M;
    const Epetra_Operator *Prec;
    const Epetra_Time MyWatch;

    double tolEigenSolve;
    int maxIterEigenSolve;

    int maxIterLinearSolve;

    int blockSize;
    int numBlock;

    double *normWeight;

    int verbose;

    int historyCount;
    double *resHistory;

    int maxSpaceSize;
    int sumSpaceSize;
    int *spaceSizeHistory;

    int maxIterPCG;
    int sumIterPCG;
    int *iterPCGHistory;

    double memRequested;
    double highMem;

    int massOp;
    int numCorrectionPrec;
    int numCorrectionSolve;
    int numPCGmassOp;
    int numPCGstifOp;
    int numRestart;
    int outerIter;
    int precOp;
    int residual;
    int stifOp;

    double timeBuildQtMPMQ;
    double timeCorrectionPrec;
    double timeCorrectionSolve; 
    double timeLocalProj;
    double timeLocalSolve;
    double timeLocalUpdate;
    double timeMassOp;
    double timeNorm;
    double timeOrtho;
    double timeOuterLoop;
    double timePCGEigCheck;
    double timePCGLoop;
    double timePCGOpMult;
    double timePCGPrec;
    double timePostProce;
    double timePrecOp;
    double timeResidual;
    double timeRestart;
    double timeStifOp;

    // Private functions
    void accuracyCheck(const Epetra_MultiVector *X, const Epetra_MultiVector *MX,
                       const Epetra_MultiVector *Q) const;

    // Don't define these functions
    JDPCG(const JDPCG &ref);
    JDPCG& operator=(const JDPCG &ref);

  protected:

    int jacobiPreconditioner(const Epetra_MultiVector &B, Epetra_MultiVector &PrecB,
                       const Epetra_MultiVector *U, const Epetra_MultiVector *Q,
                       double *invQtMPMQ, int ldQtMPMQ, double *PMQ, double *work, double *WS);
    int jacobiPCG(Epetra_MultiVector &Rlin, Epetra_MultiVector &Y,
                  const Epetra_MultiVector *U, const Epetra_MultiVector *Q,
                  double eta, double tolCG, int iterMax,
                  double *invQtMPMQ, int ldQtMPMQ, double *PMQ,
                  double *work, double *workSpace,
                  const Epetra_Vector *vectWeight, const Epetra_MultiVector *orthoVec = 0);

  public:

    JDPCG(const Epetra_Comm &_Comm, const Epetra_Operator *KK,
              const Epetra_Operator *MM, const Epetra_Operator *PP, int _blk, int _numBlk,
              double _tol = 1.0e-08, int _maxIterES = 100, int _maxIterLS = 100, int _verb = 0,
              double *_weight = 0);

    ~JDPCG();

    int solve(int numEigen, Epetra_MultiVector &Q, double *lambda);

    int reSolve(int numEigen, Epetra_MultiVector &Q, double *lambda, int startingEV = 0);

    int minimumSpaceDimension(int nev) const;

    void initializeCounters();

    void algorithmInfo() const;
    void historyInfo() const;
    void memoryInfo() const;
    void operationInfo() const;
    void timeInfo() const;

};

#endif
