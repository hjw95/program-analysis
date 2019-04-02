#include <stdio.h>

int main() {
   int n;
   // Read n from input
   while (n != 1){
       if (n % 2 == 0) n = n / 2;
       else n = 3 * n + 1;
   }
}
 
