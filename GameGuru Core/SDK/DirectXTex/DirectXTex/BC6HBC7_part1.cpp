_Use_decl_annotations_
void D3DX_BC6H::GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts, INTColor aPalette[]) const
{
    assert(pEP);
    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    assert(uNumIndices > 0);
    _Analysis_assume_(uNumIndices > 0);
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];

    // scale endpoints
    INTEndPntPair unqEndPts;
    unqEndPts.A.r = Unquantize(endPts.A.r, Prec.r, pEP->bSigned);
    unqEndPts.A.g = Unquantize(endPts.A.g, Prec.g, pEP->bSigned);
    unqEndPts.A.b = Unquantize(endPts.A.b, Prec.b, pEP->bSigned);
    unqEndPts.B.r = Unquantize(endPts.B.r, Prec.r, pEP->bSigned);
    unqEndPts.B.g = Unquantize(endPts.B.g, Prec.g, pEP->bSigned);
    unqEndPts.B.b = Unquantize(endPts.B.b, Prec.b, pEP->bSigned);

    // interpolate
    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8); _Analysis_assume_(uNumIndices <= 8); break;
    case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16); _Analysis_assume_(uNumIndices <= 16); break;
    default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
#pragma prefast(suppress:22102 22103, "writing blocks in two halves confuses tool")
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = FinishUnquantize(
            (unqEndPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].g = FinishUnquantize(
            (unqEndPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].b = FinishUnquantize(
            (unqEndPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
    }
}


// given a collection of colors and quantized endpoints, generate a palette, choose best entries, and return a single toterr
_Use_decl_annotations_
float D3DX_BC6H::MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const
{
    assert(pEP);

    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteQuantized(pEP, endPts, aPalette);

    float fTotErr = 0;
    for (size_t i = 0; i < np; ++i)
    {
        XMVECTOR vcolors = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aColors[i]));

        // Compute ErrorMetricRGB
        XMVECTOR tpal = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aPalette[0]));
        tpal = XMVectorSubtract(vcolors, tpal);
        float fBestErr = XMVectorGetX(XMVector3Dot(tpal, tpal));

        for (int j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            // Compute ErrorMetricRGB
            tpal = XMLoadSInt4(reinterpret_cast<const XMINT4*>(&aPalette[j]));
            tpal = XMVectorSubtract(vcolors, tpal);
            float fErr = XMVectorGetX(XMVector3Dot(tpal, tpal));
            if (fErr > fBestErr) break;     // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotErr += fBestErr;
    }
    return fTotErr;
}


_Use_decl_annotations_
float D3DX_BC6H::PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
    const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const
{
    assert(pEP);
    uint8_t uPrec;
    switch (ch)
    {
    case 0: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].r; break;
    case 1: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].g; break;
    case 2: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].b; break;
    default: assert(false); newEndPts = oldEndPts; return FLT_MAX;
    }
    INTEndPntPair tmpEndPts;
    float fMinErr = fOldErr;
    int beststep = 0;

    // copy real endpoints so we can perturb them
    tmpEndPts = newEndPts = oldEndPts;

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (uPrec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            if (do_b == 0)
            {
                tmpEndPts.A[ch] = newEndPts.A[ch] + sign * step;
                if (tmpEndPts.A[ch] < 0 || tmpEndPts.A[ch] >= (1 << uPrec))
                    continue;
            }
            else
            {
                tmpEndPts.B[ch] = newEndPts.B[ch] + sign * step;
                if (tmpEndPts.B[ch] < 0 || tmpEndPts.B[ch] >= (1 << uPrec))
                    continue;
            }

            float fErr = MapColorsQuantized(pEP, aColors, np, tmpEndPts);

            if (fErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fErr;
                beststep = sign * step;
            }
        }
        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
        {
            if (do_b == 0)
                newEndPts.A[ch] += beststep;
            else
                newEndPts.B[ch] += beststep;
        }
    }
    return fMinErr;
}


_Use_decl_annotations_
void D3DX_BC6H::OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
    const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const
{
    assert(pEP);
    float aOptErr = aOrgErr;
    aOptEndPts.A = aOrgEndPts.A;
    aOptEndPts.B = aOrgEndPts.B;

    INTEndPntPair new_a, new_b;
    INTEndPntPair newEndPts;
    int do_b;

    // now optimize each channel separately
    for (uint8_t ch = 0; ch < 3; ++ch)
    {
        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        float fErr0 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_a, aOptErr, 0);	// perturb endpt A
        float fErr1 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_b, aOptErr, 1);	// perturb endpt B

        if (fErr0 < fErr1)
        {
            if (fErr0 >= aOptErr) continue;
            aOptEndPts.A[ch] = new_a.A[ch];
            aOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= aOptErr) continue;
            aOptEndPts.B[ch] = new_b.B[ch];
            aOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (;;)
        {
            float fErr = PerturbOne(pEP, aColors, np, ch, aOptEndPts, newEndPts, aOptErr, do_b);
            if (fErr >= aOptErr)
                break;
            if (do_b == 0)
                aOptEndPts.A[ch] = newEndPts.A[ch];
            else
                aOptEndPts.B[ch] = newEndPts.B[ch];
            aOptErr = fErr;
            do_b = 1 - do_b;	// now move the other endpoint
        }
    }
}


