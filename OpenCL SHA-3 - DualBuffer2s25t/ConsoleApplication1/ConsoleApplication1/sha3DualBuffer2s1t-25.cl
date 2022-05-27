#define LOGG true

typedef unsigned char uint8_t;
// typedef unsigned long int ulong;

typedef char *string_t;

#define total 1600
#define rate 1088

#define rounds 24

#define totalBytes total / 8
#define rateBytes rate / 8
#define rateLanes rate / 64

#define delim_begin 0x06
#define delim_end 0x80

#define BUFFER_SIZE 400

#define ROT(a, offset)                                                         \
  ((((ulong)a) << offset) ^ (((ulong)a) >> (64 - offset))) // credit
#define state (*State)
#define lane ((ulong *)state._8)

__constant uint8_t rho[25] = {0,  1, 62, 28, 27, 36, 44, 6,  55,
                              20, 3, 10, 43, 25, 39, 41, 45, 15,
                              21, 8, 18, 2,  61, 56, 14};

__constant uint8_t pi[25] = {0,  6,  12, 18, 24, 3,  9,  10, 16, 22, 1,  7, 13,
                             19, 20, 4,  5,  11, 17, 23, 2,  8,  14, 15, 21};

__constant ulong iota[24] = {
    0x0000000000000001UL, 0x0000000000008082UL, 0x800000000000808aUL,
    0x8000000080008000UL, 0x000000000000808bUL, 0x0000000080000001UL,
    0x8000000080008081UL, 0x8000000000008009UL, 0x000000000000008aUL,
    0x0000000000000088UL, 0x0000000080008009UL, 0x000000008000000aUL,
    0x000000008000808bUL, 0x800000000000008bUL, 0x8000000000008089UL,
    0x8000000000008003UL, 0x8000000000008002UL, 0x8000000000000080UL,
    0x000000000000800aUL, 0x800000008000000aUL, 0x8000000080008081UL,
    0x8000000000008080UL, 0x0000000080000001UL, 0x8000000080008008UL};

union INTER {
  ulong _64[25];
  uint8_t _8[200];
};

__kernel void f_function(local union INTER *State) {
    local ulong CrossPlane[5];
    local union INTER iState;
    ulong D;

    uint8_t i = get_local_id(0);

    uint8_t m = i%5;
    uint8_t z = i/5;
    int indx = 0;

    for(int round = 0; round<rounds; round++){
        //Omega
        CrossPlane[m] = state._64[m] ^ state._64[m + 5] ^ state._64[m + 10] ^ state._64[m + 15] ^ state._64[m + 20];
        //CROSSPLANE(CrossPlane,m,state._64);
        state._64[i] ^= CrossPlane[m==0?4:(m-1)] ^ rotate(CrossPlane[m==4?0:(m+1)], (ulong)1);
        //OMEGA(CrossPlane, state._64, m, i);
        
        //Rho and Pi
        indx = pi[i];
        iState._64[i] = rotate(state._64[indx], (ulong) rho[indx]);
        //RHOPI(iState._64, state._64, indx, i, rho);

        //Chi
        state._64[i] = iState._64[i] ^ (~iState._64[(i+1)%5 +z*5] & iState._64[(i+2)%5 +z*5]);
        //CHI(state._64, iState._64, i, z);

        state._64[0] ^= iota[round];
        //IOTA(state._64, iota, round);
    }
}

void printState(global union INTER *State) {

  for (int i = 0; i < 32; i++)
    printf("%02x", State->_8[i]);

  // printf("\n");
}

__kernel void Keccak(global const char *input, global const int *siz, global union INTER *State, global const int *keccakCounter) {
  union INTER iState; //= { { 0 } };
  local union INTER State2;
  int size = *siz;
  int keccakCount = *keccakCounter;

  if (keccakCount==0)
  {
    for (int i = 0; i < 200; i++)
        State2._8[i] = 0;
  }
  else
  {
    for (int i = 0; i < 200; i++) {
        State2._8[i] = state._8[i];
    }
  }
    

int buffer_i = 0;
while(size>0 && (buffer_i++) < BUFFER_SIZE){
  if (size < rateBytes) {
    for (int i = 0; i < size; i++)
      State2._8[i] ^= input[i];
    State2._8[size] ^= delim_begin; // padding
    State2._8[rateBytes - 1] ^= delim_end;
  }
  else {
    for (int i = 0; i < rateBytes; i++) {
      State2._8[i] ^= input[i];
    }
    input += rateBytes;
  }

  f_function(&State2);
  size -= rateBytes;

  for (int i = 0; i < 200; i++) {
    state._8[i] = State2._8[i];
  }
  }
}
