#include <chemistry/qc/intv2/genbuild.h>
int int2v0201(){
/* the cost is 231 */
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
double t23;
double t24;
double t25;
double t26;
double t27;
double t28;
double t29;
double t30;
double t31;
double t32;
double t33;
double t34;
double t35;
double t36;
double t37;
double t38;
double t39;
double t40;
double t41;
double t42;
t1=int_v_W0-int_v_p120;
t2=t1*int_v_list.dp[0][0][2][0];
t3=int_v_p120-int_v_r10;
t4=t3*int_v_list.dp[0][0][1][0];
t5=t4+t2;
t2=0.5*int_v_ooze;
t4=t2*t5;
t6=int_v_W0-int_v_p340;
t7=t6*int_v_list.dp[0][0][2][0];
t8=int_v_p340-int_v_r30;
t9=t8*int_v_list.dp[0][0][1][0];
t10=t9+t7;
t7=int_v_zeta34*int_v_ooze;
t9=int_v_oo2zeta12*t7;
t7=(-1)*t9;
t9=t7*t10;
t11=t9+t4;
t12=t6*int_v_list.dp[0][0][1][0];
t13=t8*int_v_list.dp[0][0][0][0];
t14=t13+t12;
int_v_list.dp[0][1][0][2]=t14;
t12=int_v_oo2zeta12*t14;
t13=t12+t11;
t11=t2*int_v_list.dp[0][0][2][0];
t15=t6*int_v_list.dp[0][0][3][0];
t6=t8*int_v_list.dp[0][0][2][0];
t8=t6+t15;
t6=t1*t8;
t15=t6+t11;
t6=t3*t10;
t16=t6+t15;
t6=t1*t16;
t15=t6+t13;
t6=t2*int_v_list.dp[0][0][1][0];
t13=t1*t10;
t17=t13+t6;
t13=t3*t14;
t18=t13+t17;
int_v_list.dp[1][1][0][8]=t18;
t13=t3*t18;
t17=t13+t15;
int_v_list.dp[2][1][0][17]=t17;
t13=int_v_W2-int_v_p342;
t15=t13*int_v_list.dp[0][0][2][0];
t19=int_v_p342-int_v_r32;
t20=t19*int_v_list.dp[0][0][1][0];
t21=t20+t15;
t15=t7*t21;
t20=t13*int_v_list.dp[0][0][1][0];
t22=t19*int_v_list.dp[0][0][0][0];
t23=t22+t20;
int_v_list.dp[0][1][0][1]=t23;
t20=int_v_oo2zeta12*t23;
t22=t20+t15;
t24=t13*int_v_list.dp[0][0][3][0];
t13=t19*int_v_list.dp[0][0][2][0];
t19=t13+t24;
t13=t1*t19;
t24=t3*t21;
t25=t24+t13;
t13=t1*t25;
t24=t13+t22;
t13=t1*t21;
t26=t3*t23;
t27=t26+t13;
int_v_list.dp[1][1][0][7]=t27;
t13=t3*t27;
t26=t13+t24;
int_v_list.dp[2][1][0][16]=t26;
t13=int_v_W1-int_v_p341;
t24=t13*int_v_list.dp[0][0][2][0];
t28=int_v_p341-int_v_r31;
t29=t28*int_v_list.dp[0][0][1][0];
t30=t29+t24;
t24=t7*t30;
t29=t13*int_v_list.dp[0][0][1][0];
t31=t28*int_v_list.dp[0][0][0][0];
t32=t31+t29;
int_v_list.dp[0][1][0][0]=t32;
t29=int_v_oo2zeta12*t32;
t31=t29+t24;
t33=t13*int_v_list.dp[0][0][3][0];
t13=t28*int_v_list.dp[0][0][2][0];
t28=t13+t33;
t13=t1*t28;
t33=t3*t30;
t34=t33+t13;
t13=t1*t34;
t33=t13+t31;
t13=t1*t30;
t35=t3*t32;
t36=t35+t13;
int_v_list.dp[1][1][0][6]=t36;
t13=t3*t36;
t35=t13+t33;
int_v_list.dp[2][1][0][15]=t35;
t13=int_v_W2-int_v_p122;
t33=t13*t16;
t37=int_v_p122-int_v_r12;
t38=t37*t18;
t39=t38+t33;
int_v_list.dp[2][1][0][14]=t39;
t33=t13*t25;
t38=t4+t33;
t33=t37*t27;
t40=t33+t38;
int_v_list.dp[2][1][0][13]=t40;
t33=t13*t34;
t38=t37*t36;
t41=t38+t33;
int_v_list.dp[2][1][0][12]=t41;
t33=int_v_W1-int_v_p121;
t38=t16*t33;
t16=int_v_p121-int_v_r11;
t42=t16*t18;
t18=t42+t38;
int_v_list.dp[2][1][0][11]=t18;
t38=t33*t25;
t25=t16*t27;
t27=t25+t38;
int_v_list.dp[2][1][0][10]=t27;
t25=t33*t34;
t34=t4+t25;
t4=t16*t36;
t25=t4+t34;
int_v_list.dp[2][1][0][9]=t25;
t4=t12+t9;
t9=t13*t8;
t12=t37*t10;
t34=t12+t9;
t9=t13*t34;
t12=t9+t4;
t9=t13*t10;
t34=t37*t14;
t36=t34+t9;
int_v_list.dp[1][1][0][5]=t36;
t9=t37*t36;
t34=t9+t12;
int_v_list.dp[2][1][0][8]=t34;
t9=t13*int_v_list.dp[0][0][2][0];
t12=t37*int_v_list.dp[0][0][1][0];
t36=t12+t9;
t9=t2*t36;
t12=t15+t9;
t9=t20+t12;
t12=t13*t19;
t15=t11+t12;
t12=t37*t21;
t20=t12+t15;
t12=t13*t20;
t15=t12+t9;
t9=t13*t21;
t12=t6+t9;
t9=t37*t23;
t20=t9+t12;
int_v_list.dp[1][1][0][4]=t20;
t9=t37*t20;
t12=t9+t15;
int_v_list.dp[2][1][0][7]=t12;
t9=t13*t28;
t15=t37*t30;
t20=t15+t9;
t9=t13*t20;
t15=t31+t9;
t9=t13*t30;
t20=t37*t32;
t31=t20+t9;
int_v_list.dp[1][1][0][3]=t31;
t9=t37*t31;
t20=t9+t15;
int_v_list.dp[2][1][0][6]=t20;
t9=t33*t8;
t8=t16*t10;
t15=t8+t9;
t8=t13*t15;
t9=t33*t10;
t10=t16*t14;
t14=t10+t9;
int_v_list.dp[1][1][0][2]=t14;
t9=t37*t14;
t10=t9+t8;
int_v_list.dp[2][1][0][5]=t10;
t8=t33*t19;
t9=t16*t21;
t19=t9+t8;
t8=t13*t19;
t9=t33*int_v_list.dp[0][0][2][0];
t31=t16*int_v_list.dp[0][0][1][0];
t38=t31+t9;
t9=t2*t38;
t2=t9+t8;
t8=t33*t21;
t21=t16*t23;
t23=t21+t8;
int_v_list.dp[1][1][0][1]=t23;
t8=t37*t23;
t21=t8+t2;
int_v_list.dp[2][1][0][4]=t21;
t2=t33*t28;
t8=t11+t2;
t2=t16*t30;
t11=t2+t8;
t2=t13*t11;
t8=t33*t30;
t28=t6+t8;
t6=t16*t32;
t8=t6+t28;
int_v_list.dp[1][1][0][0]=t8;
t6=t37*t8;
t28=t6+t2;
int_v_list.dp[2][1][0][3]=t28;
t2=t33*t15;
t6=t4+t2;
t2=t16*t14;
t4=t2+t6;
int_v_list.dp[2][1][0][2]=t4;
t2=t33*t19;
t6=t22+t2;
t2=t16*t23;
t14=t2+t6;
int_v_list.dp[2][1][0][1]=t14;
t2=t24+t9;
t6=t29+t2;
t2=t33*t11;
t9=t2+t6;
t2=t16*t8;
t6=t2+t9;
int_v_list.dp[2][1][0][0]=t6;
t2=t7*int_v_list.dp[0][0][1][0];
t7=int_v_oo2zeta12*int_v_list.dp[0][0][0][0];
t8=t7+t2;
t2=t1*t5;
t7=t2+t8;
t2=t1*int_v_list.dp[0][0][1][0];
t1=t3*int_v_list.dp[0][0][0][0];
t9=t1+t2;
int_v_list.dp[1][0][0][2]=t9;
t1=t3*t9;
t2=t1+t7;
int_v_list.dp[2][0][0][5]=t2;
t1=t13*t5;
t3=t37*t9;
t7=t3+t1;
int_v_list.dp[2][0][0][4]=t7;
t1=t33*t5;
t3=t16*t9;
t5=t3+t1;
int_v_list.dp[2][0][0][3]=t5;
t1=t13*t36;
t3=t8+t1;
t1=t13*int_v_list.dp[0][0][1][0];
t9=t37*int_v_list.dp[0][0][0][0];
t11=t9+t1;
int_v_list.dp[1][0][0][1]=t11;
t1=t37*t11;
t9=t1+t3;
int_v_list.dp[2][0][0][2]=t9;
t1=t13*t38;
t3=t33*int_v_list.dp[0][0][1][0];
t11=t16*int_v_list.dp[0][0][0][0];
t13=t11+t3;
int_v_list.dp[1][0][0][0]=t13;
t3=t37*t13;
t11=t3+t1;
int_v_list.dp[2][0][0][1]=t11;
t1=t33*t38;
t3=t8+t1;
t1=t16*t13;
t8=t1+t3;
int_v_list.dp[2][0][0][0]=t8;
return 1;}