_Use_decl_annotations_
void D3DX_BC6H::OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[], const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS);
    INTColor aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[p][pEP->uShape][i] == p)
            {
                aPixels[np++] = pEP->aIPixels[i];
            }
        }

        OptimizeOne(pEP, aPixels, np, aOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}


// Swap endpoints as needed to ensure that the indices at fix up have a 0 high-order bit
_Use_decl_annotations_
void D3DX_BC6H::SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[])
{
    assert(pEP);
    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const size_t uNumIndices = size_t(1) << ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHighIndexBit = uNumIndices >> 1;

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        size_t i = g_aFixUp[uPartitions][pEP->uShape][p];
        assert(g_aPartitionTable[uPartitions][pEP->uShape][i] == p);
        if (aIndices[i] & uHighIndexBit)
        {
            // high bit is set, swap the aEndPts and indices for this region
            std::swap(aEndPts[p].A, aEndPts[p].B);

            for (size_t j = 0; j < NUM_PIXELS_PER_BLOCK; ++j)
                if (g_aPartitionTable[uPartitions][pEP->uShape][j] == p)
                    aIndices[j] = uNumIndices - 1 - aIndices[j];
        }
    }
}


// assign indices given a tile, shape, and quantized endpoints, return toterr for each region
_Use_decl_annotations_
void D3DX_BC6H::AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[], size_t aIndices[], float aTotErr[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t uNumIndices = 1 << ms_aInfo[pEP->uMode].uIndexPrec;

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    // build list of possibles
    INTColor aPalette[BC6H_MAX_REGIONS][BC6H_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        GeneratePaletteQuantized(pEP, aEndPts[p], aPalette[p]);
        aTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        const uint8_t uRegion = g_aPartitionTable[uPartitions][pEP->uShape][i];
        assert(uRegion < BC6H_MAX_REGIONS);
        _Analysis_assume_(uRegion < BC6H_MAX_REGIONS);
        float fBestErr = Norm(pEP->aIPixels[i], aPalette[uRegion][0]);
        aIndices[i] = 0;

        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            float fErr = Norm(pEP->aIPixels[i], aPalette[uRegion][j]);
            if (fErr > fBestErr) break;	// error increased, so we're done searching
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                aIndices[i] = j;
            }
        }
        aTotErr[uRegion] += fBestErr;
    }
}


_Use_decl_annotations_
void D3DX_BC6H::QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* aQntEndPts) const
{
    assert(pEP && aQntEndPts);
    const INTEndPntPair* aUnqEndPts = pEP->aUnqEndPts[pEP->uShape];
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        aQntEndPts[p].A.r = Quantize(aUnqEndPts[p].A.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].A.g = Quantize(aUnqEndPts[p].A.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].A.b = Quantize(aUnqEndPts[p].A.b, Prec.b, pEP->bSigned);
        aQntEndPts[p].B.r = Quantize(aUnqEndPts[p].B.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].B.g = Quantize(aUnqEndPts[p].B.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].B.b = Quantize(aUnqEndPts[p].B.b, Prec.b, pEP->bSigned);
    }
}


