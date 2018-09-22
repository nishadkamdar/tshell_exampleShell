#include <stdio.h>
#include <stdlib.h>

void *xcalloc(int nr_elements, int size_per_element)
{
	void *tmp;

	tmp = calloc(nr_elements, size_per_element);

	if (!tmp)
	{
		fprintf(stderr, "xcalloc:fatal:out of memory\n");
		exit(EXIT_FAILURE);
	}

	return(tmp);
}
