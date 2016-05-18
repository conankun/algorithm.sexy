/*
    COIN Core 2.0
    Author : ConanKun(JungHyun Kim, conankunioi@gmail.com)
    Date : July 23rd 2015
    Modified : November 7th 2015
    Modified 2nd : November 28th 2015

    Compile with this command : g++ -o judge judge.cc `mysql_config --cflags --libs`

    Interactive, subtask not implemented.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fstream>
#include <string>
#include <mysql/mysql.h>
#include "okcalls.h"
using namespace std;
#define STD_MB 1048576
#define MAX_SUBTASK 101
#define BUFFER_SIZE 512
#define OJ_WT0 0
#define OJ_WT1 1
#define OJ_CI 2
#define OJ_RI 3
#define OJ_AC 4 //AC
#define OJ_PE 5 //Presentation Error
#define OJ_WA 6 // Wrong Answer
#define OJ_TL 7 //TLE
#define OJ_ML 8 //MLE
#define OJ_OL 9
#define OJ_RE 10 //runtime error
#define OJ_CE 11 // compile error
#define OJ_CO 12
#define OJ_TR 13 //Trapped (System call error)
#define OJ_SJ 14 //SJ Compile error
#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi

#endif

int runid, lang, DEBUG;
int problem_type, time_limit, memory_limit,sj; //problem_type=0 : normal , 1 : subtask
int td;//normal
int subtasks, subtask_td[MAX_SUBTASK], subtask_score[MAX_SUBTASK];//subtask
char *problem_id,*user_id;

static char record_call=0;
const int call_array_size=512;
int call_counter[call_array_size]={0};

static char host_name[BUFFER_SIZE];
static char user_name[BUFFER_SIZE];
static char password [BUFFER_SIZE];
static char db_name  [BUFFER_SIZE];
static int port_number;
static MYSQL *conn;
static MYSQL_RES *res;
static MYSQL_ROW row;

inline int max(int a, int b) {
	if (a > b) return a;
	return b;
}
int executesql(const char * sql){

	if (mysql_real_query(conn,sql,strlen(sql))){
		if(DEBUG) printf("%s", mysql_error(conn));
		sleep(20);
		conn=NULL;
		return 1;
	} else
	  return 0;
}

int init_mysql(){
    if(conn==NULL){
	    sprintf(host_name,"127.0.0.1");
	    sprintf(user_name,"root");
	    sprintf(password, "~!coin0906!@");
	    sprintf(db_name, "coin");
	    port_number = 3306;
		conn=mysql_init(NULL);		// init the database connection
		/* connect the database */
		const char timeout=30;
		mysql_options(conn,MYSQL_OPT_CONNECT_TIMEOUT,&timeout);

		if(!mysql_real_connect(conn,host_name,user_name,password, db_name,port_number,0,0)){
			if(DEBUG) printf("%s", mysql_error(conn));
			sleep(2);
			return 1;
		} else {
			return 0;
	  	}
	}else{
	    return executesql("set names utf8");
	}
}

int close_mysql() {
	if(conn != NULL) {
		mysql_close(conn);
		return 1;
	} else return 0;
}
/*
0 requested
1 compiling
2 compile failed
3 grading
4 finished
*/
int status_update(int &run_id, int &status){
	char sql[BUFFER_SIZE];
	sprintf(sql,"UPDATE submit_status SET status=%d WHERE runid=%d LIMIT 1"
			,status, run_id);
	if (mysql_real_query(conn,sql,strlen(sql))){
		printf("%s",mysql_error(conn));
		return false;
	}else{
		if(mysql_affected_rows(conn)>0ul)
			return true;
		else
			return false;
	}

}

int submit_update(int &problem_id){
	char sql[BUFFER_SIZE];
	sprintf(sql,"UPDATE problems SET submit_total=submit_total+1 WHERE problem_id=%d", problem_id);
	if (mysql_real_query(conn,sql,strlen(sql))){
		printf("%s",mysql_error(conn));
		return false;
	}else{
		if(mysql_affected_rows(conn)>0ul)
			return true;
		else
			return false;
	}

}

int solved_update(int &problem_id){
	char sql[BUFFER_SIZE];
	sprintf(sql,"UPDATE problems SET solved_total=solved_total+1 WHERE problem_id=%d", problem_id);
	if (mysql_real_query(conn,sql,strlen(sql))){
		printf("%s",mysql_error(conn));
		return false;
	}else{
		if(mysql_affected_rows(conn)>0ul)
			return true;
		else
			return false;
	}

}

int finalize_mysql(int &run_id, int &time_elapsed, int &memory_used, int &score){
	char sql[BUFFER_SIZE];
	sprintf(sql,"UPDATE submit_status SET status=4,score=%d,time_elapsed=%d,memory_used=%d WHERE runid=%d LIMIT 1"
			,score,time_elapsed, memory_used, run_id);
	if (mysql_real_query(conn,sql,strlen(sql))){
		printf("%s",mysql_error(conn));
		return false;
	}else{
		if(mysql_affected_rows(conn)>0ul)
			return true;
		else
			return false;
	}

}