_Use_decl_annotations_
void D3DX_BC6H::EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[], const size_t aIndices[])
{
    assert(pEP);
    const uint8_t uRealMode = ms_aInfo[pEP->uMode].uMode;
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHeaderBits = uPartitions > 0 ? 82 : 65;
    const ModeDescriptor* desc = ms_aDesc[pEP->uMode];
    size_t uStartBit = 0;

    while (uStartBit < uHeaderBits)
    {
        switch (desc[uStartBit].m_eField)
        {
        case M:  SetBit(uStartBit, uint8_t(uRealMode >> desc[uStartBit].m_uBit) & 0x01); break;
        case D:  SetBit(uStartBit, uint8_t(pEP->uShape >> desc[uStartBit].m_uBit) & 0x01); break;
        case RW: SetBit(uStartBit, uint8_t(aEndPts[0].A.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RX: SetBit(uStartBit, uint8_t(aEndPts[0].B.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RY: SetBit(uStartBit, uint8_t(aEndPts[1].A.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case GW: SetBit(uStartBit, uint8_t(aEndPts[0].A.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GX: SetBit(uStartBit, uint8_t(aEndPts[0].B.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GY: SetBit(uStartBit, uint8_t(aEndPts[1].A.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case BW: SetBit(uStartBit, uint8_t(aEndPts[0].A.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BX: SetBit(uStartBit, uint8_t(aEndPts[0].B.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BY: SetBit(uStartBit, uint8_t(aEndPts[1].A.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.b >> desc[uStartBit].m_uBit) & 0x01); break;
        default: assert(false);
        }
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, pEP->uShape, i))
            SetBits(uStartBit, uIndexPrec - 1, static_cast<uint8_t>(aIndices[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aIndices[i]));
    }
    assert(uStartBit == 128);
}


_Use_decl_annotations_
void D3DX_BC6H::Refine(EncodeParams* pEP)
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS);

    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    float aOrgErr[BC6H_MAX_REGIONS], aOptErr[BC6H_MAX_REGIONS];
    INTEndPntPair aOrgEndPts[BC6H_MAX_REGIONS], aOptEndPts[BC6H_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK], aOptIdx[NUM_PIXELS_PER_BLOCK];

    QuantizeEndPts(pEP, aOrgEndPts);
    AssignIndices(pEP, aOrgEndPts, aOrgIdx, aOrgErr);
    SwapIndices(pEP, aOrgEndPts, aOrgIdx);

    if (bTransformed) TransformForward(aOrgEndPts);
    if (EndPointsFit(pEP, aOrgEndPts))
    {
        if (bTransformed) TransformInverse(aOrgEndPts, ms_aInfo[pEP->uMode].RGBAPrec[0][0], pEP->bSigned);
        OptimizeEndPoints(pEP, aOrgErr, aOrgEndPts, aOptEndPts);
        AssignIndices(pEP, aOptEndPts, aOptIdx, aOptErr);
        SwapIndices(pEP, aOptEndPts, aOptIdx);

        float fOrgTotErr = 0.0f, fOptTotErr = 0.0f;
        for (size_t p = 0; p <= uPartitions; ++p)
        {
            fOrgTotErr += aOrgErr[p];
            fOptTotErr += aOptErr[p];
        }

        if (bTransformed) TransformForward(aOptEndPts);
        if (EndPointsFit(pEP, aOptEndPts) && fOptTotErr < fOrgTotErr && fOptTotErr < pEP->fBestErr)
        {
            pEP->fBestErr = fOptTotErr;
            EmitBlock(pEP, aOptEndPts, aOptIdx);
        }
        else if (fOrgTotErr < pEP->fBestErr)
        {
            // either it stopped fitting when we optimized it, or there was no improvement
            // so go back to the unoptimized endpoints which we know will fit
            if (bTransformed) TransformForward(aOrgEndPts);
            pEP->fBestErr = fOrgTotErr;
            EmitBlock(pEP, aOrgEndPts, aOrgIdx);
        }
    }
}


_Use_decl_annotations_
void D3DX_BC6H::GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[])
{
    assert(pEP);
    assert(uRegion < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    _Analysis_assume_(uRegion < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
    const INTEndPntPair& endPts = pEP->aUnqEndPts[pEP->uShape][uRegion];
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    assert(uNumIndices > 0);
    _Analysis_assume_(uNumIndices > 0);

    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8); _Analysis_assume_(uNumIndices <= 8); break;
    case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16); _Analysis_assume_(uNumIndices <= 16); break;
    default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
#pragma prefast(suppress:22102 22103, "writing blocks in two halves confuses tool")
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = (endPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].g = (endPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].b = (endPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
    }
}


_Use_decl_annotations_
float D3DX_BC6H::MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const
{
    assert(pEP);
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteUnquantized(pEP, uRegion, aPalette);

    float fTotalErr = 0.0f;
    for (size_t i = 0; i < np; ++i)
    {
        float fBestErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[0]);
        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0.0f; ++j)
        {
            float fErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[j]);
            if (fErr > fBestErr) break;      // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotalErr += fBestErr;
    }

    return fTotalErr;
}

_Use_decl_annotations_
float D3DX_BC6H::RoughMSE(EncodeParams* pEP) const
{
    assert(pEP);
    assert(pEP->uShape < BC6H_MAX_SHAPES);
    _Analysis_assume_(pEP->uShape < BC6H_MAX_SHAPES);

    INTEndPntPair* aEndPts = pEP->aUnqEndPts[pEP->uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC6H_MAX_REGIONS);

    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];

    float fError = 0.0f;
    for (size_t p = 0; p <= uPartitions; ++p)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[uPartitions][pEP->uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[1]];
            continue;
        }

        HDRColorA epA, epB;
        OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
        aEndPts[p].A.Set(epA, pEP->bSigned);
        aEndPts[p].B.Set(epB, pEP->bSigned);
        if (pEP->bSigned)
        {
            aEndPts[p].A.Clamp(-F16MAX, F16MAX);
            aEndPts[p].B.Clamp(-F16MAX, F16MAX);
        }
        else
        {
            aEndPts[p].A.Clamp(0, F16MAX);
            aEndPts[p].B.Clamp(0, F16MAX);
        }

        fError += MapColors(pEP, p, np, auPixIdx);
    }

    return fError;
}


//-------------------------------------------------------------------------------------
// BC7 Compression
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void D3DX_BC7::Decode(HDRColorA* pOut) const
{
    assert(pOut);

    size_t uFirst = 0;
    while (uFirst < 128 && !GetBit(uFirst)) {}
    uint8_t uMode = uint8_t(uFirst - 1);

    if (uMode < 8)
    {
        const uint8_t uPartitions = ms_aInfo[uMode].uPartitions;
        assert(uPartitions < BC7_MAX_REGIONS);
        _Analysis_assume_(uPartitions < BC7_MAX_REGIONS);

        const uint8_t uNumEndPts = (uPartitions + 1) << 1;
        const uint8_t uIndexPrec = ms_aInfo[uMode].uIndexPrec;
        const uint8_t uIndexPrec2 = ms_aInfo[uMode].uIndexPrec2;
        size_t i;
        size_t uStartBit = uMode + 1;
        uint8_t P[6];
        uint8_t uShape = GetBits(uStartBit, ms_aInfo[uMode].uPartitionBits);
        assert(uShape < BC7_MAX_SHAPES);
        _Analysis_assume_(uShape < BC7_MAX_SHAPES);

        uint8_t uRotation = GetBits(uStartBit, ms_aInfo[uMode].uRotationBits);
        assert(uRotation < 4);

        uint8_t uIndexMode = GetBits(uStartBit, ms_aInfo[uMode].uIndexModeBits);
        assert(uIndexMode < 2);

        LDRColorA c[BC7_MAX_REGIONS << 1];
        const LDRColorA RGBAPrec = ms_aInfo[uMode].RGBAPrec;
        const LDRColorA RGBAPrecWithP = ms_aInfo[uMode].RGBAPrecWithP;

        assert(uNumEndPts <= (BC7_MAX_REGIONS << 1));

        // Red channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.r > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].r = GetBits(uStartBit, RGBAPrec.r);
        }

        // Green channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.g > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].g = GetBits(uStartBit, RGBAPrec.g);
        }

        // Blue channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.b > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].b = GetBits(uStartBit, RGBAPrec.b);
        }

        // Alpha channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.a > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].a = RGBAPrec.a ? GetBits(uStartBit, RGBAPrec.a) : 255;
        }

        // P-bits
        assert(ms_aInfo[uMode].uPBits <= 6);
        _Analysis_assume_(ms_aInfo[uMode].uPBits <= 6);
        for (i = 0; i < ms_aInfo[uMode].uPBits; i++)
        {
            if (uStartBit > 127)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            P[i] = GetBit(uStartBit);
        }

        if (ms_aInfo[uMode].uPBits)
        {
            for (i = 0; i < uNumEndPts; i++)
            {
                size_t pi = i * ms_aInfo[uMode].uPBits / uNumEndPts;
                for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
                {
                    if (RGBAPrec[ch] != RGBAPrecWithP[ch])
                    {
                        c[i][ch] = (c[i][ch] << 1) | P[pi];
                    }
                }
            }
        }

        for (i = 0; i < uNumEndPts; i++)
        {
            c[i] = Unquantize(c[i], RGBAPrecWithP);
        }

        uint8_t w1[NUM_PIXELS_PER_BLOCK], w2[NUM_PIXELS_PER_BLOCK];

        // read color indices
        for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            size_t uNumBits = IsFixUpOffset(ms_aInfo[uMode].uPartitions, uShape, i) ? uIndexPrec - 1 : uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }
            w1[i] = GetBits(uStartBit, uNumBits);
        }

        // read alpha indices
        if (uIndexPrec2)
        {
            for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            {
                size_t uNumBits = i ? uIndexPrec2 : uIndexPrec2 - 1;
                if (uStartBit + uNumBits > 128)
                {
#ifdef _DEBUG
                    OutputDebugStringA("BC7: Invalid block encountered during decoding\n");
#endif
                    FillWithErrorColors(pOut);
                    return;
                }
                w2[i] = GetBits(uStartBit, uNumBits);
            }
        }

        for (i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
            LDRColorA outPixel;
            if (uIndexPrec2 == 0)
            {
                LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w1[i], uIndexPrec, uIndexPrec, outPixel);
            }
            else
            {
                if (uIndexMode == 0)
                {
                    LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w2[i], uIndexPrec, uIndexPrec2, outPixel);
                }
                else
                {
                    LDRColorA::Interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w2[i], w1[i], uIndexPrec2, uIndexPrec, outPixel);
                }
            }

            switch (uRotation)
            {
            case 1: std::swap(outPixel.r, outPixel.a); break;
            case 2: std::swap(outPixel.g, outPixel.a); break;
            case 3: std::swap(outPixel.b, outPixel.a); break;
            }

            pOut[i] = HDRColorA(outPixel);
        }
    }
    else
    {
#ifdef _DEBUG
        OutputDebugStringA("BC7: Reserved mode 8 encountered during decoding\n");
#endif
        // Per the BC7 format spec, we must return transparent black
        memset(pOut, 0, sizeof(HDRColorA) * NUM_PIXELS_PER_BLOCK);
    }
}

