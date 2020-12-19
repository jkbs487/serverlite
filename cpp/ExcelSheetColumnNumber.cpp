// Source : https://leetcode-cn.com/problems/excel-sheet-column-number/submissions/
// Author : jkbs487
// Date   : 2020-12-19

/***************************************************************************************************** 
 *
 * Given a column title as appear in an Excel sheet, return its corresponding column number.
 * 
 * For example:
 * 
 *     A -> 1
 *     B -> 2
 *     C -> 3
 *     ...
 *     Z -> 26
 *     AA -> 27
 *     AB -> 28 
 *     ...
 * 
 * Example 1:
 * 
 * Input: "A"
 * Output: 1
 * 
 * Example 2:
 * 
 * Input: "AB"
 * Output: 28
 * 
 * Example 3:
 * 
 * Input: "ZY"
 * Output: 701
 * 
 * Constraints:
 * 
 * 	1 <= s.length <= 7
 * 	s consists only of uppercase English letters.
 * 	s is between "A" and "FXSHRXW".
 ******************************************************************************************************/

class Solution {
public:
    int titleToNumber(string s) {
        int ans = 0;
        int len = s.size();
        for(int i = 0; i < len; i++){
            ans = ans * 26 + (s[i]-'A'+1);
        }
        return ans;
    }
};
