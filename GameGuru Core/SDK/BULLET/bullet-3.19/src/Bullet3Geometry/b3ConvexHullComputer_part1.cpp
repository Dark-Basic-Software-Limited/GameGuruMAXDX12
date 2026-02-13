void b3ConvexHullInternal::compute(const void* coords, bool doubleCoords, int stride, int count)
{
	b3Vector3 min = b3MakeVector3(b3Scalar(1e30), b3Scalar(1e30), b3Scalar(1e30)), max = b3MakeVector3(b3Scalar(-1e30), b3Scalar(-1e30), b3Scalar(-1e30));
	const char* ptr = (const char*)coords;
	if (doubleCoords)
	{
		for (int i = 0; i < count; i++)
		{
			const double* v = (const double*)ptr;
			b3Vector3 p = b3MakeVector3((b3Scalar)v[0], (b3Scalar)v[1], (b3Scalar)v[2]);
			ptr += stride;
			min.setMin(p);
			max.setMax(p);
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			const float* v = (const float*)ptr;
			b3Vector3 p = b3MakeVector3(v[0], v[1], v[2]);
			ptr += stride;
			min.setMin(p);
			max.setMax(p);
		}
	}

	b3Vector3 s = max - min;
	maxAxis = s.maxAxis();
	minAxis = s.minAxis();
	if (minAxis == maxAxis)
	{
		minAxis = (maxAxis + 1) % 3;
	}
	medAxis = 3 - maxAxis - minAxis;

	s /= b3Scalar(10216);
	if (((medAxis + 1) % 3) != maxAxis)
	{
		s *= -1;
	}
	scaling = s;

	if (s[0] != 0)
	{
		s[0] = b3Scalar(1) / s[0];
	}
	if (s[1] != 0)
	{
		s[1] = b3Scalar(1) / s[1];
	}
	if (s[2] != 0)
	{
		s[2] = b3Scalar(1) / s[2];
	}

	center = (min + max) * b3Scalar(0.5);

	b3AlignedObjectArray<Point32> points;
	points.resize(count);
	ptr = (const char*)coords;
	if (doubleCoords)
	{
		for (int i = 0; i < count; i++)
		{
			const double* v = (const double*)ptr;
			b3Vector3 p = b3MakeVector3((b3Scalar)v[0], (b3Scalar)v[1], (b3Scalar)v[2]);
			ptr += stride;
			p = (p - center) * s;
			points[i].x = (btInt32_t)p[medAxis];
			points[i].y = (btInt32_t)p[maxAxis];
			points[i].z = (btInt32_t)p[minAxis];
			points[i].index = i;
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			const float* v = (const float*)ptr;
			b3Vector3 p = b3MakeVector3(v[0], v[1], v[2]);
			ptr += stride;
			p = (p - center) * s;
			points[i].x = (btInt32_t)p[medAxis];
			points[i].y = (btInt32_t)p[maxAxis];
			points[i].z = (btInt32_t)p[minAxis];
			points[i].index = i;
		}
	}
	points.quickSort(b3PointCmp);

	vertexPool.reset();
	vertexPool.setArraySize(count);
	originalVertices.resize(count);
	for (int i = 0; i < count; i++)
	{
		Vertex* v = vertexPool.newObject();
		v->edges = NULL;
		v->point = points[i];
		v->copy = -1;
		originalVertices[i] = v;
	}

	points.clear();

	edgePool.reset();
	edgePool.setArraySize(6 * count);

	usedEdgePairs = 0;
	maxUsedEdgePairs = 0;

	mergeStamp = -3;

	IntermediateHull hull;
	computeInternal(0, count, hull);
	vertexList = hull.minXy;
#ifdef DEBUG_CONVEX_HULL
	b3Printf("max. edges %d (3v = %d)", maxUsedEdgePairs, 3 * count);
#endif
}

b3Vector3 b3ConvexHullInternal::toBtVector(const Point32& v)
{
	b3Vector3 p;
	p[medAxis] = b3Scalar(v.x);
	p[maxAxis] = b3Scalar(v.y);
	p[minAxis] = b3Scalar(v.z);
	return p * scaling;
}

b3Vector3 b3ConvexHullInternal::getBtNormal(Face* face)
{
	return toBtVector(face->dir0).cross(toBtVector(face->dir1)).normalized();
}

b3Vector3 b3ConvexHullInternal::getCoordinates(const Vertex* v)
{
	b3Vector3 p;
	p[medAxis] = v->xvalue();
	p[maxAxis] = v->yvalue();
	p[minAxis] = v->zvalue();
	return p * scaling + center;
}