_Use_decl_annotations_
void D3DX_BC7::Encode(DWORD flags, const HDRColorA* const pIn)
{
    assert(pIn);

    D3DX_BC7 final = *this;
    EncodeParams EP(pIn);
    float fMSEBest = FLT_MAX;

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        EP.aLDRPixels[i].r = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].r * 255.0f + 0.01f)));
        EP.aLDRPixels[i].g = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].g * 255.0f + 0.01f)));
        EP.aLDRPixels[i].b = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].b * 255.0f + 0.01f)));
        EP.aLDRPixels[i].a = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].a * 255.0f + 0.01f)));
    }

    for (EP.uMode = 0; EP.uMode < 8 && fMSEBest > 0; ++EP.uMode)
    {
        if (!(flags & BC_FLAGS_USE_3SUBSETS) && (EP.uMode == 0 || EP.uMode == 2))
        {
            // 3 subset modes tend to be used rarely and add significant compression time
            continue;
        }

        if ((flags & TEX_COMPRESS_BC7_QUICK) && (EP.uMode != 6))
        {
            // Use only mode 6
            continue;
        }

        const size_t uShapes = size_t(1) << ms_aInfo[EP.uMode].uPartitionBits;
        assert(uShapes <= BC7_MAX_SHAPES);
        _Analysis_assume_(uShapes <= BC7_MAX_SHAPES);

        const size_t uNumRots = size_t(1) << ms_aInfo[EP.uMode].uRotationBits;
        const size_t uNumIdxMode = size_t(1) << ms_aInfo[EP.uMode].uIndexModeBits;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1, uShapes >> 2);
        float afRoughMSE[BC7_MAX_SHAPES];
        size_t auShape[BC7_MAX_SHAPES];

        for (size_t r = 0; r < uNumRots && fMSEBest > 0; ++r)
        {
            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }

            for (size_t im = 0; im < uNumIdxMode && fMSEBest > 0; ++im)
            {
                // pick the best uItems shapes and refine these.
                for (size_t s = 0; s < uShapes; s++)
                {
                    afRoughMSE[s] = RoughMSE(&EP, s, im);
                    auShape[s] = s;
                }

                // Bubble up the first uItems items
                for (size_t i = 0; i < uItems; i++)
                {
                    for (size_t j = i + 1; j < uShapes; j++)
                    {
                        if (afRoughMSE[i] > afRoughMSE[j])
                        {
                            std::swap(afRoughMSE[i], afRoughMSE[j]);
                            std::swap(auShape[i], auShape[j]);
                        }
                    }
                }

                for (size_t i = 0; i < uItems && fMSEBest > 0; i++)
                {
                    float fMSE = Refine(&EP, auShape[i], r, im);
                    if (fMSE < fMSEBest)
                    {
                        final = *this;
                        fMSEBest = fMSE;
                    }
                }
            }

            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }
        }
    }

    *this = final;
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void D3DX_BC7::GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endPts, LDRColorA aPalette[]) const
{
    assert(pEP);
    const size_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
    assert(uNumIndices > 0 && uNumIndices2 > 0);
    _Analysis_assume_(uNumIndices > 0 && uNumIndices2 > 0);
    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));
    _Analysis_assume_((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    LDRColorA a = Unquantize(endPts.A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    LDRColorA b = Unquantize(endPts.B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    if (uIndexPrec2 == 0)
    {
        for (size_t i = 0; i < uNumIndices; i++)
            LDRColorA::Interpolate(a, b, i, i, uIndexPrec, uIndexPrec, aPalette[i]);
    }
    else
    {
        for (size_t i = 0; i < uNumIndices; i++)
            LDRColorA::InterpolateRGB(a, b, i, uIndexPrec, aPalette[i]);
        for (size_t i = 0; i < uNumIndices2; i++)
            LDRColorA::InterpolateA(a, b, i, uIndexPrec2, aPalette[i]);
    }
}

_Use_decl_annotations_
float D3DX_BC7::PerturbOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    const LDREndPntPair &oldEndPts, LDREndPntPair &newEndPts, float fOldErr, uint8_t do_b) const
{
    assert(pEP);
    const int prec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmp_endPts = newEndPts = oldEndPts;
    float fMinErr = fOldErr;
    uint8_t* pnew_c = (do_b ? &newEndPts.B[ch] : &newEndPts.A[ch]);
    uint8_t* ptmp_c = (do_b ? &tmp_endPts.B[ch] : &tmp_endPts.A[ch]);

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (prec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        int beststep = 0;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            int tmp = int(*pnew_c) + sign * step;
            if (tmp < 0 || tmp >= (1 << prec))
                continue;
            else
                *ptmp_c = (uint8_t)tmp;

            float fTotalErr = MapColors(pEP, aColors, np, uIndexMode, tmp_endPts, fMinErr);
            if (fTotalErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fTotalErr;
                beststep = sign * step;
            }
        }

        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
            *pnew_c = uint8_t(int(*pnew_c) + beststep);
    }
    return fMinErr;
}

// perturb the endpoints at least -3 to 3.
// always ensure endpoint ordering is preserved (no need to overlap the scan)
_Use_decl_annotations_
void D3DX_BC7::Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    float& fOrgErr, LDREndPntPair& optEndPt) const
{
    assert(pEP);
    const uint8_t uPrec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmpEndPt;
    if (fOrgErr == 0)
        return;

    int delta = 5;

    // ok figure out the range of A and B
    tmpEndPt = optEndPt;
    int alow = std::max<int>(0, int(optEndPt.A[ch]) - delta);
    int ahigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.A[ch]) + delta);
    int blow = std::max<int>(0, int(optEndPt.B[ch]) - delta);
    int bhigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.B[ch]) + delta);
    int amin = 0;
    int bmin = 0;

    float fBestErr = fOrgErr;
    if (optEndPt.A[ch] <= optEndPt.B[ch])
    {
        // keep a <= b
        for (int a = alow; a <= ahigh; ++a)
        {
            for (int b = std::max<int>(a, blow); b < bhigh; ++b)
            {
                tmpEndPt.A[ch] = (uint8_t)a;
                tmpEndPt.B[ch] = (uint8_t)b;

                float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }
    else
    {
        // keep b <= a
        for (int b = blow; b < bhigh; ++b)
        {
            for (int a = std::max<int>(b, alow); a <= ahigh; ++a)
            {
                tmpEndPt.A[ch] = (uint8_t)a;
                tmpEndPt.B[ch] = (uint8_t)b;

                float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }

    if (fBestErr < fOrgErr)
    {
        optEndPt.A[ch] = (uint8_t)amin;
        optEndPt.B[ch] = (uint8_t)bmin;
        fOrgErr = fBestErr;
    }
}

_Use_decl_annotations_
void D3DX_BC7::OptimizeOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
    float fOrgErr, const LDREndPntPair& org, LDREndPntPair& opt) const
{
    assert(pEP);

    float fOptErr = fOrgErr;
    opt = org;

    LDREndPntPair new_a, new_b;
    LDREndPntPair newEndPts;
    uint8_t do_b;

    // now optimize each channel separately
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ++ch)
    {
        if (ms_aInfo[pEP->uMode].RGBAPrecWithP[ch] == 0)
            continue;

        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        float fErr0 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_a, fOptErr, 0);	// perturb endpt A
        float fErr1 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_b, fOptErr, 1);	// perturb endpt B

        uint8_t& copt_a = opt.A[ch];
        uint8_t& copt_b = opt.B[ch];
        uint8_t& cnew_a = new_a.A[ch];
        uint8_t& cnew_b = new_a.B[ch];

        if (fErr0 < fErr1)
        {
            if (fErr0 >= fOptErr)
                continue;
            copt_a = cnew_a;
            fOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= fOptErr)
                continue;
            copt_b = cnew_b;
            fOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (; ; )
        {
            float fErr = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, newEndPts, fOptErr, do_b);
            if (fErr >= fOptErr)
                break;
            if (do_b == 0)
                copt_a = cnew_a;
            else
                copt_b = cnew_b;
            fOptErr = fErr;
            do_b = 1 - do_b;	// now move the other endpoint
        }
    }

    // finally, do a small exhaustive search around what we think is the global minima to be sure
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        Exhaustive(pEP, aColors, np, uIndexMode, ch, fOptErr, opt);
}

