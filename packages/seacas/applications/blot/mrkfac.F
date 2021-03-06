C Copyright(C) 2009-2017 National Technology & Engineering Solutions of
C Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C NTESS, the U.S. Government retains certain rights in this software.
C 
C Redistribution and use in source and binary forms, with or without
C modification, are permitted provided that the following conditions are
C met:
C 
C     * Redistributions of source code must retain the above copyright
C       notice, this list of conditions and the following disclaimer.
C 
C     * Redistributions in binary form must reproduce the above
C       copyright notice, this list of conditions and the following
C       disclaimer in the documentation and/or other materials provided
C       with the distribution.
C     * Neither the name of NTESS nor the names of its
C       contributors may be used to endorse or promote products derived
C       from this software without specific prior written permission.
C 
C THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

C $Log: mrkfac.F,v $
C Revision 1.4  2009/03/25 12:36:45  gdsjaar
C Add copyright and license notice to all files.
C Permission to assert copyright has been granted; blot is now open source, BSD
C
C Revision 1.3  2009/01/22 21:34:21  gdsjaar
C There were several inline dbnums common blocks. Replaced with the
C include so they all have the same size with the added variable types.
C
C Added minor support for nodeset and sideset variables.
C
C It can print the count and the names, but that is all
C at this time.
C
C Revision 1.2  1997/06/26 17:51:35  caforsy
C Substituted if NeedsDoubleEscape for ifdef sun.  This define is needed on cougar, sun and ibm platforms
C
C Revision 1.1  1994/04/07 20:05:09  gdsjaar
C Initial checkin of ACCESS/graphics/blotII2
C
CRevision 1.3  1993/07/08  21:57:14  gdsjaar
CReplaced SUN with sun in all ifdef statements.
C
CRevision 1.2  1990/12/14  08:53:34  gdsjaar
CAdded RCS Id and Log to all files
C
C=======================================================================
      SUBROUTINE MRKFAC (NUMFAC, F, HIDEF, XF, YF, ZF,
     &   NMIN, NMAX, FMIN, FMAX, BLKCOL, *)
C=======================================================================

C   --*** MRKFAC *** (DETOUR) Mark minimum/maximum face values
C   --   Written by Amy Gilkey - revised 03/09/88
C   --
C   --MRKFAC marks the faces which have the minimum or maximum values
C   --with symbols.
C   --
C   --Parameters:
C   --   NUMFAC - IN - the number of surface faces
C   --   F - IN - the face variable values
C   --   HIDEF(i) - IN - true iff face i is hidden (3D only)
C   --   XF, YF, ZF - IN - the face center coordinates
C   --   NMIN, NMAX - IN - the number of variables values matching the
C   --      minimum and the maximum
C   --   FMIN, FMAX - IN - the minimum and maximum function values
C   --   BLKCOL - IN - the user selected colors of the element blocks.
C   --                    BLKCOL(0) = 1 if the user defined material
C   --                                colors should be used in mesh plots.
C   --                              = -1 if program selected colors should
C   --                                be used.
C   --                    BLKCOL(i) = the user selected color of element
C   --                               block i:
C   --                                  -2 - no color selected by user.
C   --                                  -1 - black
C   --                                   0 - white
C   --                                   1 - red
C   --                                   2 - green
C   --                                   3 - yellow
C   --                                   4 - blue
C   --                                   5 - cyan
C   --                                   6 - magenta
C   --   * - return statement if the cancel function is active
C   --
C   --Common Variables:
C   --   Uses IS3DIM, NNPSUR of /D3NUMS/
C   --   Uses MAXMIN, MAXMAX of /CNTR/

      include 'dbnums.blk'
      include 'd3nums.blk'
      include 'cntr.blk'

      REAL F(*)
      LOGICAL HIDEF(*)
      REAL XF(*), YF(*), ZF(*)
      INTEGER BLKCOL(0:NELBLK)

      LOGICAL GRABRT
      LOGICAL MRKMIN, MRKMAX

      CALL UGRCOL (0, BLKCOL)

      MRKMIN = (MAXMIN .GE. NMIN)
      MRKMAX = (MAXMAX .GE. NMAX)

      IF ((FMIN .NE. FMAX) .AND. (MRKMIN .OR. MRKMAX)) THEN

C      --Mark minimum and maximum values with centered symbols

         DO 100 IFAC = 1, NUMFAC
            IF (IS3DIM) THEN
               IF (HIDEF(IFAC)) GOTO 100
            END IF

            IF (FMIN .EQ. F(IFAC)) THEN
               IF (MRKMIN) THEN
                  IF (GRABRT ()) RETURN 1
#if NeedsDoubleEscape
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\\CCI')
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\\CCS')
#else
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\CCI')
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\CCS')
#endif
               END IF
            ELSE IF (FMAX .EQ. F(IFAC)) THEN
               IF (MRKMAX) THEN
                  IF (GRABRT ()) RETURN 1
#if NeedsDoubleEscape
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\\CX')
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\\CCS')
#else
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\CX')
                  CALL MPD2SY (1, XF(IFAC), YF(IFAC), '\CCS')
#endif
               END IF
            END IF
  100    CONTINUE

         CALL PLTFLU
      END IF

      RETURN
      END
