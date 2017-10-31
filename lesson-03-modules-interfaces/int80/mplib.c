#include "common.c"

void do_write(void)
{
	char *str = "эталонная строка для вывода!\n";
	int len = strlen(str) + 1, n;
	printf("string for write length = %d\n", len);
	n = write(1, str, len);
	if (n >= 0)
		printf("write return : %d\n", n);
	else
		printf("write error : %m\n");
}

void do_mknod(void)
{
	char *nam = "ZZZ";
	int n = mknod(nam, S_IFCHR | S_IRUSR | S_IWUSR, MKDEV(247, 0));
	if (n >= 0)
		printf("mknod return : %d\n", n);
	else
		printf("mknod error : %m\n");
}

void do_getpid(void)
{
	int n = getpid();
	if (n >= 0)
		printf("getpid return : %d\n", n);
	else
		printf("getpid error : %m\n");
}