_Use_decl_annotations_
void D3DX_BC7::OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, const float afOrgErr[],
    const LDREndPntPair aOrgEndPts[], LDREndPntPair aOptEndPts[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS && uShape < BC7_MAX_SHAPES);
    _Analysis_assume_(uPartitions < BC7_MAX_REGIONS && uShape < BC7_MAX_SHAPES);

    LDRColorA aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
                aPixels[np++] = pEP->aLDRPixels[i];

        OptimizeOne(pEP, aPixels, np, uIndexMode, afOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}

_Use_decl_annotations_
void D3DX_BC7::AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, LDREndPntPair endPts[], size_t aIndices[], size_t aIndices2[],
    float afTotErr[]) const
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
    _Analysis_assume_(uShape < BC7_MAX_SHAPES);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    const uint8_t uNumIndices2 = 1 << uIndexPrec2;

    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));
    _Analysis_assume_((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    const uint8_t uHighestIndexBit = uNumIndices >> 1;
    const uint8_t uHighestIndexBit2 = uNumIndices2 >> 1;
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    // build list of possibles
    for (size_t p = 0; p <= uPartitions; p++)
    {
        GeneratePaletteQuantized(pEP, uIndexMode, endPts[p], aPalette[p]);
        afTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        assert(uRegion < BC7_MAX_REGIONS);
        _Analysis_assume_(uRegion < BC7_MAX_REGIONS);
        afTotErr[uRegion] += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2, &(aIndices[i]), &(aIndices2[i]));
    }

    // swap endpoints as needed to ensure that the indices at index_positions have a 0 high-order bit
    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A, endPts[p].B);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);
        }
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A.r, endPts[p].B.r);
                std::swap(endPts[p].A.g, endPts[p].B.g);
                std::swap(endPts[p].A.b, endPts[p].B.b);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);

            if (aIndices2[0] & uHighestIndexBit2)
            {
                std::swap(endPts[p].A.a, endPts[p].B.a);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    aIndices2[i] = uNumIndices2 - 1 - aIndices2[i];
            }
            assert((aIndices2[0] & uHighestIndexBit2) == 0);
        }
    }
}

