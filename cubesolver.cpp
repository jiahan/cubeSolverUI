#include "cubesolver.h"
#include "ui_cubesolver.h"
#include <iostream>
//#include <string>
//#include <string.h>
using namespace std ;
char
    // RLFBUD is the face order used for input, so that a correctly oriented
    // piece in the input has its 'highest value' facelet first. The rest of the
    // program uses moves in FBRLUD order.
    *faces="RLFBUD",
    // I use char arrays here cause they can be initialised with a string
    // which is shorter than initialising other arrays.
    // Internally cube uses slightly different ordering to the input so that
    //  orbits of stage 4 are contiguous. Note also that the two corner orbits
    //  are diametrically opposite each other.
    //input:  UF UR UB UL  DF DR DB DL  FR FL BR BL  UFR URB UBL ULF   DRF DFL DLB DBR
    //        A  B  C  D   E  F  G  H   I  J  K  L   M   N   O   P     Q   R   S   T
    //        A  E  C  G   B  F  D  H   I  J  K  L   M   S   N   T     R   O   Q   P
    //intrnl: UF DF UB DB  UR DR UL DL  FR FL BR BL  UFR UBL DFL DBR   DLB DRF URB ULF
    *order="AECGBFDHIJKLMSNTROQP",
    //To quickly recognise the pieces, I construct an integer by setting a bit for each
    // facelet. The unique result is then found on the list below to map it to the correct
    // cubelet of the cube.
    //intrnl: UF DF UB DB  UR DR UL DL  FR FL BR BL  UFR UBL DFL DBR   DLB DRF URB ULF
    //bithash:20,36,24,40, 17,33,18,34, 5, 6, 9, 10, 21, 26, 38, 41,   42, 37, 25, 22
    *bithash="TdXhQaRbEFIJUZfijeYV",
    //Each move consists of two 4-cycles. This string contains these in FBRLUD order.
    //intrnl: UF DF UB DB  UR DR UL DL  FR FL BR BL  UFR UBL DFL DBR   DLB DRF URB ULF
    //        A  B  C  D   E  F  G  H   I  J  K  L   M   N   O   P     Q   R   S   T
    *perm="AIBJTMROCLDKSNQPEKFIMSPRGJHLNTOQAGCEMTNSBFDHORPQ",

    // current cube position
    poss[20],ori[20],val[20],
    // temporary variable used in swap macro
    TEMP,
    // pruning tables, 2 for each phase
    *tables[8];
    // current phase solution
int movel[20],moveamount[20],
    // current phase being searched (0,2,4,6 for phases 1 to 4)
    phase=0,
    // Length of pruning tables. (one dummy in phase 1);
    tablesize[]={1,4096,  6561,4096,  256,1536,  13824,576};

// Use very ugly and unsafe macro to swap items instead of classic routine with
//   pointers for the sole reason of brevity
#define SWAP(a,b) TEMP=a;a=b;b=TEMP;
// number 65='A' is often subtracted to convert char ABC... to number 0,1,2,...
#define CHAROFFSET 65

//A solved cube format:
//UF UR UB UL/ DF DR DB DL/ FR FL/ BR BL/ UFR URB UBL ULF/ DRF DFL DLB DBR/
static char CENTER_COLOR_POSITION[6] = {'F', 'R', 'U', 'B', 'L', 'D'};
const int SIDE_INDEX_1[12] = {2,2,2,2,5,5,5,5,0,0,3,3}; //which face
const int SIDE_INDEX_2[12] = {7,5,1,3,1,5,7,3,5,3,3,5}; //which block
const int SIDE_INDEX_3[12] = {0,1,3,4,0,1,3,4,1,4,1,4}; //which face
const int SIDE_INDEX_4[12] = {1,1,1,1,7,7,7,7,3,5,5,3}; //which block
const int CORNER_INDEX_1[8] = {2,2,2,2,5,5,5,5}; //which face
const int CORNER_INDEX_2[8] = {8,2,0,6,2,0,6,8}; //which block
const int CORNER_INDEX_3[8] = {0,1,3,4,1,0,4,3}; //which face
const int CORNER_INDEX_4[8] = {2,2,2,2,6,6,6,6}; //which block
const int CORNER_INDEX_5[8] = {1,3,4,0,0,4,3,1}; //which face
const int CORNER_INDEX_6[8] = {0,0,0,0,8,8,8,8}; //which block

