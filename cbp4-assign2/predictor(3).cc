#include "predictor.h"
#include "math.h"
#include <bitset>

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////
 struct entry
{
       bool bit1;
       bool bit2;
};

entry predictor_2bitsat[4096];

void InitPredictor_2bitsat() {
 for (int i= 0; i<4096; i++)
{
   predictor_2bitsat[i].bit1=0;
   predictor_2bitsat[i].bit2=1;
}

}

bool GetPrediction_2bitsat(UINT32 PC) {
  unsigned index = PC & 0xFFF;
  if (predictor_2bitsat[index].bit1)
  return TAKEN;
  else 
  return NOT_TAKEN; 
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  unsigned index = PC & 0xFFF;
  if( resolveDir == TAKEN)
{
   if(predictor_2bitsat[index].bit1 == 0 && predictor_2bitsat[index].bit2==0)
 {
    predictor_2bitsat[index].bit2=1;
 }
   else if(predictor_2bitsat[index].bit1 == 0 && predictor_2bitsat[index].bit2==1)
 {
    predictor_2bitsat[index].bit1=1;    
    predictor_2bitsat[index].bit2=0;
 }
   else if(predictor_2bitsat[index].bit1 == 1 && predictor_2bitsat[index].bit2==0)
 {
    predictor_2bitsat[index].bit1=1;    
    predictor_2bitsat[index].bit2=1;
 }

}
else if (resolveDir == NOT_TAKEN)
{
   if(predictor_2bitsat[index].bit1 == 1 && predictor_2bitsat[index].bit2==1)
 {
    predictor_2bitsat[index].bit2=0;
 }
   else if(predictor_2bitsat[index].bit1 == 0 && predictor_2bitsat[index].bit2==1)
 {
    predictor_2bitsat[index].bit1=0;    
    predictor_2bitsat[index].bit2=0;
 }
   else if(predictor_2bitsat[index].bit1 == 1 && predictor_2bitsat[index].bit2==0)
 {
    predictor_2bitsat[index].bit1=0;    
    predictor_2bitsat[index].bit2=1;
 }

}

}


/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

short BranchHistoryTable[512];

short predictor_2level[8][64];

void InitPredictor_2level() {

    BranchHistoryTable[512] = {0};
    for (int i= 0; i<8; i++)
    {
        for(int j=0; j<64; j++)
        {
            predictor_2level[i][j] = 1;
        }
    }
}

bool GetPrediction_2level(UINT32 PC) {
    int BHT_index = (PC & 0xFF8)>>3;
    int PHT_index = (PC & 0X7);
    
    int PHT_index2 = (int)BranchHistoryTable[BHT_index];
    
    if (predictor_2level[PHT_index][PHT_index2] == 3 ||predictor_2level[PHT_index][PHT_index2] == 2){
        return TAKEN;
}
    else if(predictor_2level[PHT_index][PHT_index2] == 0 ||predictor_2level[PHT_index][PHT_index2] == 1){
        return NOT_TAKEN;
}
    else {
        return TAKEN;
}
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
    int BHT_index = (PC & 0xFF8)>>3;
    int PHT_index = (PC & 0X7);
    int PHT_index2 = (int)BranchHistoryTable[BHT_index];
    
    if (resolveDir == NOT_TAKEN)
    {
        predictor_2level[PHT_index][PHT_index2] = max(predictor_2level[PHT_index][PHT_index2]-1,0);
        BranchHistoryTable[BHT_index] = (BranchHistoryTable[BHT_index]<<1)& 0x3f;
    }
    else
    {
        predictor_2level[PHT_index][PHT_index2] = min(predictor_2level[PHT_index][PHT_index2]+1,3);
        BranchHistoryTable[BHT_index] = ((BranchHistoryTable[BHT_index]<<1) + 1)& 0x3f;
    }
    
}







/*short BranchHistoryTable[512];
short predictor_2level[8][64];
void InitPredictor_openend() {

}

bool GetPrediction_openend(UINT32 PC) {

  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

}*/

//
//open end 
//https://www.jilp.org/vol8/v8paper1.pdf  
//might be useful, good introduction 