_Use_decl_annotations_
void D3DX_BC7::EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode, const LDREndPntPair aEndPts[], const size_t aIndex[], const size_t aIndex2[])
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC7_MAX_REGIONS);

    const size_t uPBits = ms_aInfo[pEP->uMode].uPBits;
    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = ms_aInfo[pEP->uMode].uIndexPrec2;
    const LDRColorA RGBAPrec = ms_aInfo[pEP->uMode].RGBAPrec;
    const LDRColorA RGBAPrecWithP = ms_aInfo[pEP->uMode].RGBAPrecWithP;
    size_t i;
    size_t uStartBit = 0;
    SetBits(uStartBit, pEP->uMode, 0);
    SetBits(uStartBit, 1, 1);
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uRotationBits, static_cast<uint8_t>(uRotation));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uIndexModeBits, static_cast<uint8_t>(uIndexMode));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uPartitionBits, static_cast<uint8_t>(uShape));

    if (uPBits)
    {
        const size_t uNumEP = size_t(1 + uPartitions) << 1;
        uint8_t aPVote[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        uint8_t aCount[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            uint8_t ep = 0;
            for (i = 0; i <= uPartitions; i++)
            {
                if (RGBAPrec[ch] == RGBAPrecWithP[ch])
                {
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
                }
                else
                {
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch] >> 1);
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch] >> 1);
                    size_t idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    _Analysis_assume_(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += aEndPts[i].A[ch] & 0x01;
                    aCount[idx]++;
                    idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                    _Analysis_assume_(idx < (BC7_MAX_REGIONS << 1));
                    aPVote[idx] += aEndPts[i].B[ch] & 0x01;
                    aCount[idx]++;
                }
            }
        }

        for (i = 0; i < uPBits; i++)
        {
            SetBits(uStartBit, 1, aPVote[i] > (aCount[i] >> 1) ? 1 : 0);
        }
    }
    else
    {
        for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            for (i = 0; i <= uPartitions; i++)
            {
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
            }
        }
    }

    const size_t* aI1 = uIndexMode ? aIndex2 : aIndex;
    const size_t* aI2 = uIndexMode ? aIndex : aIndex2;
    for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, uShape, i))
            SetBits(uStartBit, uIndexPrec - 1, static_cast<uint8_t>(aI1[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aI1[i]));
    }
    if (uIndexPrec2)
        for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            SetBits(uStartBit, i ? uIndexPrec2 : uIndexPrec2 - 1, static_cast<uint8_t>(aI2[i]));

    assert(uStartBit == 128);
}

