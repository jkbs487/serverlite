// Source : https://leetcode-cn.com/problems/letter-combinations-of-a-phone-number/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a string containing digits from 2-9 inclusive, return all possible letter combinations that 
 * the number could represent. Return the answer in any order.
 * 
 * A mapping of digit to letters (just like on the telephone buttons) is given below. Note that 1 does 
 * not map to any letters.
 * 
 * Example 1:
 * 
 * Input: digits = "23"
 * Output: ["ad","ae","af","bd","be","bf","cd","ce","cf"]
 * 
 * Example 2:
 * 
 * Input: digits = ""
 * Output: []
 * 
 * Example 3:
 * 
 * Input: digits = "2"
 * Output: ["a","b","c"]
 * 
 * Constraints:
 * 
 * 	0 <= digits.length <= 4
 * 	digits[i] is a digit in the range ['2', '9'].
 ******************************************************************************************************/

class Solution {
public:
    map<char, string> dict = {{'2', "abc"}, {'3', "def"}, {'4', "ghi"}, {'5', "jkl"}, {'6', "mno"}, {'7', "pqrs"}, {'8', "tuv"}, {'9', "wxyz"}};
    vector<string> letterCombinations(string digits) {
        vector<string> ans;
        int len = digits.size();
        if(len == 0) return ans;
        dfs(ans, digits, "", len, 0);
        return ans;
    }
    void dfs(vector<string> &ans, string digits, string temp, int len, int index) {
        if(index == len) {
            ans.push_back(temp);
            return;
        }
        for(int i = 0; i < dict[digits[index]].size(); i++) {
            temp.push_back(dict[digits[index]][i]);
            dfs(ans, digits, temp, len, index+1);
            temp.pop_back();
        }
        return;
    }
};
