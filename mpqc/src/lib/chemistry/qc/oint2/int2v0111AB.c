#include <chemistry/qc/intv2/genbuild.h>
int int2v0111eAB(){
/* the cost is 41 */
double t1;
double t2;
double t3;
double t4;
double t5;
double t6;
double t7;
double t8;
double t9;
double t10;
double t11;
double t12;
double t13;
double t14;
double t15;
double t16;
double t17;
double t18;
t1=0.5*int_v_ooze;
t2=t1*int_v_list.dp[0][0][1][0];
t1=int_v_W0-int_v_p340;
t3=t1*int_v_list.dp[0][0][2][0];
t4=int_v_p340-int_v_r30;
t5=t4*int_v_list.dp[0][0][1][0];
t6=t5+t3;
t3=int_v_W0-int_v_p120;
t5=t3*t6;
t7=t5+t2;
int_v_list.dp[1][1][0][8]=t7;
t5=int_v_W2-int_v_p342;
t8=t5*int_v_list.dp[0][0][2][0];
t9=int_v_p342-int_v_r32;
t10=t9*int_v_list.dp[0][0][1][0];
t11=t10+t8;
t8=t3*t11;
int_v_list.dp[1][1][0][7]=t8;
t10=int_v_W1-int_v_p341;
t12=t10*int_v_list.dp[0][0][2][0];
t13=int_v_p341-int_v_r31;
t14=t13*int_v_list.dp[0][0][1][0];
t15=t14+t12;
t12=t3*t15;
int_v_list.dp[1][1][0][6]=t12;
t3=int_v_W2-int_v_p122;
t14=t3*t6;
int_v_list.dp[1][1][0][5]=t14;
t16=t3*t11;
t17=t2+t16;
int_v_list.dp[1][1][0][4]=t17;
t16=t3*t15;
int_v_list.dp[1][1][0][3]=t16;
t3=int_v_W1-int_v_p121;
t18=t6*t3;
int_v_list.dp[1][1][0][2]=t18;
t6=t3*t11;
int_v_list.dp[1][1][0][1]=t6;
t11=t3*t15;
t3=t2+t11;
int_v_list.dp[1][1][0][0]=t3;
t2=t1*int_v_list.dp[0][0][1][0];
t1=t4*int_v_list.dp[0][0][0][0];
t4=t1+t2;
int_v_list.dp[0][1][0][2]=t4;
t1=t5*int_v_list.dp[0][0][1][0];
t2=t9*int_v_list.dp[0][0][0][0];
t5=t2+t1;
int_v_list.dp[0][1][0][1]=t5;
t1=t10*int_v_list.dp[0][0][1][0];
t2=t13*int_v_list.dp[0][0][0][0];
t9=t2+t1;
int_v_list.dp[0][1][0][0]=t9;
return 1;}
