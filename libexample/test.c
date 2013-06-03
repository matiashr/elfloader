
int main()
{
	return 1;
}

void pstr(char* data );
void listTasks();

int _runmain(int a, int b)
{
#if 0
	int i;
	int retv;
	for(i=0;i < 10000;i++ ) {
		if( a < i ) 
			b++;
	}
	retv=a*i-b;
#endif
	listTasks();
	return 0;
}

