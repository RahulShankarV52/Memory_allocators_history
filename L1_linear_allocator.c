#include <string.h>
#include <unistd.h>

void *mainHeap = NULL;

void *myalloc(int size) {
  void *allocated = sbrk(size);
  if (mainHeap == NULL) {
    mainHeap = allocated;
  }
  return allocated;
}

int myfree() {
  if (mainHeap == NULL) {
    return -1;
  }
  return brk(mainHeap);
}

int main() {
  const char *message = "Hello world";
  char *testmem = (char *)myalloc(strlen(message) * sizeof(char));
  strcpy(testmem, message);
  myfree();

  return 0;
}
