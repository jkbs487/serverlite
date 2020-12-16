// Source : https://leetcode-cn.com/problems/longest-substring-without-repeating-characters/
// Author : jkbs487
// Date   : 2020-12-15

/***************************************************************************************************** 
 *
 * Given a string s, find the length of the longest substring without repeating characters.
 * 
 * Example 1:
 * 
 * Input: s = "abcabcbb"
 * Output: 3
 * Explanation: The answer is "abc", with the length of 3.
 * 
 * Example 2:
 * 
 * Input: s = "bbbbb"
 * Output: 1
 * Explanation: The answer is "b", with the length of 1.
 * 
 * Example 3:
 * 
 * Input: s = "pwwkew"
 * Output: 3
 * Explanation: The answer is "wke", with the length of 3.
 * Notice that the answer must be a substring, "pwke" is a subsequence and not a substring.
 * 
 * Example 4:
 * 
 * Input: s = ""
 * Output: 0
 * 
 * Constraints:
 * 
 * 	0 <= s.length <= 5 * 104
 * 	s consists of English letters, digits, symbols and spaces.
 ******************************************************************************************************/

class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        int ans = 0;
        int left = 0, right = 0;
        unordered_set<char> pool;
        if(s.empty()) return 0;
        while(right < s.size()){
            if(pool.find(s[right]) == pool.end()){
                pool.insert(s[right++]);
                ans = max(right-left, ans);
            }
            else{
                pool.erase(s[left]);
                left++;
            }
        }
        return ans;
    }
};
