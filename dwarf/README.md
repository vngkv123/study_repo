# DWARF INFORMATION
**what is dwarf?**
- `Debugging With Attributed Record Formats`
- Used to stack unwinding in c/c++

**what is stack unwind?**
- Let's take an example.
```
#include <iostream>
#include <stdio.h>

using namespace std;

void f3(void){
  throw 10;
}

void f2(void){
  f3();
}

void f1(void){
  f2();
}

int main(void){
  try{
    f1();
  }
  catch(int e){
    printf("got exception : %d\n", e);
  }
}
```

- If i throw some value, it backtrace stack frame, find last catch statement.
- In above example code, throw statement is in the f3().
- Function flow is main() -> f1() -> f2() -> f3().
- So, catch statement is not defined in f3(), f2() and f1(), it has to backtrace from f3() to main().
- To successfully backtrace, DWARF information is needed, which placed in .eh_frame section or .debug_frame section.