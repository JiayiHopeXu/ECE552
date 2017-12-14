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


/*
class saturating_counter
{
public:
  short counter;
  short saturated_positive;
  short saturated_negative;

  void increment(int value)
  {
    if(counter + value < saturated_positive)
      counter += value;
    else
      counter = saturated_positive;
  }

  void decrement(int value)
  {
    if(counter - value > saturated_negative)
      counter -= value;
    else
      counter = saturated_negative;
  }
};


#define PHT_NUM 14
#define PREDICTOR_TABLE_SIZE 2048
saturating_counter ** predictor_tables;
std::bitset<PREDICTOR_TABLE_SIZE> tag_bit;

//Global Histroy Register
#define GHR_LENGTH 3360


int L[17] = {0, 3, 5, 8, 12, 19, 31, 49, 75, 125, 200, 320, 512, 820, 1312, 2100, 3360};
int history_length[PHT_NUM];



saturating_counter AC;  //9 bit saturating ailiasing counter
saturating_counter TC; //7 bit saturating ailiasing counter

int theta; // value between 0 to 31
#define MAX_THETA 31

long long phr; // path history register

#define ghr_size 6
long long ghr[ghr_size]; 

int GetIndexOfTable(UINT32 PC, int PHT_ID)
{
  // 33 bits of this input is used as 3 11-bit input for the XOR
  long long input;  
  int history_len = history_length[PHT_ID];
  int path_len = min(16, history_len);

  // for calculating the index we use history_len bits from ghr, path_len bits from phr and at least
  // 8 bits of PC and at most 20 bits. We need to calculate an 11 bit index which is an output of 3 entries XOR gates.
  // Therefor we will have 33 bits as input on our XOR gate. 

  // We check if need to ignore some bits in phr or ghr in order to include at least 8 bits form PC
  int pc_bits_len = 33 - history_len - path_len;
  if(pc_bits_len > 20)
    pc_bits_len = 20;

  if(pc_bits_len < 8)
  {
    // we need to ignore some bits in phr and ghr
    int fills[34];
    int i;
    for(i = 0; i < 33; i++)
      fills[i] =  ((i * (8 + history_len + path_len - 1)) / (32));
    fills[33] = 8 + history_len + path_len;

    input = 0;
    // ghr bits
    long long ghr_chunk = ghr[0];
    ghr_chunk >>= fills[0];
    input = (ghr_chunk & 1);
    int chunk_index = 0;

    int shift; // value to shift ghr_chunk
    for (i = 1; fills[i] < history_len; i++)
    {

      if ((fills[i] & 0xffc0) == (fills[i - 1] & 0xffc0))
      {
        shift = fills[i] - fills[i - 1];
      }
      else
      {
        ghr_chunk = ghr[chunk_index];
        chunk_index++;
        shift = fills[i] & 63;
      }
      input = (input << 1);
      ghr_chunk = ghr_chunk >> shift;
      input ^= (ghr_chunk & 1);
    }

    //pc bits
    for( ; fills[i] < history_len + 8; i++)
    {
      shift = fills[i] - history_len;
      input = (input << 1);
      input ^= ((PC >> shift) & 1);
    }

    //phr bits
    for (; fills[i] < path_len + history_len + 8; i++)
    {
      shift = fills[i] - (history_len + 8);
      input = (input << 1);
      input ^= ((phr >> shift) & 1);
    }
  } 

  else
  {
    // concatinate all the bits and store it in input
    input = ((ghr[0] & ((1 << history_len) - 1)) << (pc_bits_len + path_len)) + 
    ((PC & ((1 << pc_bits_len) - 1)) << path_len) + 
    ((phr & ((1 << path_len) - 1)));
  }

// now we have the 33 bits input ready to be XORed.

  int identifier = (PHT_ID & 3) + 1;


  long long index = input & ((1 << 11) - 1);
  int i;
  for (i = 1; i < 3; i++)
    {
      input = input >> 11;

      index ^=
        ((input & ((1 << 11) - 1)) >> identifier) ^((input & ((1 << identifier) - 1)) << ((11 - identifier)));

      identifier = (identifier + 1) % 11;
    }

  return ((int) index);

}




void InitPredictor_openend() 
{
    int i,j;
   
    // initializing predictor tables and setting all counters to zero
    predictor_tables = (saturating_counter **)malloc(PHT_NUM * sizeof(saturating_counter *));
    for (i = 0; i < PHT_NUM; i++)
    {
        predictor_tables[i] = (saturating_counter *)malloc(PREDICTOR_TABLE_SIZE * sizeof(saturating_counter));
        for(j = 0; j < PREDICTOR_TABLE_SIZE; j++)
        {
            predictor_tables[i][j].counter = 0;
            if(i==0 || i == 1)
            {
                // T0 and T1 are 5 bits saturating counters
                predictor_tables[i][j].saturated_positive = 15;
                predictor_tables[i][j].saturated_negative = -16;
            }
            else
            {
                // other tables are 4 bits saturating counters
                predictor_tables[i][j].saturated_positive = 7;
                predictor_tables[i][j].saturated_negative = -8;
            }
        }
    }

    // set the tag bit to zero
    tag_bit.set(0);


    AC.counter = 0;
    AC.saturated_positive = 255;
    AC.saturated_negative = -256;

    TC.counter = 0;
    TC.saturated_positive = 63;
    TC.saturated_negative = -64;

    theta = PHT_NUM;

    for(i = 0; i < PHT_NUM; i++)
    {
      history_length[i] = L[i];
    }

    phr = 0;
    for (i = 0 ; i < ghr_size; i++)
      ghr[i] = 0;
   
}

bool GetPrediction_openend(UINT32 PC) 
{
  
  int S = PHT_NUM / 2;
  int i;
  int index[PHT_NUM];

  for(i = 0; i < PHT_NUM; i++)
  {
     index[i] = GetIndexOfTable(PC, i);
     S += predictor_tables[i][index[i]].counter;
  }

  if(S >= 0)
      return TAKEN;

  return NOT_TAKEN;
}


void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) 
{
  int S = PHT_NUM / 2;
  int i;
  int index[PHT_NUM];
  for(i = 0; i < PHT_NUM; i++)
  {
      index[i] = GetIndexOfTable(PC, i);
      S += predictor_tables[i][index[i]].counter;
  }


  //dynamic threshhold fitting
  if(resolveDir != predDir)
  {
      TC.increment(1);
      if(TC.counter == TC.saturated_positive)
      {
          if(theta < MAX_THETA)
          {
            theta++;
            TC.counter = 0; 
          } 
      }
  }
  else if((S >= -theta) && (S < theta))
  {
    TC.decrement(1);
    if(TC.counter == TC.saturated_negative)
    {
      if(theta > 0)
      {
        theta--;
        TC.counter = 0;
      }
    }
  }
 

  //Updating Predictor Tables
  if(resolveDir != predDir || ((S < theta) && (S >= -theta)))
  {
      for(i = 0; i < PHT_NUM; i++)
      {
          if(resolveDir)
              predictor_tables[i][index[i]].increment(1);
          else
              predictor_tables[i][index[i]].decrement(1);
      }
  }



  // Dynamic History Length Fitting
  if(resolveDir != predDir)
  {
      
      if((PC & 1) == (int)tag_bit[index[PHT_NUM -1]])
          AC.increment(1);
      else
          AC.decrement(4);

      if(AC.counter == AC.saturated_positive)
      {
         history_length[2] = L[PHT_NUM]; 
         history_length[4] = L[PHT_NUM + 1]; 
         history_length[6] = L[PHT_NUM + 2]; 
      }
      if(AC.counter == AC.saturated_negative)
      {
         history_length[2] = L[2]; 
         history_length[4] = L[4]; 
         history_length[6] = L[6]; 
      }
  }

  tag_bit.set(index[PHT_NUM -1], (PC & 1));


  // updating ghr
  for(i = ghr_size; i > 0; i--)
  {
    ghr[i] = (ghr[i] << 1) + (ghr[i -1] < 0);
  }
  ghr[0] = ghr[0] << 1;

  if(resolveDir)
    ghr[0]++;

  // updating phr
  phr = (phr << 1) | (PC & 1);
}
*/

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