CubeSolver::CubeSolver(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CubeSolver)
{
    ui->setupUi(this);
}

CubeSolver::~CubeSolver()
{
    delete ui;
}


char transformedInputFormat[20][3] = {
    {'R','D'},
    {'L','U'},
    {'F','D'},
    {'R','U'},
    {'B','R'},
    {'R','F'},
    {'D','L'},
    {'L','B'},
    {'F','L'},
    {'B','D'},
    {'F','U'},
    {'U','B'},
    {'D','F','L'},
    {'R','U','F'},
    {'B','R','D'},
    {'L','F','U'},
    {'L','U','B'},
    {'F','D','R'},
    {'R','B','U'},
    {'D','L','B'}
};

#define use_matrix 1

int colorConfig[6][9] = {
    {5,1,1,5,0,1,5,5,3}, //Front    0
    {0,3,0,0,1,3,1,1,1}, //Right    1
    {0,2,5,0,2,4,4,2,5}, //Up       2
    {4,4,2,2,3,5,0,0,2}, //Back     3
    {4,4,3,4,4,3,4,3,3}, //Left     4
    {1,0,2,1,5,5,3,2,2}  //Down     5
};


void CubeSolver::inputFormatTransform(int colorConfig[6][9])
{
    int i;
    char centerColorPosition[6];

    for(i=0; i<6; i++) centerColorPosition[colorConfig[i][4]] = CENTER_COLOR_POSITION[i];

    for(i=0; i<12; i++){
        transformedInputFormat[i][0] = centerColorPosition[colorConfig[SIDE_INDEX_1[i]][SIDE_INDEX_2[i]]];
        transformedInputFormat[i][1] = centerColorPosition[colorConfig[SIDE_INDEX_3[i]][SIDE_INDEX_4[i]]];
        transformedInputFormat[i][2] = '\0';
    }

    for(i=0; i<8; i++){
        transformedInputFormat[i+12][0] = centerColorPosition[colorConfig[CORNER_INDEX_1[i]][CORNER_INDEX_2[i]]];
        transformedInputFormat[i+12][1] = centerColorPosition[colorConfig[CORNER_INDEX_3[i]][CORNER_INDEX_4[i]]];
        transformedInputFormat[i+12][2] = centerColorPosition[colorConfig[CORNER_INDEX_5[i]][CORNER_INDEX_6[i]]];
    }
}

void CubeSolver::calculateStep(){
    //transfer 6x9 input into Mike Reid's input format
    if(use_matrix) inputFormatTransform(colorConfig);

    cout << "Input:" << endl;
    for (int i = 0; i < 20; ++i)
    {
        for (int j = 0; j < 3; ++j) cout << transformedInputFormat[i][j];
        cout << " ";
    }
    cout << endl;
    cout << "Output:" << endl;
    ui->moveSolution->setText("");

    int f,i=0,j=0,k=0,pc,mor;

    // initialise tables
    for(; k<20; k++) val[k]=k<12?2:3;
    for(; j<8; j++) filltable(j);

    // read input, 20 pieces worth
    for(; i<20; i++){
        f=pc=k=mor=0;
        for(;f<val[i];f++){
            // read input from stdin, or...
            //     do{cin>>c;}while(c==' ');
            //     j=strchr(faces,c)-faces;
            // ...from command line and get face number of facelet
            j=strchr(faces,transformedInputFormat[i][f])-faces;
            // keep track of principal facelet for orientation
            if(j>k) {k=j;mor=f;}
            //construct bit hash code
            pc+= 1<<j;
        }

        // find which cubelet it belongs, i.e. the label for this piece
        for(f=0; f<20; f++)
            if( pc==bithash[f]-64 ) break;
        // store piece
        poss[order[i]-CHAROFFSET]=f;
        ori[order[i]-CHAROFFSET]=mor%val[i];
    }

    //solve the cube
    // four phases
    phase=0;
    for( ; phase<8; phase+=2){
        // try each depth till solved
        for( j=0; !searchphase(j,0,9); j++);
        //output result of this phase
        for( i=0; i<j; i++){
            cout<<"FBRLUD"[movel[i]]<<moveamount[i] << " ";
            ui->moveSolution->setText(ui->moveSolution->toPlainText() + "FBRLUD"[movel[i]] + QString::number(moveamount[i]) + " ");
        }
    }

    cout<<endl;
}

