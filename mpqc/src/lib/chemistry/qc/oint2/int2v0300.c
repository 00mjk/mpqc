#include <chemistry/qc/intv2/genbuild.h>
int int2v0300(){
/* the cost is 140 */
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
double t19;
double t20;
double t21;
double t22;
t1=int_v_W0-int_v_p120;
t2=t1*int_v_list.dp[0][0][2][0];
t3=int_v_p120-int_v_r10;
t4=t3*int_v_list.dp[0][0][1][0];
t5=t4+t2;
t2=int_v_ooze*2;
t4=int_v_zeta34*t2;
t2=int_v_oo2zeta12*t4;
t4=(-1)*t2;
t2=t4*t5;
t6=t1*int_v_list.dp[0][0][1][0];
t7=t3*int_v_list.dp[0][0][0][0];
t8=t7+t6;
int_v_list.dp[1][0][0][2]=t8;
t6=int_v_oo2zeta12*2;
t7=t6*t8;
t9=t7+t2;
t2=int_v_zeta34*int_v_ooze;
t7=int_v_oo2zeta12*t2;
t2=(-1)*t7;
t7=t2*int_v_list.dp[0][0][2][0];
t10=int_v_oo2zeta12*int_v_list.dp[0][0][1][0];
t11=t10+t7;
t7=t1*int_v_list.dp[0][0][3][0];
t10=t3*int_v_list.dp[0][0][2][0];
t12=t10+t7;
t7=t1*t12;
t10=t7+t11;
t7=t3*t5;
t13=t7+t10;
t7=t1*t13;
t10=t7+t9;
t7=t2*int_v_list.dp[0][0][1][0];
t9=int_v_oo2zeta12*int_v_list.dp[0][0][0][0];
t14=t9+t7;
t7=t1*t5;
t1=t7+t14;
t7=t3*t8;
t9=t7+t1;
int_v_list.dp[2][0][0][5]=t9;
t1=t3*t9;
t3=t1+t10;
int_v_list.dp[3][0][0][9]=t3;
t1=int_v_W2-int_v_p122;
t7=t1*t13;
t10=int_v_p122-int_v_r12;
t15=t10*t9;
t16=t15+t7;
int_v_list.dp[3][0][0][8]=t16;
t7=int_v_W1-int_v_p121;
t15=t13*t7;
t13=int_v_p121-int_v_r11;
t17=t13*t9;
t9=t17+t15;
int_v_list.dp[3][0][0][7]=t9;
t15=t2*t5;
t17=int_v_oo2zeta12*t8;
t18=t17+t15;
t15=t1*t12;
t17=t10*t5;
t19=t17+t15;
t15=t1*t19;
t17=t15+t18;
t15=t1*t5;
t19=t10*t8;
t20=t19+t15;
int_v_list.dp[2][0][0][4]=t20;
t15=t10*t20;
t19=t15+t17;
int_v_list.dp[3][0][0][6]=t19;
t15=t7*t12;
t12=t13*t5;
t17=t12+t15;
t12=t1*t17;
t15=t7*t5;
t5=t13*t8;
t8=t5+t15;
int_v_list.dp[2][0][0][3]=t8;
t5=t10*t8;
t15=t5+t12;
int_v_list.dp[3][0][0][5]=t15;
t5=t7*t17;
t12=t18+t5;
t5=t13*t8;
t8=t5+t12;
int_v_list.dp[3][0][0][4]=t8;
t5=t1*int_v_list.dp[0][0][2][0];
t12=t10*int_v_list.dp[0][0][1][0];
t17=t12+t5;
t5=t4*t17;
t12=t1*int_v_list.dp[0][0][1][0];
t18=t10*int_v_list.dp[0][0][0][0];
t20=t18+t12;
int_v_list.dp[1][0][0][1]=t20;
t12=t6*t20;
t18=t12+t5;
t5=t1*int_v_list.dp[0][0][3][0];
t12=t10*int_v_list.dp[0][0][2][0];
t21=t12+t5;
t5=t1*t21;
t12=t11+t5;
t5=t10*t17;
t21=t5+t12;
t5=t1*t21;
t12=t5+t18;
t5=t1*t17;
t17=t14+t5;
t5=t10*t20;
t18=t5+t17;
int_v_list.dp[2][0][0][2]=t18;
t5=t10*t18;
t17=t5+t12;
int_v_list.dp[3][0][0][3]=t17;
t5=t7*int_v_list.dp[0][0][2][0];
t12=t13*int_v_list.dp[0][0][1][0];
t18=t12+t5;
t5=t2*t18;
t2=t7*int_v_list.dp[0][0][1][0];
t12=t13*int_v_list.dp[0][0][0][0];
t20=t12+t2;
int_v_list.dp[1][0][0][0]=t20;
t2=int_v_oo2zeta12*t20;
t12=t2+t5;
t2=t7*int_v_list.dp[0][0][3][0];
t5=t13*int_v_list.dp[0][0][2][0];
t21=t5+t2;
t2=t1*t21;
t5=t10*t18;
t22=t5+t2;
t2=t1*t22;
t5=t2+t12;
t2=t1*t18;
t12=t10*t20;
t22=t12+t2;
int_v_list.dp[2][0][0][1]=t22;
t2=t10*t22;
t12=t2+t5;
int_v_list.dp[3][0][0][2]=t12;
t2=t7*t21;
t5=t11+t2;
t2=t13*t18;
t11=t2+t5;
t2=t1*t11;
t1=t7*t18;
t5=t14+t1;
t1=t13*t20;
t14=t1+t5;
int_v_list.dp[2][0][0][0]=t14;
t1=t10*t14;
t5=t1+t2;
int_v_list.dp[3][0][0][1]=t5;
t1=t4*t18;
t2=t6*t20;
t4=t2+t1;
t1=t7*t11;
t2=t1+t4;
t1=t13*t14;
t4=t1+t2;
int_v_list.dp[3][0][0][0]=t4;
return 1;}
