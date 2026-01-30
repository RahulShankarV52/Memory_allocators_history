#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBEROFROUNDS 1000000
#define NUMBEROFOBJECTS 100

typedef struct {
  int x, y, z;
} Vector3;

typedef struct {
  Vector3 obj;
  bool allocated;
} PoolObject;

// objectpool
PoolObject PoolObjects[NUMBEROFOBJECTS] = {0};

Vector3 *GetAllocation(void) {
  for (int i = 0; i < NUMBEROFOBJECTS; i++) {
    if (PoolObjects[i].allocated == false) {
      PoolObjects[i].allocated = true;
      return &(PoolObjects[i].obj);
    }
  }
  return NULL; // if none available
}

void FreeAllocation(Vector3 *v) {
  PoolObject *returned = (PoolObject *)v;
  returned->allocated = false;
}

int main() {
  for (int i = 0; i < NUMBEROFROUNDS; i++) {
    int numobjs = rand() % NUMBEROFOBJECTS;
    Vector3 *vectors[numobjs];
    for (int j = 0; j < numobjs; j++) {
      vectors[j] = GetAllocation();
    }
    printf("round %d -- got %d vectors \n", i, numobjs);
    for (int j = 0; j < numobjs; j++) {
      FreeAllocation(vectors[j]);
    }
  }
}
