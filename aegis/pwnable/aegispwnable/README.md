
1. easy

-> urandom에서 32바이트의 값을 읽어오는데, 이 urandom값과 key값을 동일하게 맞춰줄 경우, gPwn함수에서 bof가 발생하게 됩니다.

ukey에 유저 입력 값을, skey에 urandom값을 담는데, 둘 다 같은 bss영역에 연달아 붙어있습니다.
gEdit()함수에서 ukey에 담긴 값을 변경할 수 있는데, 사이즈체크를 strlen()함수로 합니다.
여기서 skey와 값이 끊기지않고 담겨있다면 strlen에서 64바이트를 리턴하게되며, 이를 통해 ukey -> skey까지 모두 덮을 수 있게 됩니다.

이를 이용하여 gPwn에서의 skey == ukey조건을 맞추어주고나면 puts함수를 이용한 library leak이 가능해지며, 다시 main으로 돌아와 leak된 주소를 바탕으로 쉘을 획득하면 됩니다.

간단한 rop문제입니다.


2. heap

-> 0x68, 0x88, 0x228 크기로 동적할당할 수 있으며, 전역배열을 통해 heap chunk를 관리하게됩니다.
바이너리는 full relro + pie + nx, 그리고 기본 환경 보호옵션은 aslr입니다.
gClear를 통해 전역배열에 위치하는 heap pointer를 free할 수 있으며, 하자마자 초기화 시켜버립니다.
gEdit을 통해 1번의 easy문제처럼 strlen()으로 구한 길이만큼 수정할 수 있습니다.
여기서 size overwrite나 unsafe unlink류의 heap 취약점이 발생합니다.

1차 버전은 마음대로 free를 할 수 없으며, top chunk size overwrite를 통해, 앞의 chunk를 강제로 free시키는 로직을 넣었었는데, 중급 문제치고는 너무 어려워지는 경향이 있어서 수정을 했습니다.

unsorted bin의 fd, bk가 설정되는 것을 이용하여 gWrite()에서 memory leak이 가능합니다.
pie옵션 때문에, 전역배열에서 heap pointer들을 관리한다고 하더라도, code base를 알 수 있는 방법이 없기때문에, 여기서는 unsafe unlink를 사용할 수 없습니다.

의도한 풀이는 fastbin attack입니다.
0x68이라는 사이즈를 힌트삼아 __malloc_hook류나 다른 쪽 overwrite를 의도했습니다.
전역배열의 heap pointer는 free하는 순간 초기화시키기때문에, 다른 루틴이 한 번 필요합니다.
chunk의 size를 overwrite하여 chunk overlapping을 유도합니다.
overlapping이 되면, 같은 heap위치에 2개의 chunk가 공존할 수 있으므로, double free가 가능해집니다.
그리고 이를 이용하여 fastbin을 다시 할당하면서 fastbin->fd를 적당한 영역으로 수정해주게되면, arbitrary memory write가 가능해집니다.


3. fb

Fastbin attack과 chunk overlapping, House of Orange 테크닉을 아는지 종합적으로 물어보는 문제입니다.
물론 이 테크닉들은 다 유명하고 여러 CTF에서 사용된 것들이기에 크게 어렵진 않습니다.

이번 ASIS CTF에 출제된 fifty dollars라는 문제처럼 fastbin attack을 통해 fake file stream을 구성하도록 할려다가
편의상 쉽게 file structure를 만들 수 있도록 once()라는 함수를 추가했습니다.

간단하게만 설명하자면, Fastbin attack을 통해 특정 chunk의 size를 unsorted bin chunk로 맞추어줍니다.
그리고 해당 chunk를 free하면 main_arena + 88의 값이 쓰이게되고, 이 값을 show를 통해 leak할 수 있습니다.
할당되는 chunk들은 전역배열에서 관리되며, free후에 초기화시키지않아 UAF가 모든 chunk에 있어서 발생합니다.

같은 방식으로, 이 free된 unsorted bin chunk에 fake file stream을 구성하여 House of Orange테크닉으로 끌고가여 쉘을 획득하면 되는 문제입니다.