void CubeSolver::on_calculate_clicked()
{
    /*
    cout << F0 << " " << F1<< " " << F2<< endl << F3<< " " << F4<< " " << F5<< endl << F6<< " " << F7<< " " << F8 << endl;
    cout << endl;
    cout << R0 << " " << R1<< " " << R2<< endl << R3<< " " << R4<< " " << R5<< endl << R6<< " " << R7<< " " << R8 << endl;
    cout << endl;
    cout << U0 << " " << U1<< " " << U2<< endl << U3<< " " << U4<< " " << U5<< endl << U6<< " " << U7<< " " << U8 << endl;
    cout << endl;
    cout << B0 << " " << B1<< " " << B2<< endl << B3<< " " << B4<< " " << B5<< endl << B6<< " " << B7<< " " << B8 << endl;
    cout << endl;
    cout << L0 << " " << L1<< " " << L2<< endl << L3<< " " << L4<< " " << L5<< endl << L6<< " " << L7<< " " << L8 << endl;
    cout << endl;
    cout << D0 << " " << D1<< " " << D2<< endl << D3<< " " << D4<< " " << D5<< endl << D6<< " " << D7<< " " << D8 << endl;
    */

    colorConfig[0][0] = F0;
    colorConfig[0][1] = F1;
    colorConfig[0][2] = F2;
    colorConfig[0][3] = F3;
    colorConfig[0][4] = F4;
    colorConfig[0][5] = F5;
    colorConfig[0][6] = F6;
    colorConfig[0][7] = F7;
    colorConfig[0][8] = F8;
    colorConfig[1][0] = R0;
    colorConfig[1][1] = R1;
    colorConfig[1][2] = R2;
    colorConfig[1][3] = R3;
    colorConfig[1][4] = R4;
    colorConfig[1][5] = R5;
    colorConfig[1][6] = R6;
    colorConfig[1][7] = R7;
    colorConfig[1][8] = R8;
    colorConfig[2][0] = U0;
    colorConfig[2][1] = U1;
    colorConfig[2][2] = U2;
    colorConfig[2][3] = U3;
    colorConfig[2][4] = U4;
    colorConfig[2][5] = U5;
    colorConfig[2][6] = U6;
    colorConfig[2][7] = U7;
    colorConfig[2][8] = U8;
    colorConfig[3][0] = B0;
    colorConfig[3][1] = B1;
    colorConfig[3][2] = B2;
    colorConfig[3][3] = B3;
    colorConfig[3][4] = B4;
    colorConfig[3][5] = B5;
    colorConfig[3][6] = B6;
    colorConfig[3][7] = B7;
    colorConfig[3][8] = B8;
    colorConfig[4][0] = L0;
    colorConfig[4][1] = L1;
    colorConfig[4][2] = L2;
    colorConfig[4][3] = L3;
    colorConfig[4][4] = L4;
    colorConfig[4][5] = L5;
    colorConfig[4][6] = L6;
    colorConfig[4][7] = L7;
    colorConfig[4][8] = L8;
    colorConfig[5][0] = D0;
    colorConfig[5][1] = D1;
    colorConfig[5][2] = D2;
    colorConfig[5][3] = D3;
    colorConfig[5][4] = D4;
    colorConfig[5][5] = D5;
    colorConfig[5][6] = D6;
    colorConfig[5][7] = D7;
    colorConfig[5][8] = D8;

    calculateStep();
}

// Cycles 4 pieces in array p, the piece indices given by a[0..3].
void CubeSolver::cycle(char*p,char*a){
    SWAP(p[*a-CHAROFFSET],p[a[1]-CHAROFFSET]);
    SWAP(p[*a-CHAROFFSET],p[a[2]-CHAROFFSET]);
    SWAP(p[*a-CHAROFFSET],p[a[3]-CHAROFFSET]);
}

