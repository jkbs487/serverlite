// Source : https://leetcode-cn.com/problems/palindrome-number/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Determine whether an integer is a palindrome. An integer is a palindrome when it reads the same 
 * backward as forward.
 * 
 * Follow up: Could you solve it without converting the integer to a string?
 * 
 * Example 1:
 * 
 * Input: x = 121
 * Output: true
 * 
 * Example 2:
 * 
 * Input: x = -121
 * Output: false
 * Explanation: From left to right, it reads -121. From right to left, it becomes 121-. Therefore it 
 * is not a palindrome.
 * 
 * Example 3:
 * 
 * Input: x = 10
 * Output: false
 * Explanation: Reads 01 from right to left. Therefore it is not a palindrome.
 * 
 * Example 4:
 * 
 * Input: x = -101
 * Output: false
 * 
 * Constraints:
 * 
 * 	-231 <= x <= 231 - 1
 ******************************************************************************************************/

class Solution {
public:
    bool isPalindrome(int x) {
        if(x < 0) return false;
        if(x % 10 == 0 && x != 0) return false;
        int reverseNum = 0;
        while(reverseNum < x) {
            reverseNum = reverseNum * 10 + x % 10;
            x /= 10;
        }
        return (x == reverseNum) || (x == reverseNum / 10);
    }
};
