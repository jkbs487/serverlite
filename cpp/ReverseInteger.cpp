// Source : https://leetcode-cn.com/problems/reverse-integer/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a 32-bit signed integer, reverse digits of an integer.
 * 
 * Note:
 * Assume we are dealing with an environment that could only store integers within the 32-bit signed 
 * integer range: [&minus;231,  231 &minus; 1]. For the purpose of this problem, assume that your 
 * function returns 0 when the reversed integer overflows.
 * 
 * Example 1:
 * Input: x = 123
 * Output: 321
 * Example 2:
 * Input: x = -123
 * Output: -321
 * Example 3:
 * Input: x = 120
 * Output: 21
 * Example 4:
 * Input: x = 0
 * Output: 0
 * 
 * Constraints:
 * 
 * 	-231 <= x <= 231 - 1
 ******************************************************************************************************/

class Solution {
public:
    int reverse(int x) {
        long long r = 0;
        while(x != 0) {
            r = r * 10 + x % 10;
            x = x / 10;
          if(r > INT_MAX || r < INT_MIN) 
            return 0;
        }
        return (int)r;
    }
};
