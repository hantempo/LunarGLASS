#version 100

int foo(int a, const int b, in int c, const in int d, out int e, inout int f)
{
    int sum = a + b + c + d + f; // no e, it is out only
	// sum should be 47 now

	a *= 64;
	// no b, it is read only
	c *= 64;
	// no d, it is read only
	e = 64 * 16; // e starts undefined
	f *= 64;

	sum += a + 64 * b + c + 64 * d + e + f; // everything has a value now, totaling of 64(1+2+4+8+16+32) = 64*63 = 4032
	// sum should be 4032 + 47  = 4079
	
	return sum;
}

void main()
{
    int e;
	int t = 2;
	struct s {
	    ivec4 t;
	} f;
	f.t.y = 32;

    int color = foo(1, 2, t+t, 8, e, f.t.y);

	color += 128 * (e + f.t.y); // right side should be 128(64(16 + 32)) = 393216
	// sum should be 4079 + 393216 = 397295
    
    gl_FragColor = vec4(color);
}
