#include <iostream>
#include <memory>

class CBase{
public:
    int v1;
};

class CDerived : public CBase{
public:
    int v2;
};


int main(int argc, char *argv[]){

    CBase *pBase = new CBase;
    CDerived *pDerived = new CDerived;
    pBase->v1 = 0xdeadbeef;

    pDerived->v1 = 0xdeadbeef;
    pDerived->v2 = 0xcafebabe;

    std::cout << pBase << std::endl;
    std::cout << pDerived << std::endl;
    std::cout << pBase->v1 << std::endl;
    std::cout << pDerived->v1 << std::endl;
    std::cout << pDerived->v2 << std::endl;

    CBase *confusion = new CDerived;            // Isn't type-confusion case?
    std::cout << confusion << std::endl;        // Why didn't warn some message?

    int v1 = 50;
    int *p1 = &v1;
//    char *p2 = p1;        // Error ! ( Type mismatch )
//    char *p2 = static_cast<char *>(p1);       // Error ! ( Type mismatch )
    char *p2 = reinterpret_cast<char *>(p1);        // Only allow pointer type casting

    /* const_cast<>() is only used for const and reference type */

    return 0;
}
