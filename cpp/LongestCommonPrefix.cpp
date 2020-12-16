// Source : https://leetcode-cn.com/problems/longest-common-prefix/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Write a function to find the longest common prefix string amongst an array of strings.
 * 
 * If there is no common prefix, return an empty string "".
 * 
 * Example 1:
 * 
 * Input: strs = ["flower","flow","flight"]
 * Output: "fl"
 * 
 * Example 2:
 * 
 * Input: strs = ["dog","racecar","car"]
 * Output: ""
 * Explanation: There is no common prefix among the input strings.
 * 
 * Constraints:
 * 
 * 	0 <= strs.length <= 200
 * 	0 <= strs[i].length <= 200
 * 	strs[i] consists of only lower-case English letters.
 ******************************************************************************************************/

class Solution {
public:
    string longestCommonPrefix(vector<string>& strs) {
        string ans;
        if(strs.empty()) return ans;
        int len = INT_MAX;
        auto min = [](int a, int b) { return a < b ? a : b; };
        for(string str : strs)
            len = min(len, str.size());
        for(int i = 0; i < len; i++) {
            for(int j = 0; j < strs.size(); j++) {
                if(j == 0) ans.push_back(strs[j][i]);
                if(strs[j][i] != ans[i]) {
                    ans.pop_back();
                    break;
                }
            }        
        }
        return ans;
    }
};