struct tage_entry{
short u;       //2 bits counter
short ctr;     //2 bits counter
unsigned long long tag;
};


//have no idea whats the different between the following two regs 
unsigned long long bhist;   //branch history 
short phist;              // 16 bits path history- consist 1 address bit per branch 

short Bi_Model[4096];   
tage_entry T[4][128];       // 4,8,16,32,64 bits history

short altpred;

void InitPredictor_openend() {
   Bi_Model[4096]={1};
for(int j=0;j < 4;j++)
{ 
   for(int i=0; i<128;i++)
  {
   T[j][i].u=0;
   T[j][i].ctr = 0;
   T[j][i].tag = 0;
   
  }
}
}

bool GetPrediction_openend(UINT32 PC) {
 
for(int j=3; j>=0; j--)
{
   unsigned long long index = 0;
   if (j==3)   //64
   index = (PC^(bhist & 0xffffffff));//(((bhist>>32)<<32)+(PC^(bhist&0xffffffff)));
else if (j==2)  //32
   index = (PC^(bhist & 0xffffffff));
else if (j==1) //16
   index = (((PC>>16)<<16)+((PC&0xffff)^(bhist&0xffff)));
else if (j==0) //8
   index = (((PC>>8)<<8)+((PC&0xff)^(bhist&0xff)));
//else if (j==0) //4
//   index = (((PC>>4)<<4)+((PC&0xf)^(bhist&0xf)));

    for( int i=0; i<128;i++)
  {
    if(T[j][i].tag==index && (((T[j][i].u)>>1)&0x1)==1)
    {
       //printf("j=%d, index=%lld\n", j,index);
       if(((T[j][i].ctr>>2)&0x1)==1)
        return TAKEN;
       else if (((T[j][i].ctr>>2)&0x1)==0)
        return NOT_TAKEN;
    }
  }
}
//printf("in here");
unsigned int BiModelIndex = PC&0xfff;
 if( (((Bi_Model[BiModelIndex])>>1)&0x1) == 1)
  return TAKEN;
 else 
  return NOT_TAKEN;
}



void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
//when prediction is incorrect: update provider component prediction counter, allocate an entry at a longer history table
struct entry_table{
     unsigned long long index;
     int J;
     int I;
     int correctness;
};
entry_table used_prediction_entry;
used_prediction_entry.J = 0;
used_prediction_entry.I = 0;
used_prediction_entry.index = 0;
used_prediction_entry.correctness = -1;

for(int j=0; j<=3; j++)
{
   unsigned long long index = 0;
    if (j==3)   //64
       index = (PC^(bhist & 0xffffffff));//(((bhist>>32)<<32)+(PC^(bhist&0xffffffff)));
    else if (j==2)  //32
       index = (PC^(bhist & 0xffffffff));
    else if (j==1) //16
       index = (((PC>>16)<<16)+((PC&0xffff)^(bhist&0xffff)));
    else if (j==0) //8
       index = (((PC>>8)<<8)+((PC&0xff)^(bhist&0xff)));
    //else if (j==0) //4
    //   index = (((PC>>4)<<4)+((PC&0xf)^(bhist&0xf)));
    used_prediction_entry.index = index;

   for( int i=0; i<128;i++)
   {
    if(T[j][i].tag==index && (((T[j][i].u)>>1)&0x1)==1)
    {
       used_prediction_entry.J = j;
       used_prediction_entry.I = i;

       if(resolveDir == ((T[j][i].ctr>>2)&0x1))
       {
          used_prediction_entry.correctness = 1;
          T[j][i].u = min(T[j][i].u+1,3);
          if(resolveDir == TAKEN)
             T[j][i].ctr = min(T[j][i].ctr+1,7);
	  else
             T[j][i].ctr = max(T[j][i].ctr-1,0);
       }

       else
       {
	  used_prediction_entry.correctness = 0;
          T[j][i].u = max(T[j][i].u-1,0);
          if(resolveDir == TAKEN)
             T[j][i].ctr = min(T[j][i].ctr+1,7);
          else
             T[j][i].ctr = max(T[j][i].ctr-1,0);
       }
       continue;
    }
   }
}

