// Source : https://leetcode-cn.com/problems/string-to-integer-atoi/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Implement atoi which converts a string to an integer.
 * 
 * The function first discards as many whitespace characters as necessary until the first 
 * non-whitespace character is found. Then, starting from this character takes an optional initial 
 * plus or minus sign followed by as many numerical digits as possible, and interprets them as a 
 * numerical value.
 * 
 * The string can contain additional characters after those that form the integral number, which are 
 * ignored and have no effect on the behavior of this function.
 * 
 * If the first sequence of non-whitespace characters in str is not a valid integral number, or if no 
 * such sequence exists because either str is empty or it contains only whitespace characters, no 
 * conversion is performed.
 * 
 * If no valid conversion could be performed, a zero value is returned.
 * 
 * Note:
 * 
 * 	Only the space character ' ' is considered a whitespace character.
 * 	Assume we are dealing with an environment that could only store integers within the 32-bit 
 * signed integer range: [&minus;231,  231 &minus; 1]. If the numerical value is out of the range of 
 * representable values, 231 &minus; 1 or &minus;231 is returned.
 * 
 * Example 1:
 * 
 * Input: str = "42"
 * Output: 42
 * 
 * Example 2:
 * 
 * Input: str = "   -42"
 * Output: -42
 * Explanation: The first non-whitespace character is '-', which is the minus sign. Then take as many 
 * numerical digits as possible, which gets 42.
 * 
 * Example 3:
 * 
 * Input: str = "4193 with words"
 * Output: 4193
 * Explanation: Conversion stops at digit '3' as the next character is not a numerical digit.
 * 
 * Example 4:
 * 
 * Input: str = "words and 987"
 * Output: 0
 * Explanation: The first non-whitespace character is 'w', which is not a numerical digit or a +/- 
 * sign. Therefore no valid conversion could be performed.
 * 
 * Example 5:
 * 
 * Input: str = "-91283472332"
 * Output: -2147483648
 * Explanation: The number "-91283472332" is out of the range of a 32-bit signed integer. Thefore 
 * INT_MIN (&minus;231) is returned.
 * 
 * Constraints:
 * 
 * 	0 <= s.length <= 200
 * 	s consists of English letters (lower-case and upper-case), digits, ' ', '+', '-' and '.'.
 ******************************************************************************************************/

class Solution {
public:
    int myAtoi(string str) {
        long long ans = 0;
        int index = 0, symbol = 1;
        while(str[index] == ' ') index++;
        if(str[index] == '-') {
            index++;
            symbol = 0;
        } 
        else if(str[index] == '+') index++;  
        if(str[index] > '9' || str[index] < '0') return 0;
        for(int i = index; i < str.size(); i++) {
            if(str[i] <= '9' && str[i] >= '0') 
                ans = 10 * ans + (str[i] - '0');
            else break;
            if(ans >= (long long)INT_MAX) break;
        }
        if(symbol) return (int)min(ans, (long long)INT_MAX);
        else return (int)max(-ans, (long long)INT_MIN);
    }
};