void CubeSolver::setValue(int &value)
{
    if(ui->targetColor->styleSheet() == "background-color: rgba(255, 255, 255, 255);")
        value = 0;

    if(ui->targetColor->styleSheet() == "background-color: rgba(0, 0, 127, 255);")
        value = 1;

    if(ui->targetColor->styleSheet() == "background-color: rgba(255, 85, 127, 255);")
        value = 2;

    if(ui->targetColor->styleSheet() == "background-color: rgba(255, 255, 0, 255);")
        value = 3;

    if(ui->targetColor->styleSheet() == "background-color: rgba(0, 255, 0, 255);")
        value = 4;

    if(ui->targetColor->styleSheet() == "background-color: rgba(255, 0, 0, 255);")
        value = 5;
}

// twists i-th piece a+1 times.
void CubeSolver::twist(int i,int a){
    i-=CHAROFFSET;
    ori[i]=(ori[i]+a+1)%val[i];
}


// set cube to solved position
void CubeSolver::reset(){
    for( int i=0; i<20; poss[i]=i, ori[i++]=0);
}

// convert permutation of 4 chars to a number in range 0..23
int CubeSolver::permtonum(char* p){
    int n=0;
    for ( int a=0; a<4; a++) {
        n*=4-a;
        for( int b=a; ++b<4; )
            if (p[b]<p[a]) n++;
    }
    return n;
}

// convert number in range 0..23 to permutation of 4 chars.
void CubeSolver::numtoperm(char* p,int n,int o){
    p+=o;
    p[3]=o;
    for (int a=3; a--;){
        p[a] = n%(4-a) +o;
        n/=4-a;
        for (int b=a; ++b<4; )
            if ( p[b] >= p[a]) p[b]++;
    }
}

// get index of cube position from table t
int CubeSolver::getposition(int t){
    int i=-1,n=0;
    switch(t){
    // case 0 does nothing so returns 0
    case 1://edgeflip
        // 12 bits, set bit if edge is flipped
        for(;++i<12;) n+= ori[i]<<i;
        break;
    case 2://cornertwist
        // get base 3 number of 8 digits - each digit is corner twist
        for(i=20;--i>11;) n=n*3+ori[i];
        break;
    case 3://middle edge choice
        // 12 bits, set bit if edge belongs in Um middle slice
        for(;++i<12;) n+= (poss[i]&8)?(1<<i):0;
        break;
    case 4://ud slice choice
        // 8 bits, set bit if UD edge belongs in Fm middle slice
        for(;++i<8;) n+= (poss[i]&4)?(1<<i):0;
        break;
    case 5://tetrad choice, twist and parity
        int corn[8],j,k,l,corn2[4];
        // 8 bits, set bit if corner belongs in second tetrad.
        // also separate pieces for twist/parity determination
        k=j=0;
        for(;++i<8;)
            if((l=poss[i+12]-12)&4){
                corn[l]=k++;
                n+=1<<i;
            }else corn[j++]=l;
        //Find permutation of second tetrad after solving first
        for(i=0;i<4;i++) corn2[i]=corn[4+corn[i]];
        //Solve one piece of second tetrad
        for(;--i;) corn2[i]^=corn2[0];

        // encode parity/tetrad twist
        n=n*6+corn2[1]*2-2;
        if(corn2[3]<corn2[2])n++;
        break;
    case 6://two edge and one corner orbit, permutation
        n=permtonum(poss)*576+permtonum(poss+4)*24+permtonum(poss+12);
        break;
    case 7://one edge and one corner orbit, permutation
        n=permtonum(poss+8)*24+permtonum(poss+16);
        break;
    }
    return n;
}