bool table_full = 1;
//printf("%daa%dbb%dcc\n", used_prediction_entry.J, used_prediction_entry.correctness, PC);
if (used_prediction_entry.correctness == -1){
   for (int a=0; a<4; a++)
   {
       for (int b=0; b<128; b++)
       {
           if(T[a][b].u==0)
           {
	       unsigned long long index = 0;
               if (a==3)   //64
                  index = (PC^(bhist & 0xffffffff));//(((bhist>>32)<<32)+(PC^(bhist&0xffffffff)));
               else if (a==2)  //32
                  index = (PC^(bhist&0xffffffff));
               else if (a==1) //16
                  index = (((PC>>16)<<16)+((PC&0xffff)^(bhist&0xffff)));
               else if (a==0) //8
                  index = (((PC>>8)<<8)+((PC&0xff)^(bhist&0xff)));
               //else if (a==0) //4
               //   index = (((PC>>4)<<4)+((PC&0xf)^(bhist&0xf)));

               T[a][b].tag=index;
               T[a][b].u=2;
               T[a][b].ctr=(resolveDir?6:1);
	       table_full =0;
	       break;
           }
      }
   }
}
else if (used_prediction_entry.correctness == 0){
   printf("j=%d\n",used_prediction_entry.J);
   for (int a=used_prediction_entry.J+1; a<4; a++)
   {
       for (int b=0; b<128; b++)
       {
           if(T[a][b].u==0)
           {
	       unsigned long long index = 0;
               if (a==3)   //64
                  index = (PC^(bhist & 0xffffffff));//(((bhist>>32)<<32)+(PC^(bhist&0xffffffff)));
               else if (a==2)  //32
                  index = (PC^(bhist&0xffffffff));
               else if (a==1) //16
                  index = (((PC>>16)<<16)+((PC&0xffff)^(bhist&0xffff)));
               else if (a==0) //8
                  index = (((PC>>8)<<8)+((PC&0xff)^(bhist&0xff)));
               //else if (a==0) //4
               //   index = (((PC>>4)<<4)+((PC&0xf)^(bhist&0xf)));
	       
               T[a][b].tag=index;
               T[a][b].u=2;
               T[a][b].ctr=(resolveDir?6:1);
	       table_full =0;
	       break;
           }
       }
   }
}


if (table_full ==1){
   for(int j=3;j < 4;j++)
   { 
      for(int i=0; i<128;i++)
      {
      T[j][i].u=0;
      T[j][i].ctr = 0;
      T[j][i].tag = 0;      
      }
   }

}

/*          if(j!=5)    //allocate entry 
        {
         for (int a=j+1; a<5, a++)
         {
             for (int b=0; b<128, b++)
             {
                 if(T[a][b].u==0)
                 {
                     T[a][b].tag=index;
                     T[a][b].u=2;
                     T[a][b].ctr=1;
                 }
             }
         }
        }
      }
    }
  }*/

unsigned int BiModelIndex = PC&0xfff;
 if(resolveDir)
  Bi_Model[BiModelIndex] = min(Bi_Model[BiModelIndex]+1,3);
 else 
  Bi_Model[BiModelIndex] = max(Bi_Model[BiModelIndex]-1,0);

bhist = ((bhist<<1)+resolveDir) & 0xffffffffffffffff;
  
}



  