_Use_decl_annotations_
float D3DX_BC7::Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode)
{
    assert( pEP );
    assert( uShape < BC7_MAX_SHAPES );
    _Analysis_assume_( uShape < BC7_MAX_SHAPES );
    const LDREndPntPair* aEndPts = pEP->aEndPts[uShape];

    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert( uPartitions < BC7_MAX_REGIONS );
    _Analysis_assume_( uPartitions < BC7_MAX_REGIONS );

    LDREndPntPair aOrgEndPts[BC7_MAX_REGIONS];
    LDREndPntPair aOptEndPts[BC7_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOrgIdx2[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx2[NUM_PIXELS_PER_BLOCK];
    float aOrgErr[BC7_MAX_REGIONS];
    float aOptErr[BC7_MAX_REGIONS];

    for(size_t p = 0; p <= uPartitions; p++)
    {
        aOrgEndPts[p].A = Quantize(aEndPts[p].A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
        aOrgEndPts[p].B = Quantize(aEndPts[p].B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    }

    AssignIndices(pEP, uShape, uIndexMode, aOrgEndPts, aOrgIdx, aOrgIdx2, aOrgErr);
    OptimizeEndPoints(pEP, uShape, uIndexMode, aOrgErr, aOrgEndPts, aOptEndPts);
    AssignIndices(pEP, uShape, uIndexMode, aOptEndPts, aOptIdx, aOptIdx2, aOptErr);

    float fOrgTotErr = 0, fOptTotErr = 0;
    for(size_t p = 0; p <= uPartitions; p++)
    {
        fOrgTotErr += aOrgErr[p];
        fOptTotErr += aOptErr[p];
    }
    if(fOptTotErr < fOrgTotErr)
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, aOptEndPts, aOptIdx, aOptIdx2);
        return fOptTotErr;
    }
    else
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, aOrgEndPts, aOrgIdx, aOrgIdx2);
        return fOrgTotErr;
    }
}

