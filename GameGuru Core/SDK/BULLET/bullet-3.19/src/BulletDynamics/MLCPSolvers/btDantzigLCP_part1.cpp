#endif  // btLCP_FAST

//***************************************************************************
// an optimized Dantzig LCP driver routine for the lo-hi LCP problem.

bool btSolveDantzigLCP(int n, btScalar *A, btScalar *x, btScalar *b,
					   btScalar *outer_w, int nub, btScalar *lo, btScalar *hi, int *findex, btDantzigScratchMemory &scratchMem)
{
	s_error = false;

	//	printf("btSolveDantzigLCP n=%d\n",n);
	btAssert(n > 0 && A && x && b && lo && hi && nub >= 0 && nub <= n);
	btAssert(outer_w);

#ifdef BT_DEBUG
	{
		// check restrictions on lo and hi
		for (int k = 0; k < n; ++k)
			btAssert(lo[k] <= 0 && hi[k] >= 0);
	}
#endif

	// if all the variables are unbounded then we can just factor, solve,
	// and return
	if (nub >= n)
	{
		int nskip = (n);
		btFactorLDLT(A, outer_w, n, nskip);
		btSolveLDLT(A, outer_w, b, n, nskip);
		memcpy(x, b, n * sizeof(btScalar));

		return !s_error;
	}

	const int nskip = (n);
	scratchMem.L.resize(n * nskip);

	scratchMem.d.resize(n);

	btScalar *w = outer_w;
	scratchMem.delta_w.resize(n);
	scratchMem.delta_x.resize(n);
	scratchMem.Dell.resize(n);
	scratchMem.ell.resize(n);
	scratchMem.Arows.resize(n);
	scratchMem.p.resize(n);
	scratchMem.C.resize(n);

	// for i in N, state[i] is 0 if x(i)==lo(i) or 1 if x(i)==hi(i)
	scratchMem.state.resize(n);

	// create LCP object. note that tmp is set to delta_w to save space, this
	// optimization relies on knowledge of how tmp is used, so be careful!
	btLCP lcp(n, nskip, nub, A, x, b, w, lo, hi, &scratchMem.L[0], &scratchMem.d[0], &scratchMem.Dell[0], &scratchMem.ell[0], &scratchMem.delta_w[0], &scratchMem.state[0], findex, &scratchMem.p[0], &scratchMem.C[0], &scratchMem.Arows[0]);
	int adj_nub = lcp.getNub();

	// loop over all indexes adj_nub..n-1. for index i, if x(i),w(i) satisfy the
	// LCP conditions then i is added to the appropriate index set. otherwise
	// x(i),w(i) is driven either +ve or -ve to force it to the valid region.
	// as we drive x(i), x(C) is also adjusted to keep w(C) at zero.
	// while driving x(i) we maintain the LCP conditions on the other variables
	// 0..i-1. we do this by watching out for other x(i),w(i) values going
	// outside the valid region, and then switching them between index sets
	// when that happens.

	bool hit_first_friction_index = false;
	for (int i = adj_nub; i < n; ++i)
	{
		s_error = false;
		// the index i is the driving index and indexes i+1..n-1 are "dont care",
		// i.e. when we make changes to the system those x's will be zero and we
		// don't care what happens to those w's. in other words, we only consider
		// an (i+1)*(i+1) sub-problem of A*x=b+w.

		// if we've hit the first friction index, we have to compute the lo and
		// hi values based on the values of x already computed. we have been
		// permuting the indexes, so the values stored in the findex vector are
		// no longer valid. thus we have to temporarily unpermute the x vector.
		// for the purposes of this computation, 0*infinity = 0 ... so if the
		// contact constraint's normal force is 0, there should be no tangential
		// force applied.

		if (!hit_first_friction_index && findex && findex[i] >= 0)
		{
			// un-permute x into delta_w, which is not being used at the moment
			for (int j = 0; j < n; ++j) scratchMem.delta_w[scratchMem.p[j]] = x[j];

			// set lo and hi values
			for (int k = i; k < n; ++k)
			{
				btScalar wfk = scratchMem.delta_w[findex[k]];
				if (wfk == 0)
				{
					hi[k] = 0;
					lo[k] = 0;
				}
				else
				{
					hi[k] = btFabs(hi[k] * wfk);
					lo[k] = -hi[k];
				}
			}
			hit_first_friction_index = true;
		}

		// thus far we have not even been computing the w values for indexes
		// greater than i, so compute w[i] now.
		w[i] = lcp.AiC_times_qC(i, x) + lcp.AiN_times_qN(i, x) - b[i];

		// if lo=hi=0 (which can happen for tangential friction when normals are
		// 0) then the index will be assigned to set N with some state. however,
		// set C's line has zero size, so the index will always remain in set N.
		// with the "normal" switching logic, if w changed sign then the index
		// would have to switch to set C and then back to set N with an inverted
		// state. this is pointless, and also computationally expensive. to
		// prevent this from happening, we use the rule that indexes with lo=hi=0
		// will never be checked for set changes. this means that the state for
		// these indexes may be incorrect, but that doesn't matter.

		// see if x(i),w(i) is in a valid region
		if (lo[i] == 0 && w[i] >= 0)
		{
			lcp.transfer_i_to_N(i);
			scratchMem.state[i] = false;
		}
		else if (hi[i] == 0 && w[i] <= 0)
		{
			lcp.transfer_i_to_N(i);
			scratchMem.state[i] = true;
		}
		else if (w[i] == 0)
		{
			// this is a degenerate case. by the time we get to this test we know
			// that lo != 0, which means that lo < 0 as lo is not allowed to be +ve,
			// and similarly that hi > 0. this means that the line segment
			// corresponding to set C is at least finite in extent, and we are on it.
			// NOTE: we must call lcp.solve1() before lcp.transfer_i_to_C()
			lcp.solve1(&scratchMem.delta_x[0], i, 0, 1);

			lcp.transfer_i_to_C(i);
		}
		else
		{
			// we must push x(i) and w(i)
			for (;;)
			{
				int dir;
				btScalar dirf;
				// find direction to push on x(i)
				if (w[i] <= 0)
				{
					dir = 1;
					dirf = btScalar(1.0);
				}
				else
				{
					dir = -1;
					dirf = btScalar(-1.0);
				}

				// compute: delta_x(C) = -dir*A(C,C)\A(C,i)
				lcp.solve1(&scratchMem.delta_x[0], i, dir);

				// note that delta_x[i] = dirf, but we wont bother to set it

				// compute: delta_w = A*delta_x ... note we only care about
				// delta_w(N) and delta_w(i), the rest is ignored
				lcp.pN_equals_ANC_times_qC(&scratchMem.delta_w[0], &scratchMem.delta_x[0]);
				lcp.pN_plusequals_ANi(&scratchMem.delta_w[0], i, dir);
				scratchMem.delta_w[i] = lcp.AiC_times_qC(i, &scratchMem.delta_x[0]) + lcp.Aii(i) * dirf;

				// find largest step we can take (size=s), either to drive x(i),w(i)
				// to the valid LCP region or to drive an already-valid variable
				// outside the valid region.

				int cmd = 1;  // index switching command
				int si = 0;   // si = index to switch if cmd>3
				btScalar s = -w[i] / scratchMem.delta_w[i];
				if (dir > 0)
				{
					if (hi[i] < BT_INFINITY)
					{
						btScalar s2 = (hi[i] - x[i]) * dirf;  // was (hi[i]-x[i])/dirf	// step to x(i)=hi(i)
						if (s2 < s)
						{
							s = s2;
							cmd = 3;
						}
					}
				}
				else
				{
					if (lo[i] > -BT_INFINITY)
					{
						btScalar s2 = (lo[i] - x[i]) * dirf;  // was (lo[i]-x[i])/dirf	// step to x(i)=lo(i)
						if (s2 < s)
						{
							s = s2;
							cmd = 2;
						}
					}
				}

				{
					const int numN = lcp.numN();
					for (int k = 0; k < numN; ++k)
					{
						const int indexN_k = lcp.indexN(k);
						if (!scratchMem.state[indexN_k] ? scratchMem.delta_w[indexN_k] < 0 : scratchMem.delta_w[indexN_k] > 0)
						{
							// don't bother checking if lo=hi=0
							if (lo[indexN_k] == 0 && hi[indexN_k] == 0) continue;
							btScalar s2 = -w[indexN_k] / scratchMem.delta_w[indexN_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 4;
								si = indexN_k;
							}
						}
					}
				}

				{
					const int numC = lcp.numC();
					for (int k = adj_nub; k < numC; ++k)
					{
						const int indexC_k = lcp.indexC(k);
						if (scratchMem.delta_x[indexC_k] < 0 && lo[indexC_k] > -BT_INFINITY)
						{
							btScalar s2 = (lo[indexC_k] - x[indexC_k]) / scratchMem.delta_x[indexC_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 5;
								si = indexC_k;
							}
						}
						if (scratchMem.delta_x[indexC_k] > 0 && hi[indexC_k] < BT_INFINITY)
						{
							btScalar s2 = (hi[indexC_k] - x[indexC_k]) / scratchMem.delta_x[indexC_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 6;
								si = indexC_k;
							}
						}
					}
				}

				//static char* cmdstring[8] = {0,"->C","->NL","->NH","N->C",
				//			     "C->NL","C->NH"};
				//printf ("cmd=%d (%s), si=%d\n",cmd,cmdstring[cmd],(cmd>3) ? si : i);

				// if s <= 0 then we've got a problem. if we just keep going then
				// we're going to get stuck in an infinite loop. instead, just cross
				// our fingers and exit with the current solution.
				if (s <= btScalar(0.0))
				{
					//          printf("LCP internal error, s <= 0 (s=%.4e)",(double)s);
					if (i < n)
					{
						btSetZero(x + i, n - i);
						btSetZero(w + i, n - i);
					}
					s_error = true;
					break;
				}

				// apply x = x + s * delta_x
				lcp.pC_plusequals_s_times_qC(x, s, &scratchMem.delta_x[0]);
				x[i] += s * dirf;

				// apply w = w + s * delta_w
				lcp.pN_plusequals_s_times_qN(w, s, &scratchMem.delta_w[0]);
				w[i] += s * scratchMem.delta_w[i];

				//        void *tmpbuf;
				// switch indexes between sets if necessary
				switch (cmd)
				{
					case 1:  // done
						w[i] = 0;
						lcp.transfer_i_to_C(i);
						break;
					case 2:  // done
						x[i] = lo[i];
						scratchMem.state[i] = false;
						lcp.transfer_i_to_N(i);
						break;
					case 3:  // done
						x[i] = hi[i];
						scratchMem.state[i] = true;
						lcp.transfer_i_to_N(i);
						break;
					case 4:  // keep going
						w[si] = 0;
						lcp.transfer_i_from_N_to_C(si);
						break;
					case 5:  // keep going
						x[si] = lo[si];
						scratchMem.state[si] = false;
						lcp.transfer_i_from_C_to_N(si, scratchMem.m_scratch);
						break;
					case 6:  // keep going
						x[si] = hi[si];
						scratchMem.state[si] = true;
						lcp.transfer_i_from_C_to_N(si, scratchMem.m_scratch);
						break;
				}

				if (cmd <= 3) break;
			}  // for (;;)
		}      // else

		if (s_error)
		{
			break;
		}
	}  // for (int i=adj_nub; i<n; ++i)

	lcp.unpermute();

	return !s_error;
}