// sets cube to any position which has index n in table t
void CubeSolver::setposition(int t, int n){
    int i=0,j=12,k=0;
    char *corn="QRSTQRTSQSRTQTRSQSTRQTSR";
    reset();
    switch(t){
    // case 0 does nothing so leaves cube solved
    case 1://edgeflip
        for(;i<12;i++,n>>=1) ori[i]=n&1;
        break;
    case 2://cornertwist
        for(i=12;i<20;i++,n/=3) ori[i]=n%3;
        break;
    case 3://middle edge choice
        for(;i<12;i++,n>>=1) poss[i]= 8*n&8;
        break;
    case 4://ud slice choice
        for(;i<8;i++,n>>=1) poss[i]= 4*n&4;
        break;
    case 5://tetrad choice,parity,twist
        corn+=n%6*4;
        n/=6;
        for(;i<8;i++,n>>=1)
            poss[i+12]= n&1 ? corn[k++]-CHAROFFSET : j++;
        break;
    case 6://slice permutations
        numtoperm(poss,n%24,12);n/=24;
        numtoperm(poss,n%24,4); n/=24;
        numtoperm(poss,n   ,0);
        break;
    case 7://corner permutations
        numtoperm(poss,n/24,8);
        numtoperm(poss,n%24,16);
        break;
    }
}


//do a clockwise quarter turn cube move
void CubeSolver::domove(int m){
    char *p=perm+8*m, i=8;
    //cycle the edges
    cycle(poss,p);
    cycle(ori,p);
    //cycle the corners
    cycle(poss,p+4);
    cycle(ori,p+4);
    //twist corners if RLFB
    if(m<4)
        for(;--i>3;) twist(p[i],i&1);
    //flip edges if FB
    if(m<2)
        for(i=4;i--;) twist(p[i],0);
}

// calculate a pruning table
void CubeSolver::filltable(int ti){
    int n=1,l=1, tl=tablesize[ti];
    // alocate table memory
    char* tb = tables[ti]=new char[tl];
    //clear table
    memset( tb, 0, tl);
    //mark solved position as depth 1
    reset();
    tb[getposition(ti)]=1;

    // while there are positions of depth l
    while(n){
        n=0;
        // find each position of depth l
        for(int i=0;i<tl;i++){
            if( tb[i]==l ){
                //construct that cube position
                setposition(ti,i);
                // try each face any amount
                for( int f=0; f<6; f++){
                    for( int q=1;q<4;q++){
                        domove(f);
                        // get resulting position
                        int r=getposition(ti);
                        // if move as allowed in that phase, and position is a new one
                        if( ( q==2 || f>=(ti&6) ) && !tb[r]){
                            // mark that position as depth l+1
                            tb[r]=l+1;
                            n++;
                        }
                    }
                    domove(f);
                }
            }
        }
        l++;
    }
}

// Pruned tree search. recursive.
bool CubeSolver::searchphase(int movesleft, int movesdone,int lastmove){
    // prune - position must still be solvable in the remaining moves available
    if( tables[phase  ][getposition(phase  )]-1 > movesleft ||
        tables[phase+1][getposition(phase+1)]-1 > movesleft ) return false;

    // If no moves left to do, we have solved this phase
    if(!movesleft) return true;

    // not solved. try each face move
    for( int i=6;i--;){
        // do not repeat same face, nor do opposite after DLB.
        if( i-lastmove && (i-lastmove+1 || i|1 ) ){
            movel[movesdone]=i;
            // try 1,2,3 quarter turns of that face
            for(int j=0;++j<4;){
                //do move and remember it
                domove(i);
                moveamount[movesdone]=j;
                //Check if phase only allows half moves of this face
                if( (j==2 || i>=phase ) &&
                    //search on
                    searchphase(movesleft-1,movesdone+1,i) ) return true;
            }
            // put face back to original position.
            domove(i);
        }
    }
    // no solution found
    return false;
}

void CubeSolver::on_F0_clicked()
{
    ui->F0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F0);
}

void CubeSolver::on_F1_clicked()
{
    ui->F1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F1);
}

void CubeSolver::on_F2_clicked()
{
    ui->F2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F2);
}

void CubeSolver::on_F3_clicked()
{
    ui->F3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F3);
}

void CubeSolver::on_F4_clicked()
{
    ui->F4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F4);
}

void CubeSolver::on_F5_clicked()
{
    ui->F5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F5);
}

void CubeSolver::on_F6_clicked()
{
    ui->F6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F6);
}

void CubeSolver::on_F7_clicked()
{
    ui->F7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F7);
}

void CubeSolver::on_F8_clicked()
{
    ui->F8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(F8);
}

void CubeSolver::on_R0_clicked()
{
    ui->R0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R0);
}

void CubeSolver::on_R1_clicked()
{
    ui->R1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R1);
}