int execute_cmd(const char * fmt, ...) {
        char cmd[BUFFER_SIZE];

        int ret = 0;
        va_list ap;

        va_start(ap, fmt);
        vsprintf(cmd, fmt, ap);
        ret = system(cmd);
        va_end(ap);
        return ret;
}

void copy_shell_runtime(char * work_dir) {
        execute_cmd("mkdir -p %s/lib", work_dir);
        execute_cmd("mkdir -p %s/lib64", work_dir);
        execute_cmd("mkdir -p %s/bin", work_dir);
        execute_cmd("cp -r /lib/* %s/lib/", work_dir);
        execute_cmd("cp -r -a /lib/i386-linux-gnu %s/lib/", work_dir);
        execute_cmd("cp -r -a /lib/x86_64-linux-gnu %s/lib/", work_dir);
        execute_cmd("cp -r /lib64/* %s/lib64/", work_dir);
        execute_cmd("cp -r -a /lib32 %s/", work_dir);
        execute_cmd("cp -r /bin/busybox %s/bin/", work_dir);
        execute_cmd("ln -s /bin/busybox %s/bin/sh", work_dir);
        execute_cmd("cp -r /bin/bash %s/bin/bash", work_dir);
}

void copy_php_runtime(char * work_dir) {
        copy_shell_runtime(work_dir);
        execute_cmd("mkdir -p %s/usr", work_dir);
        execute_cmd("mkdir -p %s/usr/lib", work_dir);
        execute_cmd("cp -r /usr/lib/libedit* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/libdb* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/libgssapi_krb5* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/libkrb5* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/libk5crypto* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/*/libedit* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/*/libdb* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/*/libgssapi_krb5* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/*/libkrb5* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/*/libk5crypto* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/lib/libxml2* %s/usr/lib/", work_dir);
        execute_cmd("cp -r /usr/bin/php* %s/", work_dir);
        execute_cmd("chmod +rx %s/main.php", work_dir);
}
void copy_python_runtime(char * work_dir) {

        copy_shell_runtime(work_dir);
        execute_cmd("mkdir -p %s/usr/include", work_dir);
        execute_cmd("mkdir -p %s/usr/lib", work_dir);
        execute_cmd("cp /usr/bin/python* %s/", work_dir);
        execute_cmd("cp -a /usr/lib/python* %s/usr/lib/", work_dir);
        execute_cmd("cp -a /usr/include/python* %s/usr/include/", work_dir);
        execute_cmd("cp -a /usr/lib/libpython* %s/usr/lib/", work_dir);

}
void init_syscalls_limits(int lang) {
        int i;
        memset(call_counter, 0, sizeof(call_counter));

        if (lang <= 1 || lang == 13 || lang == 14) { // C & C++
                for (i = 0; i<call_array_size; i++) {
                        call_counter[i] = 0;
                }
                for (i = 0; LANG_CC[i]; i++) {
                        call_counter[LANG_CV[i]] = LANG_CC[i];
                }
        } else if (lang == 2) { // Pascal
                for (i = 0; LANG_PC[i]; i++)
                        call_counter[LANG_PV[i]] = LANG_PC[i];
        } else if (lang == 3) { // Java
                for (i = 0; LANG_JC[i]; i++)
                        call_counter[LANG_JV[i]] = LANG_JC[i];
        } else if (lang == 4) { // Ruby
                for (i = 0; LANG_RC[i]; i++)
                        call_counter[LANG_RV[i]] = LANG_RC[i];
        } else if (lang == 5) { // Bash
                for (i = 0; LANG_BC[i]; i++)
                        call_counter[LANG_BV[i]] = LANG_BC[i];
        }else if (lang == 6) { // Python
                for (i = 0; LANG_YC[i]; i++)
                        call_counter[LANG_YV[i]] = LANG_YC[i];
        }else if (lang == 7) { // php
            for (i = 0; LANG_PHC[i]; i++)
                    call_counter[LANG_PHV[i]] = LANG_PHC[i];
    }else if (lang == 8) { // perl
            for (i = 0; LANG_PLC[i]; i++)
                    call_counter[LANG_PLV[i]] = LANG_PLC[i];
    }else if (lang == 9) { // mono c#
            for (i = 0; LANG_CSC[i]; i++)
                    call_counter[LANG_CSV[i]] = LANG_CSC[i];
    }else if (lang==10){//objective c
	    for (i = 0; LANG_OC[i]; i++)
                    call_counter[LANG_OV[i]] = LANG_OC[i];
    }else if (lang==11){//free basic
	    for (i = 0; LANG_BASICC[i]; i++)
                    call_counter[LANG_BASICV[i]] = LANG_BASICC[i];
    }else if (lang==12){//free basic
	    for (i = 0; LANG_SC[i]; i++)
                    call_counter[LANG_SV[i]] = LANG_SC[i];
    }
}
void print_runtimeerror(char * err){
        FILE *ferr=fopen("error.txt","a+");
        fprintf(ferr,"Runtime Error:%s\n",err);
        fclose(ferr);
}
int get_proc_status(int pid, const char * mark) {
        FILE * pf;
        char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
        int ret = 0;
        sprintf(fn, "/proc/%d/status", pid);
        pf = fopen(fn, "r");
        int m = strlen(mark);
        while (pf && fgets(buf, BUFFER_SIZE - 1, pf)) {

                buf[strlen(buf) - 1] = 0;
                if (strncmp(buf, mark, m) == 0) {
                        sscanf(buf + m + 1, "%d", &ret);
						//if(DEBUG) printf("%s\n",buf);
                }

        }
     //  if(DEBUG) printf("PID : %d pf:%d ret:%d\n",pid, pf, ret);
        if (pf)
                fclose(pf);

        return ret;
}
int get_page_fault_mem(struct rusage & ruse, pid_t & pidApp) {
        //java use pagefault
        int m_vmpeak, m_vmdata, m_minflt;
        m_minflt = ruse.ru_minflt * getpagesize();
        if (0 && DEBUG) {
                m_vmpeak = get_proc_status(pidApp, "VmPeak:");
                m_vmdata = get_proc_status(pidApp, "VmData:");
                printf("VmPeak:%d KB VmData:%d KB minflt:%d KB\n", m_vmpeak, m_vmdata,
                                m_minflt >> 10);
        }
        return m_minflt;
}
long get_file_size(const char * filename) {
        struct stat f_stat;

        if (stat(filename, &f_stat) == -1) {
                return 0;
        }

        return (long) f_stat.st_size;
}
int compile(int lang) {

    int pid;
    /*
    lang
    0: gcc //ok
    1 : g++ //ok
    2 : pascal
    3 : java
    4 : ruby
    5 : linux shell
    6 : python
    7 : php //no
    8 : perl
    9 : gmcs(??)
    10 : Objective C
    11 : bas(??)
    12 : Objective C++
    13 : g++ 4.8(C++ 11)//ok
    14 : g++ 4.9(C++ 14)//ok
    {"gcc","-fobjc-arc","-framework","Foundation","main.m", "-o", "main"};
    */
    const char * CP_C[] = { "gcc","-Wall","-lm","--static", "-O2","-o", "main", "main.c",NULL }; //confirmed
    const char * CP_X[] = { "g++","-Wall","-lm","--static", "-O2","-o", "main", "main.cc",NULL }; //confirmed
    const char * CP_X48[] = { "g++-4.8","-Wall","-lm","--static", "-std=c++11","-O2","-o", "main", "main.cc",NULL }; //confirmed
    const char * CP_X49[] = { "g++-4.9","-Wall","-lm","--static", "-std=c++14","-O2","-o", "main", "main.cc",NULL }; //confirmed

    //const char * CP_X14[] = { "g++","-Wall", "-O2","-o", "main", "main.cc",NULL };
    //-std=c++11
    const char * CP_P[] = { "fpc", "main.pas", "-O2","-Co", "-Ct","-Ci", NULL };
    const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m", "main.java",NULL };

    const char * CP_R[] = { "ruby", "-c", "main.rb", NULL };
    const char * CP_B[] = { "chmod", "+rx", "main.sh", NULL };
    const char * CP_Y[] = { "python","-c","import py_compile; py_compile.compile(r'main.py')", NULL };
    const char * CP_PH[] = { "php", "-l","main.php", NULL };
    const char * CP_PL[] = { "perl","-c", "main.pl", NULL };
    const char * CP_CS[] = { "gmcs","-warn:0", "main.cs", NULL };
    const char * CP_OC[] = {"gcc","-fobjc-arc","-framework","Foundation","main.m", "-o", "main"};
    //const char * CP_OC[]={"gcc","-o","main","main.m","-fconstant-string-class=NSConstantString","-I","/usr/include/GNUstep/","-L","/usr/lib/GNUstep/Libraries/","-lobjc","-lgnustep-base",NULL};
    const char * CP_BS[]={"fbc","main.bas",NULL};
    const char * CP_OCP[] = {"g++","-fobjc-arc","-framework","Foundation","main.m", "-o", "main"};
    char javac_buf[4][16];
  /*  char *CP_J[5];
    for(int i=0;i<4;i++) CP_J[i]=javac_buf[i];
        sprintf(CP_J[0],"javac");
        sprintf(CP_J[1],"-J%s",java_xms);
        sprintf(CP_J[2],"-J%s",java_xmx);
        sprintf(CP_J[3],"Main.java");
        CP_J[4]=(char *)NULL;*/

        pid = fork();
        if (pid == 0) {
                struct rlimit LIM;
                LIM.rlim_max = 60;
                LIM.rlim_cur = 60;
                setrlimit(RLIMIT_CPU, &LIM);
                alarm(60);
                LIM.rlim_max = 900 * STD_MB;
                LIM.rlim_cur = 900 * STD_MB;
                setrlimit(RLIMIT_FSIZE, &LIM);

                LIM.rlim_max =  STD_MB<<11;
                LIM.rlim_cur =  STD_MB<<11;
                setrlimit(RLIMIT_AS, &LIM);
                if (lang != 2&& lang != 11) {
                        freopen("ce.txt", "w", stderr);
                        //freopen("/dev/null", "w", stdout);
                } else {
                        freopen("ce.txt", "w", stdout);
                }
                switch (lang) {
                case 0:
                        execvp(CP_C[0], (char * const *) CP_C);
                        break;
                case 1:
                        execvp(CP_X[0], (char * const *) CP_X);
                        break;
                case 2:
                        execvp(CP_P[0], (char * const *) CP_P);
                        break;
                case 3:
                        execvp(CP_J[0], (char * const *) CP_J);
                        break;
                case 4:
                        execvp(CP_R[0], (char * const *) CP_R);
                        break;
                case 5:
                        execvp(CP_B[0], (char * const *) CP_B);
                        break;
                case 6:
                        execvp(CP_Y[0], (char * const *) CP_Y);
                        break;
                case 7:
                        execvp(CP_PH[0], (char * const *) CP_PH);
                        break;
                case 8:
                        execvp(CP_PL[0], (char * const *) CP_PL);
                        break;
                case 9:
                        execvp(CP_CS[0], (char * const *) CP_CS);
                        break;

                case 10:
                        execvp(CP_OC[0], (char * const *) CP_OC);
                        break;
                case 11:
                        execvp(CP_BS[0], (char * const *) CP_BS);
                        break;
                case 12:
                        execvp(CP_OCP[0], (char * const *) CP_OCP);
                        break;
				case 13:
						execvp(CP_X48[0], (char * const *)CP_X48);
						break;
				case 14:
						execvp(CP_X49[0], (char * const *)CP_X49);
						break;

				default:
                        printf("nothing to do!\n");
                }
                //if (DEBUG)
                        //printf("compile end!\n");
                //exit(!system("cat ce.txt"));
				close_mysql();
                exit(0);
        } else {
                int status=0;

                waitpid(pid, &status, 0);
                if(lang>3&&lang<7)
                        status=get_file_size("ce.txt");
                //if (DEBUG)
                  //      printf("status=%d\n", status);
                return status;
        }

}

