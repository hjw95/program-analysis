int main()
{
    int b, c, d, e, f, g, sink, source, N;
    // read source from input
    int i = 0;
    while (i < N)
    {
        if (i % 2 == 0)
        {
            b = source;
            d = source + 1;
        }
        else
        {
            c = b;
            g = i + 1;
            f = g / d;
        }
        i++;
    }
    sink = c;
}