/*resolveDir
/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

//notes:
// option 1: neutral network model
//http://ieeexplore.ieee.org/document/903263/#full-text-section
// long history length may not help,
// vairable length path


// currently used: 21264 predictors
// didnt do caculation, probabaly stupid... will fix the numbers based on performance

short calHistoryTable[512];
short GlobalPredictionTable[32768];    //2^15
unsigned long long GlobalHistory;
short LocalPredictionTable[8][64];
short ChoicePredictionTable[4096];
short tageTable1[4096];                //history length:4
short tageTable2[4096];                //history length:8
short tageTable3[4096];                //history length:12

void InitPredictor_openend() {
    
    LocalHistoryTable[512] = {0};  //6-bits for entry
    GlobalPredictionTable[32768] = {1};
    ChoicePredictionTable[4096] = {0};
    GlobalHistory = 0;
    
    for (int i= 0; i<8; i++)
    {
        for(int j=0; j<64; j++)
        {
            LocalPredictionTable[i][j] = 1;
        }
    }
}
resolveDir
bool GetPrediction_openend(UINT32 PC) {
    int Local_BHT_index = (PC & 0xFF8)>>3;
    int Local_PHT_index = (PC & 0X7);
    int Choice_Prediction_Index = (PC & 0xfff);
    int GlobalHistoryIndex = (((GlobalHistory>>5)<<5) + ((GlobalHistory&0x1f) ^ ((PC>>16)^PC)))&0x7fff;
    //int GlobalHistoryIndex = (GlobalHistory ^ ((PC>>16)^PC))&0x7fff;
     // choice prediction: 1 bias towords local prediction, 0 bias towards global prediction
    if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1) == 1)
    {
        int Local_PHT_index2 = (int)LocalHistoryTable[Local_BHT_index];
        
        if (((LocalPredictionTable[Local_PHT_index][Local_PHT_index2] >>1) &0x1) == 1)
            return TAKEN;
        else if(((LocalPredictionTable[Local_PHT_index][Local_PHT_index2]>>1)&0x1) == 0)
            return NOT_TAKEN;
    }
    else if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)== 0 )
    {
        if(((GlobalPredictionTable[GlobalHistoryIndex]>>1)&0x1) == 1)
            return TAKEN;
        else if(((GlobalPredictionTable[GlobalHistoryIndex]>>1)&0x1) == 0)
            return NOT_TAKEN;
    }
  return TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

    int Local_BHT_index = (PC & 0xFF8)>>3;
    int Local_PHT_index = (PC & 0X7);
    int Choice_Prediction_Index = (PC & 0xfff);
    int Local_PHT_index2 = (int)LocalHistoryTable[Local_BHT_index];
    
    LocalHistoryTable[Local_BHT_index] = ((LocalHistoryTable[Local_BHT_index]<<1) + (resolveDir?1:0))& 0x3f;
    unsigned int GlobalHistoryIndex = (((GlobalHistory>>5)<<5) + ((GlobalHistory&0x1f) ^ ((PC>>16)^PC)))&0x7fff;
    //unsigned int GlobalHistoryIndex = (GlobalHistory ^ ((PC>>16)^PC))&0x7fff; 
    GlobalHistory = (((GlobalHistory<<1) +(resolveDir?1:0)))& 0x7fff;
    

   if(resolveDir)
   {
       LocalPredictionTable[Local_PHT_index][Local_PHT_index2] = min(LocalPredictionTable[Local_PHT_index][Local_PHT_index2]+1, 3);
       GlobalPredictionTable[GlobalHistoryIndex] = min (GlobalPredictionTable[GlobalHistoryIndex]+1,3);
   }
   else
   {
       LocalPredictionTable[Local_PHT_index][Local_PHT_index2] = max(LocalPredictionTable[Local_PHT_index][Local_PHT_index2]-1, 0);
       GlobalPredictionTable[GlobalHistoryIndex] = max (GlobalPredictionTable[GlobalHistoryIndex]-1,0);
   }
    

    
   if(predDir == resolveDir)
   {
      if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)== 1)
         ChoicePredictionTable[Choice_Prediction_Index]=min(ChoicePredictionTable[Choice_Prediction_Index]+1,3);
      else if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)==0)
          ChoicePredictionTable[Choice_Prediction_Index]=max(ChoicePredictionTable[Choice_Prediction_Index]-1,0);
   }
    else
    {
        if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)== 1)
            ChoicePredictionTable[Choice_Prediction_Index]=max(ChoicePredictionTable[Choice_Prediction_Index]-1,0);
        else if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)==0)
            ChoicePredictionTable[Choice_Prediction_Index]=min(ChoicePredictionTable[Choice_Prediction_Index]+1,3);
    }

    //printf("%d\n", ChoicePredictionTable[Choice_Prediction_Index]);
    
    
}
*/
