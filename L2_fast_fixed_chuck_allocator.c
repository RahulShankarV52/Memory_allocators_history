#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBEROFROUNDS 1000000
#define NUMBEROFOBJECTS 100

#define FLAG (struct PoolObject *)0x1

typedef struct {
  int x, y, z;
} Vector3;

typedef struct PoolObject {
  Vector3 obj;
  struct PoolObject *next;
} PoolObject;

PoolObject *freelist = NULL;

// objectpool
PoolObject PoolObjects[NUMBEROFOBJECTS] = {0};

void initialize_pool() {
  for (int i = 0; i < NUMBEROFOBJECTS - 1; i++) {
    PoolObjects[i].next = &(PoolObjects[i + 1]);
  }
  PoolObjects[NUMBEROFOBJECTS - 1].next = NULL;
  freelist = &(PoolObjects[0]);
}

Vector3 *GetAllocation(void) {
  if (freelist) {
    PoolObject *poolobj = freelist;
    freelist = freelist->next;
    poolobj->next = FLAG;
    return &(poolobj->obj);
  }

  return NULL; // if none available
}

void FreeAllocation(Vector3 *v) {
  PoolObject *returned = (PoolObject *)v;
  if (returned->next != FLAG) {
    printf("Error: Attempting to free free memory");
    return;
  }
  returned->next = freelist;
  freelist = returned;
}

int main() {
  initialize_pool();
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
