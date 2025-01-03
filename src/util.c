void float_map_multiply_and_add(float *dst, float *src, int w, int h, float multiplier)
{
    int x, y;
    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
        {
            dst[y * w + x] += src[y * w + x] * multiplier;
        }
    }
}