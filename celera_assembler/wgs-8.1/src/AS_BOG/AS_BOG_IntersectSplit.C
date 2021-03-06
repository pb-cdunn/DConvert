
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 1999-2004, The Venter Institute. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received (LICENSE.txt) a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *************************************************************************/

static const char *rcsid = "$Id: AS_BOG_IntersectSplit.C 4371 2013-08-01 17:19:47Z brianwalenz $";

#include "AS_BOG_Datatypes.H"
#include "AS_BOG_UnitigGraph.H"
#include "AS_BOG_BestOverlapGraph.H"

#include "MultiAlignStore.H"



void UnitigGraph::breakUnitigs(ContainerMap &cMap, char *output_prefix, bool enableIntersectionBreaking) {
  FILE *breakFile = NULL;

  {
    char name[FILENAME_MAX];
    sprintf(name, "%s.breaks.ovl", output_prefix);

    errno = 0;
    breakFile = fopen(name, "w");
    if (errno) {
      fprintf(logFile, "Failed to open '%s' to write unitig breaking overlaps: %s\n", name, strerror(errno));
      fprintf(logFile, "Will not write unitig breaking overlaps.\n");
    }
  }

  fprintf(logFile, "==> BREAKING UNITIGS.\n");

  //  Stop when we've seen all current unitigs.  Replace tiMax
  //  in the for loop below with unitigs.size() to recursively
  //  split unitigs.
  int  tiMax = unitigs.size();

  for (int  ti=0; ti<tiMax; ti++) {
    Unitig             *tig = unitigs[ti];

    if (tig == NULL)
      continue;

    UnitigBreakPoints   breaks;
    ufNode        lastBackbone;

    uint32   numFragsInUnitig = 0;
    uint32   fragCount        = 0;
    uint32   fragIdx;

    //  Enabling this sort should do nothing -- there are no contained fragments placed yet, and any
    //  bubbles that have been merged in have already had their unitigs sorted.
    //
    //  HOWEVER, unless the sort function (in AS_BOG_Unitig.cc) is enabled to preserve the ordering
    //  of THE ORIGINAL dovetail fragments, enabling the sort actually breaks unitigging.  We assume
    //  that fragments that intersect other unitigs are either the first or last fragment.
    //
    tig->sort();

    //  Count the number of fragments in this unitig, including
    //  yet-to-be-placed contained fragments.
    for (fragIdx=0; fragIdx<tig->ufpath.size(); fragIdx++) {
      ufNode  *f = &tig->ufpath[fragIdx];

      if (cMap.find(f->ident) != cMap.end())
        numFragsInUnitig += cMap[f->ident].size();

      numFragsInUnitig++;

      //  Contained fragments should not be placed yet.
      assert(f->contained == 0);
    }


    for (fragIdx=0; fragIdx<tig->ufpath.size(); fragIdx++) {
      ufNode  *f = &tig->ufpath[fragIdx];

      fragCount++;
      if (cMap.find(f->ident) != cMap.end())
        fragCount += cMap[f->ident].size();

      if (logFileFlagSet(LOG_INTERSECTION_BREAKING)) {
        if (fragIdx == 0) {
          uint32             dt3p  = (isReverse(f->position) == true);
          BestEdgeOverlap   *bEdge = OG->getBestEdgeOverlap(f->ident, dt3p);

          if ((bEdge) && (bEdge->fragId() > 0))
            fprintf(logFile,"unitig %d %c' frag %d points to unitig %d frag %d\n",
                    tig->id(), dt3p ? '3' : '5', f->ident,
                    Unitig::fragIn(bEdge->fragId()), bEdge->fragId());
        }

        if (fragIdx + 1 == tig->ufpath.size()) {
          uint32             dt3p  = (isReverse(f->position) == false);
          BestEdgeOverlap   *bEdge = OG->getBestEdgeOverlap(f->ident, dt3p);

          if ((bEdge) && (bEdge->fragId() > 0))
            fprintf(logFile,"unitig %d %c' frag %d points to unitig %d frag %d\n",
                    tig->id(), dt3p ? '3' : '5', f->ident,
                    Unitig::fragIn(bEdge->fragId()), bEdge->fragId());
        }
      }

      FragmentEdgeList::const_iterator edge_itr = unitigIntersect.find(f->ident);

      if (edge_itr == unitigIntersect.end())
        continue;

      // We have a set of best edges incoming from other unitigs
      for (FragmentList::const_iterator fragItr = edge_itr->second.begin();
           fragItr != edge_itr->second.end();
           fragItr++) {
        uint32 inFrag = *fragItr;

        BestEdgeOverlap  *best5 = OG->getBestEdgeOverlap(inFrag, false);
        BestEdgeOverlap  *best3 = OG->getBestEdgeOverlap(inFrag, true);

        // check if it's incoming frag's 5' best edge.  If not, it must be the 3' edge.

        bool              best3p;
        BestEdgeOverlap  *bestEdge;

        if        (best5->fragId() == f->ident) {
          best3p   = false;
          bestEdge = best5;
        } else if (best3->fragId() == f->ident) {
          best3p   = true;
          bestEdge = best3;
        } else {
          assert(0);
          continue;
        }

        int pos = (bestEdge->frag3p() == false) ? f->position.bgn : f->position.end;

        Unitig *inTig = unitigs[Unitig::fragIn(inFrag)];
        assert(inTig->id() == Unitig::fragIn(inFrag));

        //  Don't break on spur fragments!  These will only chop off the ends of unitigs anyway.
        if ((inTig->ufpath.size() == 1) &&
            ((best5->fragId() == 0) || (best3->fragId() == 0))) {
          if (logFileFlagSet(LOG_INTERSECTION_BREAKING))
            fprintf(logFile, "unitig %d (%d frags, len %d) frag %d end %c' into unitig %d frag %d end %c' pos %d -- IS A SPUR, skip it\n",
                    Unitig::fragIn(inFrag),
                    inTig->getNumFrags(),
                    inTig->getLength(),
                    inFrag,
                    best3p ? '3' : '5',
                    tig->id(),
                    f->ident,
                    bestEdge->frag3p() ? '3' : '5',
                    pos);
          continue;
        }

        //  If the incoming fragment is in this unitig (this unitig == 'tig'), AND it isn't listed
        //  as being a true self intersection, don't use it.  This fragment was placed here by short
        //  unitig merging, and is no longer an intersection.
        if (Unitig::fragIn(inFrag) == tig->id()) {
          if (selfIntersect.find(inFrag) != selfIntersect.end()) {
            if (logFileFlagSet(LOG_INTERSECTION_BREAKING))
              fprintf(logFile, "unitig %d frag %d - TRUE self intersection from frag %d\n",
                      tig->id(), f->ident, inFrag);
          } else {
            if (logFileFlagSet(LOG_INTERSECTION_BREAKING))
              fprintf(logFile, "unitig %d frag %d - skipping false self intersection from frag %d\n",
                      tig->id(), f->ident, inFrag);
            continue;
          }
        }

        UnitigBreakPoint breakPoint(f->ident, bestEdge->frag3p());

        breakPoint.fragPos     = f->position;
        breakPoint.fragsBefore = fragCount;
        breakPoint.fragsAfter  = numFragsInUnitig - fragCount;
        breakPoint.inSize      = inTig->getLength();
        breakPoint.inFrags     = inTig->getNumFrags();

        breaks.push_back(breakPoint);

        if (logFileFlagSet(LOG_INTERSECTION_BREAKING))
          fprintf(logFile, "unitig %d (%d frags, len %d) frag %d end %c' into unitig %d frag %d end %c' pos %d\n",
                  Unitig::fragIn(inFrag),
                  inTig->getNumFrags(),
                  inTig->getLength(),
                  inFrag,
                  best3p ? '3' : '5',
                  tig->id(),
                  f->ident,
                  bestEdge->frag3p() ? '3' : '5',
                  pos);

        if (breakFile) {
          GenericMesg  pmesg;
          OverlapMesg  omesg;

          omesg.aifrag          = inFrag;
          omesg.bifrag          = f->ident;
          omesg.ahg             = bestEdge->ahang();
          omesg.bhg             = bestEdge->bhang();
          omesg.orientation.setIsUnknown();
          omesg.overlap_type    = AS_DOVETAIL;
          omesg.quality         = 0.0;
          omesg.min_offset      = 0;
          omesg.max_offset      = 0;
          omesg.polymorph_ct    = 0;
          omesg.alignment_trace = NULL;
#ifdef AS_MSG_USE_OVL_DELTA
          omesg.alignment_delta = NULL;
#endif

          if ((best3p == false) && (bestEdge->frag3p() == false))
            omesg.orientation.setIsOuttie();
          if ((best3p == false) && (bestEdge->frag3p() == true))
            omesg.orientation.setIsAnti();
          if ((best3p == true) && (bestEdge->frag3p() == false))
            omesg.orientation.setIsNormal();
          if ((best3p == true) && (bestEdge->frag3p() == true))
            omesg.orientation.setIsInnie();

          pmesg.t = MESG_OVL;
          pmesg.m = &omesg;

          WriteProtoMesg_AS(breakFile, &pmesg);
        }
      }  //  Over all fragments that intersect with us
    }  //  Over all fragments in the unitig

    if (breaks.empty() == false) {
      ufNode  *f = &tig->ufpath.back();

      //  create a final fake bp for the last frag so we
      //  have a reference point to the end of the tig for
      //  filtering the breakpoints.  fakeEnd, for searching.
      //
      UnitigBreakPoint breakPoint(f->ident, (isReverse(f->position) == false));

      //  +1 below seems like a bug, but it reproduces what was here
      //  before.
#warning possible bug

      breakPoint.fragPos     = f->position;
      breakPoint.fragsBefore = fragCount + 1;
      breakPoint.fragsAfter  = 0;
      breakPoint.inSize      = std::numeric_limits<int>::max();
      breakPoint.inFrags     = 0;

      breaks.push_back(breakPoint);

      filterBreakPoints(cMap, tig, breaks);

      //  Report where breaks occur.  'breaks' is a list, not a vector.
      if (logFileFlagSet(LOG_INTERSECTION_BREAKING) ||
          logFileFlagSet(LOG_MATE_SPLIT_COVERAGE_PLOT))
        for (uint32 i=0; i<breaks.size(); i++)
          fprintf(logFile, "BREAK unitig %d at position %d,%d from inSize %d inFrags %d.\n",
                  tig->id(),
                  breaks.front().fragPos.bgn,
                  breaks.front().fragPos.end,
                  breaks.front().inSize,
                  breaks.front().inFrags);

      //  Actually do the breaking.  Do NOT break if we are plotting the coverage.
      if (enableIntersectionBreaking) {
        UnitigVector* newUs = breakUnitigAt(tig, breaks);

        if (newUs != NULL) {
          delete tig;
          unitigs[ti] = NULL;
          unitigs.insert(unitigs.end(), newUs->begin(), newUs->end());
        }

        delete newUs;
      }

      breaks.clear();
    }
  }  //  Over all tigs

  if (breakFile)
    fclose(breakFile);
}
