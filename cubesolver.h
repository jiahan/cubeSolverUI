#ifndef CUBESOLVER_H
#define CUBESOLVER_H

#include <QMainWindow>

namespace Ui {
class CubeSolver;
}

class CubeSolver : public QMainWindow
{
    Q_OBJECT

public:
    explicit CubeSolver(QWidget *parent = 0);
    ~CubeSolver();

    void calculateStep();

private slots:
    void on_F0_clicked();
    void on_F1_clicked();
    void on_F2_clicked();
    void on_F3_clicked();
    void on_F4_clicked();
    void on_F5_clicked();
    void on_F6_clicked();
    void on_F7_clicked();
    void on_F8_clicked();
    void on_R0_clicked();
    void on_R1_clicked();
    void on_R2_clicked();
    void on_R3_clicked();
    void on_R4_clicked();
    void on_R5_clicked();
    void on_R6_clicked();
    void on_R7_clicked();
    void on_R8_clicked();
    void on_B0_clicked();
    void on_B1_clicked();
    void on_B2_clicked();
    void on_B3_clicked();
    void on_B4_clicked();
    void on_B5_clicked();
    void on_B6_clicked();
    void on_B7_clicked();
    void on_B8_clicked();
    void on_L0_clicked();
    void on_L1_clicked();
    void on_L2_clicked();
    void on_L3_clicked();
    void on_L4_clicked();
    void on_L5_clicked();
    void on_L6_clicked();
    void on_L7_clicked();
    void on_L8_clicked();
    void on_U0_clicked();
    void on_U1_clicked();
    void on_U2_clicked();
    void on_U3_clicked();
    void on_U4_clicked();
    void on_U5_clicked();
    void on_U6_clicked();
    void on_U7_clicked();
    void on_U8_clicked();
    void on_D0_clicked();
    void on_D1_clicked();
    void on_D2_clicked();
    void on_D3_clicked();
    void on_D4_clicked();
    void on_D5_clicked();
    void on_D6_clicked();
    void on_D7_clicked();
    void on_D8_clicked();
    void on_calculate_clicked();
    void on_color0_clicked();
    void on_color1_clicked();
    void on_color2_clicked();
    void on_color3_clicked();
    void on_color4_clicked();
    void on_color5_clicked();

private:
    Ui::CubeSolver *ui;
    void inputFormatTransform(int colorConfig[6][9]);
    bool searchphase(int movesleft, int movesdone,int lastmove);
    void filltable(int ti);
    void domove(int m);
    void setposition(int t, int n);
    int getposition(int t);
    void numtoperm(char* p,int n,int o);
    int permtonum(char* p);
    void reset();
    void twist(int i,int a);
    void cycle(char*p,char*a);
    void setValue(int &value);

    int F1 = 0, F2 = 0, F3 = 0, F4 = 0, F5 = 0, F6 = 0, F7 = 0, F0 = 0, F8 = 0,
        L1 = 4, L2 = 4, L3 = 4, L4 = 4, L5 = 4, L6 = 4, L7 = 4, L0 = 4, L8 = 4,
        R1 = 1, R2 = 1, R3 = 1, R4 = 1, R5 = 1, R6 = 1, R7 = 1, R0 = 1, R8 = 1,
        U1 = 2, U2 = 2, U3 = 2, U4 = 2, U5 = 2, U6 = 2, U7 = 2, U0 = 2, U8 = 2,
        D1 = 5, D2 = 5, D3 = 5, D4 = 5, D5 = 5, D6 = 5, D7 = 5, D0 = 5, D8 = 5,
        B1 = 3, B2 = 3, B3 = 3, B4 = 3, B5 = 3, B6 = 3, B7 = 3, B0 = 3, B8 = 3;
};

#endif // CUBESOLVER_H
