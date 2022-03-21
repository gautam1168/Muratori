#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define Kilobytes(count) ((count) * 1024)
#define Megabytes(count) (Kilobytes(count) * 1024)
#define Gigabytes(count) (Megabytes(count) * 1024)
#define Terabytes(count) (Gigabytes(count) * 1024)

int WINAPI wWinMain(
  HINSTANCE Instance, 
  HINSTANCE PrevInstance, 
  PWSTR CommandLine, 
  int ShowCode) {

  LPVOID BaseAddress = (LPVOID)Terabytes((uint64_t)2); 
  void *Mem = VirtualAlloc(BaseAddress, 64 * 1024, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
  DWORD Error = GetLastError();
  char Buffer[256];
  printf("Error code: %d\n", Error);
  if (Mem) {
    // VirtualFree(Mem);
    printf("Memory was allocated and freed\n");
  } else {
    printf("Memory was not allocated\n");
  }

  return 0;
}