void CubeSolver::on_R2_clicked()
{
    ui->R2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R2);
}

void CubeSolver::on_R3_clicked()
{
    ui->R3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R3);
}

void CubeSolver::on_R4_clicked()
{
    ui->R4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R4);
}

void CubeSolver::on_R5_clicked()
{
    ui->R5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R5);
}

void CubeSolver::on_R6_clicked()
{
    ui->R6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R6);
}

void CubeSolver::on_R7_clicked()
{
    ui->R7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R7);
}

void CubeSolver::on_R8_clicked()
{
    ui->R8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(R8);
}

void CubeSolver::on_B0_clicked()
{
    ui->B0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B0);
}

void CubeSolver::on_B1_clicked()
{
    ui->B1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B1);
}

void CubeSolver::on_B2_clicked()
{
    ui->B2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B2);
}

void CubeSolver::on_B3_clicked()
{
    ui->B3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B3);
}

void CubeSolver::on_B4_clicked()
{
    ui->B4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B4);
}

void CubeSolver::on_B5_clicked()
{
    ui->B5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B5);
}

void CubeSolver::on_B6_clicked()
{
    ui->B6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B6);
}

void CubeSolver::on_B7_clicked()
{
    ui->B7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B7);
}

void CubeSolver::on_B8_clicked()
{
    ui->B8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(B8);
}

void CubeSolver::on_L0_clicked()
{
    ui->L0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L0);
}

void CubeSolver::on_L1_clicked()
{
    ui->L1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L1);
}

void CubeSolver::on_L2_clicked()
{
    ui->L2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L2);
}

void CubeSolver::on_L3_clicked()
{
    ui->L3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L3);
}

void CubeSolver::on_L4_clicked()
{
    ui->L4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L4);
}

void CubeSolver::on_L5_clicked()
{
    ui->L5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L5);
}

void CubeSolver::on_L6_clicked()
{
    ui->L6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L6);
}

void CubeSolver::on_L7_clicked()
{
    ui->L7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L7);
}

void CubeSolver::on_L8_clicked()
{
    ui->L8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(L8);
}

void CubeSolver::on_U0_clicked()
{
    ui->U0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U0);
}

void CubeSolver::on_U1_clicked()
{
    ui->U1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U1);
}

void CubeSolver::on_U2_clicked()
{
    ui->U2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U2);
}

void CubeSolver::on_U3_clicked()
{
    ui->U3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U3);
}

void CubeSolver::on_U4_clicked()
{
    ui->U4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U4);
}

void CubeSolver::on_U5_clicked()
{
    ui->U5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U5);
}

void CubeSolver::on_U6_clicked()
{
    ui->U6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U6);
}

void CubeSolver::on_U7_clicked()
{
    ui->U7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U7);
}

void CubeSolver::on_U8_clicked()
{
    ui->U8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(U8);
}

void CubeSolver::on_D0_clicked()
{
    ui->D0->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D0);
}

void CubeSolver::on_D1_clicked()
{
    ui->D1->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D1);
}

void CubeSolver::on_D2_clicked()
{
    ui->D2->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D2);
}

void CubeSolver::on_D3_clicked()
{
    ui->D3->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D3);
}

void CubeSolver::on_D4_clicked()
{
    ui->D4->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D4);
}

void CubeSolver::on_D5_clicked()
{
    ui->D5->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D5);
}

void CubeSolver::on_D6_clicked()
{
    ui->D6->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D6);
}

void CubeSolver::on_D7_clicked()
{
    ui->D7->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D7);
}

void CubeSolver::on_D8_clicked()
{
    ui->D8->setStyleSheet(ui->targetColor->styleSheet());
    setValue(D8);
}


void CubeSolver::on_color0_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(255, 255, 255, 255);");
}

void CubeSolver::on_color1_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(0, 0, 127, 255);");
}

void CubeSolver::on_color2_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(255, 85, 127, 255);");
}

void CubeSolver::on_color3_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(255, 255, 0, 255);");
}

void CubeSolver::on_color4_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(0, 255, 0, 255);");
}

void CubeSolver::on_color5_clicked()
{
    ui->targetColor->setStyleSheet("background-color: rgba(255, 0, 0, 255);");
}


