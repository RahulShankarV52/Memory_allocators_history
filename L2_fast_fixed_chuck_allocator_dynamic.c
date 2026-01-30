#include <stdbool.h>
#include <stddef.h>
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

#define CHUCKSIZE 10

bool grow_pool() {
  size_t bytes_needed = sizeof(PoolObject) * CHUCKSIZE;
  bytes_needed = (bytes_needed + 15) & ~15;
  PoolObject *PoolObjects = sbrk(bytes_needed);
  if (PoolObjects == (void *)-1) {
    return false;
  }
  for (int i = 0; i < CHUCKSIZE - 1; i++) {
    PoolObjects[i].next = &(PoolObjects[i + 1]);
    PoolObjects[i].obj = (Vector3){0, 0, 0};
  }
  PoolObjects[CHUCKSIZE - 1].next = freelist;
  PoolObjects[CHUCKSIZE - 1].obj = (Vector3){0, 0, 0};
  freelist = &(PoolObjects[0]);
  return true;
}

Vector3 *GetAllocation(void) {
  if (freelist == NULL) {
    if (!grow_pool()) {
      return NULL; // if none available
    }
  }

  PoolObject *poolobj = freelist;
  freelist = freelist->next;
  poolobj->next = FLAG;
  return &(poolobj->obj);
}

void FreeAllocation(Vector3 *v) {
  PoolObject *returned =
      (PoolObject *)((char *)v - offsetof(PoolObject, obj)); // more robust
  if (returned->next != FLAG) {
    printf("Error: Attempting to free free memory");
    return;
  }
  returned->next = freelist;
  freelist = returned;
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
