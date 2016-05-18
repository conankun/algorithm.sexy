#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
int str[211111];
int main() {
	int a,b;
	int i;
	for(i=1;i<=200000;i++) str[i]=i;
//	while(1);
 	//for(;;) fork();
	//while(1);
	scanf("%d%d",&a,&b);
	printf("%d",a+b);
}