_Use_decl_annotations_
float D3DX_BC7::MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, const LDREndPntPair& endPts, float fMinErr) const
{
    assert(pEP);
    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    LDRColorA aPalette[BC7_MAX_INDICES];
    float fTotalErr = 0;

    GeneratePaletteQuantized(pEP, uIndexMode, endPts, aPalette);
    for (size_t i = 0; i < np; ++i)
    {
        fTotalErr += ComputeError(aColors[i], aPalette, uIndexPrec, uIndexPrec2);
        if (fTotalErr > fMinErr)   // check for early exit
        {
            fTotalErr = FLT_MAX;
            break;
        }
    }

    return fTotalErr;
}

_Use_decl_annotations_
float D3DX_BC7::RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode)
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
    _Analysis_assume_(uShape < BC7_MAX_SHAPES);
    LDREndPntPair* aEndPts = pEP->aEndPts[uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);
    _Analysis_assume_(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    const uint8_t uNumIndices2 = 1 << uIndexPrec2;
    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; p++)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[1]];
            continue;
        }

        if (uIndexPrec2 == 0)
        {
            HDRColorA epA, epB;
            OptimizeRGBA(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            epA *= 255.0f;
            epB *= 255.0f;
            aEndPts[p].A = epA.ToLDRColorA();
            aEndPts[p].B = epB.ToLDRColorA();
        }
        else
        {
            uint8_t uMinAlpha = 255, uMaxAlpha = 0;
            for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            {
                uMinAlpha = std::min<uint8_t>(uMinAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
                uMaxAlpha = std::max<uint8_t>(uMaxAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
            }

            HDRColorA epA, epB;
            OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            epA *= 255.0f;
            epB *= 255.0f;
            aEndPts[p].A = epA.ToLDRColorA();
            aEndPts[p].B = epB.ToLDRColorA();
            aEndPts[p].A.a = uMinAlpha;
            aEndPts[p].B.a = uMaxAlpha;
        }
    }

    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
            for (size_t i = 0; i < uNumIndices; i++)
                LDRColorA::Interpolate(aEndPts[p].A, aEndPts[p].B, i, i, uIndexPrec, uIndexPrec, aPalette[p][i]);
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            for (size_t i = 0; i < uNumIndices; i++)
                LDRColorA::InterpolateRGB(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec, aPalette[p][i]);
            for (size_t i = 0; i < uNumIndices2; i++)
                LDRColorA::InterpolateA(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec2, aPalette[p][i]);
        }
    }

    float fTotalErr = 0;
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        fTotalErr += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2);
    }

    return fTotalErr;
}


//=====================================================================================
// Entry points
//=====================================================================================

//-------------------------------------------------------------------------------------
// BC6H Compression
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void DirectX::D3DXDecodeBC6HU(XMVECTOR *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<const D3DX_BC6H*>(pBC)->Decode(false, reinterpret_cast<HDRColorA*>(pColor));
}

_Use_decl_annotations_
void DirectX::D3DXDecodeBC6HS(XMVECTOR *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<const D3DX_BC6H*>(pBC)->Decode(true, reinterpret_cast<HDRColorA*>(pColor));
}

_Use_decl_annotations_
void DirectX::D3DXEncodeBC6HU(uint8_t *pBC, const XMVECTOR *pColor, DWORD flags)
{
    UNREFERENCED_PARAMETER(flags);
    assert(pBC && pColor);
    static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<D3DX_BC6H*>(pBC)->Encode(false, reinterpret_cast<const HDRColorA*>(pColor));
}

_Use_decl_annotations_
void DirectX::D3DXEncodeBC6HS(uint8_t *pBC, const XMVECTOR *pColor, DWORD flags)
{
    UNREFERENCED_PARAMETER(flags);
    assert(pBC && pColor);
    static_assert(sizeof(D3DX_BC6H) == 16, "D3DX_BC6H should be 16 bytes");
    reinterpret_cast<D3DX_BC6H*>(pBC)->Encode(true, reinterpret_cast<const HDRColorA*>(pColor));
}


//-------------------------------------------------------------------------------------
// BC7 Compression
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void DirectX::D3DXDecodeBC7(XMVECTOR *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(D3DX_BC7) == 16, "D3DX_BC7 should be 16 bytes");
    reinterpret_cast<const D3DX_BC7*>(pBC)->Decode(reinterpret_cast<HDRColorA*>(pColor));
}

_Use_decl_annotations_
void DirectX::D3DXEncodeBC7(uint8_t *pBC, const XMVECTOR *pColor, DWORD flags)
{
    assert(pBC && pColor);
    static_assert(sizeof(D3DX_BC7) == 16, "D3DX_BC7 should be 16 bytes");
    reinterpret_cast<D3DX_BC7*>(pBC)->Encode(flags, reinterpret_cast<const HDRColorA*>(pColor));
}
