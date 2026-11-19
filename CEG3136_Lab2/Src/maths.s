// Math routines for Calculator app
// Author: Jason Tong and Ken Shan
.syntax unified
.cpu cortex-m33
.thumb
.section .text


// uint32_t Increment(uint32_t n);
.global Increment
.type Increment, %function
Increment:
add r0, r0, #1
bx lr

// uint32_t Decrement(uint32_t n);
.global Decrement
.type Decrement, %function
Decrement:
sub r0, r0, #1
bx lr



// Classic 4-function: Add, subtract, multiply, or divide two numbers.

// Addition r0+r1
// uint32_t Add(uint32_t n1, uint32_t n2);
.global Add
.type Add, %function
Add:
add r0, r0, r1
bx lr

// Subtraction r0-r1
// uint32_t Sub(uint32_t n1, uint32_t n2);
.global Sub
.type Sub, %function
Sub:
sub r0, r0, r1
bx lr

// Multiply r0*r1
// uint32_t Mul(uint32_t n1, uint32_t n2);
.global Mul
.type Mul, %function
Mul:
mul r0, r0, r1
bx lr

// Divide r0/r1
// uint32_t Div(uint32_t n1, uint32_t n2);
.global Div
.type Div, %function
Div:
udiv r0, r0, r1
bx lr



// GCD: Find the greatest common divisor of two numbers using Euclid's algorithm.

// uint32_t GCD(uint32_t n1, uint32_t n2);
.global GCD
.type GCD, %function
GCD:
// Perform n1 - n2 if n1 > n2, n2 - n1 if n1 < n2, and repeat until n1==n2, in which case the 
// result will already be in r0.
cmp r0, r1
it hi
subhi r0, r0, r1
it lo
sublo r1, r1, r0
bne GCD
bx lr



// Factorial: For input number n >= 0, calculate the value of n! using an iterative 
// algorithm. By definition 0! = 1 because 1 is the multiplicative identity.

// uint32_t Factorial(uint32_t n);
.global Factorial
.type Factorial, %function
Factorial:
mov r1, #1      // r1 is the loop counter, i = 1
mov r2, #1      // r2 is the accumulated product
b check
loop:
mul r2, r2, r1
add r1, r1, #1
check:
cmp r1, r0      // Multiply r2 by i in the loop until i > n
bls loop
mov r0, r2      // Move the result to r0 for return
bx lr



// Fibonacci: For input number n >= 0, display the nth term in the Fibonacci sequence.
// Assume that 0 and 1 are the 0th and 1st terms in the sequence respectively. Use a 
// recursive algorithm.

// uint32_t Fibonacci(uint32_t n);
.global Fibonacci
.type Fibonacci, %function
Fibonacci:
// base case n = 0 or 1, the result is n itself
cmp r0, #1
it ls
bxls lr

sub r0, r0, #1  // r0 is set to n-1
push {r0, lr}
bl Fibonacci    // Fib(n-1)
mov r1, r0      // Result of Fib(n-1) is saved in r1
pop {r0, lr}
sub r0, r0, #1  // r0 is set to n-2
push {r1, lr}
bl Fibonacci    // Fib(n-2)
mov r2, r0      // Result of Fib(n-2) is saved in r2
pop {r1, lr}
add r0, r1, r2  // Move Fib(n-1) + Fib(n-2) to r0 for return
bx lr
// Recursive calls can be seen as a black box. Internal processes are irrelevant, we only
// need to know that correct results will be in r0 and preserved registers will be restored.



// Sort: Sort an array of numbers (max 10 items) using the algorithm of your choice.  
// Display the sorted numbers using a method of your choice.

// void Sort(uint32_t array[], uint32_t size);
.global Sort
.type Sort, %function
// Bubble sort algorithm is used here with elements sorted in ascending order.
Sort:
push {r4, r5, r6, r7, r8, r10, r11} // Push preserved registers onto the stack

cmp r1, #1      // Special case if size is 1 or less
bls sort_done

sub r1, r1, #1  // r1 is iteration counter, i = size - 1

loop_i:
mov r2, #0      // r2 is inner loop counter, j = 0

loop_j:
lsls r3, r2, #2 // r3 is address offset of element j for 32-bit memory
add r4, r0, r3  // r4 is address of element j

ldr r5, [r4]        // r5 is value of array[j]
ldr r6, [r4, #4]    // r6 is value of array[j+1]

cmp r5, r6      // Swap if array[j] > array[j+1]
bls no_swap
str r6, [r4]
str r5, [r4, #4]

no_swap:
adds r2, r2, #1 // Increment inner loop counter and do loop j again if j < i
cmp r2, r1
blo loop_j

subs r1, r1, #1 // Decrement loop counter i while setting Zero (Z) flag and do loop i again if Z = 0
bne loop_i
sort_done:
pop {r4, r5, r6, r7, r8, r10, r11} // Restore the states of preserved registers
bx lr



// Average: Find the mean value of an array of numbers (max 10 items). Include 1 decimal 
// place in your final display.

// uint32_t Ave(uint32_t array[], uint32_t size);
.global Ave
.type Ave, %function
Ave:
push {r4, r5, r6, r7, r8, r10, r11} // Push preserved registers onto the stack
mov r2, #0      // r2 is iteration counter, i = 0
mov r3, #0      // r3 is accumulated sum of the integers in the array
mov r12, #10

loop_avg:
cmp r2, r1      // Exit the loop if i = size
beq end
ldr r4, [r0], #4    // Load the next integer in the array and increment the pointer
mul r4, r4, r12     // Multiply r4 by 10 to keep 1 decimal place
add r3, r3, r4      // Add the multiplied value to the sum in r3
add r2, r2, #1      // i++
b loop_avg

end:
udiv r0, r3, r1     // Integer division of the sum by the size to get 10 times the average
pop {r4, r5, r6, r7, r8, r10, r11} // Restore the states of preserved registers
bx lr

