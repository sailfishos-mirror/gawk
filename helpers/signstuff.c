#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int
main(int argc, char **argv)
{
	double nan = strtod("nan", NULL);
	double neg_nan = strtod("-nan", NULL);

	printf("nan = %g\n", nan);
	printf("neg_nan = %g\n", neg_nan);

	printf("nan + 0 = %g\n", nan + 0.0);
	printf("neg_nan + 0 = %g\n", neg_nan + 0.0);

	printf("nan + -0 = %g\n", nan + -0.0);
	printf("neg_nan + -0 = %g\n", neg_nan + -0.0);

	printf("nan - 0 = %g\n", nan - 0.0);
	printf("neg_nan - 0 = %g\n", neg_nan - 0.0);

	printf("nan - -0 = %g\n", nan - -0.0);
	printf("neg_nan - -0 = %g\n", neg_nan - -0.0);

	printf("nan * 1 = %g\n", nan * 1.0);
	printf("neg_nan * 1 = %g\n", neg_nan * 1.0);

	printf("nan / 1 = %g\n", nan / 1.0);
	printf("neg_nan / 1 = %g\n", neg_nan / 1.0);

	printf("nan * -1 = %g\n", nan * -1.0);
	printf("neg_nan * -1 = %g\n", neg_nan * -1.0);

	printf("nan / -1 = %g\n", nan / -1.0);
	printf("neg_nan / -1 = %g\n", neg_nan / -1.0);

	return 0;
}
