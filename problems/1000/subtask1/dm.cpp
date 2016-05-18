#include<stdio.h>
#include<stdlib.h>
#include<time.h>
int main() {
	srand(time(NULL));
	for(int i=1;i<=1000;i++) {
		int a= rand() % 1000000000;
		int b= rand() % 1000000000;
		char s[1000]="";
		sprintf(s,"input%d.txt",i);
		freopen(s,"w",stdout);
		printf("%d %d",a,b);
		sprintf(s,"output%d.txt",i);
		freopen(s,"w",stdout);
		printf("%d",a+b);
	}
}