int compile_sj(int lang) {

    int pid;
    /*
    lang
    0: gcc
    1 : g++
    2 : pascal
    3 : java
    4 : ruby
    5 : linux shell
    6 : python
    7 : php
    8 : perl
    9 : gmcs(??)
    10 : Objective C
    11 : bas(??)
    */
    char work_dir[BUFFER_SIZE];
    sprintf(work_dir,"cp ../../problems/%s/sj.cc sj.cc",problem_id);
    system(work_dir);

    const char * CP_C[] = { "gcc","-Wall", "-O2","-o", "sj", "sj.c",NULL }; //confirmed
    const char * CP_X[] = { "g++","-Wall", "-O2","-o", "sj", "sj.cc",NULL }; //confirmed
    //const char * CP_X14[] = { "g++","-Wall", "-O2","-o", "sj", "sj.cc",NULL };
    //-std=c++11
    const char * CP_P[] = { "fpc", "sj.pas", "-O2","-Co", "-Ct","-Ci", NULL };
    const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m", "sj.java",NULL };

    const char * CP_R[] = { "ruby", "-c", "sj.rb", NULL };
    const char * CP_B[] = { "chmod", "+rx", "sj.sh", NULL };
    const char * CP_Y[] = { "python","-c","import py_compile; py_compile.compile(r'sj.py')", NULL };
    const char * CP_PH[] = { "php", "-l","sj.php", NULL };
    const char * CP_PL[] = { "perl","-c", "sj.pl", NULL };
    const char * CP_CS[] = { "gmcs","-warn:0", "sj.cs", NULL };
    const char * CP_OC[]={"gcc","-o","sj","sj.m","-fconstant-string-class=NSConstantString","-I","/usr/include/GNUstep/","-L","/usr/lib/GNUstep/Libraries/","-lobjc","-lgnustep-base",NULL};
    const char * CP_BS[]={"fbc","sj.bas",NULL};
    char javac_buf[4][16];
  /*  char *CP_J[5];
    for(int i=0;i<4;i++) CP_J[i]=javac_buf[i];
        sprintf(CP_J[0],"javac");
        sprintf(CP_J[1],"-J%s",java_xms);
        sprintf(CP_J[2],"-J%s",java_xmx);
        sprintf(CP_J[3],"sj.java");
        CP_J[4]=(char *)NULL;*/

        pid = fork();
        if (pid == 0) {
                struct rlimit LIM;
                LIM.rlim_max = 60;
                LIM.rlim_cur = 60;
                setrlimit(RLIMIT_CPU, &LIM);
                alarm(60);
                LIM.rlim_max = 900 * STD_MB;
                LIM.rlim_cur = 900 * STD_MB;
                setrlimit(RLIMIT_FSIZE, &LIM);

                LIM.rlim_max =  STD_MB<<11;
                LIM.rlim_cur =  STD_MB<<11;
                setrlimit(RLIMIT_AS, &LIM);
                if (lang != 2&& lang != 11) {
                        freopen("ce2.txt", "w", stderr);
                        //freopen("/dev/null", "w", stdout);
                } else {
                        freopen("ce2.txt", "w", stdout);
                }
                switch (lang) {
                case 0:
                        execvp(CP_C[0], (char * const *) CP_C);
                        break;
                case 1:
                        execvp(CP_X[0], (char * const *) CP_X);
                        break;
                case 2:
                        execvp(CP_P[0], (char * const *) CP_P);
                        break;
                case 3:
                        execvp(CP_J[0], (char * const *) CP_J);
                        break;
                case 4:
                        execvp(CP_R[0], (char * const *) CP_R);
                        break;
                case 5:
                        execvp(CP_B[0], (char * const *) CP_B);
                        break;
                case 6:
                        execvp(CP_Y[0], (char * const *) CP_Y);
                        break;
                case 7:
                        execvp(CP_PH[0], (char * const *) CP_PH);
                        break;
                case 8:
                        execvp(CP_PL[0], (char * const *) CP_PL);
                        break;
                case 9:
                        execvp(CP_CS[0], (char * const *) CP_CS);
                        break;

                case 10:
                        execvp(CP_OC[0], (char * const *) CP_OC);
                        break;
                case 13:
                        execvp(CP_BS[0], (char * const *) CP_BS);
                        break;
        default:
                        printf("nothing to do!\n");
                }
                //if (DEBUG)
                        //printf("compile end!\n");
                //exit(!system("cat ce.txt"));
                close_mysql();
                exit(0);
        } else {
                int status=0;

                waitpid(pid, &status, 0);
                if(lang>3&&lang<7)
                        status=get_file_size("ce2.txt");
                //if (DEBUG)
                  //      printf("status=%d\n", status);
                return status;
        }

}
void fetch_problem_info() {
    /*
        int problem_type, time_limit, memory_limit,sj;
        int subtasks, subtask_td[MAX_SUBTASK], subtask_score[MAX_SUBTASK];
    */
    char work_dir[BUFFER_SIZE];
    sprintf(work_dir,"../../problems/%s/info.txt",problem_id);
    FILE *fp = fopen(work_dir,"r");
    if(!fp) {
        if(DEBUG) printf("Unable to fetch problem information - %s\n",work_dir);
        close_mysql();
        exit(0);
    }
    char isST[BUFFER_SIZE];
    fscanf(fp,"%s\n",isST);
    fscanf(fp,"%d",&time_limit);
    fscanf(fp,"%d",&memory_limit);
    fscanf(fp,"%d",&sj);
    if(strcmp(isST,"subtask") == 0) {
        problem_type=1;
        fscanf(fp,"%d",&subtasks);
        if(subtasks >= MAX_SUBTASK) {
            if(DEBUG) printf("the number of subtasks cannot exceed 100\n");
            if(fp) fclose(fp);
            close_mysql();
            exit(0);
        }
        for(int i=1;i<=subtasks;i++) {
            fscanf(fp,"%d",subtask_td+i);
            fscanf(fp,"%d",subtask_score+i);
        }
    } else {
        //exception:atoi
        problem_type=0;
        td = atoi(isST);
    }
    if(fp) fclose(fp);
}
void prepare_files(int num,int subtask) {
    //cp input,output to judging dir.
    // num of test data, subtask -> subtask of test data
    char work_dir[BUFFER_SIZE];
    if(subtask) {
        //Input File
        sprintf(work_dir,"../../problems/%s/subtask%d/input%d.txt",problem_id,subtask,num);
        FILE *fp = fopen(work_dir,"r");
        if(!fp) {
            if(DEBUG) printf("Input File doesn't exist : %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            fclose(fp);
        }
        sprintf(work_dir,"cp ../../problems/%s/subtask%d/input%d.txt input.txt",problem_id,subtask,num);
        system(work_dir);

        fp = fopen("input.txt","r");
        if(!fp) {
            if(DEBUG) printf("Input file Copy UnSuccessful - %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            //if(DEBUG) printf("Input file Copy Successful - %s\n",work_dir);
            fclose(fp);
        }

        //Output File
        sprintf(work_dir,"../../problems/%s/subtask%d/output%d.txt",problem_id,subtask,num);
        fp = fopen(work_dir,"r");
        if(!fp) {
            if(DEBUG) printf("Output File doesn't exist : %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            fclose(fp);
        }
        sprintf(work_dir,"cp ../../problems/%s/subtask%d/output%d.txt answer.txt",problem_id,subtask,num);
        system(work_dir);

        fp = fopen("answer.txt","r");
        if(!fp) {
            if(DEBUG) printf("Output file Copy UnSuccessful - %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            //if(DEBUG) printf("Output file Copy Successful - %s\n",work_dir);
            fclose(fp);
        }
    } else {
        //Input File
        sprintf(work_dir,"../../problems/%s/input%d.txt",problem_id,num);
        FILE *fp = fopen(work_dir,"r");
        if(!fp) {
            if(DEBUG) printf("Input File doesn't exist : %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            fclose(fp);
        }
        sprintf(work_dir,"cp ../../problems/%s/input%d.txt input.txt",problem_id,num);
        system(work_dir);

        fp = fopen("input.txt","r");
        if(!fp) {
            if(DEBUG) printf("Input file Copy UnSuccessful - %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            //if(DEBUG) printf("Input file Copy Successful - %s\n",work_dir);
            fclose(fp);
        }

        //Output File
        sprintf(work_dir,"../../problems/%s/output%d.txt",problem_id,num);
        fp = fopen(work_dir,"r");
        if(!fp) {
            if(DEBUG) printf("Output File doesn't exist : %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
            fclose(fp);
        }
        sprintf(work_dir,"cp ../../problems/%s/output%d.txt answer.txt",problem_id,num);
        system(work_dir);

        fp = fopen("answer.txt","r");
        if(!fp) {
            if(DEBUG) printf("Output file Copy UnSuccessful - %s\n",work_dir);
            close_mysql();
            exit(0);
        } else {
        //  if(DEBUG) printf("Output file Copy Successful - %s\n",work_dir);
            fclose(fp);
        }
    }
}
void run_solution(char *work_dir) {
    // open the files
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    freopen("error.txt", "a+", stderr);
	ptrace(PTRACE_TRACEME,0,NULL,NULL);
    //set limits
    struct rlimit LIM; // time limit, file limit& memory limit
    //time
    LIM.rlim_cur = time_limit/1000+10;
    setrlimit(RLIMIT_CPU, &LIM);

    //memory
    LIM.rlim_cur = STD_MB *memory_limit/2*3;
    LIM.rlim_max = STD_MB *memory_limit*2;

  //  if(lang<3)
  //      setrlimit(RLIMIT_AS, &LIM);

  if(lang != 3)
    chroot(work_dir);

	alarm(0);//pending -> cancel
	//ualarm(time_limit*1000,0);
    alarm((time_limit/1000)+1);
    switch (lang) {
        case 0:
        case 1:
        case 2:
        case 10:
        case 11:
        case 13:
        case 14:
                execl("./main", "./main", (char *)NULL);
                break;
        case 3:
//              sprintf(java_p1, "-Xms%dM", mem_lmt / 2);
//              sprintf(java_p2, "-Xmx%dM", mem_lmt);

                execl("/usr/bin/java", "/usr/bin/java", "-Xms32m","-Xmx256m",
                                "-Djava.security.manager",
                                "-Djava.security.policy=../etc/java0.policy", "main", (char *)NULL);
                break;
        case 4:
                //system("/ruby Main.rb<data.in");
                execl("/ruby","/ruby", "main.rb", (char *)NULL);
                break;
        case 5: //bash
                execl("/bin/bash","/bin/bash" ,"main.sh",(char *)NULL);
                break;
        case 6: //Python
                execl("python","python","main.py",(char *)NULL);
                break;
        case 7: //php
                execl("php","php","main.php",(char *)NULL);
                break;
        case 8: //perl
                execl("/perl","/perl","main.pl",(char *)NULL);
                break;
        case 9: //Mono C#
                execl("/mono","/mono","--debug","main.exe",(char *)NULL);
                break;
        case 12: //guile
                execl("/guile","/guile","main.scm",(char *)NULL);
                break;

        }
        close_mysql();
        exit(0);
}
void watch_solution(pid_t pidApp,int &used_time,int &flag,int &memory_used) {
        int tempmemory;
        int topmemory=0;
        flag = OJ_AC;

        int status, sig, exitcode;
        struct user_regs_struct reg;
        struct rusage ruse;
        int sub = 0;
        while (1) {
		        wait4(pidApp, &status, 0, &ruse);
                // check the usage
                //jvm gc ask VM before need,so used kernel page fault times and page size
                if (lang == 3) {
                    tempmemory = get_page_fault_mem(ruse, pidApp);
                } else {//other use VmPeak
                    tempmemory = get_proc_status(pidApp, "VmPeak:") << 10;
                }

				used_time = (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
				used_time += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);

				if(flag == OJ_AC && used_time > time_limit) {
					flag=OJ_TL;
				}

                if (tempmemory > topmemory) {
                    topmemory = tempmemory;
                }
                memory_used = topmemory;
                if (topmemory > memory_limit * STD_MB) {
                        if (DEBUG)
                                printf("Process - Out of memory %d\n", topmemory);
                        if (flag == OJ_AC)
                                flag = OJ_ML;
                        ptrace(PTRACE_KILL, pidApp, NULL, NULL);
                        break;
                }
                  //sig = status >> 8;/*status >> 8 Ã¥Â·Â®Ã¤Â¸ÂÃ¥Â¤Å¡Ã¦ËÂ¯EXITCODE*/
				//if(DEBUG) printf("topmemory : %d limit : %d\n",topmemory,memory_limit*STD_MB);


                if (WIFEXITED(status)) {
                    break;
                }


                exitcode = WEXITSTATUS(status);
                /*exitcode == 5 waiting for next CPU allocation          * ruby using system to run,exit 17 ok
                 *  */
                if ((lang >= 3 && exitcode == 17) || exitcode == 0x05 || exitcode == 0)
                        //go on and on
                        ;
                else {

                        if (DEBUG) {
                                printf("status>>8=%d\n", exitcode);


                        }
                        psignal(exitcode, NULL);
                        if (flag == OJ_AC){
                                switch (exitcode) {
                                        case SIGCHLD:
                                        case SIGALRM:
                                                alarm(0);
                                        case SIGKILL:
                                        case SIGXCPU:
                                                flag = OJ_TL;
                                                break;
                                        case SIGXFSZ:
                                                flag = OJ_OL;
                                                break;
                                        case SIGTRAP:
                                                flag = OJ_TR;
                                                break;
                                        default:
                                                flag = OJ_RE;
                                }
                                print_runtimeerror(strsignal(exitcode));
                        }
                        ptrace(PTRACE_KILL, pidApp, NULL, NULL);

                        break;
                }
                if (WIFSIGNALED(status)) {
                        /*  WIFSIGNALED: if the process is terminated by signal
                         *
                         *  psignal(int sig, char *s)，like perror(char *s)，print out s, with error msg from system of sig
                         * sig = 5 means Trace/breakpoint trap
                         * sig = 11 means Segmentation fault
                         * sig = 25 means File size limit exceeded
                         */
                        sig = WTERMSIG(status);

                        if (DEBUG) {
                                printf("WTERMSIG=%d\n", sig);
                                psignal(sig, NULL);
                        }
                        if (flag == OJ_AC){
                                switch (sig) {
                                case SIGCHLD:
                                case SIGALRM:
                                     alarm(0);
                                case SIGKILL:
                                case SIGXCPU:
                                        flag = OJ_TL;
                                        break;
                                case SIGXFSZ:
                                        flag = OJ_OL;
                                        break;
                                case SIGTRAP:
                                        flag = OJ_TR;
                                        break;
                                default:
                                        flag = OJ_RE;
                                }
                                print_runtimeerror(strsignal(sig));
                        }
                        break;
                }
                /*     comment from http://www.felix021.com/blog/read.php?1662

                      WIFSTOPPED: return true if the process is paused or stopped while ptrace is watching on it
                      WSTOPSIG: get the signal if it was stopped by signal
                 */

                // check the system calls
                ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);


				/*if ((lang > 1 && lang != 13 && lang != 14) &&call_counter[reg.REG_SYSCALL] == 0) { //do not limit JVM syscall for using different JVM
                        flag = OJ_RE;
                        char error[BUFFER_SIZE];
                        sprintf(error,"[ERROR] A Not allowed system call: runid:%d callid:%ld\n",
                                        runid, reg.REG_SYSCALL);
                        print_runtimeerror(error);
                        ptrace(PTRACE_KILL, pidApp, NULL, NULL);

                }else if(record_call){
                        call_counter[reg.REG_SYSCALL]=1;
                } else {
                        if (sub == 1 && call_counter[reg.REG_SYSCALL] > 0)
                                call_counter[reg.REG_SYSCALL]--;
                }
                sub = 1 - sub;*/


                ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
        }

}
void validate_answer(int &flag) {
    if(sj) {
        //only cpp judges are allowed.
        int compile_code = compile_sj(1);
        if(compile_code != 0) {
            if(DEBUG) printf("SJ file Compilation failed\n");
            close_mysql();
            exit(0);
        }
        pid_t pid_s = fork();
        if(pid_s == 0) {
            alarm(0);
            alarm(2);
            execl("./sj", "./sj", (char *)NULL);

        } else {
            int statss=0;
            waitpid(pid_s,&statss,0);
            if (WIFEXITED(statss)) {
  //              if(DEBUG) printf("%s\n", (WEXITSTATUS(statss) == 0 ? "AC" : "WA"));
                if(WEXITSTATUS(statss) != 0) flag=OJ_WA;
            }
        }
    } else {
        //strict / loose
        //load from db*****
        int mode=1;//0->strict / 1->loose
        if(mode == 0) {
            ifstream f1("output.txt");
            ifstream f2("answer.txt");
            f1.sync_with_stdio(false);
            f2.sync_with_stdio(false);
            if(!(f1.is_open() & f2.is_open())) {
                printf("filestream error\n");
                if(f2.is_open()) f2.close();
                if(f1.is_open())f1.close();
                close_mysql();
                exit(0);
            }
            bool tr=true;
            string str1,str2;
            while(getline(f1,str1)) {
                getline(f2,str2);
                if(str1 != str2) {
	                printf("expected : %s\nactual : %s\n",str2.c_str(),str1.c_str());
                    tr=false;
                    break;
                }
            }
            while(f2>>str2) {
	            printf("More printed than expected.\n");
                //f2에 찌꺼기가 더 출력되었다면
                tr=false;
                break;
            }
            if(f2.is_open()) f2.close();
            if(f1.is_open())f1.close();
            if(!tr) {
                flag = OJ_WA;
            }
        } else if(mode == 1) {
            ifstream f1("output.txt");
            ifstream f2("answer.txt");
            f1.sync_with_stdio(false);
            f2.sync_with_stdio(false);
            if(!(f1.is_open() & f2.is_open())) {
                printf("filestream error\n");
                if(f2.is_open()) f2.close();
                if(f1.is_open())f1.close();
                close_mysql();
                exit(0);
            }
            bool tr=true;
            string str1,str2;
            while(f1>>str1) {
                f2>>str2;
                if(str1 != str2) {
	                printf("expected : %s\nactual : %s\n",str2.c_str(),str1.c_str());
                    tr=false;
                    break;
                }
            }
            while(f2>>str2) {
	            printf("More printed than expected.\n");
                //f2에 찌꺼기가 더 출력되었다면
                tr=false;
                break;
            }
            if(f2.is_open()) f2.close();
            if(f1.is_open())f1.close();
            if(!tr) {
                flag = OJ_WA;
            }
        } else {
            printf("Undefined grading mode\n");
            close_mysql();
            exit(0);
        }
    }
}
int main(int argc, char *argv[]) {
    //argv[1] -> run-id
    //argv[2] -> language
    //argv[3] -> problem-id
    //argv[4] -> user-id
    //argv[5] -> DEBUG
    if(argc < 6) {
        if(DEBUG) printf("Initialization failed - few arguments\n");
        close_mysql();
        exit(0);
    }
    //load data
    //exception:atoi
    runid = atoi(argv[1]);
    lang = atoi(argv[2]);
    problem_id = argv[3];
    user_id = argv[4];
    DEBUG = atoi(argv[5]);

    char work_dir[BUFFER_SIZE];
    //change permissions, ******
    sprintf(work_dir,"RUN/%d",runid);
    chdir(work_dir);

	init_mysql();
	int stat = 1;
	status_update(runid, stat);

    //compile
    int compile_code = compile(lang);
    if(compile_code != 0) {
        //compile failed
        if(DEBUG) {
            printf("Compilation failed\n");
            system("cat ce.txt");
        }
        stat=2;
        status_update(runid, stat);
        close_mysql();
        exit(0);
    }
    if(DEBUG) printf("Compilation succeded\n");
    //fetch data
    fetch_problem_info();
    if(DEBUG) {
        printf("Type:%s Time:%dms Memory:%dMB SJ:%s\n",(problem_type == 0 ? "normal" : "subtask"),time_limit,memory_limit,(sj == 0 ? "No":"Yes"));
        if(problem_type == 0) {
            //normal
            printf("# TD : %d\n",td);
        } else if(problem_type == 1) {
            printf("# of subtasks : %d\n",subtasks);
            for(int i=1;i<=subtasks;i++) {
                printf("Subtask #%d :: # TD : %d -- Score : %d\n",i,subtask_td[i],subtask_score[i]);
            }
        }
    }

	stat=3;
	status_update(runid, stat);
	int problem_id_int = atoi(problem_id);
	submit_update(problem_id_int);
    //run
    if(problem_type == 0) {
        int right=0;
        sprintf(work_dir,"/home/coin/www/SERVER/RUN/%d",runid);
		if(lang == 6) copy_python_runtime(work_dir);
		else if(lang == 7) copy_php_runtime(work_dir);
		int total_time = 0;
		int total_memory = 0;
        for(int i=1;i<=td;i++) {
            printf("==========Data #%d==========\n",i);
            int used_time=0,flag=0,memory_used;
            prepare_files(i,0);
            init_syscalls_limits(lang);
            pid_t pidApp = fork();
            if(pidApp == 0) {
                run_solution(work_dir);
            } else {
                watch_solution(pidApp,used_time,flag,memory_used);
                if(flag == 7) used_time = time_limit;
                if(flag == 4) validate_answer(flag);
                string judged="";
                if(flag == 4) judged="Good";
                else if(flag == 6) judged="Wrong";
                else if(flag == 7) judged="Time Limit Exceeded";
                else if(flag == 8) judged="Memory Limit Exceeded";
                else if(flag == 10) judged="Runtime Error";
                printf("Running time : %d / Memory used : %dKB / flag:%d / Judged : %s\n",used_time,memory_used>>10,flag,judged.c_str());
                if(flag == OJ_AC) right++;
                total_time = max(used_time, total_time);
                total_memory = max(memory_used>>10, total_time);
				char sql[BUFFER_SIZE*5];
                sprintf(sql, "insert into submit_status_detail values(NULL, %d, 0, %d, %d, %d, %d, '')",runid, i, used_time, memory_used>>10, flag);
                executesql(sql);

            }
        }
        int total_score = right/(double)td*100;
        finalize_mysql(runid, total_time, total_memory, total_score);
        printf("Total Score : %.2lf\n",right/(double)td*100);
        if(total_score == 100) {
	        solved_update(problem_id_int);
        }
    } else if(problem_type == 1) {

    } else {
        if(DEBUG) printf("error - problem type doesn't exist\n");
        close_mysql();
        exit(0);
    }

	stat=4;
	status_update(runid, stat);

	close_mysql();

    return 0;
}
/*
-- g++-4.9 installation --
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install g++-4.9
-- end --
*/