short LocalHistoryTable[512];
short GlobalPredictionTable[int(pow(2,15))];
std::bitset<int(pow(2,15))> tag_bit;
unsigned long long GlobalHistory;
short LocalPredictionTable[8][64];
short ChoicePredictionTable[4096];

void InitPredictor_openend() {
    
    LocalHistoryTable[512] = {0};  //6-bits for entry
    GlobalPredictionTable[4096] = {1};
    ChoicePredictionTable[4096] = {0};
    tag_bit.set(0);
    GlobalHistory = 0;
    
    for (int i= 0; i<8; i++)
    {
        for(int j=0; j<64; j++)
        {
            LocalPredictionTable[i][j] = 1;
        }
    }
}

bool GetPrediction_openend(UINT32 PC) {
    int Local_BHT_index = (PC & 0xFF8)>>3;
    int Local_PHT_index = (PC & 0X7);
    int Choice_Prediction_Index = (PC & 0xfff);
    int GlobalHistoryIndex = (GlobalHistory ^ PC)&0x7fff;
  
     // choice prediction: 1 bias towords local prediction, 0 bias towards global prediction
    if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)== 0 && tag_bit[GlobalHistoryIndex] == 1)
    {
        if(((GlobalPredictionTable[GlobalHistoryIndex]>>1)&0x1) == 1)
            return TAKEN;
        else if(((GlobalPredictionTable[GlobalHistoryIndex]>>1)&0x1) == 0)
            return NOT_TAKEN;
    }
    else //if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1) == 1)
    {
        int Local_PHT_index2 = (int)LocalHistoryTable[Local_BHT_index];
        
        if (((LocalPredictionTable[Local_PHT_index][Local_PHT_index2] >>1) &0x1) == 1)
            return TAKEN;
        else if(((LocalPredictionTable[Local_PHT_index][Local_PHT_index2]>>1)&0x1) == 0)
            return NOT_TAKEN;
    }
    return NOT_TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

    int Local_BHT_index = (PC & 0xFF8)>>3;
    int Local_PHT_index = (PC & 0X7);
    int Choice_Prediction_Index = (PC & 0xfff);
    int Local_PHT_index2 = (int)LocalHistoryTable[Local_BHT_index];
    
    LocalHistoryTable[Local_BHT_index] = ((LocalHistoryTable[Local_BHT_index]<<1) + (resolveDir?1:0))& 0x3f;
    unsigned int GlobalHistoryIndex = (GlobalHistory ^ PC)& 0x7fff;
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
      else if(((ChoicePredictionTable[Choice_Prediction_Index]>>1)&0x1)==0) {
	  tag_bit.set(GlobalHistoryIndex, 1);
          ChoicePredictionTable[Choice_Prediction_Index]=max(ChoicePredictionTable[Choice_Prediction_Index]-1,0);
      }
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

