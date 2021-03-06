// Source : https://leetcode-cn.com/problems/longest-substring-with-at-least-k-repeating-characters/
// Author : jkbs487
// Date   : 2021-02-28

/***************************************************************************************************** 
 *
 * Given a string s and an integer k, return the length of the longest substring of s such that the 
 * frequency of each character in this substring is greater than or equal to k.
 * 
 * Example 1:
 * 
 * Input: s = "aaabb", k = 3
 * Output: 3
 * Explanation: The longest substring is "aaa", as 'a' is repeated 3 times.
 * 
 * Example 2:
 * 
 * Input: s = "ababbc", k = 2
 * Output: 5
 * Explanation: The longest substring is "ababb", as 'a' is repeated 2 times and 'b' is repeated 3 
 * times.
 * 
 * Constraints:
 * 
 * 	1 <= s.length <= 104
 * 	s consists of only lowercase English letters.
 * 	1 <= k <= 105
 ******************************************************************************************************/

//Divide and conquer
class Solution {
public:
    int longestSubstring(string s, int k) {
        if(s.size() < k) return 0;
        vector<int> counter(26);
        for(auto c : s) {
            counter[c-'a']++;
        }
        for(auto c : s) {
            if(counter[c-'a'] < k) {
                vector<string> word_list = split(s, c);
                int ans = 0;
                for(auto word : word_list) {
                    ans = max(ans, longestSubstring(word, k));
                }
                return ans;
            }
        }
        return s.size();
    }

    vector<string> split(string str, char c) {
        vector<string> ans;
        string temp;
        istringstream ss(str);
        while(getline(ss, temp, c)) {
            ans.push_back(temp);
        }
        return ans;
    }
}
