// Source : https://leetcode-cn.com/problems/zigzag-conversion/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * The string "PAYPALISHIRING" is written in a zigzag pattern on a given number of rows like this: 
 * (you may want to display this pattern in a fixed font for better legibility)
 * 
 * P   A   H   N
 * A P L S I I G
 * Y   I   R
 * 
 * And then read line by line: "PAHNAPLSIIGYIR"
 * 
 * Write the code that will take a string and make this conversion given a number of rows:
 * 
 * string convert(string s, int numRows);
 * 
 * Example 1:
 * 
 * Input: s = "PAYPALISHIRING", numRows = 3
 * Output: "PAHNAPLSIIGYIR"
 * 
 * Example 2:
 * 
 * Input: s = "PAYPALISHIRING", numRows = 4
 * Output: "PINALSIGYAHRPI"
 * Explanation:
 * P     I    N
 * A   L S  I G
 * Y A   H R
 * P     I
 * 
 * Example 3:
 * 
 * Input: s = "A", numRows = 1
 * Output: "A"
 * 
 * Constraints:
 * 
 * 	1 <= s.length <= 1000
 * 	s consists of English letters (lower-case and upper-case), ',' and '.'.
 * 	1 <= numRows <= 1000
 ******************************************************************************************************/

class Solution {
public:
    string convert(string s, int numRows) {
        string ans;
        if(numRows <= 1) return s.empty() ? ans : s;
        vector<string> arr(numRows);
        int len = s.size();
        for(int i = 0; i < len; ++i) {
            int div = i / (numRows - 1);
            int rem = i % (numRows - 1);
            (div & 1 ? arr.at(numRows - rem - 1) : arr.at(rem)) += s.at(i);
        }
        for(const auto a : arr) ans += a;
        return ans;
    }
};