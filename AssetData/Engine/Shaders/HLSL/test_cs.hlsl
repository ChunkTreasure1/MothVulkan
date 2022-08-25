[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(const uint2 pixCoord : SV_DispatchThreadID)
{
    const int2 outputPixCoords = pixCoord;
}