b3Scalar b3ConvexHullInternal::shrink(b3Scalar amount, b3Scalar clampAmount)
{
	if (!vertexList)
	{
		return 0;
	}
	int stamp = --mergeStamp;
	b3AlignedObjectArray<Vertex*> stack;
	vertexList->copy = stamp;
	stack.push_back(vertexList);
	b3AlignedObjectArray<Face*> faces;

	Point32 ref = vertexList->point;
	Int128 hullCenterX(0, 0);
	Int128 hullCenterY(0, 0);
	Int128 hullCenterZ(0, 0);
	Int128 volume(0, 0);

	while (stack.size() > 0)
	{
		Vertex* v = stack[stack.size() - 1];
		stack.pop_back();
		Edge* e = v->edges;
		if (e)
		{
			do
			{
				if (e->target->copy != stamp)
				{
					e->target->copy = stamp;
					stack.push_back(e->target);
				}
				if (e->copy != stamp)
				{
					Face* face = facePool.newObject();
					face->init(e->target, e->reverse->prev->target, v);
					faces.push_back(face);
					Edge* f = e;

					Vertex* a = NULL;
					Vertex* b = NULL;
					do
					{
						if (a && b)
						{
							btInt64_t vol = (v->point - ref).dot((a->point - ref).cross(b->point - ref));
							b3Assert(vol >= 0);
							Point32 c = v->point + a->point + b->point + ref;
							hullCenterX += vol * c.x;
							hullCenterY += vol * c.y;
							hullCenterZ += vol * c.z;
							volume += vol;
						}

						b3Assert(f->copy != stamp);
						f->copy = stamp;
						f->face = face;

						a = b;
						b = f->target;

						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != v->edges);
		}
	}

	if (volume.getSign() <= 0)
	{
		return 0;
	}

	b3Vector3 hullCenter;
	hullCenter[medAxis] = hullCenterX.toScalar();
	hullCenter[maxAxis] = hullCenterY.toScalar();
	hullCenter[minAxis] = hullCenterZ.toScalar();
	hullCenter /= 4 * volume.toScalar();
	hullCenter *= scaling;

	int faceCount = faces.size();

	if (clampAmount > 0)
	{
		b3Scalar minDist = B3_INFINITY;
		for (int i = 0; i < faceCount; i++)
		{
			b3Vector3 normal = getBtNormal(faces[i]);
			b3Scalar dist = normal.dot(toBtVector(faces[i]->origin) - hullCenter);
			if (dist < minDist)
			{
				minDist = dist;
			}
		}

		if (minDist <= 0)
		{
			return 0;
		}

		amount = b3Min(amount, minDist * clampAmount);
	}

	unsigned int seed = 243703;
	for (int i = 0; i < faceCount; i++, seed = 1664525 * seed + 1013904223)
	{
		b3Swap(faces[i], faces[seed % faceCount]);
	}

	for (int i = 0; i < faceCount; i++)
	{
		if (!shiftFace(faces[i], amount, stack))
		{
			return -amount;
		}
	}

	return amount;
}

bool b3ConvexHullInternal::shiftFace(Face* face, b3Scalar amount, b3AlignedObjectArray<Vertex*> stack)
{
	b3Vector3 origShift = getBtNormal(face) * -amount;
	if (scaling[0] != 0)
	{
		origShift[0] /= scaling[0];
	}
	if (scaling[1] != 0)
	{
		origShift[1] /= scaling[1];
	}
	if (scaling[2] != 0)
	{
		origShift[2] /= scaling[2];
	}
	Point32 shift((btInt32_t)origShift[medAxis], (btInt32_t)origShift[maxAxis], (btInt32_t)origShift[minAxis]);
	if (shift.isZero())
	{
		return true;
	}
	Point64 normal = face->getNormal();
#ifdef DEBUG_CONVEX_HULL
	b3Printf("\nShrinking face (%d %d %d) (%d %d %d) (%d %d %d) by (%d %d %d)\n",
			 face->origin.x, face->origin.y, face->origin.z, face->dir0.x, face->dir0.y, face->dir0.z, face->dir1.x, face->dir1.y, face->dir1.z, shift.x, shift.y, shift.z);
#endif
	btInt64_t origDot = face->origin.dot(normal);
	Point32 shiftedOrigin = face->origin + shift;
	btInt64_t shiftedDot = shiftedOrigin.dot(normal);
	b3Assert(shiftedDot <= origDot);
	if (shiftedDot >= origDot)
	{
		return false;
	}

	Edge* intersection = NULL;

	Edge* startEdge = face->nearbyVertex->edges;
#ifdef DEBUG_CONVEX_HULL
	b3Printf("Start edge is ");
	startEdge->print();
	b3Printf(", normal is (%lld %lld %lld), shifted dot is %lld\n", normal.x, normal.y, normal.z, shiftedDot);
#endif
	Rational128 optDot = face->nearbyVertex->dot(normal);
	int cmp = optDot.compare(shiftedDot);
#ifdef SHOW_ITERATIONS
	int n = 0;
#endif
	if (cmp >= 0)
	{
		Edge* e = startEdge;
		do
		{
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			b3Assert(dot.compare(origDot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			b3Printf("Moving downwards, edge is ");
			e->print();
			b3Printf(", dot is %f (%f %lld)\n", (float)dot.toScalar(), (float)optDot.toScalar(), shiftedDot);
#endif
			if (dot.compare(optDot) < 0)
			{
				int c = dot.compare(shiftedDot);
				optDot = dot;
				e = e->reverse;
				startEdge = e;
				if (c < 0)
				{
					intersection = e;
					break;
				}
				cmp = c;
			}
			e = e->prev;
		} while (e != startEdge);

		if (!intersection)
		{
			return false;
		}
	}
	else
	{
		Edge* e = startEdge;
		do
		{
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			b3Assert(dot.compare(origDot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			b3Printf("Moving upwards, edge is ");
			e->print();
			b3Printf(", dot is %f (%f %lld)\n", (float)dot.toScalar(), (float)optDot.toScalar(), shiftedDot);
#endif
			if (dot.compare(optDot) > 0)
			{
				cmp = dot.compare(shiftedDot);
				if (cmp >= 0)
				{
					intersection = e;
					break;
				}
				optDot = dot;
				e = e->reverse;
				startEdge = e;
			}
			e = e->prev;
		} while (e != startEdge);

		if (!intersection)
		{
			return true;
		}
	}

#ifdef SHOW_ITERATIONS
	b3Printf("Needed %d iterations to find initial intersection\n", n);
#endif

	if (cmp == 0)
	{
		Edge* e = intersection->reverse->next;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (e->target->dot(normal).compare(shiftedDot) <= 0)
		{
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->next;
			if (e == intersection->reverse)
			{
				return true;
			}
#ifdef DEBUG_CONVEX_HULL
			b3Printf("Checking for outwards edge, current edge is ");
			e->print();
			b3Printf("\n");
#endif
		}
#ifdef SHOW_ITERATIONS
		b3Printf("Needed %d iterations to check for complete containment\n", n);
#endif
	}

	Edge* firstIntersection = NULL;
	Edge* faceEdge = NULL;
	Edge* firstFaceEdge = NULL;

#ifdef SHOW_ITERATIONS
	int m = 0;
#endif
	while (true)
	{
#ifdef SHOW_ITERATIONS
		m++;
#endif
#ifdef DEBUG_CONVEX_HULL
		b3Printf("Intersecting edge is ");
		intersection->print();
		b3Printf("\n");
#endif
		if (cmp == 0)
		{
			Edge* e = intersection->reverse->next;
			startEdge = e;
#ifdef SHOW_ITERATIONS
			n = 0;
#endif
			while (true)
			{
#ifdef SHOW_ITERATIONS
				n++;
#endif
				if (e->target->dot(normal).compare(shiftedDot) >= 0)
				{
					break;
				}
				intersection = e->reverse;
				e = e->next;
				if (e == startEdge)
				{
					return true;
				}
			}
#ifdef SHOW_ITERATIONS
			b3Printf("Needed %d iterations to advance intersection\n", n);
#endif
		}

#ifdef DEBUG_CONVEX_HULL
		b3Printf("Advanced intersecting edge to ");
		intersection->print();
		b3Printf(", cmp = %d\n", cmp);
#endif

		if (!firstIntersection)
		{
			firstIntersection = intersection;
		}
		else if (intersection == firstIntersection)
		{
			break;
		}

		int prevCmp = cmp;
		Edge* prevIntersection = intersection;
		Edge* prevFaceEdge = faceEdge;

		Edge* e = intersection->reverse;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (true)
		{
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->reverse->prev;
			b3Assert(e != intersection->reverse);
			cmp = e->target->dot(normal).compare(shiftedDot);
#ifdef DEBUG_CONVEX_HULL
			b3Printf("Testing edge ");
			e->print();
			b3Printf(" -> cmp = %d\n", cmp);
#endif
			if (cmp >= 0)
			{
				intersection = e;
				break;
			}
		}
#ifdef SHOW_ITERATIONS
		b3Printf("Needed %d iterations to find other intersection of face\n", n);
#endif

		if (cmp > 0)
		{
			Vertex* removed = intersection->target;
			e = intersection->reverse;
			if (e->prev == e)
			{
				removed->edges = NULL;
			}
			else
			{
				removed->edges = e->prev;
				e->prev->link(e->next);
				e->link(e);
			}
#ifdef DEBUG_CONVEX_HULL
			b3Printf("1: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif

			Point64 n0 = intersection->face->getNormal();
			Point64 n1 = intersection->reverse->face->getNormal();
			btInt64_t m00 = face->dir0.dot(n0);
			btInt64_t m01 = face->dir1.dot(n0);
			btInt64_t m10 = face->dir0.dot(n1);
			btInt64_t m11 = face->dir1.dot(n1);
			btInt64_t r0 = (intersection->face->origin - shiftedOrigin).dot(n0);
			btInt64_t r1 = (intersection->reverse->face->origin - shiftedOrigin).dot(n1);
			Int128 det = Int128::mul(m00, m11) - Int128::mul(m01, m10);
			b3Assert(det.getSign() != 0);
			Vertex* v = vertexPool.newObject();
			v->point.index = -1;
			v->copy = -1;
			v->point128 = PointR128(Int128::mul(face->dir0.x * r0, m11) - Int128::mul(face->dir0.x * r1, m01) + Int128::mul(face->dir1.x * r1, m00) - Int128::mul(face->dir1.x * r0, m10) + det * shiftedOrigin.x,
									Int128::mul(face->dir0.y * r0, m11) - Int128::mul(face->dir0.y * r1, m01) + Int128::mul(face->dir1.y * r1, m00) - Int128::mul(face->dir1.y * r0, m10) + det * shiftedOrigin.y,
									Int128::mul(face->dir0.z * r0, m11) - Int128::mul(face->dir0.z * r1, m01) + Int128::mul(face->dir1.z * r1, m00) - Int128::mul(face->dir1.z * r0, m10) + det * shiftedOrigin.z,
									det);
			v->point.x = (btInt32_t)v->point128.xvalue();
			v->point.y = (btInt32_t)v->point128.yvalue();
			v->point.z = (btInt32_t)v->point128.zvalue();
			intersection->target = v;
			v->edges = e;

			stack.push_back(v);
			stack.push_back(removed);
			stack.push_back(NULL);
		}

		if (cmp || prevCmp || (prevIntersection->reverse->next->target != intersection->target))
		{
			faceEdge = newEdgePair(prevIntersection->target, intersection->target);
			if (prevCmp == 0)
			{
				faceEdge->link(prevIntersection->reverse->next);
			}
			if ((prevCmp == 0) || prevFaceEdge)
			{
				prevIntersection->reverse->link(faceEdge);
			}
			if (cmp == 0)
			{
				intersection->reverse->prev->link(faceEdge->reverse);
			}
			faceEdge->reverse->link(intersection->reverse);
		}
		else
		{
			faceEdge = prevIntersection->reverse->next;
		}

		if (prevFaceEdge)
		{
			if (prevCmp > 0)
			{
				faceEdge->link(prevFaceEdge->reverse);
			}
			else if (faceEdge != prevFaceEdge->reverse)
			{
				stack.push_back(prevFaceEdge->target);
				while (faceEdge->next != prevFaceEdge->reverse)
				{
					Vertex* removed = faceEdge->next->target;
					removeEdgePair(faceEdge->next);
					stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
					b3Printf("2: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
				}
				stack.push_back(NULL);
			}
		}
		faceEdge->face = face;
		faceEdge->reverse->face = intersection->face;

		if (!firstFaceEdge)
		{
			firstFaceEdge = faceEdge;
		}
	}
#ifdef SHOW_ITERATIONS
	b3Printf("Needed %d iterations to process all intersections\n", m);
#endif

	if (cmp > 0)
	{
		firstFaceEdge->reverse->target = faceEdge->target;
		firstIntersection->reverse->link(firstFaceEdge);
		firstFaceEdge->link(faceEdge->reverse);
	}
	else if (firstFaceEdge != faceEdge->reverse)
	{
		stack.push_back(faceEdge->target);
		while (firstFaceEdge->next != faceEdge->reverse)
		{
			Vertex* removed = firstFaceEdge->next->target;
			removeEdgePair(firstFaceEdge->next);
			stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
			b3Printf("3: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
		}
		stack.push_back(NULL);
	}

	b3Assert(stack.size() > 0);
	vertexList = stack[0];

#ifdef DEBUG_CONVEX_HULL
	b3Printf("Removing part\n");
#endif
#ifdef SHOW_ITERATIONS
	n = 0;
#endif
	int pos = 0;
	while (pos < stack.size())
	{
		int end = stack.size();
		while (pos < end)
		{
			Vertex* kept = stack[pos++];
#ifdef DEBUG_CONVEX_HULL
			kept->print();
#endif
			bool deeper = false;
			Vertex* removed;
			while ((removed = stack[pos++]) != NULL)
			{
#ifdef SHOW_ITERATIONS
				n++;
#endif
				kept->receiveNearbyFaces(removed);
				while (removed->edges)
				{
					if (!deeper)
					{
						deeper = true;
						stack.push_back(kept);
					}
					stack.push_back(removed->edges->target);
					removeEdgePair(removed->edges);
				}
			}
			if (deeper)
			{
				stack.push_back(NULL);
			}
		}
	}
#ifdef SHOW_ITERATIONS
	b3Printf("Needed %d iterations to remove part\n", n);
#endif

	stack.resize(0);
	face->origin = shiftedOrigin;

	return true;
}

static int getVertexCopy(b3ConvexHullInternal::Vertex* vertex, b3AlignedObjectArray<b3ConvexHullInternal::Vertex*>& vertices)
{
	int index = vertex->copy;
	if (index < 0)
	{
		index = vertices.size();
		vertex->copy = index;
		vertices.push_back(vertex);
#ifdef DEBUG_CONVEX_HULL
		b3Printf("Vertex %d gets index *%d\n", vertex->point.index, index);
#endif
	}
	return index;
}

b3Scalar b3ConvexHullComputer::compute(const void* coords, bool doubleCoords, int stride, int count, b3Scalar shrink, b3Scalar shrinkClamp)
{
	if (count <= 0)
	{
		vertices.clear();
		edges.clear();
		faces.clear();
		return 0;
	}

	b3ConvexHullInternal hull;
	hull.compute(coords, doubleCoords, stride, count);

	b3Scalar shift = 0;
	if ((shrink > 0) && ((shift = hull.shrink(shrink, shrinkClamp)) < 0))
	{
		vertices.clear();
		edges.clear();
		faces.clear();
		return shift;
	}

	vertices.resize(0);
	edges.resize(0);
	faces.resize(0);

	b3AlignedObjectArray<b3ConvexHullInternal::Vertex*> oldVertices;
	getVertexCopy(hull.vertexList, oldVertices);
	int copied = 0;
	while (copied < oldVertices.size())
	{
		b3ConvexHullInternal::Vertex* v = oldVertices[copied];
		vertices.push_back(hull.getCoordinates(v));
		b3ConvexHullInternal::Edge* firstEdge = v->edges;
		if (firstEdge)
		{
			int firstCopy = -1;
			int prevCopy = -1;
			b3ConvexHullInternal::Edge* e = firstEdge;
			do
			{
				if (e->copy < 0)
				{
					int s = edges.size();
					edges.push_back(Edge());
					edges.push_back(Edge());
					Edge* c = &edges[s];
					Edge* r = &edges[s + 1];
					e->copy = s;
					e->reverse->copy = s + 1;
					c->reverse = 1;
					r->reverse = -1;
					c->targetVertex = getVertexCopy(e->target, oldVertices);
					r->targetVertex = copied;
#ifdef DEBUG_CONVEX_HULL
					b3Printf("      CREATE: Vertex *%d has edge to *%d\n", copied, c->getTargetVertex());
#endif
				}
				if (prevCopy >= 0)
				{
					edges[e->copy].next = prevCopy - e->copy;
				}
				else
				{
					firstCopy = e->copy;
				}
				prevCopy = e->copy;
				e = e->next;
			} while (e != firstEdge);
			edges[firstCopy].next = prevCopy - firstCopy;
		}
		copied++;
	}

	for (int i = 0; i < copied; i++)
	{
		b3ConvexHullInternal::Vertex* v = oldVertices[i];
		b3ConvexHullInternal::Edge* firstEdge = v->edges;
		if (firstEdge)
		{
			b3ConvexHullInternal::Edge* e = firstEdge;
			do
			{
				if (e->copy >= 0)
				{
#ifdef DEBUG_CONVEX_HULL
					b3Printf("Vertex *%d has edge to *%d\n", i, edges[e->copy].getTargetVertex());
#endif
					faces.push_back(e->copy);
					b3ConvexHullInternal::Edge* f = e;
					do
					{
#ifdef DEBUG_CONVEX_HULL
						b3Printf("   Face *%d\n", edges[f->copy].getTargetVertex());
#endif
						f->copy = -1;
						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != firstEdge);
		}
	}

	return shift;
}
